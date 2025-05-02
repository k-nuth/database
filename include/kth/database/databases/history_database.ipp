// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_HISTORY_DATABASE_IPP_
#define KTH_DATABASE_HISTORY_DATABASE_IPP_

#include <kth/infrastructure/log/source.hpp>

namespace kth::database {

template <typename Clock>
result_code internal_database_basis<Clock>::insert_history_db(domain::wallet::payment_address const& address, data_chunk const& entry, leveldb::WriteBatch& batch) {
    auto key_arr = address.hash20();
    
    // Create a composite key that includes the address hash and entry ID to handle duplicates
    // This is needed because LevelDB doesn't natively support duplicate keys like LMDB does
    
    // Extract the ID from the entry data (assuming it's at the beginning)
    uint64_t id = 0;
    if (entry.size() >= sizeof(id)) {
        id = *reinterpret_cast<const uint64_t*>(entry.data());
    }
    
    // Create composite key: address_hash + id
    std::string composite_key;
    composite_key.append(reinterpret_cast<const char*>(key_arr.data()), key_arr.size());
    composite_key.append(reinterpret_cast<const char*>(&id), sizeof(id));
    
    leveldb::Slice key(composite_key.data(), composite_key.size());
    leveldb::Slice value(reinterpret_cast<const char*>(entry.data()), entry.size());
    
    batch.Put(cf_history_db_, key, value);
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_input_history(domain::chain::input_point const& inpoint, uint32_t height, domain::chain::input const& input, leveldb::WriteBatch& batch) {
    auto const& prevout = input.previous_output();

    if (prevout.validation.cache.is_valid()) {
        uint64_t history_count = get_history_count();
        if (history_count == max_uint64) {
            LOG_INFO(LOG_DATABASE, "Error getting history items count");
            return result_code::other;
        }

        uint64_t id = history_count;

        // This results in a complete and unambiguous history for the
        // address since standard outputs contain unambiguous address data.
        for (auto const& address : prevout.validation.cache.addresses()) {
            auto valuearr = history_entry::factory_to_data(id, inpoint, domain::chain::point_kind::spend, height, inpoint.index(), prevout.checksum());
            auto res = insert_history_db(address, valuearr, batch);
            if (res != result_code::success) {
                return res;
            }
            ++id;
        }
    } else {
        //During an IBD with checkpoints some previous output info is missing.
        //We can recover it by accessing the database
        auto entry = get_utxo(prevout);

        if (entry.is_valid()) {
            uint64_t history_count = get_history_count();
            if (history_count == max_uint64) {
                LOG_INFO(LOG_DATABASE, "Error getting history items count");
                return result_code::other;
            }

            uint64_t id = history_count;

            auto const& out_output = entry.output();
            for (auto const& address : out_output.addresses()) {
                auto valuearr = history_entry::factory_to_data(id, inpoint, domain::chain::point_kind::spend, height, inpoint.index(), prevout.checksum());
                auto res = insert_history_db(address, valuearr, batch);
                if (res != result_code::success) {
                    return res;
                }
                ++id;
            }
        }
        else {
            LOG_INFO(LOG_DATABASE, "Error finding UTXO for input history [insert_input_history]");
        }
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_output_history(hash_digest const& tx_hash, uint32_t height, uint32_t index, domain::chain::output const& output, leveldb::WriteBatch& batch) {
    uint64_t history_count = get_history_count();
    if (history_count == max_uint64) {
        LOG_INFO(LOG_DATABASE, "Error getting history items count");
        return result_code::other;
    }

    uint64_t id = history_count;

    auto const outpoint = domain::chain::output_point {tx_hash, index};
    auto const value = output.value();

    // Standard outputs contain unambiguous address data.
    for (auto const& address : output.addresses()) {
        auto valuearr = history_entry::factory_to_data(id, outpoint, domain::chain::point_kind::output, height, index, value);
        auto res = insert_history_db(address, valuearr, batch);
        if (res != result_code::success) {
            return res;
        }
        ++id;
    }

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
// static
domain::chain::history_compact internal_database_basis<Clock>::history_entry_to_history_compact(history_entry const& entry) {
    return domain::chain::history_compact{entry.point_kind(), entry.point(), entry.height(), entry.value_or_checksum()};
}

template <typename Clock>
domain::chain::history_compact::list internal_database_basis<Clock>::get_history(short_hash const& key, size_t limit, size_t from_height) const {
    domain::chain::history_compact::list result;

    if (limit == 0) {
        return result;
    }

    // Prefix for our keys is the address hash
    std::string prefix(reinterpret_cast<const char*>(key.data()), key.size());
    leveldb::Slice prefix_slice(prefix.data(), prefix.size());
    
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_history_db_));
    
    // Seek to the first key with our prefix
    it->Seek(prefix_slice);
    
    // Iterate through all keys with our prefix
    while (it->Valid()) {
        leveldb::Slice current_key = it->key();
        
        // Check if the current key still starts with our prefix
        if (current_key.size() < prefix.size() ||
            memcmp(current_key.data(), prefix.data(), prefix.size()) != 0) {
            break;
        }
        
        // Extract the value
        leveldb::Slice value_slice = it->value();
        data_chunk data(value_slice.data(), value_slice.data() + value_slice.size());
        auto entry = domain::create_old<history_entry>(data);
        
        // Filter by height if needed
        if (from_height == 0 || entry.height() >= from_height) {
            result.push_back(history_entry_to_history_compact(entry));
        }
        
        // Check limit
        if (limit > 0 && result.size() >= limit) {
            break;
        }
        
        it->Next();
    }

    return result;
}

template <typename Clock>
std::vector<hash_digest> internal_database_basis<Clock>::get_history_txns(short_hash const& key, size_t limit, size_t from_height) const {
    std::set<hash_digest> temp;
    std::vector<hash_digest> result;

    // Stop once we reach the limit (if specified).
    if (limit == 0)
        return result;

    // Prefix for our keys is the address hash
    std::string prefix(reinterpret_cast<const char*>(key.data()), key.size());
    leveldb::Slice prefix_slice(prefix.data(), prefix.size());
    
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_history_db_));
    
    // Seek to the first key with our prefix
    it->Seek(prefix_slice);
    
    // Iterate through all keys with our prefix
    while (it->Valid()) {
        leveldb::Slice current_key = it->key();
        
        // Check if the current key still starts with our prefix
        if (current_key.size() < prefix.size() ||
            memcmp(current_key.data(), prefix.data(), prefix.size()) != 0) {
            break;
        }
        
        // Extract the value
        leveldb::Slice value_slice = it->value();
        data_chunk data(value_slice.data(), value_slice.data() + value_slice.size());
        auto entry = domain::create_old<history_entry>(data);
        
        // Filter by height if needed
        if (from_height == 0 || entry.height() >= from_height) {
            // Avoid inserting the same tx
            auto const & pair = temp.insert(entry.point().hash());
            if (pair.second){
                // Add valid txns to the result vector
                result.push_back(*pair.first);
            }
        }
        
        // Check limit
        if (limit > 0 && result.size() >= limit) {
            break;
        }
        
        it->Next();
    }

    return result;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::remove_transaction_history_db(domain::chain::transaction const& tx, size_t height, leveldb::WriteBatch& batch) {
    for (auto const& output: tx.outputs()) {
        for (auto const& address : output.addresses()) {
            auto res = remove_history_db(address.hash20(), height, batch);
            if (res != result_code::success) {
                return res;
            }
        }
    }

    for (auto const& input: tx.inputs()) {
        auto const& prevout = input.previous_output();

        if (prevout.validation.cache.is_valid()) {
            for (auto const& address : prevout.validation.cache.addresses()) {
                auto res = remove_history_db(address.hash20(), height, batch);
                if (res != result_code::success) {
                    return res;
                }
            }
        }
        else {
            auto const& entry = get_transaction(prevout.hash(), max_uint32);

            if (entry.is_valid()) {
                auto const& tx = entry.transaction();
                auto const& out_output = tx.outputs()[prevout.index()];

                for (auto const& address : out_output.addresses()) {
                    auto res = remove_history_db(address.hash20(), height, batch);
                    if (res != result_code::success) {
                        return res;
                    }
                }
            }
        }
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_history_db(short_hash const& key, size_t height, leveldb::WriteBatch& batch) {
    // Prefix for our keys is the address hash
    std::string prefix(reinterpret_cast<const char*>(key.data()), key.size());
    leveldb::Slice prefix_slice(prefix.data(), prefix.size());
    
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_history_db_));
    
    // Seek to the first key with our prefix
    it->Seek(prefix_slice);
    
    // Collect keys to delete
    std::vector<std::string> keys_to_delete;
    
    // Iterate through all keys with our prefix
    while (it->Valid()) {
        leveldb::Slice current_key = it->key();
        
        // Check if the current key still starts with our prefix
        if (current_key.size() < prefix.size() ||
            memcmp(current_key.data(), prefix.data(), prefix.size()) != 0) {
            break;
        }
        
        // Extract the value
        leveldb::Slice value_slice = it->value();
        data_chunk data(value_slice.data(), value_slice.data() + value_slice.size());
        auto entry = domain::create_old<history_entry>(data);
        
        // Check if this entry matches the height we're removing
        if (entry.height() == height) {
            keys_to_delete.push_back(std::string(current_key.data(), current_key.size()));
        }
        
        it->Next();
    }
    
    // Delete all matching entries
    for (const auto& key_to_delete : keys_to_delete) {
        batch.Delete(cf_history_db_, leveldb::Slice(key_to_delete.data(), key_to_delete.size()));
    }

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
uint64_t internal_database_basis<Clock>::get_history_count() const {
    // For LevelDB, we need to manually count the entries
    uint64_t count = 0;
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_history_db_));
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        count++;
    }
    
    if (!it->status().ok()) {
        return max_uint64;
    }
    
    return count;
}

} // namespace kth::database

#endif // KTH_DATABASE_HISTORY_DATABASE_IPP_

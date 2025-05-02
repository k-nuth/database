// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_HPP_
#define KTH_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_HPP_

#include <kth/infrastructure/log/source.hpp>

namespace kth::database {

template <typename Clock>
transaction_unconfirmed_entry internal_database_basis<Clock>::get_transaction_unconfirmed(hash_digest const& hash) const {
    leveldb::Slice key(reinterpret_cast<const char*>(hash.data()), hash.size());
    std::string value;
    
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_transaction_unconfirmed_db_, key, &value);
    if (!status.ok()) {
        return {};
    }
    
    data_chunk data(value.begin(), value.end());
    auto res = domain::create_old<transaction_unconfirmed_entry>(data);
    
    return res;
}

template <typename Clock>
transaction_unconfirmed_entry internal_database_basis<Clock>::get_transaction_unconfirmed(hash_digest const& hash, KTH_DB_txn* /*unused*/) const {
    return get_transaction_unconfirmed(hash);
}

template <typename Clock>
std::vector<transaction_unconfirmed_entry> internal_database_basis<Clock>::get_all_transaction_unconfirmed() const {
    std::vector<transaction_unconfirmed_entry> result;
    
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_transaction_unconfirmed_db_));
    
    // Iterate through all entries
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        leveldb::Slice value_slice = it->value();
        data_chunk data(value_slice.data(), value_slice.data() + value_slice.size());
        auto entry = domain::create_old<transaction_unconfirmed_entry>(data);
        result.push_back(entry);
    }
    
    if (!it->status().ok()) {
        // Log error if needed
        return {};
    }
    
    return result;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::remove_transaction_unconfirmed(hash_digest const& tx_id, leveldb::WriteBatch& batch) {
    leveldb::Slice key(reinterpret_cast<const char*>(tx_id.data()), tx_id.size());
    
    // Check if key exists before deleting
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_transaction_unconfirmed_db_, key, &value);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            //LOG_DEBUG(LOG_DATABASE, "Key not found deleting transaction unconfirmed DB in LevelDB [remove_transaction_unconfirmed]");
            return result_code::key_not_found;
        }
        LOG_INFO(LOG_DATABASE, "Error checking transaction unconfirmed DB in LevelDB [remove_transaction_unconfirmed]");
        return result_code::other;
    }
    
    // Delete the transaction
    batch.Delete(cf_transaction_unconfirmed_db_, key);
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
inline
uint32_t internal_database_basis<Clock>::get_clock_now() const {
    auto const now = std::chrono::high_resolution_clock::now();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_transaction_unconfirmed(domain::chain::transaction const& tx, uint32_t height, leveldb::WriteBatch& batch) {
    uint32_t arrival_time = get_clock_now();
    
    auto const& hash = tx.hash();
    leveldb::Slice key(reinterpret_cast<const char*>(hash.data()), hash.size());
    
    // Check if key already exists to prevent overwrite
    std::string existing_value;
    leveldb::Status check_status = db_->Get(leveldb::ReadOptions(), cf_transaction_unconfirmed_db_, key, &existing_value);
    if (check_status.ok()) {
        LOG_INFO(LOG_DATABASE, "Duplicate key in Transaction Unconfirmed DB [insert_transaction_unconfirmed]");
        return result_code::duplicated_key;
    }
    
    auto value_arr = transaction_unconfirmed_entry::factory_to_data(tx, arrival_time, height);
    leveldb::Slice value(reinterpret_cast<const char*>(value_arr.data()), value_arr.size());
    
    // Add to batch
    batch.Put(cf_transaction_unconfirmed_db_, key, value);
    
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_HPP_

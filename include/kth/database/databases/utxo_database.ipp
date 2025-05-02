// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_UTXO_DATABASE_HPP_
#define KTH_DATABASE_UTXO_DATABASE_HPP_

#include <kth/infrastructure/log/source.hpp>

namespace kth::database {

template <typename Clock>
utxo_entry internal_database_basis<Clock>::get_utxo(domain::chain::output_point const& point) const {
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);
    leveldb::Slice key(reinterpret_cast<const char*>(keyarr.data()), keyarr.size());
    
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_utxo_db_, key, &value);
    
    if (!status.ok()) {
        if (status.IsNotFound()) {
            LOG_DEBUG(LOG_DATABASE, "Key not found getting UTXO [get_utxo]");
        } else {
            LOG_INFO(LOG_DATABASE, "Error getting UTXO [get_utxo]");
        }
        return {};
    }
    
    data_chunk data(value.begin(), value.end());
    return domain::create_old<utxo_entry>(data);
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::remove_utxo(uint32_t height, domain::chain::output_point const& point, bool insert_reorg, leveldb::WriteBatch& batch) {
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);
    leveldb::Slice key(reinterpret_cast<const char*>(keyarr.data()), keyarr.size());

    if (insert_reorg) {
        auto res0 = insert_reorg_pool(height, point, batch);
        if (res0 != result_code::success) return res0;
    }

    // Check if key exists before deleting
    std::string existing_value;
    leveldb::Status check_status = db_->Get(leveldb::ReadOptions(), cf_utxo_db_, key, &existing_value);
    if (!check_status.ok()) {
        if (check_status.IsNotFound()) {
            LOG_INFO(LOG_DATABASE, "Key not found deleting UTXO [remove_utxo]");
            return result_code::key_not_found;
        }
        LOG_INFO(LOG_DATABASE, "Error checking UTXO [remove_utxo]");
        return result_code::other;
    }
    
    // Delete from database
    batch.Delete(cf_utxo_db_, key);
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_utxo(domain::chain::output_point const& point, domain::chain::output const& output, data_chunk const& fixed_data, leveldb::WriteBatch& batch) {
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);
    auto valuearr = utxo_entry::to_data_with_fixed(output, fixed_data);

    leveldb::Slice key(reinterpret_cast<const char*>(keyarr.data()), keyarr.size());
    leveldb::Slice value(reinterpret_cast<const char*>(valuearr.data()), valuearr.size());
    
    // Check if key already exists to prevent overwrite (simulating KTH_DB_NOOVERWRITE)
    std::string existing_value;
    leveldb::Status check_status = db_->Get(leveldb::ReadOptions(), cf_utxo_db_, key, &existing_value);
    if (check_status.ok()) {
        LOG_DEBUG(LOG_DATABASE, "Duplicate Key inserting UTXO [insert_utxo]");
        return result_code::duplicated_key;
    }
    
    // Insert into database
    batch.Put(cf_utxo_db_, key, value);
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_utxos(hash_digest const& tx_hash, size_t tx_height, domain::chain::transaction const& tx, leveldb::WriteBatch& batch) {
    uint32_t index = 0;
    auto const& outputs = tx.outputs();
    
    for (auto const& output : outputs) {
        auto point = domain::chain::output_point{tx_hash, index};
        
        // Store tx height with output
        auto fixed_data = to_chunk(tx_height);
        
        auto res = insert_utxo(point, output, fixed_data, batch);
        if (res != result_code::success && res != result_code::duplicated_key) {
            return res;
        }
        
        ++index;
    }
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::spend_utxos(domain::chain::transaction const& tx, uint32_t height, bool insert_reorg, leveldb::WriteBatch& batch) {
    auto const& inputs = tx.inputs();
    
    for (auto const& input : inputs) {
        auto const& prevout = input.previous_output();
        
        if (prevout.validation.cache.is_valid()) {
            auto res = remove_utxo(height, prevout, insert_reorg, batch);
            if (res != result_code::success && res != result_code::key_not_found) {
                return res;
            }
        }
    }
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::populate_utxo_set(domain::chain::transaction const& tx, uint32_t height, leveldb::WriteBatch& batch) {
    auto const& hash = tx.hash();
    
    // Add outputs to UTXO set
    auto res = insert_utxos(hash, height, tx, batch);
    if (res != result_code::success) {
        return res;
    }
    
    // Remove inputs from UTXO set
    res = spend_utxos(tx, height, true, batch);
    if (res != result_code::success) {
        return res;
    }
    
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_UTXO_DATABASE_HPP_

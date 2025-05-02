// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_SPEND_DATABASE_IPP_
#define KTH_DATABASE_SPEND_DATABASE_IPP_

#include <kth/infrastructure/log/source.hpp>

namespace kth::database {

//public
template <typename Clock>
domain::chain::input_point internal_database_basis<Clock>::get_spend(domain::chain::output_point const& point) const {
    // Convert output point to key
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);
    leveldb::Slice key(reinterpret_cast<const char*>(keyarr.data()), keyarr.size());
    
    // Get value from database
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_spend_db_, key, &value);
    
    // Check if the key was found
    if (!status.ok()) {
        if (status.IsNotFound()) {
            LOG_INFO(LOG_DATABASE, "Key not found getting spend [get_spend]");
        } else {
            LOG_INFO(LOG_DATABASE, "Error getting spend [get_spend]");
        }
        return domain::chain::input_point{};
    }
    
    // Convert value to data chunk
    data_chunk data(value.begin(), value.end());
    
    // Deserialize input point
    auto res = domain::create_old<domain::chain::input_point>(data);
    return res;
}

#if ! defined(KTH_DB_READONLY)

//private
template <typename Clock>
result_code internal_database_basis<Clock>::insert_spend(domain::chain::output_point const& out_point, domain::chain::input_point const& in_point, leveldb::WriteBatch& batch) {
    // Convert output point to key
    auto keyarr = out_point.to_data(KTH_INTERNAL_DB_WIRE);
    leveldb::Slice key(reinterpret_cast<const char*>(keyarr.data()), keyarr.size());
    
    // Check if key already exists to prevent overwrite
    std::string existing_value;
    leveldb::Status check_status = db_->Get(leveldb::ReadOptions(), cf_spend_db_, key, &existing_value);
    if (check_status.ok()) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting spend [insert_spend]");
        return result_code::duplicated_key;
    }
    
    // Convert input point to value
    auto value_arr = in_point.to_data();
    leveldb::Slice value(reinterpret_cast<const char*>(value_arr.data()), value_arr.size());
    
    // Add to batch
    batch.Put(cf_spend_db_, key, value);
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_transaction_spend_db(domain::chain::transaction const& tx, leveldb::WriteBatch& batch) {
    for (auto const& input: tx.inputs()) {
        auto const& prevout = input.previous_output();
        
        auto res = remove_spend(prevout, batch);
        if (res != result_code::success) {
            return res;
        }
        
        /*res = set_unspend(prevout, batch);
        if (res != result_code::success) {
            return res;
        }*/
    }
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_spend(domain::chain::output_point const& out_point, leveldb::WriteBatch& batch) {
    // Convert output point to key
    auto keyarr = out_point.to_data(KTH_INTERNAL_DB_WIRE);
    leveldb::Slice key(reinterpret_cast<const char*>(keyarr.data()), keyarr.size());
    
    // Check if key exists before deleting
    std::string existing_value;
    leveldb::Status check_status = db_->Get(leveldb::ReadOptions(), cf_spend_db_, key, &existing_value);
    if (!check_status.ok()) {
        if (check_status.IsNotFound()) {
            LOG_INFO(LOG_DATABASE, "Key not found deleting spend [remove_spend]");
            return result_code::key_not_found;
        } else {
            LOG_INFO(LOG_DATABASE, "Error checking spend [remove_spend]");
            return result_code::other;
        }
    }
    
    // Add delete operation to batch
    batch.Delete(cf_spend_db_, key);
    
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_SPEND_DATABASE_IPP_

// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_REORG_DATABASE_HPP_
#define KTH_DATABASE_REORG_DATABASE_HPP_

#include <kth/infrastructure/log/source.hpp>

namespace kth::database {

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_reorg_pool(uint32_t height, const domain::chain::output_point& point, leveldb::WriteBatch& batch) {
    // Convert point to key
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);
    leveldb::Slice key(reinterpret_cast<const char*>(keyarr.data()), keyarr.size());
    
    // Get the UTXO value
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_utxo_db_, key, &value);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            LOG_INFO(LOG_DATABASE, "Key not found getting UTXO [insert_reorg_pool]");
            return result_code::key_not_found;
        }
        LOG_INFO(LOG_DATABASE, "Error getting UTXO [insert_reorg_pool]");
        return result_code::other;
    }
    
    // Put the UTXO in the reorg pool
    batch.Put(cf_reorg_pool_, key, value);
    
    // Create index entry: height -> point
    leveldb::Slice height_key(reinterpret_cast<const char*>(&height), sizeof(height));
    leveldb::Slice point_value(reinterpret_cast<const char*>(keyarr.data()), keyarr.size());
    batch.Put(cf_reorg_index_, height_key, point_value);
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_block_reorg(domain::chain::block const& block, uint32_t height, leveldb::WriteBatch& batch) {
    // Serialize the block
    auto valuearr = block.to_data(false);
    
    // Create height key and block value
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));
    leveldb::Slice value(reinterpret_cast<const char*>(valuearr.data()), valuearr.size());
    
    // Put the block in the reorg blocks
    batch.Put(cf_reorg_block_, key, value);
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_output_from_reorg_and_remove(domain::chain::output_point const& point, leveldb::WriteBatch& batch) {
    // Convert point to key
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);
    leveldb::Slice key(reinterpret_cast<const char*>(keyarr.data()), keyarr.size());
    
    // Get the value from reorg pool
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_reorg_pool_, key, &value);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            LOG_INFO(LOG_DATABASE, "Key not found in reorg pool [insert_output_from_reorg_and_remove]");
            return result_code::key_not_found;
        }
        LOG_INFO(LOG_DATABASE, "Error in reorg pool [insert_output_from_reorg_and_remove]");
        return result_code::other;
    }
    
    // Put it back in the UTXO set
    batch.Put(cf_utxo_db_, key, value);
    
    // Remove from reorg pool
    batch.Delete(cf_reorg_pool_, key);
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_block_reorg(uint32_t height, leveldb::WriteBatch& batch) {
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));
    
    // Check if key exists before deleting
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_reorg_block_, key, &value);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            LOG_INFO(LOG_DATABASE, "Key not found deleting reorg block in LevelDB [remove_block_reorg]");
            return result_code::key_not_found;
        }
        LOG_INFO(LOG_DATABASE, "Error checking reorg block in LevelDB [remove_block_reorg]");
        return result_code::other;
    }
    
    // Delete the block
    batch.Delete(cf_reorg_block_, key);
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_reorg_index(uint32_t height, leveldb::WriteBatch& batch) {
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));
    
    // Check if key exists before deleting
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_reorg_index_, key, &value);
    if (!status.ok()) {
        if (status.IsNotFound()) {
            LOG_DEBUG(LOG_DATABASE, "Key not found deleting reorg index in LevelDB [remove_reorg_index] - height: ", height);
            return result_code::key_not_found;
        }
        LOG_DEBUG(LOG_DATABASE, "Error checking reorg index in LevelDB [remove_reorg_index] - height: ", height);
        return result_code::other;
    }
    
    // Delete the index entry
    batch.Delete(cf_reorg_index_, key);
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
domain::chain::block internal_database_basis<Clock>::get_block_reorg(uint32_t height, KTH_DB_txn* /*unused*/) const {
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));
    std::string value;
    
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_reorg_block_, key, &value);
    if (!status.ok()) {
        return {};
    }
    
    data_chunk data(value.begin(), value.end());
    auto res = domain::create_old<domain::chain::block>(data);
    return res;
}

template <typename Clock>
domain::chain::block internal_database_basis<Clock>::get_block_reorg(uint32_t height) const {
    return get_block_reorg(height, nullptr);
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::prune_reorg_index(uint32_t remove_until, leveldb::WriteBatch& batch) {
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_reorg_index_));
    
    // Collect keys to delete
    std::vector<std::string> height_keys_to_delete;
    std::vector<std::string> pool_keys_to_delete;
    
    // Iterate through all entries
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        leveldb::Slice key_slice = it->key();
        uint32_t current_height = *reinterpret_cast<const uint32_t*>(key_slice.data());
        
        if (current_height < remove_until) {
            // Add height key to delete list
            height_keys_to_delete.push_back(std::string(key_slice.data(), key_slice.size()));
            
            // Add pool key to delete list
            leveldb::Slice value_slice = it->value();
            pool_keys_to_delete.push_back(std::string(value_slice.data(), value_slice.size()));
        } else {
            break;
        }
    }
    
    if (!it->status().ok()) {
        return result_code::other;
    }
    
    // Delete all entries from reorg pool
    for (const auto& pool_key : pool_keys_to_delete) {
        batch.Delete(cf_reorg_pool_, leveldb::Slice(pool_key.data(), pool_key.size()));
    }
    
    // Delete all entries from reorg index
    for (const auto& height_key : height_keys_to_delete) {
        batch.Delete(cf_reorg_index_, leveldb::Slice(height_key.data(), height_key.size()));
    }
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::prune_reorg_block(uint32_t amount_to_delete, leveldb::WriteBatch& batch) {
    // Precondition: amount_to_delete >= 1
    
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_reorg_block_));
    
    // Collect keys to delete
    std::vector<std::string> keys_to_delete;
    uint32_t deleted = 0;
    
    // Iterate through entries
    for (it->SeekToFirst(); it->Valid() && deleted < amount_to_delete; it->Next()) {
        leveldb::Slice key_slice = it->key();
        keys_to_delete.push_back(std::string(key_slice.data(), key_slice.size()));
        deleted++;
    }
    
    if (!it->status().ok()) {
        return result_code::other;
    }
    
    // Delete all collected keys
    for (const auto& key : keys_to_delete) {
        batch.Delete(cf_reorg_block_, leveldb::Slice(key.data(), key.size()));
    }
    
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::get_first_reorg_block_height(uint32_t& out_height) const {
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_reorg_block_));
    
    it->SeekToFirst();
    if (!it->Valid()) {
        return result_code::db_empty;
    }
    
    leveldb::Slice key_slice = it->key();
    if (key_slice.size() != sizeof(uint32_t)) {
        return result_code::other;
    }
    
    out_height = *reinterpret_cast<const uint32_t*>(key_slice.data());
    
    return result_code::success;
}

} // namespace kth::database

#endif // KTH_DATABASE_REORG_DATABASE_HPP_

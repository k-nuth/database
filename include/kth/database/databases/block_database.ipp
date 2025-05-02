// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_BLOCK_DATABASE_IPP_
#define KTH_DATABASE_BLOCK_DATABASE_IPP_

#include <kth/infrastructure/log/source.hpp>

namespace kth::database {

//public
template <typename Clock>
std::pair<domain::chain::block, uint32_t> internal_database_basis<Clock>::get_block(hash_digest const& hash) const {
    // Create key from hash
    leveldb::Slice key(reinterpret_cast<const char*>(hash.data()), hash.size());
    std::string value;

    // Get height by hash
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_block_header_by_hash_, key, &value);
    if (!status.ok()) {
        return {};
    }

    uint32_t height = *reinterpret_cast<const uint32_t*>(value.data());
    auto block = get_block(height);

    return {block, height};
}

//public
template <typename Clock>
domain::chain::block internal_database_basis<Clock>::get_block(uint32_t height) const {
    return get_block(height, nullptr);
}

template <typename Clock>
domain::chain::block internal_database_basis<Clock>::get_block(uint32_t height, KTH_DB_txn* /*unused*/) const {
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));

    if (db_mode_ == db_mode_type::full) {
        auto header = get_header(height);
        if (!header.is_valid()) {
            return {};
        }

        domain::chain::transaction::list tx_list;

        // Iterate through all entries with this key
        std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_block_db_));
        
        // Seek to the first entry with this height
        it->Seek(key);
        
        // Verify we found a valid entry
        if (!it->Valid() || it->key().compare(key) != 0) {
            return {};
        }

        do {
            if (it->key().compare(key) != 0) {
                break; // No more entries with this key
            }

            auto tx_id = *reinterpret_cast<const uint32_t*>(it->value().data());
            auto const entry = get_transaction(tx_id);

            if (!entry.is_valid()) {
                return {};
            }

            tx_list.push_back(std::move(entry.transaction()));
            it->Next();
        } while (it->Valid());

        return domain::chain::block{header, std::move(tx_list)};
    } else if (db_mode_ == db_mode_type::blocks) {
        std::string value;
        leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_block_db_, key, &value);
        
        if (!status.ok()) {
            return domain::chain::block{};
        }

        data_chunk data(value.begin(), value.end());
        auto res = domain::create_old<domain::chain::block>(data);
        return res;
    }
    
    // db_mode_ == db_mode_type::pruned
    auto block = get_block_reorg(height);
    return block;
}


#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_block(domain::chain::block const& block, uint32_t height, uint64_t tx_count, leveldb::WriteBatch& batch) {
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));

    if (db_mode_ == db_mode_type::full) {
        auto const& txs = block.transactions();

        for (uint64_t i = tx_count; i < tx_count + txs.size(); ++i) {
            // For LevelDB to support duplicates, we need to encode the duplicate info in the key
            // We could use a composite key format like: height + separator + i
            // Or use a specific value serialization method
            
            // Here's a simple approach - the value contains the tx_id
            leveldb::Slice value(reinterpret_cast<const char*>(&i), sizeof(i));
            
            // Create a composite key that includes the height and the transaction index
            // This allows us to retrieve all transactions for a block by prefix scanning
            std::string composite_key;
            composite_key.append(reinterpret_cast<const char*>(&height), sizeof(height));
            composite_key.append(reinterpret_cast<const char*>(&i), sizeof(i));
            
            leveldb::Slice comp_key(composite_key.data(), composite_key.size());
            
            batch.Put(cf_block_db_, comp_key, value);
        }
    } else if (db_mode_ == db_mode_type::blocks) {
        //TODO: store tx hash
        auto data = block.to_data(false);
        leveldb::Slice value(reinterpret_cast<const char*>(data.data()), data.size());

        batch.Put(cf_block_db_, key, value);
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_blocks_db(uint32_t height, leveldb::WriteBatch& batch) {
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));

    if (db_mode_ == db_mode_type::full) {
        // For LevelDB, we need to find all keys with the specified height prefix
        std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_block_db_));
        
        // Create a prefix to find all entries with this height
        std::string prefix;
        prefix.append(reinterpret_cast<const char*>(&height), sizeof(height));
        leveldb::Slice prefix_slice(prefix.data(), prefix.size());
        
        // Seek to the first entry with this height
        it->Seek(prefix_slice);
        
        // Delete all entries that match this prefix
        while (it->Valid()) {
            leveldb::Slice current_key = it->key();
            
            // Check if current key still starts with our prefix
            if (current_key.size() < prefix.size() || 
                memcmp(current_key.data(), prefix.data(), prefix.size()) != 0) {
                break;
            }
            
            batch.Delete(cf_block_db_, current_key);
            it->Next();
        }
    } else if (db_mode_ == db_mode_type::blocks) {
        batch.Delete(cf_block_db_, key);
    }

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_BLOCK_DATABASE_IPP_

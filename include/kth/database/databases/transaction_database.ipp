// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TRANSACTION_DATABASE_HPP_
#define KTH_DATABASE_TRANSACTION_DATABASE_HPP_

#include <kth/infrastructure/log/source.hpp>

namespace kth::database {

//public
template <typename Clock>
transaction_entry internal_database_basis<Clock>::get_transaction(hash_digest const& hash, size_t fork_height) const {
    auto entry = get_transaction(hash, fork_height, nullptr);
    return entry;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::insert_transactions(I f, I l, uint32_t height, uint32_t median_time_past, uint64_t tx_count, leveldb::WriteBatch& batch) {
    auto id = tx_count;
    uint32_t pos = 0;

    while (f != l) {
        auto const& tx = *f;

        //TODO: (Mario) : Implement tx.Confirm to update existing transactions
        auto res = insert_transaction(id, tx, height, median_time_past, pos, batch);
        if (res != result_code::success && res != result_code::duplicated_key) {
            return res;
        }

        //remove unconfirmed transaction if exists
        //TODO (Mario): don't recalculate tx.hash
        res = remove_transaction_unconfirmed(tx.hash(), batch);
        if (res != result_code::success && res != result_code::key_not_found) {
            return res;
        }

        ++f;
        ++pos;
        ++id;
    }

    return result_code::success;
}
#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
transaction_entry internal_database_basis<Clock>::get_transaction(uint64_t id, KTH_DB_txn* /*unused*/) const {
    leveldb::Slice key(reinterpret_cast<const char*>(&id), sizeof(id));
    std::string value;
    
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_transaction_db_, key, &value);
    if (!status.ok()) {
        return {};
    }
    
    data_chunk data(value.begin(), value.end());
    auto entry = domain::create_old<transaction_entry>(data);

    return entry;
}

template <typename Clock>
transaction_entry internal_database_basis<Clock>::get_transaction(hash_digest const& hash, size_t fork_height, KTH_DB_txn* /*unused*/) const {
    leveldb::Slice key(reinterpret_cast<const char*>(hash.data()), hash.size());
    std::string value;
    
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_transaction_hash_db_, key, &value);
    if (!status.ok()) {
        return {};
    }
    
    uint32_t tx_id = *reinterpret_cast<const uint32_t*>(value.data());
    
    auto const entry = get_transaction(tx_id, nullptr);
    
    if (entry.height() > fork_height) {
        return {};
    }
    
    //Note(Knuth): Transaction stored in cf_transaction_db_ are always confirmed
    //the parameter requiere_confirmed is never used.
    /*if (!require_confirmed) {
        return entry;
    }

    auto const confirmed = entry.confirmed();

    return (confirmed && entry.height() > fork_height) || (require_confirmed && ! confirmed) ? transaction_entry{} : entry;
    */
    
    return entry;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_transaction(uint64_t id, domain::chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position, leveldb::WriteBatch& batch) {
    leveldb::Slice key(reinterpret_cast<const char*>(&id), sizeof(id));
    
    auto valuearr = transaction_entry::factory_to_data(tx, height, median_time_past, position);
    leveldb::Slice value(reinterpret_cast<const char*>(valuearr.data()), valuearr.size());
    
    // Check if key already exists (simulating KTH_DB_APPEND behavior)
    std::string existing_value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_transaction_db_, key, &existing_value);
    if (status.ok()) {
        LOG_INFO(LOG_DATABASE, "Duplicate key in Transaction DB [insert_transaction]");
        return result_code::duplicated_key;
    }
    
    // Insert to transaction DB
    batch.Put(cf_transaction_db_, key, value);
    
    // Insert hash->id mapping
    auto const& hash = tx.hash();
    leveldb::Slice key_tx(reinterpret_cast<const char*>(hash.data()), hash.size());
    
    // Check if hash already exists (simulating KTH_DB_NOOVERWRITE behavior)
    status = db_->Get(leveldb::ReadOptions(), cf_transaction_hash_db_, key_tx, &existing_value);
    if (status.ok()) {
        LOG_INFO(LOG_DATABASE, "Duplicate key in Transaction Hash DB [insert_transaction]");
        return result_code::duplicated_key;
    }
    
    batch.Put(cf_transaction_hash_db_, key_tx, key);
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_transactions(domain::chain::block const& block, uint32_t height, leveldb::WriteBatch& batch) {
    auto const& txs = block.transactions();
    uint32_t pos = 0;
    
    for (auto const& tx : txs) {
        auto const& hash = tx.hash();
        
        auto res0 = remove_transaction_history_db(tx, height, batch);
        if (res0 != result_code::success) {
            return res0;
        }
        
        if (pos > 0) {
            auto res0 = remove_transaction_spend_db(tx, batch);
            if (res0 != result_code::success && res0 != result_code::key_not_found) {
                return res0;
            }
        }
        
        // Get transaction ID from hash
        leveldb::Slice key(reinterpret_cast<const char*>(hash.data()), hash.size());
        std::string value;
        
        leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_transaction_hash_db_, key, &value);
        if (!status.ok()) {
            return result_code::other;
        }
        
        uint32_t tx_id = *reinterpret_cast<const uint32_t*>(value.data());
        
        // Delete from transaction DB
        leveldb::Slice key_tx(reinterpret_cast<const char*>(&tx_id), sizeof(tx_id));
        batch.Delete(cf_transaction_db_, key_tx);
        
        // Delete from transaction hash DB
        batch.Delete(cf_transaction_hash_db_, key);
        
        ++pos;
    }
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::update_transaction(domain::chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position, leveldb::WriteBatch& batch) {
    auto const& hash = tx.hash();
    leveldb::Slice key_hash(reinterpret_cast<const char*>(hash.data()), hash.size());
    
    // Get the transaction ID
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_transaction_hash_db_, key_hash, &value);
    if (!status.ok()) {
        return result_code::key_not_found;
    }
    
    uint32_t tx_id = *reinterpret_cast<const uint32_t*>(value.data());
    leveldb::Slice key_tx(reinterpret_cast<const char*>(&tx_id), sizeof(tx_id));
    
    // Create updated entry
    auto valuearr = transaction_entry::factory_to_data(tx, height, median_time_past, position);
    leveldb::Slice value_tx(reinterpret_cast<const char*>(valuearr.data()), valuearr.size());
    
    // Update the transaction
    batch.Put(cf_transaction_db_, key_tx, value_tx);
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::set_spend(domain::chain::output_point const& point, uint32_t spender_height, leveldb::WriteBatch& batch) {
    // Limit search to confirmed transactions at or below the spender height,
    // since a spender cannot spend above its own height.
    // Transactions are not marked as spent unless the spender is confirmed.
    // This is consistent with support for unconfirmed double spends.
    
    auto entry = get_transaction(point.hash(), spender_height, nullptr);
    
    // The transaction does not exist as confirmed at or below the height.
    if (!entry.is_valid()) {
        return result_code::other;
    }
    
    auto const& tx = entry.transaction();
    
    if (point.index() >= tx.outputs().size()) {
        return result_code::other;
    }
    
    //update spender_height
    auto& output = tx.outputs()[point.index()];
    output.validation.spender_height = spender_height;
    
    //overwrite transaction
    auto ret = update_transaction(tx, entry.height(), entry.median_time_past(), entry.position(), batch);
    if (ret != result_code::success && ret != result_code::duplicated_key) {
        return ret;
    }
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::set_unspend(domain::chain::output_point const& point, leveldb::WriteBatch& batch) {
    return set_spend(point, domain::chain::output::validation::not_spent, batch);
}
#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
uint64_t internal_database_basis<Clock>::get_tx_count(KTH_DB_txn* /*unused*/) const {
    // For LevelDB, we need to manually count the entries
    uint64_t count = 0;
    std::unique_ptr<leveldb::Iterator> it(db_->NewIterator(leveldb::ReadOptions(), cf_transaction_db_));
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        count++;
    }
    
    if (!it->status().ok()) {
        return max_uint64;
    }
    
    return count;
}

} // namespace kth::database

#endif // KTH_DATABASE_TRANSACTION_DATABASE_HPP_

/**
 * Copyright (c) 2016-2018 Bitprim Inc.
 *
 * This file is part of Bitprim.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef BITPRIM_DATABASE_INDEX_SERVICE_IPP_
#define BITPRIM_DATABASE_INDEX_SERVICE_IPP_

namespace libbitcoin {
namespace database {


#if defined(BITPRIM_DB_NEW_FULL_ASYNC)

template <typename Clock>
result_code internal_database_basis<Clock>::start_indexing() {

    MDB_txn* db_txn;

    auto res = mdb_txn_begin(env_, NULL, 0, &db_txn);
    if (res != MDB_SUCCESS) {
        return result_code::other;
    }

    if (is_indexing_completed(db_txn)) {
        mdb_txn_abort(db_txn);
        LOG_INFO(LOG_DATABASE) << "Error starting indexing because the process is already done";
        return result_code::other;
    }

    //get block from height
    uint32_t last_height;
    auto res1 = get_last_height(last_height);
    if (res1 == result_code::db_empty) {
        mdb_txn_abort(db_txn);
        LOG_INFO(LOG_DATABASE) << "The database is empty";
        return res1;
    }
    else if (res1 != result_code::success)
    {
        mdb_txn_abort(db_txn);
        LOG_INFO(LOG_DATABASE) << "Error getting last height";
        return result_code::other;
    }

    LOG_INFO(LOG_DATABASE) << "Last height is " << last_height;

    //get last height indexed
    uint32_t last_indexed; 
    if ( ! get_last_indexed_height(last_indexed, db_txn)) {
        mdb_txn_abort(db_txn);
        LOG_INFO(LOG_DATABASE) << "Error getting last indexed height";
        return result_code::other;
    }
    
    LOG_INFO(LOG_DATABASE) << "Last indexed height is " << last_indexed;

    while (last_indexed < last_height) {
        ++last_indexed;

        //process block
        res1 = push_block_height(last_indexed, db_txn);
        if (res1 != result_code::success) {
            mdb_txn_abort(db_txn);
            LOG_INFO(LOG_DATABASE) << "Error indexing block " << static_cast<uint32_t>(res1);
            return res1;
        }

        //update last indexed height
        res1 = update_last_height_indexed(last_height, db_txn);
        if (res1 != result_code::success) {
            mdb_txn_abort(db_txn);
            LOG_INFO(LOG_DATABASE) << "Error updating last indexed height " << static_cast<uint32_t>(res1);
            return res1;
        }

        //delete raw block
        res1 = remove_serialized_blocks_db(last_height, db_txn);
        if (res1 != result_code::success) {
            mdb_txn_abort(db_txn);
            LOG_INFO(LOG_DATABASE) << "Error removing block " << static_cast<uint32_t>(res1);
            return res1;
        }

        LOG_INFO(LOG_DATABASE) << "Indexed height " << last_height;
    }

    //mark end of indexing
    auto res2 = set_indexing_completed(db_txn);
    if (res2 != result_code::success) {
        mdb_txn_abort(db_txn);
        LOG_INFO(LOG_DATABASE) << "Error setting indexing complete flag " << static_cast<uint32_t>(res2);
        return res2;
    }

    res = mdb_txn_commit(db_txn);
    if (res != MDB_SUCCESS) {
        return result_code::other;
    }

    LOG_INFO(LOG_DATABASE) << "Finished indexing at height " << last_height;

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_block_height(uint32_t height, MDB_txn* db_txn) {
    //get block by height
    auto const block = get_serialized_block(height, db_txn);
    auto const median_time_past = block.header().validation.median_time_past;
    auto const& txs = block.transactions();
    auto tx_count = get_tx_count(db_txn);
    //insert block_index
    auto res = insert_block_index_db(block, height, tx_count, db_txn);
    if (res != result_code::success) {
        return res;
    }

    auto id = tx_count;
    uint32_t pos = 0;
    for (auto const& tx : txs) {

        //insert transaction        
        res = push_transaction_index(id, tx, height, median_time_past, pos, db_txn);
        if (res != result_code::success) {
            return res;
        }

        //TODO (Mario): Remove transaction unconfirmed ?????

        ++pos;
        ++id;
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_block_index_db(chain::block block, uint32_t height, uint64_t tx_count, MDB_txn* db_txn) {

    MDB_val key {sizeof(height), &height};
    auto const& txs = block.transactions();
    
    for (uint64_t i = tx_count; i<tx_count + txs.size();i++ ) {

        MDB_val value {sizeof(i), &i};

        auto res = mdb_put(db_txn, dbi_blocks_index_db_, &key, &value, MDB_APPENDDUP);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate key in Block DB [insert_block_index_db] " << res;
            return result_code::duplicated_key;
        }

        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error saving in Block DB [insert_block_index_db] " << res;
            return result_code::other;
        }
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_transaction_index(uint64_t id, chain::transaction tx, uint32_t height, uint32_t median_time_past, uint32_t tx_pos, MDB_txn* db_txn) {

    auto res = insert_transaction(id, tx, height, median_time_past, tx_pos, db_txn);
    if (res != result_code::success && res != result_code::duplicated_key) {
        return res;
    }
        
    auto const& tx_hash = tx.hash();

    uint32_t pos = 0;
    for (auto const& output : tx.outputs()) {
        auto res = insert_output_history(tx_hash, height, pos, output, db_txn);
        if (res != result_code::success) {
            return res;
        }
        ++pos;
    }    

    pos = 0;
    for (auto const& input : tx.inputs()) {
        chain::input_point const inpoint {tx_hash, pos};
        auto const& prevout = input.previous_output();

        auto res = insert_input_history_transaction(inpoint, height, input, db_txn);            
        if (res != result_code::success) {
            return res;
        }

        res = insert_spend(prevout, inpoint, db_txn);
        if (res != result_code::success) {
            return res;
        }

        ++pos;
    }
    
    return result_code::success;
}


template <typename Clock>
bool internal_database_basis<Clock>::is_indexing_completed(MDB_txn* db_txn) {
    
    property_code property_code_ = property_code::indexing_completed;
    
    MDB_val key {sizeof(property_code_), &property_code_};
    MDB_val value;

    auto res = mdb_get(db_txn, dbi_properties_, &key, &value);
    
    if (res == MDB_NOTFOUND) {
        return false;
    }
    
    if (res != MDB_SUCCESS) {  
        //Error....
        return false;
    }

    return *static_cast<bool*>(value.mv_data);
}


template <typename Clock>
result_code internal_database_basis<Clock>::set_indexing_completed() {
    
    MDB_txn* db_txn;

    auto res = mdb_txn_begin(env_, NULL, 0, &db_txn);
    if (res != MDB_SUCCESS) {
        return result_code::other;
    }

    auto ret = set_indexing_completed(db_txn);
    if (ret != result_code::success) {
        mdb_txn_abort(db_txn);
        return result_code::other;
    }

    res = mdb_txn_commit(db_txn);
    if (res != MDB_SUCCESS) {
        return result_code::other;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::set_indexing_completed(MDB_txn* db_txn) {
    
    property_code property_code_ = property_code::indexing_completed;
    bool val_bool = true;
    
    MDB_val key {sizeof(property_code_), &property_code_};
    MDB_val value {sizeof(val_bool), &val_bool};

    auto res = mdb_put(db_txn, dbi_properties_, &key, &value, 0);
    if (res != MDB_SUCCESS) {  
        return result_code::other;
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::update_last_height_indexed(uint32_t height, MDB_txn* db_txn) {

    property_code property_code_ = property_code::last_indexed_height;
    
    MDB_val key {sizeof(property_code_), &property_code_};
    MDB_val value {sizeof(height), &height};

    auto res = mdb_put(db_txn, dbi_properties_, &key, &value, 0);
    if (res != MDB_SUCCESS) {  
        return result_code::other;
    }

    return result_code::success;
}


template <typename Clock>
bool internal_database_basis<Clock>::get_last_indexed_height(uint32_t& out_height, MDB_txn* db_txn) {
    
    property_code property_code_ = property_code::last_indexed_height;
    
    MDB_val key {sizeof(property_code_), &property_code_};
    MDB_val value;

    auto res = mdb_get(db_txn, dbi_properties_, &key, &value);
    
    if (res == MDB_NOTFOUND) {
        out_height = max_uint32;
        return true;
    }
    
    if (res != MDB_SUCCESS) {  
        return false;
    }

    out_height = *static_cast<uint32_t*>(value.mv_data);

    return true;
} 

#endif

} // namespace database
} // namespace libbitcoin


#endif // BITPRIM_DATABASE_INDEX_SERVICE_IPP_

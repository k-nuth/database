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
#ifndef BITPRIM_DATABASE_BLOCK_DATABASE_IPP_
#define BITPRIM_DATABASE_BLOCK_DATABASE_IPP_

namespace libbitcoin {
namespace database {

/*
#if defined(BITPRIM_DB_NEW_FULL)
    
template <typename Clock>
data_chunk internal_database_basis<Clock>::serialize_txs(chain::block const& block) {
    data_chunk ret;
    auto const& txs = block.transactions();
    ret.reserve(txs.size() * libbitcoin::hash_size);

    for (auto const& tx : txs) {
        auto hash = tx.hash();
        ret.insert(ret.end(), hash.begin(), hash.end());
    }
    return ret;
}

#endif // defined(BITPRIM_DB_NEW_FULL)
*/


#if defined(BITPRIM_DB_NEW_BLOCKS) || defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
//public
template <typename Clock>
std::pair<chain::block, uint32_t> internal_database_basis<Clock>::get_block(hash_digest const& hash) const {
    MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};

    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res != MDB_SUCCESS) {
        return {};
    }

    MDB_val value;
    if (mdb_get(db_txn, dbi_block_header_by_hash_, &key, &value) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        // mdb_txn_abort(db_txn);
        return {};
    }

    // assert value.mv_size == 4;
    auto height = *static_cast<uint32_t*>(value.mv_data);

    auto block = get_block(height, db_txn);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return {};
    }

    return {block, height};
}

//public
template <typename Clock>
chain::block internal_database_basis<Clock>::get_block(uint32_t height) const { 
    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res != MDB_SUCCESS) {
        return chain::block{};
    }

    auto block = get_block(height, db_txn);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return chain::block{};
    }

    return block;
}

#if defined(BITPRIM_DB_NEW_BLOCKS) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
template <typename Clock>
result_code internal_database_basis<Clock>::insert_block_serialized(chain::block const& block, uint32_t height, MDB_txn* db_txn) { 
    MDB_val key {sizeof(height), &height};
    auto data = block.to_data(false);
    MDB_val value {data.size(), data.data()};

    auto res = mdb_put(db_txn, dbi_block_db_, &key, &value, MDB_APPEND);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key in Block DB [insert_block_serialized] " << res;
        return result_code::duplicated_key;
    }

    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error saving in Block DB [insert_block_serialized] " << res;
        return result_code::other;
    }
    return result_code::success;    
}
#endif

#if defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
template <typename Clock>
result_code internal_database_basis<Clock>::insert_block_with_tx_id(chain::block const& block, uint32_t height, uint64_t tx_count, MDB_txn* db_txn) {

#if defined(BITPRIM_DB_NEW_FULL)
    MDB_dbi dbi_ = dbi_block_db_;
#else
    MDB_dbi dbi_ = dbi_blocks_index_db_;
#endif
  
    MDB_val key {sizeof(height), &height};
    auto const& txs = block.transactions();
    
    for (uint64_t i = tx_count; i < tx_count + txs.size(); i++ ) {

        MDB_val value {sizeof(i), &i};

        auto res = mdb_put(db_txn, dbi_, &key, &value, MDB_APPENDDUP);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate key in Block DB [insert_block_with_tx_id] " << res;
            return result_code::duplicated_key;
        }

        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error saving in Block DB [insert_block_with_tx_id] " << res;
            return result_code::other;
        }
    }

    return result_code::success;
}

#endif


template <typename Clock>
chain::block internal_database_basis<Clock>::get_block(uint32_t height, MDB_txn* db_txn) const {

    MDB_val key {sizeof(height), &height};
    
#if defined(BITPRIM_DB_NEW_BLOCKS)
    
    MDB_val value;

    if (mdb_get(db_txn, dbi_block_db_, &key, &value) != MDB_SUCCESS) {
        return chain::block{};
    }

    auto data = db_value_to_data_chunk(value);
    auto res = chain::block::factory_from_data(data);
    return res;


#elif defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)

#if defined(BITPRIM_DB_NEW_FULL)
    MDB_dbi dbi_ = dbi_block_db_;
#else
    MDB_dbi dbi_ = dbi_blocks_index_db_;
#endif

    auto header = get_header(height, db_txn);
    if ( ! header.is_valid() )
    {
        return {};
    }

    chain::transaction::list tx_list;

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_, &cursor) != MDB_SUCCESS) {
        return {};
    }

    MDB_val value;
    int rc;
    if ((rc = mdb_cursor_get(cursor, &key, &value, MDB_SET)) == 0) {
       
        auto tx_id = *static_cast<uint32_t*>(value.mv_data);;
        auto const entry = get_transaction(tx_id, db_txn);
        tx_list.push_back(std::move(entry.transaction()));
    
        while ((rc = mdb_cursor_get(cursor, &key, &value, MDB_NEXT_DUP)) == 0) {
            auto tx_id = *static_cast<uint32_t*>(value.mv_data);;
            auto const entry = get_transaction(tx_id, db_txn);
            tx_list.push_back(std::move(entry.transaction()));    
        }
    } 
    
    mdb_cursor_close(cursor);

    return chain::block{header, std::move(tx_list)};
#endif //defined(BITPRIM_DB_NEW_FULL)
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_blocks_db(uint32_t height, MDB_txn* db_txn) {
    
    MDB_val key {sizeof(height), &height};
    
 #if defined(BITPRIM_DB_NEW_BLOCKS)   
    
    auto res = mdb_del(db_txn, dbi_block_db_, &key, NULL);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE) << "Key not found deleting blocks DB in LMDB [remove_blocks_db] - mdb_del: " << res;
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error deleting blocks DB in LMDB [remove_blocks_db] - mdb_del: " << res;
        return result_code::other;
    }

#elif defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)

#if defined(BITPRIM_DB_NEW_FULL)
    MDB_dbi dbi_ = dbi_block_db_;
#else
    MDB_dbi dbi_ = dbi_blocks_index_db_;
#endif

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_, &cursor) != MDB_SUCCESS) {
        return {};
    }

    MDB_val value;
    int rc;
    if ((rc = mdb_cursor_get(cursor, &key, &value, MDB_SET)) == 0) {
           
        if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
            mdb_cursor_close(cursor);
            return result_code::other;
        }
        
        while ((rc = mdb_cursor_get(cursor, &key, &value, MDB_NEXT_DUP)) == 0) {
            if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
                mdb_cursor_close(cursor);
                return result_code::other;
            }    
        }
    } 
    
    mdb_cursor_close(cursor);

#endif

    return result_code::success;
}
#endif //defined(BITPRIM_DB_NEW_BLOCKS) || defined(BITPRIM_DB_NEW_FULL)


} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_BLOCK_DATABASE_IPP_

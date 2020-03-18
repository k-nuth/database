// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_REORG_DATABASE_HPP_
#define KTH_DATABASE_REORG_DATABASE_HPP_

namespace kth::database {

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_reorg_pool(uint32_t height, MDB_val& key, MDB_txn* db_txn) {
    MDB_val value;
    //TODO: use cursors
    auto res = mdb_get(db_txn, dbi_utxo_, &key, &value);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found getting UTXO [insert_reorg_pool] ", res);
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error getting UTXO [insert_reorg_pool] ", res);
        return result_code::other;
    }

    res = mdb_put(db_txn, dbi_reorg_pool_, &key, &value, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg pool [insert_reorg_pool] ", res);
        return result_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error inserting in reorg pool [insert_reorg_pool] ", res);
        return result_code::other;
    }

    MDB_val key_index {sizeof(height), &height};        //TODO(fernando): podría estar afuera de la DBTx
    MDB_val value_index {key.mv_size, key.mv_data};     //TODO(fernando): podría estar afuera de la DBTx
    res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, 0);
    
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg index [insert_reorg_pool] ", res);
        return result_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error inserting in reorg index [insert_reorg_pool] ", res);
        return result_code::other;
    }

    return result_code::success;
}

//TODO : remove this database in db_new_with_blocks and db_new_full
template <typename Clock>
result_code internal_database_basis<Clock>::push_block_reorg(chain::block const& block, uint32_t height, MDB_txn* db_txn) {

    auto valuearr = block.to_data(false);               //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key {sizeof(height), &height};              //TODO(fernando): podría estar afuera de la DBTx
    MDB_val value {valuearr.size(), valuearr.data()};   //TODO(fernando): podría estar afuera de la DBTx
    
    auto res = mdb_put(db_txn, dbi_reorg_block_, &key, &value, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg block [push_block_reorg] ", res);
        return result_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error inserting in reorg block [push_block_reorg] ", res);
        return result_code::other;
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_output_from_reorg_and_remove(chain::output_point const& point, MDB_txn* db_txn) {
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);
    MDB_val key {keyarr.size(), keyarr.data()};

    MDB_val value;
    auto res = mdb_get(db_txn, dbi_reorg_pool_, &key, &value);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found in reorg pool [insert_output_from_reorg_and_remove] ", res);
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error in reorg pool [insert_output_from_reorg_and_remove] ", res);
        return result_code::other;
    }

    res = mdb_put(db_txn, dbi_utxo_, &key, &value, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in UTXO [insert_output_from_reorg_and_remove] ", res);
        return result_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error inserting in UTXO [insert_output_from_reorg_and_remove] ", res);
        return result_code::other;
    }

    res = mdb_del(db_txn, dbi_reorg_pool_, &key, NULL);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting in reorg pool [insert_output_from_reorg_and_remove] ", res);
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error deleting in reorg pool [insert_output_from_reorg_and_remove] ", res);
        return result_code::other;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_block_reorg(uint32_t height, MDB_txn* db_txn) {

    MDB_val key {sizeof(height), &height};
    auto res = mdb_del(db_txn, dbi_reorg_block_, &key, NULL);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting reorg block in LMDB [remove_block_reorg] - mdb_del: ", res);
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error deleting reorg block in LMDB [remove_block_reorg] - mdb_del: ", res);
        return result_code::other;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_reorg_index(uint32_t height, MDB_txn* db_txn) {

    MDB_val key {sizeof(height), &height};
    auto res = mdb_del(db_txn, dbi_reorg_index_, &key, NULL);
    if (res == MDB_NOTFOUND) {
        LOG_DEBUG(LOG_DATABASE, "Key not found deleting reorg index in LMDB [remove_reorg_index] - height: ", height, " - mdb_del: ", res);
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_DEBUG(LOG_DATABASE, "Error deleting reorg index in LMDB [remove_reorg_index] - height: ", height, " - mdb_del: ", res);
        return result_code::other;
    }
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)


template <typename Clock>
chain::block internal_database_basis<Clock>::get_block_reorg(uint32_t height, MDB_txn* db_txn) const {
    MDB_val key {sizeof(height), &height};
    MDB_val value;

    if (mdb_get(db_txn, dbi_reorg_block_, &key, &value) != MDB_SUCCESS) {
        return {};
    }

    auto data = db_value_to_data_chunk(value);
    auto res = chain::block::factory_from_data(data);       //TODO(fernando): mover fuera de la DbTx
    return res;
}

// //public
// template <typename Clock>
// std::pair<chain::block, uint32_t> internal_database_basis<Clock>::get_block_reorg(hash_digest const& hash) const {
//     MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};

//     MDB_txn* db_txn;
//     auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
//     if (res != MDB_SUCCESS) {
//         return {};
//     }

//     MDB_val value;
//     if (mdb_get(db_txn, dbi_block_header_by_hash_, &key, &value) != MDB_SUCCESS) {
//         mdb_txn_commit(db_txn);
//         // mdb_txn_abort(db_txn);
//         return {};
//     }

//     // assert value.mv_size == 4;
//     auto height = *static_cast<uint32_t*>(value.mv_data);

//     auto block = get_block_reorg(height, db_txn);

//     if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
//         return {};
//     }

//     return {block, height};
// }

template <typename Clock>
chain::block internal_database_basis<Clock>::get_block_reorg(uint32_t height) const {
    MDB_txn* db_txn;
    auto zzz = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (zzz != MDB_SUCCESS) {
        return {};
    }

    auto res = get_block_reorg(height, db_txn);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return {};
    }

    return res;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::prune_reorg_index(uint32_t remove_until, MDB_txn* db_txn) {
    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_reorg_index_, &cursor) != MDB_SUCCESS) {
        return result_code::other;
    }

    MDB_val key;
    MDB_val value;
    int rc;
    while ((rc = mdb_cursor_get(cursor, &key, &value, MDB_NEXT)) == MDB_SUCCESS) {
        auto current_height = *static_cast<uint32_t*>(key.mv_data);
        if (current_height < remove_until) {

            auto res = mdb_del(db_txn, dbi_reorg_pool_, &value, NULL);
            if (res == MDB_NOTFOUND) {
                LOG_INFO(LOG_DATABASE, "Key not found deleting reorg pool in LMDB [prune_reorg_index] - mdb_del: ", res);
                return result_code::key_not_found;
            }
            if (res != MDB_SUCCESS) {
                LOG_INFO(LOG_DATABASE, "Error deleting reorg pool in LMDB [prune_reorg_index] - mdb_del: ", res);
                return result_code::other;
            }

            if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
                mdb_cursor_close(cursor);
                return result_code::other;
            }
        } else {
            break;
        }
    }
    
    mdb_cursor_close(cursor);
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::prune_reorg_block(uint32_t amount_to_delete, MDB_txn* db_txn) {
    //precondition: amount_to_delete >= 1

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_reorg_block_, &cursor) != MDB_SUCCESS) {
        return result_code::other;
    }

    int rc;
    while ((rc = mdb_cursor_get(cursor, nullptr, nullptr, MDB_NEXT)) == MDB_SUCCESS) {
        if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
            mdb_cursor_close(cursor);
            return result_code::other;
        }
        if (--amount_to_delete == 0) break;
    }
    
    mdb_cursor_close(cursor);
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::get_first_reorg_block_height(uint32_t& out_height) const {
    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res != MDB_SUCCESS) {
        return result_code::other;
    }

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_reorg_block_, &cursor) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        return result_code::other;
    }

    MDB_val key;
    int rc;
    if ((rc = mdb_cursor_get(cursor, &key, nullptr, MDB_FIRST)) != MDB_SUCCESS) {
        return result_code::db_empty;  
    }

    // assert key.mv_size == 4;
    out_height = *static_cast<uint32_t*>(key.mv_data);
    
    mdb_cursor_close(cursor);

    // mdb_txn_abort(db_txn); 
    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return result_code::other;
    }

    return result_code::success;
}    


} // namespace kth::database

#endif // KTH_DATABASE_REORG_DATABASE_HPP_

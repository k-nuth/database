// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/databases/reorg_database.hpp>

#include <kth/infrastructure.hpp>

#include <kth/database/databases/db_parameters.hpp>
#include <kth/database/databases/tools.hpp>
#include <kth/database/define.hpp>

namespace kth::database {

#if ! defined(KTH_DB_READONLY)

result_code insert_reorg_pool(uint32_t height, KTH_DB_val& key, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_utxo, KTH_DB_dbi dbi_reorg_pool, KTH_DB_dbi dbi_reorg_index) {
    KTH_DB_val value;
    //TODO: use cursors
    auto res = kth_db_get(db_txn, dbi_utxo, &key, &value);
    if (res == KTH_DB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found getting UTXO [insert_reorg_pool] ", res);
        return result_code::key_not_found;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error getting UTXO [insert_reorg_pool] ", res);
        return result_code::other;
    }

    res = kth_db_put(db_txn, dbi_reorg_pool, &key, &value, KTH_DB_NOOVERWRITE);
    if (res == KTH_DB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg pool [insert_reorg_pool] ", res);
        return result_code::duplicated_key;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error inserting in reorg pool [insert_reorg_pool] ", res);
        return result_code::other;
    }

    auto key_index = kth_db_make_value(sizeof(height), &height);        //TODO(fernando): podría estar afuera de la DBTx
    auto value_index = kth_db_make_value(kth_db_get_size(key), kth_db_get_data(key));     //TODO(fernando): podría estar afuera de la DBTx
    res = kth_db_put(db_txn, dbi_reorg_index, &key_index, &value_index, 0);
    
    if (res == KTH_DB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg index [insert_reorg_pool] ", res);
        return result_code::duplicated_key;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error inserting in reorg index [insert_reorg_pool] ", res);
        return result_code::other;
    }

    return result_code::success;
}

//TODO : remove this database in db_new_with_blocks and db_new_full
result_code push_block_reorg(domain::chain::block const& block, uint32_t height, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_block) {

    auto valuearr = block.to_data(false);                               //TODO(fernando): podría estar afuera de la DBTx
    auto key = kth_db_make_value(sizeof(height), &height);              //TODO(fernando): podría estar afuera de la DBTx
    auto value = kth_db_make_value(valuearr.size(), valuearr.data());   //TODO(fernando): podría estar afuera de la DBTx
    
    auto res = kth_db_put(db_txn, dbi_reorg_block, &key, &value, KTH_DB_NOOVERWRITE);
    if (res == KTH_DB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg block [push_block_reorg] ", res);
        return result_code::duplicated_key;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error inserting in reorg block [push_block_reorg] ", res);
        return result_code::other;
    }

    return result_code::success;
}

result_code insert_output_from_reorg_and_remove(domain::chain::output_point const& point, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_utxo, KTH_DB_dbi dbi_reorg_pool) {
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);
    auto key = kth_db_make_value(keyarr.size(), keyarr.data());

    KTH_DB_val value;
    auto res = kth_db_get(db_txn, dbi_reorg_pool, &key, &value);
    if (res == KTH_DB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found in reorg pool [insert_output_from_reorg_and_remove] ", res);
        return result_code::key_not_found;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error in reorg pool [insert_output_from_reorg_and_remove] ", res);
        return result_code::other;
    }

    res = kth_db_put(db_txn, dbi_utxo, &key, &value, KTH_DB_NOOVERWRITE);
    if (res == KTH_DB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in UTXO [insert_output_from_reorg_and_remove] ", res);
        return result_code::duplicated_key;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error inserting in UTXO [insert_output_from_reorg_and_remove] ", res);
        return result_code::other;
    }

    res = kth_db_del(db_txn, dbi_reorg_pool, &key, NULL);
    if (res == KTH_DB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting in reorg pool [insert_output_from_reorg_and_remove] ", res);
        return result_code::key_not_found;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error deleting in reorg pool [insert_output_from_reorg_and_remove] ", res);
        return result_code::other;
    }
    return result_code::success;
}

result_code remove_block_reorg(uint32_t height, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_block) {

    auto key = kth_db_make_value(sizeof(height), &height);
    auto res = kth_db_del(db_txn, dbi_reorg_block, &key, NULL);
    if (res == KTH_DB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting reorg block in LMDB [remove_block_reorg] - kth_db_del: ", res);
        return result_code::key_not_found;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error deleting reorg block in LMDB [remove_block_reorg] - kth_db_del: ", res);
        return result_code::other;
    }
    return result_code::success;
}

result_code remove_reorg_index(uint32_t height, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_index) {

    auto key = kth_db_make_value(sizeof(height), &height);
    auto res = kth_db_del(db_txn, dbi_reorg_index, &key, NULL);
    if (res == KTH_DB_NOTFOUND) {
        LOG_DEBUG(LOG_DATABASE, "Key not found deleting reorg index in LMDB [remove_reorg_index] - height: ", height, " - kth_db_del: ", res);
        return result_code::key_not_found;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_DEBUG(LOG_DATABASE, "Error deleting reorg index in LMDB [remove_reorg_index] - height: ", height, " - kth_db_del: ", res);
        return result_code::other;
    }
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

domain::chain::block get_block_reorg(uint32_t height, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_block) {
    auto key = kth_db_make_value(sizeof(height), &height);
    KTH_DB_val value;

    if (kth_db_get(db_txn, dbi_reorg_block, &key, &value) != KTH_DB_SUCCESS) {
        return {};
    }

    auto data = db_value_to_data_chunk(value);
    auto res = domain::create<domain::chain::block>(data);       //TODO(fernando): mover fuera de la DbTx
    return res;
}

domain::chain::block get_block_reorg(uint32_t height, KTH_DB_dbi dbi_reorg_block, KTH_DB_env* env) {
    KTH_DB_txn* db_txn;
    auto res = kth_db_txn_begin(env, NULL, KTH_DB_RDONLY, &db_txn);
    if (res != KTH_DB_SUCCESS) {
        return {};
    }

    auto block = get_block_reorg(height, db_txn, dbi_reorg_block);
    if (kth_db_txn_commit(db_txn) != KTH_DB_SUCCESS) {
        return {};
    }

    return block;
}

#if ! defined(KTH_DB_READONLY)

result_code prune_reorg_index(uint32_t remove_until, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_pool, KTH_DB_dbi dbi_reorg_index) {
    KTH_DB_cursor* cursor;
    if (kth_db_cursor_open(db_txn, dbi_reorg_index, &cursor) != KTH_DB_SUCCESS) {
        return result_code::other;
    }

    KTH_DB_val key;
    KTH_DB_val value;
    int rc;
    while ((rc = kth_db_cursor_get(cursor, &key, &value, KTH_DB_NEXT)) == KTH_DB_SUCCESS) {
        auto current_height = *static_cast<uint32_t*>(kth_db_get_data(key));
        if (current_height < remove_until) {

            auto res = kth_db_del(db_txn, dbi_reorg_pool, &value, NULL);
            if (res == KTH_DB_NOTFOUND) {
                LOG_INFO(LOG_DATABASE, "Key not found deleting reorg pool in LMDB [prune_reorg_index] - kth_db_del: ", res);
                return result_code::key_not_found;
            }
            if (res != KTH_DB_SUCCESS) {
                LOG_INFO(LOG_DATABASE, "Error deleting reorg pool in LMDB [prune_reorg_index] - kth_db_del: ", res);
                return result_code::other;
            }

            if (kth_db_cursor_del(cursor, 0) != KTH_DB_SUCCESS) {
                kth_db_cursor_close(cursor);
                return result_code::other;
            }
        } else {
            break;
        }
    }
    
    kth_db_cursor_close(cursor);
    return result_code::success;
}

result_code prune_reorg_block(uint32_t amount_to_delete, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_block) {
    //precondition: amount_to_delete >= 1

    KTH_DB_cursor* cursor;
    if (kth_db_cursor_open(db_txn, dbi_reorg_block, &cursor) != KTH_DB_SUCCESS) {
        return result_code::other;
    }

    int rc;
    while ((rc = kth_db_cursor_get(cursor, nullptr, nullptr, KTH_DB_NEXT)) == KTH_DB_SUCCESS) {
        if (kth_db_cursor_del(cursor, 0) != KTH_DB_SUCCESS) {
            kth_db_cursor_close(cursor);
            return result_code::other;
        }
        if (--amount_to_delete == 0) break;
    }
    
    kth_db_cursor_close(cursor);
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

result_code get_first_reorg_block_height(uint32_t& out_height, KTH_DB_dbi dbi_reorg_block, KTH_DB_env* env) {
    KTH_DB_txn* db_txn;
    auto res = kth_db_txn_begin(env, NULL, KTH_DB_RDONLY, &db_txn);
    if (res != KTH_DB_SUCCESS) {
        return result_code::other;
    }

    KTH_DB_cursor* cursor;
    if (kth_db_cursor_open(db_txn, dbi_reorg_block, &cursor) != KTH_DB_SUCCESS) {
        kth_db_txn_commit(db_txn);
        return result_code::other;
    }

    KTH_DB_val key;
    int rc;
    if ((rc = kth_db_cursor_get(cursor, &key, nullptr, KTH_DB_FIRST)) != KTH_DB_SUCCESS) {
        return result_code::db_empty;  
    }

    // assert kth_db_get_size(key) == 4;
    out_height = *static_cast<uint32_t*>(kth_db_get_data(key));
    
    kth_db_cursor_close(cursor);

    // kth_db_txn_abort(db_txn); 
    if (kth_db_txn_commit(db_txn) != KTH_DB_SUCCESS) {
        return result_code::other;
    }

    return result_code::success;
}    

} // namespace kth::database

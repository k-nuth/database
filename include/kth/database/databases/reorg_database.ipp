// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_REORG_DATABASE_HPP_
#define KTH_DATABASE_REORG_DATABASE_HPP_

namespace kth::database {

#if ! defined(KTH_DB_READONLY)

// template <typename Clock>
// result_code internal_database_basis<Clock>::insert_reorg_pool_lmdb(uint32_t height, KTH_DB_val& key, KTH_DB_txn* db_txn) {
//     KTH_DB_val value;
//     //TODO: use cursors
//     auto res = kth_db_get(db_txn, dbi_utxo_, &key, &value);
//     if (res == KTH_DB_NOTFOUND) {
//         LOG_INFO(LOG_DATABASE, "Key not found getting UTXO [insert_reorg_pool] ", res);
//         return result_code::key_not_found;
//     }
//     if (res != KTH_DB_SUCCESS) {
//         LOG_INFO(LOG_DATABASE, "Error getting UTXO [insert_reorg_pool] ", res);
//         return result_code::other;
//     }

//     res = kth_db_put(db_txn, dbi_reorg_pool_, &key, &value, KTH_DB_NOOVERWRITE);
//     if (res == KTH_DB_KEYEXIST) {
//         LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg pool [insert_reorg_pool] ", res);
//         return result_code::duplicated_key;
//     }
//     if (res != KTH_DB_SUCCESS) {
//         LOG_INFO(LOG_DATABASE, "Error inserting in reorg pool [insert_reorg_pool] ", res);
//         return result_code::other;
//     }

//     auto key_index = kth_db_make_value(sizeof(height), &height);        //TODO(fernando): podría estar afuera de la DBTx
//     auto value_index = kth_db_make_value(kth_db_get_size(key), kth_db_get_data(key));     //TODO(fernando): podría estar afuera de la DBTx
//     res = kth_db_put(db_txn, dbi_reorg_index_, &key_index, &value_index, 0);

//     if (res == KTH_DB_KEYEXIST) {
//         LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg index [insert_reorg_pool] ", res);
//         return result_code::duplicated_key;
//     }
//     if (res != KTH_DB_SUCCESS) {
//         LOG_INFO(LOG_DATABASE, "Error inserting in reorg index [insert_reorg_pool] ", res);
//         return result_code::other;
//     }

//     return result_code::success;
// }

template <typename Clock>
result_code internal_database_basis<Clock>::insert_reorg_pool(uint32_t height, domain::chain::output_point const& point) {

    // Buscamos la entrada UTXO en la base de datos
    auto it_utxo = utxoset_.find(point);
    if (it_utxo == utxoset_.end()) {
        LOG_INFO(LOG_DATABASE, "Key not found getting UTXO [insert_reorg_pool]");
        return result_code::key_not_found;
    }

    // Insertamos la entrada UTXO en el reorg_pool
    auto pair_res = reorg_pool_.insert({point, it_utxo->second});
    if (!pair_res.second) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg pool [insert_reorg_pool]");
        return result_code::duplicated_key;
    }

    // Insertamos la entrada en reorg_index
    auto& reorg_index_height = reorg_index_[height];  // crea la entrada si no existe aún
    reorg_index_height.push_back(point);

    return result_code::success;
}


//TODO : remove this database in db_new_with_blocks and db_new_full
// template <typename Clock>
// result_code internal_database_basis<Clock>::push_block_reorg_lmdb(domain::chain::block const& block, uint32_t height, KTH_DB_txn* db_txn) {

//     auto valuearr = block.to_data(false);               //TODO(fernando): podría estar afuera de la DBTx
//     auto key = kth_db_make_value(sizeof(height), &height);              //TODO(fernando): podría estar afuera de la DBTx
//     auto value = kth_db_make_value(valuearr.size(), valuearr.data());   //TODO(fernando): podría estar afuera de la DBTx

//     auto res = kth_db_put(db_txn, dbi_reorg_block_, &key, &value, KTH_DB_NOOVERWRITE);
//     if (res == KTH_DB_KEYEXIST) {
//         LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg block [push_block_reorg] ", res);
//         return result_code::duplicated_key;
//     }
//     if (res != KTH_DB_SUCCESS) {
//         LOG_INFO(LOG_DATABASE, "Error inserting in reorg block [push_block_reorg] ", res);
//         return result_code::other;
//     }

//     return result_code::success;
// }

template <typename Clock>
result_code internal_database_basis<Clock>::push_block_reorg(domain::chain::block const& block, uint32_t height) {
    auto res = reorg_block_map_.insert({height, block});
    if ( ! res.second) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg block [push_block_reorg]");
        return result_code::duplicated_key;
    }
    return result_code::success;
}

// template <typename Clock>
// result_code internal_database_basis<Clock>::insert_output_from_reorg_and_remove_lmdb(domain::chain::output_point const& point, KTH_DB_txn* db_txn) {
//     auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);
//     auto key = kth_db_make_value(keyarr.size(), keyarr.data());

//     KTH_DB_val value;
//     auto res = kth_db_get(db_txn, dbi_reorg_pool_, &key, &value);
//     if (res == KTH_DB_NOTFOUND) {
//         LOG_INFO(LOG_DATABASE, "Key not found in reorg pool [insert_output_from_reorg_and_remove] ", res);
//         return result_code::key_not_found;
//     }
//     if (res != KTH_DB_SUCCESS) {
//         LOG_INFO(LOG_DATABASE, "Error in reorg pool [insert_output_from_reorg_and_remove] ", res);
//         return result_code::other;
//     }

//     res = kth_db_put(db_txn, dbi_utxo_, &key, &value, KTH_DB_NOOVERWRITE);
//     if (res == KTH_DB_KEYEXIST) {
//         LOG_INFO(LOG_DATABASE, "Duplicate key inserting in UTXO [insert_output_from_reorg_and_remove] ", res);
//         return result_code::duplicated_key;
//     }
//     if (res != KTH_DB_SUCCESS) {
//         LOG_INFO(LOG_DATABASE, "Error inserting in UTXO [insert_output_from_reorg_and_remove] ", res);
//         return result_code::other;
//     }

//     res = kth_db_del(db_txn, dbi_reorg_pool_, &key, NULL);
//     if (res == KTH_DB_NOTFOUND) {
//         LOG_INFO(LOG_DATABASE, "Key not found deleting in reorg pool [insert_output_from_reorg_and_remove] ", res);
//         return result_code::key_not_found;
//     }
//     if (res != KTH_DB_SUCCESS) {
//         LOG_INFO(LOG_DATABASE, "Error deleting in reorg pool [insert_output_from_reorg_and_remove] ", res);
//         return result_code::other;
//     }
//     return result_code::success;
// }

template <typename Clock>
result_code internal_database_basis<Clock>::insert_output_from_reorg_and_remove(domain::chain::output_point const& point) {
    auto const key = point.to_data(KTH_INTERNAL_DB_WIRE);

    auto it = reorg_pool_.find(key);
    if (it == reorg_pool_.end()) {
        LOG_INFO(LOG_DATABASE, "Key not found in reorg pool [insert_output_from_reorg_and_remove]");
        return result_code::key_not_found;
    }

    auto result = utxoset_.insert(*it);
    if (! result.second) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in UTXO [insert_output_from_reorg_and_remove]");
        return result_code::duplicated_key;
    }

    reorg_pool_.erase(it);
    return result_code::success;
}

// template <typename Clock>
// result_code internal_database_basis<Clock>::remove_block_reorg_lmdb(uint32_t height, KTH_DB_txn* db_txn) {

//     auto key = kth_db_make_value(sizeof(height), &height);
//     auto res = kth_db_del(db_txn, dbi_reorg_block_, &key, NULL);
//     if (res == KTH_DB_NOTFOUND) {
//         LOG_INFO(LOG_DATABASE, "Key not found deleting reorg block in LMDB [remove_block_reorg] - kth_db_del: ", res);
//         return result_code::key_not_found;
//     }
//     if (res != KTH_DB_SUCCESS) {
//         LOG_INFO(LOG_DATABASE, "Error deleting reorg block in LMDB [remove_block_reorg] - kth_db_del: ", res);
//         return result_code::other;
//     }
//     return result_code::success;
// }

template <typename Clock>
result_code internal_database_basis<Clock>::remove_block_reorg(uint32_t height) {

    auto it = reorg_block_map_.find(height);
    if (it == reorg_block_map_.end()) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting reorg block [remove_block_reorg]");
        return result_code::key_not_found;
    }

    reorg_block_map_.erase(it);

    return result_code::success;
}

// template <typename Clock>
// result_code internal_database_basis<Clock>::remove_reorg_index_lmdb(uint32_t height, KTH_DB_txn* db_txn) {

//     auto key = kth_db_make_value(sizeof(height), &height);
//     auto res = kth_db_del(db_txn, dbi_reorg_index_, &key, NULL);
//     if (res == KTH_DB_NOTFOUND) {
//         LOG_DEBUG(LOG_DATABASE, "Key not found deleting reorg index in LMDB [remove_reorg_index] - height: ", height, " - kth_db_del: ", res);
//         return result_code::key_not_found;
//     }
//     if (res != KTH_DB_SUCCESS) {
//         LOG_DEBUG(LOG_DATABASE, "Error deleting reorg index in LMDB [remove_reorg_index] - height: ", height, " - kth_db_del: ", res);
//         return result_code::other;
//     }
//     return result_code::success;
// }

template <typename Clock>
result_code internal_database_basis<Clock>::remove_reorg_index(uint32_t height) {

    auto it = reorg_index_.find(height);
    if (it == reorg_index_.end()) {
        LOG_DEBUG(LOG_DATABASE, "Key not found deleting reorg index [remove_reorg_index] - height: ", height);
        return result_code::key_not_found;
    }

    reorg_index_.erase(it);

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

// template <typename Clock>
// domain::chain::block internal_database_basis<Clock>::get_block_reorg_lmdb(uint32_t height, KTH_DB_txn* db_txn) const {
//     auto key = kth_db_make_value(sizeof(height), &height);
//     KTH_DB_val value;

//     if (kth_db_get(db_txn, dbi_reorg_block_, &key, &value) != KTH_DB_SUCCESS) {
//         return {};
//     }

//     auto data = db_value_to_data_chunk(value);
//     auto res = domain::create<domain::chain::block>(data);       //TODO(fernando): mover fuera de la DbTx
//     return res;
// }

// template <typename Clock>
// domain::chain::block internal_database_basis<Clock>::get_block_reorg_lmdb(uint32_t height) const {
//     KTH_DB_txn* db_txn;
//     auto zzz = kth_db_txn_begin(env_, NULL, KTH_DB_RDONLY, &db_txn);
//     if (zzz != KTH_DB_SUCCESS) {
//         return {};
//     }

//     auto res = get_block_reorg(height, db_txn);

//     if (kth_db_txn_commit(db_txn) != KTH_DB_SUCCESS) {
//         return {};
//     }

//     return res;
// }

template <typename Clock>
domain::chain::block internal_database_basis<Clock>::get_block_reorg(uint32_t height) const {
    auto it = reorg_block_map_.find(height);
    if (it == reorg_block_map_.end()) {
        return domain::chain::block{};
    }
    return it->second;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::prune_reorg_index_lmdb(uint32_t remove_until, KTH_DB_txn* db_txn) {
    KTH_DB_cursor* cursor;
    if (kth_db_cursor_open(db_txn, dbi_reorg_index_, &cursor) != KTH_DB_SUCCESS) {
        return result_code::other;
    }

    KTH_DB_val key;
    KTH_DB_val value;
    int rc;
    while ((rc = kth_db_cursor_get(cursor, &key, &value, KTH_DB_NEXT)) == KTH_DB_SUCCESS) {
        auto current_height = *static_cast<uint32_t*>(kth_db_get_data(key));
        if (current_height < remove_until) {

            auto res = kth_db_del(db_txn, dbi_reorg_pool_, &value, NULL);
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

template <typename Clock>
result_code internal_database_basis<Clock>::prune_reorg_index(uint32_t remove_until) {
    for (auto it = reorg_index_.begin(); it != reorg_index_.end();) {
        if (it->first < remove_until) {
            // Eliminar entradas correspondientes en reorg_pool_
            for (auto const& entry : it->second) {
                auto output_it = reorg_pool_.find(entry.point());
                if (output_it == reorg_pool_.end()) {
                    LOG_INFO(LOG_DATABASE, "Key not found deleting reorg pool [prune_reorg_index]");
                    return result_code::key_not_found;
                }
                reorg_pool_.erase(output_it);
            }
            // Eliminar la entrada en reorg_index_
            it = reorg_index_.erase(it);
        } else {
            ++it;
        }
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::prune_reorg_block_lmdb(uint32_t amount_to_delete, KTH_DB_txn* db_txn) {
    //precondition: amount_to_delete >= 1

    KTH_DB_cursor* cursor;
    if (kth_db_cursor_open(db_txn, dbi_reorg_block_, &cursor) != KTH_DB_SUCCESS) {
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

template <typename Clock>
result_code internal_database_basis<Clock>::get_first_reorg_block_height(uint32_t& out_height) const {
    KTH_DB_txn* db_txn;
    auto res = kth_db_txn_begin(env_, NULL, KTH_DB_RDONLY, &db_txn);
    if (res != KTH_DB_SUCCESS) {
        return result_code::other;
    }

    KTH_DB_cursor* cursor;
    if (kth_db_cursor_open(db_txn, dbi_reorg_block_, &cursor) != KTH_DB_SUCCESS) {
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

#endif // KTH_DATABASE_REORG_DATABASE_HPP_

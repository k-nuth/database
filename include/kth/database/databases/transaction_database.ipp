// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TRANSACTION_DATABASE_HPP_
#define KTH_DATABASE_TRANSACTION_DATABASE_HPP_

namespace kth::database {

#if defined(KTH_DB_NEW_FULL)

//public
template <typename Clock>
transaction_entry internal_database_basis<Clock>::get_transaction(hash_digest const& hash, size_t fork_height) const {
    
    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res != MDB_SUCCESS) {
        return transaction_entry{};
    }

    auto entry = get_transaction(hash, fork_height, db_txn);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return transaction_entry{};
    }

    return entry;

}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::insert_transactions(I f, I l, uint32_t height, uint32_t median_time_past, uint64_t tx_count, MDB_txn* db_txn) {
    
    auto id = tx_count;
    uint32_t pos = 0;
    
    while (f != l) {
        auto const& tx = *f;

        //TODO: (Mario) : Implement tx.Confirm to update existing transactions
        auto res = insert_transaction(id, tx, height, median_time_past, pos, db_txn);
        if (res != result_code::success && res != result_code::duplicated_key) {
            return res;
        }

        //remove unconfirmed transaction if exists
        //TODO (Mario): don't recalculate tx.hash
        res = remove_transaction_unconfirmed(tx.hash(), db_txn);
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
transaction_entry internal_database_basis<Clock>::get_transaction(uint64_t id, MDB_txn* db_txn) const {

    MDB_val key {sizeof(id), &id};
    MDB_val value;

    auto res = mdb_get(db_txn, dbi_transaction_db_, &key, &value);
    
    if (res != MDB_SUCCESS) {
        return {};
    }

    auto data = db_value_to_data_chunk(value);
    auto entry = transaction_entry::factory_from_data(data);

    return entry;
}

template <typename Clock>
transaction_entry internal_database_basis<Clock>::get_transaction(hash_digest const& hash, size_t fork_height, MDB_txn* db_txn) const {
    MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};
    MDB_val value;

    auto res = mdb_get(db_txn, dbi_transaction_hash_db_, &key, &value);
    if (res != MDB_SUCCESS) {
        return {};
    }

    auto tx_id = *static_cast<uint32_t*>(value.mv_data);;
    
    auto const entry = get_transaction(tx_id, db_txn);

    if (entry.height() > fork_height) {
        return {};
    }

    //Note(Knuth): Transaction stored in dbi_transaction_db_ are always confirmed
    //the parameter requiere_confirmed is never used.
    /*if (! require_confirmed) {
        return entry;
    }

    auto const confirmed = entry.confirmed();
    
    return (confirmed && entry.height() > fork_height) || (require_confirmed && ! confirmed) ? transaction_entry{} : entry;
    */

    return entry;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_transaction(uint64_t id, chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position, MDB_txn* db_txn) {
    
    MDB_val key {sizeof(id), &id};
    
    //auto key_arr = tx.hash();                                    //TODO(fernando): podría estar afuera de la DBTx
    //MDB_val key {key_arr.size(), key_arr.data()};

    auto valuearr = transaction_entry::factory_to_data(tx, height, median_time_past, position);
    MDB_val value {valuearr.size(), valuearr.data()}; 

    auto res = mdb_put(db_txn, dbi_transaction_db_, &key, &value, MDB_APPEND);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key in Transaction DB [insert_transaction] " << res;
        return result_code::duplicated_key;
    }        

    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error saving in Transaction DB [insert_transaction] " << res;
        return result_code::other;
    }


    auto key_arr = tx.hash();                                    //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key_tx {key_arr.size(), key_arr.data()};
    
    res = mdb_put(db_txn, dbi_transaction_hash_db_, &key_tx, &key, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key in Transaction DB [insert_transaction] " << res;
        return result_code::duplicated_key;
    }        

    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error saving in Transaction DB [insert_transaction] " << res;
        return result_code::other;
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_transactions(chain::block const& block, uint32_t height, MDB_txn* db_txn) {
    
    auto const& txs = block.transactions();
    uint32_t pos = 0;
    for (auto const& tx : txs) {

        auto const& hash = tx.hash();
        
        auto res0 = remove_transaction_history_db(tx, height, db_txn);
        if (res0 != result_code::success) {
            return res0;
        }
        
        if (pos > 0) {
            auto res0 = remove_transaction_spend_db(tx, db_txn);
            if (res0 != result_code::success && res0 != result_code::key_not_found) {
                return res0;
            }
        }
        
        MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};
        MDB_val value;

        auto res = mdb_get(db_txn, dbi_transaction_hash_db_, &key, &value);
        if (res != MDB_SUCCESS) {
            return result_code::other;
        }

        auto tx_id = *static_cast<uint32_t*>(value.mv_data);;
        MDB_val key_tx {sizeof(tx_id), &tx_id};

        res = mdb_del(db_txn, dbi_transaction_db_, &key_tx, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::other;
        }

        res = mdb_del(db_txn, dbi_transaction_hash_db_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::other;
        }

        ++pos;
    }
    
    
    /*MDB_val key {sizeof(height), &height};
    MDB_val value;

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_block_db_, &cursor) != MDB_SUCCESS) {
        return {};
    }

    uint32_t pos = 0;
    int rc;
    if ((rc = mdb_cursor_get(cursor, &key, &value, MDB_SET)) == 0) {
       
        auto tx_id = *static_cast<uint32_t*>(value.mv_data);;
        auto const entry = get_transaction(tx_id, db_txn);
        auto const& tx = entry.transaction();
        
        auto res0 = remove_transaction_history_db(tx, height, db_txn);
        if (res0 != result_code::success) {
            return res0;
        }
        
        if (pos > 0) {
            res0 = remove_transaction_spend_db(tx, db_txn);
            if (res0 != result_code::success && res0 != result_code::key_not_found) {
                return res0;
            }
        }
        
        MDB_val key_tx {sizeof(tx_id), &tx_id};
        auto res = mdb_del(db_txn, dbi_transaction_db_, &key_tx, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::other;
        }

        MDB_val key_hash {tx.hash().size(), tx.hash().data()};
        res = mdb_del(db_txn, dbi_transaction_hash_db_, &key_hash, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::other;
        }
    
        while ((rc = mdb_cursor_get(cursor, &key, &value, MDB_NEXT_DUP)) == 0) {
            ++pos;
            auto tx_id = *static_cast<uint32_t*>(value.mv_data);;
            auto const entry = get_transaction(tx_id, db_txn);
            auto const& tx = entry.transaction();
            
            auto res0 = remove_transaction_history_db(tx, height, db_txn);
            if (res0 != result_code::success) {
                return res0;
            }
            
            if (pos > 0) {
                res0 = remove_transaction_spend_db(tx, db_txn);
                if (res0 != result_code::success && res0 != result_code::key_not_found) {
                    return res0;
                }
            }
            
            MDB_val key_tx {sizeof(tx_id), &tx_id};
            auto res = mdb_del(db_txn, dbi_transaction_db_, &key_tx, NULL);
            if (res == MDB_NOTFOUND) {
                LOG_INFO(LOG_DATABASE) << "Key not found deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
                return result_code::key_not_found;
            }
            if (res != MDB_SUCCESS) {
                LOG_INFO(LOG_DATABASE) << "Error deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
                return result_code::other;
            }

            MDB_val key_hash {tx.hash().size(), tx.hash().data()};
            res = mdb_del(db_txn, dbi_transaction_hash_db_, &key_hash, NULL);
            if (res == MDB_NOTFOUND) {
                LOG_INFO(LOG_DATABASE) << "Key not found deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
                return result_code::key_not_found;
            }
            if (res != MDB_SUCCESS) {
                LOG_INFO(LOG_DATABASE) << "Error deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
                return result_code::other;
            }    
        }
    } 
    
    mdb_cursor_close(cursor);*/


    /*//To get the tx hashes to remove, we need to read the block db
    if (mdb_get(db_txn, dbi_block_db_, &key, &value) != MDB_SUCCESS) {
        return result_code::other;
    }

    auto n = value.mv_size;
    auto f = static_cast<uint8_t*>(value.mv_data); 
    //precondition: mv_size es multiplo de 32
    
    uint32_t pos = 0;
    while (n != 0) {
        hash_digest h;
        std::copy(f, f + kth::hash_size, h.data());
        
        MDB_val key_tx {h.size(), h.data()};
        
        auto const& entry = get_transaction(h, height, db_txn);
        if ( ! entry.is_valid() ) {
            return result_code::other;
        }

        auto const& tx = entry.transaction();
        
        auto res0 = remove_transaction_history_db(tx, height, db_txn);
        if (res0 != result_code::success) {
            return res0;
        }
        
        if (pos > 0) {
            res0 = remove_transaction_spend_db(tx, db_txn);
            if (res0 != result_code::success && res0 != result_code::key_not_found) {
                return res0;
            }
        }
        
        auto res = mdb_del(db_txn, dbi_transaction_db_, &key_tx, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::other;
        }
    
        n -= kth::hash_size;
        f += kth::hash_size;
        ++pos;
    }*/

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::update_transaction(chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position, MDB_txn* db_txn) {
    auto key_arr = tx.hash();                                    //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key {key_arr.size(), key_arr.data()};

    auto valuearr = transaction_entry::factory_to_data(tx, height, median_time_past, position);
    MDB_val value {valuearr.size(), valuearr.data()}; 

    auto res = mdb_put(db_txn, dbi_transaction_db_, &key, &value, 0);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key in Transaction DB [insert_transaction] " << res;
        return result_code::duplicated_key;
    }        

    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error saving in Transaction DB [insert_transaction] " << res;
        return result_code::other;
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::set_spend(chain::output_point const& point, uint32_t spender_height, MDB_txn* db_txn) {

    // Limit search to confirmed transactions at or below the spender height,
    // since a spender cannot spend above its own height.
    // Transactions are not marked as spent unless the spender is confirmed.
    // This is consistent with support for unconfirmed double spends.

    auto entry = get_transaction(point.hash(), spender_height, db_txn);

    // The transaction is not exist as confirmed at or below the height.
    if ( ! entry.is_valid() ) {
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
    auto ret = update_transaction(tx, entry.height(), entry.median_time_past(), entry.position(), db_txn);
    if (ret != result_code::success & ret != result_code::duplicated_key) {
        return ret;
    }
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::set_unspend(chain::output_point const& point, MDB_txn* db_txn) {
    return set_spend(point, chain::output::validation::not_spent, db_txn);
}
#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
uint64_t internal_database_basis<Clock>::get_tx_count(MDB_txn* db_txn) const {
  MDB_stat db_stats;
  auto ret = mdb_stat(db_txn, dbi_transaction_db_, &db_stats);
  if (ret != MDB_SUCCESS) {
      return max_uint64;
  }
  return db_stats.ms_entries;
}

#endif //KTH_NEW_DB_FULL

} // namespace kth::database

#endif // KTH_DATABASE_TRANSACTION_DATABASE_HPP_

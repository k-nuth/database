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
#ifndef BITPRIM_DATABASE_HISTORY_DATABASE_IPP_
#define BITPRIM_DATABASE_HISTORY_DATABASE_IPP_

namespace libbitcoin {
namespace database {

#if defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_history_db(wallet::payment_address const& address, data_chunk const& entry, MDB_txn* db_txn) {

    auto key_arr = address.hash();                                    
    MDB_val key {key_arr.size(), key_arr.data()};   
    MDB_val value {entry.size(), const_cast<data_chunk&>(entry).data()};

    auto res = mdb_put(db_txn, dbi_history_db_, &key, &value, MDB_APPENDDUP);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key inserting history [insert_history_db] " << res;        
        return result_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error inserting history [insert_history_db] " << res;        
        return result_code::other;
    }

    return result_code::success;
}

#if defined(BITPRIM_DB_NEW_FULL_ASYNC)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_input_history_transaction(chain::input_point const& inpoint, uint32_t height, chain::input const& input, MDB_txn* db_txn) {
    
    auto const& prevout = input.previous_output();

    if (prevout.validation.cache.is_valid()) {
        
        uint64_t history_count = get_history_count(db_txn);
    
        if (history_count == max_uint64) {
            LOG_INFO(LOG_DATABASE) << "Error getting history items count";
            return result_code::other;
        }

        uint64_t id = history_count;
        
        // This results in a complete and unambiguous history for the
        // address since standard outputs contain unambiguous address data.
        for (auto const& address : prevout.validation.cache.addresses()) {
            auto valuearr = history_entry::factory_to_data(id, inpoint, chain::point_kind::spend, height, inpoint.index(), prevout.checksum());
            auto res = insert_history_db(address, valuearr, db_txn); 
            if (res != result_code::success) {
                return res;
            }
            ++id;        
        }
    } else {
            //During an IBD with checkpoints some previous output info is missing.
            //We can recover it by accessing the database
            
            //TODO (Mario) check if we can query UTXO

            //auto entry = get_utxo(prevout, db_txn);

            auto entry = get_transaction(prevout.hash(), max_size_t, db_txn);

            if (entry.is_valid()) {
                
                auto const& tx = entry.transaction();

                auto const& out_output = tx.outputs()[prevout.index()];

                uint64_t history_count = get_history_count(db_txn);
                if (history_count == max_uint64) {
                    LOG_INFO(LOG_DATABASE) << "Error getting history items count [insert_input_history_transaction]";
                    return result_code::other;
                }

                uint64_t id = history_count;
            
                //auto const& out_output = entry.output();
                for (auto const& address : out_output.addresses()) {

                    //std::cout << "ccc " << encode_hash(tx_hash) << std::endl;

                    auto valuearr = history_entry::factory_to_data(id, inpoint, chain::point_kind::spend, height, inpoint.index(), prevout.checksum());
                    auto res = insert_history_db(address, valuearr, db_txn); 
                    if (res != result_code::success) {
                        return res;
                    }   
                    ++id;
                }
            }
            else {
                LOG_INFO(LOG_DATABASE) << "Error finding transaction for input history [insert_input_history_transaction]";        
            }
    }

    return result_code::success;
}

#endif

template <typename Clock>
result_code internal_database_basis<Clock>::insert_input_history_utxo(chain::input_point const& inpoint, uint32_t height, chain::input const& input, MDB_txn* db_txn) {
    
    auto const& prevout = input.previous_output();

    if (prevout.validation.cache.is_valid()) {
        
        uint64_t history_count = get_history_count(db_txn);
    
        if (history_count == max_uint64) {
            LOG_INFO(LOG_DATABASE) << "Error getting history items count";
            return result_code::other;
        }

        uint64_t id = history_count;
        
        // This results in a complete and unambiguous history for the
        // address since standard outputs contain unambiguous address data.
        for (auto const& address : prevout.validation.cache.addresses()) {
            auto valuearr = history_entry::factory_to_data(id, inpoint, chain::point_kind::spend, height, inpoint.index(), prevout.checksum());
            auto res = insert_history_db(address, valuearr, db_txn); 
            if (res != result_code::success) {
                return res;
            }
            ++id;        
        }
    } else {
            //During an IBD with checkpoints some previous output info is missing.
            //We can recover it by accessing the database
            
            //TODO (Mario) check if we can query UTXO
            //TODO (Mario) requiere_confirmed = true ??

            auto entry = get_utxo(prevout, db_txn);

            //auto entry = get_transaction(prevout.hash(), max_uint32, true, db_txn);

            if (entry.is_valid()) {
                
                //auto const& tx = entry.transaction();

                //auto const& out_output = tx.outputs()[prevout.index()];

                uint64_t history_count = get_history_count(db_txn);
                if (history_count == max_uint64) {
                    LOG_INFO(LOG_DATABASE) << "Error getting history items count";
                    return result_code::other;
                }

                uint64_t id = history_count;
            
                auto const& out_output = entry.output();
                for (auto const& address : out_output.addresses()) {

                    //std::cout << "ccc " << encode_hash(tx_hash) << std::endl;

                    auto valuearr = history_entry::factory_to_data(id, inpoint, chain::point_kind::spend, height, inpoint.index(), prevout.checksum());
                    auto res = insert_history_db(address, valuearr, db_txn); 
                    if (res != result_code::success) {
                        return res;
                    }   
                    ++id;
                }
            }
            else {
                LOG_INFO(LOG_DATABASE) << "Error finding UTXO for input history [insert_input_history]";        
            }
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_output_history(hash_digest const& tx_hash,uint32_t height, uint32_t index, chain::output const& output, MDB_txn* db_txn ) {
    
    uint64_t history_count = get_history_count(db_txn);
    if (history_count == max_uint64) {
        LOG_INFO(LOG_DATABASE) << "Error getting history items count";
        return result_code::other;
    }

    uint64_t id = history_count;
    
    auto const outpoint = chain::output_point {tx_hash, index};
    auto const value = output.value();

    // Standard outputs contain unambiguous address data.
    for (auto const& address : output.addresses()) {
        //std::cout << "eee " << address << std::endl;
        //std::cout << "ddd " << encode_hash(tx_hash) << std::endl; 
        auto valuearr = history_entry::factory_to_data(id, outpoint, chain::point_kind::output, height, index, value);
        auto res = insert_history_db(address, valuearr, db_txn); 
        if (res != result_code::success) {
            return res;
        }
        ++id;
    }

    return result_code::success;
}

template <typename Clock>
// static
chain::history_compact internal_database_basis<Clock>::history_entry_to_history_compact(history_entry const& entry) {
    return chain::history_compact{entry.point_kind(), entry.point(), entry.height(), entry.value_or_checksum()};
}

template <typename Clock>
chain::history_compact::list internal_database_basis<Clock>::get_history(short_hash const& key, size_t limit, size_t from_height) const {

    chain::history_compact::list result;

    if (limit == 0) {
        return result;
    }

    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res != MDB_SUCCESS) {
        return result;
    }

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_history_db_, &cursor) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        return result;
    }

    MDB_val key_hash{key.size(), const_cast<short_hash&>(key).data()};
    MDB_val value;
    int rc;
    if ((rc = mdb_cursor_get(cursor, &key_hash, &value, MDB_SET)) == 0) {
       
        auto data = db_value_to_data_chunk(value);
        auto entry = history_entry::factory_from_data(data);
        
        if (from_height == 0 || entry.height() >= from_height) {
            result.push_back(history_entry_to_history_compact(entry));
        }

        while ((rc = mdb_cursor_get(cursor, &key_hash, &value, MDB_NEXT_DUP)) == 0) {
        
            if (limit > 0 && result.size() >= limit) {
                break;
            }

            auto data = db_value_to_data_chunk(value);
            auto entry = history_entry::factory_from_data(data);

            if (from_height == 0 || entry.height() >= from_height) {
                result.push_back(history_entry_to_history_compact(entry));
            }
            
        }
    } 
    
    mdb_cursor_close(cursor);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return result;
    }

    return result;
}

template <typename Clock>
std::vector<hash_digest> internal_database_basis<Clock>::get_history_txns(short_hash const& key, size_t limit, size_t from_height) const {

    std::set<hash_digest> temp;
    std::vector<hash_digest> result;

    // Stop once we reach the limit (if specified).
    if (limit == 0)
        return result;

    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res != MDB_SUCCESS) {
        return result;
    }

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_history_db_, &cursor) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        return result;
    }

    MDB_val key_hash{key.size(), const_cast<short_hash&>(key).data()};
    MDB_val value;
    int rc;
    if ((rc = mdb_cursor_get(cursor, &key_hash, &value, MDB_SET)) == 0) {
       
        auto data = db_value_to_data_chunk(value);
        auto entry = history_entry::factory_from_data(data);
        
        if (from_height == 0 || entry.height() >= from_height) {
            // Avoid inserting the same tx
            const auto & pair = temp.insert(entry.point().hash());
            if (pair.second){
                // Add valid txns to the result vector
                result.push_back(*pair.first);
            }
        }

        while ((rc = mdb_cursor_get(cursor, &key_hash, &value, MDB_NEXT_DUP)) == 0) {
        
            if (limit > 0 && result.size() >= limit) {
                break;
            }

            auto data = db_value_to_data_chunk(value);
            auto entry = history_entry::factory_from_data(data);

            if (from_height == 0 || entry.height() >= from_height) {
                // Avoid inserting the same tx
                const auto & pair = temp.insert(entry.point().hash());
                if (pair.second){
                    // Add valid txns to the result vector
                    result.push_back(*pair.first);
                }
            }
            
        }
    } 
    
    mdb_cursor_close(cursor);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return result;
    }

    return result;
}


template <typename Clock>
result_code internal_database_basis<Clock>::remove_transaction_history_db(chain::transaction const& tx, size_t height, MDB_txn* db_txn) {

    for (auto const& output: tx.outputs()) {
        for (auto const& address : output.addresses()) {
            auto res = remove_history_db(address, height, db_txn);
            if (res != result_code::success) {
                return res;
            }
        }
    }

    for (auto const& input: tx.inputs()) {
        
        auto const& prevout = input.previous_output();

        if (prevout.validation.cache.is_valid()) { 
            for (auto const& address : prevout.validation.cache.addresses()) { 
                auto res = remove_history_db(address, height, db_txn);
                if (res != result_code::success) {
                    return res;
                }
            }
        }
        else {

            auto const& entry = get_transaction(prevout.hash(), max_uint32, db_txn);

            if (entry.is_valid()) {
                
                auto const& tx = entry.transaction();
                auto const& out_output = tx.outputs()[prevout.index()];

                for (auto const& address : out_output.addresses()) {

                    auto res = remove_history_db(address, height, db_txn);
                    if (res != result_code::success) {
                        return res;
                    }
                }
            }
        }
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_history_db(const short_hash& key, size_t height, MDB_txn* db_txn) {

    MDB_cursor* cursor;

    if (mdb_cursor_open(db_txn, dbi_history_db_, &cursor) != MDB_SUCCESS) {
        return result_code::other;
    }

    MDB_val key_hash{key.size(), const_cast<short_hash&>(key).data()};
    MDB_val value;
    int rc;
    if ((rc = mdb_cursor_get(cursor, &key_hash, &value, MDB_SET)) == 0) {
       
        auto data = db_value_to_data_chunk(value);
        auto entry = history_entry::factory_from_data(data);
        
        if (entry.height() == height) {
            
            if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
                mdb_cursor_close(cursor);
                return result_code::other;
            }
        }

        while ((rc = mdb_cursor_get(cursor, &key_hash, &value, MDB_NEXT_DUP)) == 0) {
        
            auto data = db_value_to_data_chunk(value);
            auto entry = history_entry::factory_from_data(data);

            if (entry.height() == height) {
                if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
                    mdb_cursor_close(cursor);
                    return result_code::other;
                }
            }
        }
    } 
    
    mdb_cursor_close(cursor);

    return result_code::success;
}

template <typename Clock>
uint64_t internal_database_basis<Clock>::get_history_count(MDB_txn* db_txn) {
  MDB_stat db_stats;
  auto ret = mdb_stat(db_txn, dbi_history_db_, &db_stats);
  if (ret != MDB_SUCCESS) {
      return max_uint64;
  }
  return db_stats.ms_entries;
}

#endif //BITPRIM_NEW_DB_FULL

} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_HISTORY_DATABASE_IPP_

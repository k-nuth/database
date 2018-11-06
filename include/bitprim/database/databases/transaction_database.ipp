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
#ifndef BITPRIM_DATABASE_TRANSACTION_DATABASE_HPP_
#define BITPRIM_DATABASE_TRANSACTION_DATABASE_HPP_

namespace libbitcoin {
namespace database {


#if defined(BITPRIM_DB_NEW_FULL)

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::insert_transactions(I f, I l, uint32_t height, MDB_txn* db_txn) {
    while (f != l) {
        auto const& tx = *f;
        auto res = insert_transaction(tx, height,  db_txn);
        if (res != result_code::success) {    
            return res;
        }
        ++f;
    }

    return result_code::success;
}

template <typename Clock>
chain::transaction internal_database_basis<Clock>::get_transaction(hash_digest const& hash, MDB_txn* db_txn) const {
    MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};
    MDB_val value;

    if (mdb_get(db_txn, dbi_transaction_db_, &key, &value) != MDB_SUCCESS) {
        return chain::transaction{};
    }

    auto data = db_value_to_data_chunk(value);
    auto res = chain::transaction::factory_from_data(data,true,true);
    return res;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_transaction(chain::transaction const& tx, uint32_t height,  MDB_txn* db_txn) {
    auto key_arr = tx.hash();                                    //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key {key_arr.size(), key_arr.data()};

    //TODO (Mario): store height        
    auto value_arr = tx.to_data(true,true);                                //TODO(fernando): podría estar afuera de la DBTx
    MDB_val value {value_arr.size(), value_arr.data()}; 

    auto res = mdb_put(db_txn, dbi_transaction_db_, &key, &value, MDB_NOOVERWRITE);
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
result_code internal_database_basis<Clock>::remove_transactions(uint32_t height, MDB_txn* db_txn) {
    MDB_val key {sizeof(height), &height};
    MDB_val value;

    if (mdb_get(db_txn, dbi_block_db_, &key, &value) != MDB_SUCCESS) {
        return result_code::other;
    }

    auto n = value.mv_size;
    auto f = static_cast<uint8_t*>(value.mv_data); 
    //precondition: mv_size es multiplo de 32
    
    while (n != 0) {
        hash_digest h;
        std::copy(f, f + libbitcoin::hash_size, h.data());
        
        MDB_val key_tx {h.size(), h.data()};
        auto res = mdb_del(db_txn, dbi_transaction_db_, &key_tx, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting transaction DB in LMDB [remove_blocks_db] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting transaction DB in LMDB [remove_blocks_db] - mdb_del: " << res;
            return result_code::other;
        }
    
        n -= libbitcoin::hash_size;
        f += libbitcoin::hash_size;
    }

    return result_code::success;
}


#endif //BITPRIM_NEW_DB_FULL


} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_TRANSACTION_DATABASE_HPP_

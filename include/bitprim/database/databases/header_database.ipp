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
#ifndef BITPRIM_DATABASE_HEADER_DATABASE_IPP_
#define BITPRIM_DATABASE_HEADER_DATABASE_IPP_

namespace libbitcoin {
namespace database {

template <typename Clock>
result_code internal_database_basis<Clock>::push_block_header(chain::block const& block, uint32_t height, MDB_txn* db_txn) {
    auto valuearr = block.header().to_data(true);               //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key {sizeof(height), &height};                      //TODO(fernando): podría estar afuera de la DBTx
    MDB_val value {valuearr.size(), valuearr.data()};           //TODO(fernando): podría estar afuera de la DBTx
    auto res = mdb_put(db_txn, dbi_block_header_, &key, &value, MDB_APPEND);
    if (res == MDB_KEYEXIST) {
        //TODO(fernando): El logging en general no está bueno que esté en la DbTx.
        LOG_INFO(LOG_DATABASE) << "Duplicate key inserting block header [push_block_header] " << res;        //TODO(fernando): podría estar afuera de la DBTx. 
        return result_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error inserting block header  [push_block_header] " << res;        
        return result_code::other;
    }
    
    auto key_by_hash_arr = block.hash();                                    //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key_by_hash {key_by_hash_arr.size(), key_by_hash_arr.data()};   //TODO(fernando): podría estar afuera de la DBTx
    res = mdb_put(db_txn, dbi_block_header_by_hash_, &key_by_hash, &key, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key inserting block header by hash [push_block_header] " << res;        
        return result_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error inserting block header by hash [push_block_header] " << res;        
        return result_code::other;
    }

    return result_code::success;
}


template <typename Clock>
chain::header internal_database_basis<Clock>::get_header(uint32_t height, MDB_txn* db_txn) const {
    MDB_val key {sizeof(height), &height};
    MDB_val value;

    if (mdb_get(db_txn, dbi_block_header_, &key, &value) != MDB_SUCCESS) {
        return chain::header{};
    }

    auto data = db_value_to_data_chunk(value);
    auto res = chain::header::factory_from_data(data);
    return res;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_block_header(hash_digest const& hash, uint32_t height, MDB_txn* db_txn) {

    MDB_val key {sizeof(height), &height};
    auto res = mdb_del(db_txn, dbi_block_header_, &key, NULL);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE) << "Key not found deleting block header in LMDB [remove_block_header] - mdb_del: " << res;
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Erro deleting block header in LMDB [remove_block_header] - mdb_del: " << res;
        return result_code::other;
    }

    MDB_val key_hash {hash.size(), const_cast<hash_digest&>(hash).data()};
    res = mdb_del(db_txn, dbi_block_header_by_hash_, &key_hash, NULL);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE) << "Key not found deleting block header by hash in LMDB [remove_block_header] - mdb_del: " << res;
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Erro deleting block header by hash in LMDB [remove_block_header] - mdb_del: " << res;
        return result_code::other;
    }

    return result_code::success;
}


} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_HEADER_DATABASE_IPP_

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
#ifndef BITPRIM_DATABASE_UTXO_DATABASE_HPP_
#define BITPRIM_DATABASE_UTXO_DATABASE_HPP_

namespace libbitcoin {
namespace database {

template <typename Clock>
result_code internal_database_basis<Clock>::remove_utxo(uint32_t height, chain::output_point const& point, bool insert_reorg, MDB_txn* db_txn) {
    auto keyarr = point.to_data(BITPRIM_INTERNAL_DB_WIRE);      //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key {keyarr.size(), keyarr.data()};                 //TODO(fernando): podría estar afuera de la DBTx

    if (insert_reorg) {
        auto res0 = insert_reorg_pool(height, key, db_txn);
        if (res0 != result_code::success) return res0;
    }

    auto res = mdb_del(db_txn, dbi_utxo_, &key, NULL);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE) << "Key not found deleting UTXO [remove_utxo] " << res;
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error deleting UTXO [remove_utxo] " << res;
        return result_code::other;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_utxo(chain::output_point const& point, chain::output const& output, data_chunk const& fixed_data, MDB_txn* db_txn) {
    auto keyarr = point.to_data(BITPRIM_INTERNAL_DB_WIRE);                  //TODO(fernando): podría estar afuera de la DBTx
    auto valuearr = utxo_entry::to_data_with_fixed(output, fixed_data);     //TODO(fernando): podría estar afuera de la DBTx

    MDB_val key   {keyarr.size(), keyarr.data()};                           //TODO(fernando): podría estar afuera de la DBTx
    MDB_val value {valuearr.size(), valuearr.data()};                       //TODO(fernando): podría estar afuera de la DBTx
    auto res = mdb_put(db_txn, dbi_utxo_, &key, &value, MDB_NOOVERWRITE);

    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate Key inserting UTXO [insert_utxo] " << res;        
        return result_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error inserting UTXO [insert_utxo] " << res;        
        return result_code::other;
    }
    return result_code::success;
}


} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_UTXO_DATABASE_HPP_

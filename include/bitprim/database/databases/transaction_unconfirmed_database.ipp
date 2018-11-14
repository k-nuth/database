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
#ifndef BITPRIM_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_HPP_
#define BITPRIM_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_HPP_

namespace libbitcoin {
namespace database {


#if defined(BITPRIM_DB_NEW_FULL)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_transaction_unconfirmed(chain::transaction const& tx,  MDB_txn* db_txn) {
    auto key_arr = tx.hash();                                    //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key {key_arr.size(), key_arr.data()};

    //TODO (Mario): store height        
    auto value_arr = tx.to_data(false,true,true);                                //TODO(fernando): podría estar afuera de la DBTx
    MDB_val value {value_arr.size(), value_arr.data()}; 

    auto res = mdb_put(db_txn, dbi_transaction_unconfirmed_db_, &key, &value, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key in Transaction Unconfirmed DB [insert_transaction_unconfirmed] " << res;
        return result_code::duplicated_key;
    }        

    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error saving in Transaction Unconfirmed DB [insert_transaction_unconfirmed] " << res;
        return result_code::other;
    }

    return result_code::success;
}



#endif //BITPRIM_NEW_DB_FULL


} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_TRANSACTION_UNCONFIRMED_DATABASE_HPP_

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
#ifndef BITPRIM_DATABASE_SPEND_DATABASE_IPP_
#define BITPRIM_DATABASE_SPEND_DATABASE_IPP_

namespace libbitcoin {
namespace database {

#if defined(BITPRIM_DB_NEW_FULL)

//public
template <typename Clock>
chain::input_point internal_database_basis<Clock>::get_spend(chain::output_point const& point) const {

    auto keyarr = point.to_data(BITPRIM_INTERNAL_DB_WIRE);
    MDB_val key {keyarr.size(), keyarr.data()};
    MDB_val value;

    MDB_txn* db_txn;
    auto res0 = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res0 != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error begining LMDB Transaction [get_spend] " << res0;
        return chain::input_point{};
    }

    res0 = mdb_get(db_txn, dbi_spend_db_, &key, &value);
    if (res0 != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        // mdb_txn_abort(db_txn);  
        return chain::input_point{};
    }

    auto data = db_value_to_data_chunk(value);

    res0 = mdb_txn_commit(db_txn);
    if (res0 != MDB_SUCCESS) {
        LOG_DEBUG(LOG_DATABASE) << "Error commiting LMDB Transaction [get_spend] " << res0;        
        return chain::input_point{};
    }

    auto res = chain::input_point::factory_from_data(data);
    return res;
}


//pivate
template <typename Clock>
result_code internal_database_basis<Clock>::insert_spend(chain::output_point const& out_point, chain::input_point const& in_point, MDB_txn* db_txn) {

    auto keyarr = out_point.to_data(BITPRIM_INTERNAL_DB_WIRE);
    MDB_val key {keyarr.size(), keyarr.data()};   

    auto value_arr = in_point.to_data();
    MDB_val value {value_arr.size(), value_arr.data()};

    auto res = mdb_put(db_txn, dbi_spend_db_, &key, &value, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key inserting spend [insert_spend] " << res;        
        return result_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error inserting spend [insert_spend] " << res;        
        return result_code::other;
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_transaction_spend_db(chain::transaction const& tx, MDB_txn* db_txn) {

    for (auto const& input: tx.inputs()) {

        auto const& prevout = input.previous_output();

        auto res = remove_spend(prevout, db_txn);
        if (res != result_code::success) {
            return res;
        }

        /*res = set_unspend(prevout, db_txn);
        if (res != result_code::success) {
            return res;
        }*/
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_spend(chain::output_point const& out_point, MDB_txn* db_txn) {

    auto keyarr = out_point.to_data(BITPRIM_INTERNAL_DB_WIRE);      //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key {keyarr.size(), keyarr.data()};                     //TODO(fernando): podría estar afuera de la DBTx

    auto res = mdb_del(db_txn, dbi_spend_db_, &key, NULL);
    
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE) << "Key not found deleting spend [remove_spend] " << res;
        return result_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error deleting spend [remove_spend] " << res;
        return result_code::other;
    }
    return result_code::success;
}


#endif // defined(BITPRIM_DB_NEW_FULL)

} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_SPEND_DATABASE_IPP_

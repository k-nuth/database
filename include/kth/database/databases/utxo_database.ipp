// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_UTXO_DATABASE_HPP_
#define KTH_DATABASE_UTXO_DATABASE_HPP_

namespace kth {
namespace database {

template <typename Clock>
result_code internal_database_basis<Clock>::remove_utxo(uint32_t height, chain::output_point const& point, bool insert_reorg, MDB_txn* db_txn) {
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);      //TODO(fernando): podría estar afuera de la DBTx
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
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);                  //TODO(fernando): podría estar afuera de la DBTx
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
} // namespace kth

#endif // KTH_DATABASE_UTXO_DATABASE_HPP_

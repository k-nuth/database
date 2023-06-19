// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_UTXO_DATABASE_HPP_
#define KTH_DATABASE_UTXO_DATABASE_HPP_

namespace kth::database {

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::remove_utxo_lmdb(uint32_t height, domain::chain::output_point const& point, bool insert_reorg, KTH_DB_txn* db_txn) {
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);      //TODO(fernando): podría estar afuera de la DBTx
    auto key = kth_db_make_value(keyarr.size(), keyarr.data());                 //TODO(fernando): podría estar afuera de la DBTx

    if (insert_reorg) {
        auto res0 = insert_reorg_pool(height, key, db_txn);
        if (res0 != result_code::success) return res0;
    }

    auto res = kth_db_del(db_txn, dbi_utxo_, &key, NULL);
    if (res == KTH_DB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting UTXO [remove_utxo] ", res);
        return result_code::key_not_found;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error deleting UTXO [remove_utxo] ", res);
        return result_code::other;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_utxo(uint32_t height, domain::chain::output_point const& point, bool insert_reorg) {
    auto const key = point.to_data(KTH_INTERNAL_DB_WIRE);

    if (insert_reorg) {
        auto res0 = insert_reorg_pool(height, key);
        if (res0 != result_code::success) return res0;
    }

    auto it = utxoset_.find(key);
    if (it == utxoset_.end()) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting UTXO [remove_utxo] ");
        return result_code::key_not_found;
    }

    utxoset_.erase(it);
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_utxo_lmdb(domain::chain::output_point const& point, domain::chain::output const& output, data_chunk const& fixed_data, KTH_DB_txn* db_txn) {
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);                  //TODO(fernando): podría estar afuera de la DBTx
    auto valuearr = utxo_entry::to_data_with_fixed(output, fixed_data);     //TODO(fernando): podría estar afuera de la DBTx

    auto key = kth_db_make_value(keyarr.size(), keyarr.data());                           //TODO(fernando): podría estar afuera de la DBTx
    auto value = kth_db_make_value(valuearr.size(), valuearr.data());                       //TODO(fernando): podría estar afuera de la DBTx
    auto res = kth_db_put(db_txn, dbi_utxo_, &key, &value, KTH_DB_NOOVERWRITE);

    if (res == KTH_DB_KEYEXIST) {
        LOG_DEBUG(LOG_DATABASE, "Duplicate Key inserting UTXO [insert_utxo] ", res);
        return result_code::duplicated_key;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error inserting UTXO [insert_utxo] ", res);
        return result_code::other;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_utxo(domain::chain::output_point const& point, domain::chain::output const& output, data_chunk const& fixed_data) {
    auto const key = point.to_data(KTH_INTERNAL_DB_WIRE);
    auto const value = utxo_entry::to_data_with_fixed(output, fixed_data);

    auto [it, inserted] = utxoset_.insert({key, value});

    if (!inserted) {
        LOG_DEBUG(LOG_DATABASE, "Duplicate Key inserting UTXO [insert_utxo]");
        return result_code::duplicated_key;
    }
    return result_code::success;
}


#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_UTXO_DATABASE_HPP_

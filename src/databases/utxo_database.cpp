// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/databases/utxo_database.hpp>

#include <kth/database/databases/db_parameters.hpp>
#include <kth/database/databases/reorg_database.hpp>

namespace kth::database {

#if ! defined(KTH_DB_READONLY)

result_code remove_utxo(uint32_t height, domain::chain::output_point const& point, bool insert_reorg, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_utxo, KTH_DB_dbi dbi_reorg_pool, KTH_DB_dbi dbi_reorg_index) {
    auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);                          //TODO(fernando): podría estar afuera de la DBTx
    auto key = kth_db_make_value(keyarr.size(), keyarr.data());                 //TODO(fernando): podría estar afuera de la DBTx

    if (insert_reorg) {
        std::cout << "insert_reorg: " << insert_reorg << std::endl;
        auto res0 = insert_reorg_pool(height, key, db_txn, dbi_utxo, dbi_reorg_pool, dbi_reorg_index);
        if (res0 != result_code::success) return res0;
    }

    auto res = kth_db_del(db_txn, dbi_utxo, &key, NULL);
    // if (res == KTH_DB_NOTFOUND) {
    //     LOG_INFO(LOG_DATABASE, "Key not found deleting UTXO [remove_utxo] ", res);
    //     return result_code::key_not_found;
    // }
    // if (res != KTH_DB_SUCCESS) {
    //     LOG_INFO(LOG_DATABASE, "Error deleting UTXO [remove_utxo] ", res);
    //     return result_code::other;
    // }
    return result_code::success;
}

result_code insert_utxo(domain::chain::output_point const& point, domain::chain::output const& output, data_chunk const& fixed_data, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_utxo) {
    // auto keyarr = point.to_data(KTH_INTERNAL_DB_WIRE);                  //TODO(fernando): podría estar afuera de la DBTx
    // auto valuearr = utxo_entry::to_data_with_fixed(output, fixed_data);     //TODO(fernando): podría estar afuera de la DBTx

    // auto key = kth_db_make_value(keyarr.size(), keyarr.data());                           //TODO(fernando): podría estar afuera de la DBTx
    // auto value = kth_db_make_value(valuearr.size(), valuearr.data());                       //TODO(fernando): podría estar afuera de la DBTx

    // auto res = kth_db_put(db_txn, dbi_utxo, &key, &value, KTH_DB_NOOVERWRITE);

    // if (res == KTH_DB_KEYEXIST) {
    //     LOG_DEBUG(LOG_DATABASE, "Duplicate Key inserting UTXO [insert_utxo] ", res);
    //     return result_code::duplicated_key;
    // }
    // if (res != KTH_DB_SUCCESS) {
    //     LOG_INFO(LOG_DATABASE, "Error inserting UTXO [insert_utxo] ", res);
    //     return result_code::other;
    // }
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

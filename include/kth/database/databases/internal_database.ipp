// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_INTERNAL_DATABASE_IPP_
#define KTH_DATABASE_INTERNAL_DATABASE_IPP_

#include <kth/infrastructure.hpp>

#include <kth/database/databases/reorg_database.hpp>
#include <kth/database/databases/utxo_database.hpp>

namespace kth::database {

using utxo_pool_t = std::unordered_map<domain::chain::point, utxo_entry>;


#if ! defined(KTH_DB_READONLY)

//TODO(fernando): optimization: consider passing a list of outputs to insert and a list of inputs to delete instead of an entire Block.
//                  avoiding inserting and erasing internal spenders
template <typename Clock>
result_code internal_database_basis::push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past) {

    // KTH_DB_txn* db_txn;
    // auto res0 = kth_db_txn_begin(env_, NULL, 0, &db_txn);
    // if (res0 != KTH_DB_SUCCESS) {
    //     LOG_ERROR(LOG_DATABASE, "Error begining LMDB Transaction [push_block] ", res0);
    //     return result_code::other;
    // }

    // // print_stats(db_txn);

    // //TODO: save reorg blocks after the last checkpoint
    // auto res = push_block(block, height, median_time_past, ! is_old_block<Clock>(block), db_txn);
    // if ( !  succeed(res)) {
    //     kth_db_txn_abort(db_txn);
    //     return res;
    // }

    // // print_stats(db_txn);

    // auto res2 = kth_db_txn_commit(db_txn);
    // if (res2 != KTH_DB_SUCCESS) {
    //     LOG_ERROR(LOG_DATABASE, "Error commiting LMDB Transaction [push_block] ", res2);
    //     return result_code::other;
    // }

    // return res;

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

// Private functions
// ------------------------------------------------------------------------------------------------------
template <typename Clock>
bool internal_database_basis::is_old_block(domain::chain::block const& block) const {
    return is_old_block_<Clock>(block, limit_);
}

// inline 
// int compare_uint64(KTH_DB_val const* a, KTH_DB_val const* b) {
//     //TODO(fernando): check this casts... something smells bad
//     const uint64_t va = *(const uint64_t *)kth_db_get_data(*a);
//     const uint64_t vb = *(const uint64_t *)kth_db_get_data(*b);
//     return (va < vb) ? -1 : va > vb;
// }

#if ! defined(KTH_DB_READONLY)

template <typename I>
result_code internal_database_basis::push_transactions_outputs_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, KTH_DB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        auto res = insert_outputs_error_treatment(height, fixed_data, tx.hash(), tx.outputs(), db_txn);
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }
    return result_code::success;
}

template <typename I>
result_code internal_database_basis::remove_transactions_inputs_non_coinbase(uint32_t height, I f, I l, bool insert_reorg, KTH_DB_txn* db_txn) {
    while (f != l) {
        auto const& tx = *f;
        auto res = remove_inputs(tx.hash(), height, tx.inputs(), insert_reorg, db_txn);
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }
    return result_code::success;
}

template <typename I>
result_code internal_database_basis::push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, KTH_DB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    auto res = push_transactions_outputs_non_coinbase(height, fixed_data, f, l, db_txn);
    if (res != result_code::success) {
        return res;
    }
    return remove_transactions_inputs_non_coinbase(height, f, l, insert_reorg, db_txn);
}

template <typename I>
result_code internal_database_basis::insert_transactions_inputs_non_coinbase(I f, I l, KTH_DB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
    
    while (f != l) {
        auto const& tx = *f;
        auto res = insert_inputs(tx.inputs(), db_txn);
        if (res != result_code::success) {
            return res;
        }
        ++f;
    } 

    return result_code::success;
}

template <typename I>
result_code internal_database_basis::remove_transactions_outputs_non_coinbase(I f, I l, KTH_DB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
    
    while (f != l) {
        auto const& tx = *f;
        auto res = remove_outputs(tx.hash(), tx.outputs(), db_txn);
        if (res != result_code::success) {
            return res;
        }
        ++f;
    } 

    return result_code::success;
}

template <typename I>
result_code internal_database_basis::remove_transactions_non_coinbase(I f, I l, KTH_DB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    auto res = insert_transactions_inputs_non_coinbase(f, l, db_txn);
    if (res != result_code::success) {
        return res;
    }
    return remove_transactions_outputs_non_coinbase(f, l, db_txn);
}

#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_INTERNAL_DATABASE_IPP_

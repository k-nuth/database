// Copyright (c) 2016-2022 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_INTERNAL_DATABASE_IPP_
#define KTH_DATABASE_INTERNAL_DATABASE_IPP_

#include <kth/infrastructure.hpp>

namespace kth::database {

using utxo_pool_t = std::unordered_map<domain::chain::point, utxo_entry>;

template <typename Clock>
internal_database_basis<Clock>::internal_database_basis(path const& db_dir, uint32_t reorg_pool_limit, uint64_t db_max_size, bool safe_mode)
    : limit_(blocks_to_seconds(reorg_pool_limit))
{}

template <typename Clock>
internal_database_basis<Clock>::~internal_database_basis() {
    close();
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
bool internal_database_basis<Clock>::create() {
    // std::cout << "********************** internal_database_basis::create()\n";
    std::error_code ec;

    auto ret = open_internal();
    if ( ! ret ) {
        return false;
    }

    ret = create_db_mode_property();
    if ( ! ret ) {
        return false;
    }

    // std::cout << "****************** create() ************ OK\n";
    return true;
}

template <typename Clock>
bool internal_database_basis<Clock>::create_db_mode_property() {
    // std::cout << "********************** internal_database_basis::create_db_mode_property()\n";
    return true;
}

#endif // ! defined(KTH_DB_READONLY)


template <typename Clock>
bool internal_database_basis<Clock>::open() {
    // std::cout << "********************** internal_database_basis::open()\n";

    auto ret = open_internal();
    if ( ! ret ) {
        // std::cout << "********************** internal_database_basis::open() - 2\n";
        return false;
    }

    ret = verify_db_mode_property();
    if ( ! ret ) {
        // std::cout << "********************** internal_database_basis::open() - 3\n";
        return false;
    }

    // std::cout << "****************** open() ************ OK\n";
    return true;
}


template <typename Clock>
bool internal_database_basis<Clock>::open_internal() {
    // std::cout << "********************** internal_database_basis::open_internal()\n";

    if ( ! create_and_open_environment()) {
        LOG_ERROR(LOG_DATABASE, "Error configuring LMDB environment.");
        return false;
    }

    return open_databases();
}

template <typename Clock>
bool internal_database_basis<Clock>::verify_db_mode_property() const {
    // std::cout << "********************** internal_database_basis::verify_db_mode_property()\n";
    return true;
}

template <typename Clock>
bool internal_database_basis<Clock>::close() {
    // std::cout << "********************** internal_database_basis::close()\n";
    if (db_opened_) {
        db_opened_ = false;
    }

    return true;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past) {
    // std::cout << "********************** internal_database_basis::push_block()\n";

    auto res = push_block(block, height, median_time_past, ! is_old_block(block));
    return res;
}

#endif // ! defined(KTH_DB_READONLY)


template <typename Clock>
utxo_entry internal_database_basis<Clock>::get_utxo(domain::chain::output_point const& point) const {
    // std::cout << "********************** internal_database_basis::get_utxo()\n";
    auto f = utxo_set_.find(point);
    if (f == utxo_set_.end()) {
        return utxo_entry{};
    }
    return f->second;

}

template <typename Clock>
result_code internal_database_basis<Clock>::get_last_height(uint32_t& out_height) const {
    // // std::cout << "++++++++++++++++++++++++++++++++++++++++ internal_database_basis::get_last_height()" << std::endl;

    // printf("-*-*-*-*-*-*-*--*-* this: %x\n", this);

    out_height = last_height_;

    // // std::cout << "++++++++++++++++++++++++++++++++++++++++ internal_database_basis::get_last_height() - last_height_: " << last_height_  << std::endl;
    // // std::cout << "++++++++++++++++++++++++++++++++++++++++ internal_database_basis::get_last_height() - out_height: " << out_height  << std::endl;
    return result_code::success;
}

template <typename Clock>
std::pair<domain::chain::header, uint32_t> internal_database_basis<Clock>::get_header(hash_digest const& hash) const {
    // std::cout << "********************** internal_database_basis::get_header()\n";

    auto f = headers_by_hash_.find(hash);
    if (f == headers_by_hash_.end()) {
        return {};
    }
    auto height = f->second;
    auto header = get_header(height);
    return {header, height};
}

template <typename Clock>
domain::chain::header::list internal_database_basis<Clock>::get_headers(uint32_t from, uint32_t to) const {
    // std::cout << "********************** internal_database_basis::get_headers()\n";
    return {};
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::pop_block(domain::chain::block& out_block) {
    // std::cout << "********************** internal_database_basis::pop_block()\n";
    uint32_t height;

    //TODO: (Mario) use only one transaction ?

    //TODO: (Mario) add overload with tx
    // The blockchain is empty (nothing to pop, not even genesis).
    auto res = get_last_height(height);
    if (res != result_code::success ) {
        return res;
    }

    //TODO: (Mario) add overload with tx
    // This should never become invalid if this call is protected.
    out_block = get_block_reorg(height);
    if ( ! out_block.is_valid()) {
        return result_code::key_not_found;
    }

    res = remove_block(out_block, height);
    if (res != result_code::success) {
        return res;
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::prune() {
    // std::cout << "********************** internal_database_basis::prune()\n";
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)


template <typename Clock>
std::pair<result_code, utxo_pool_t> internal_database_basis<Clock>::get_utxo_pool_from(uint32_t from, uint32_t to) const {
    // std::cout << "********************** internal_database_basis::get_utxo_pool_from()\n";
    return {};

    // throw "get_utxo_pool_from";


}


// Private functions
// ------------------------------------------------------------------------------------------------------

template <typename Clock>
bool internal_database_basis<Clock>::is_old_block(domain::chain::block const& block) const {
    // std::cout << "********************** internal_database_basis::is_old_block()\n";
    return is_old_block_<Clock>(block, limit_);
}

template <typename Clock>
bool internal_database_basis<Clock>::create_and_open_environment() {
    // std::cout << "********************** internal_database_basis::create_and_open_environment()\n";
    return true;
}

inline
int compare_uint64(KTH_DB_val const* a, KTH_DB_val const* b) {

    //TODO(fernando): check this casts... something smells bad
    const uint64_t va = *(const uint64_t *)kth_db_get_data(*a);
    const uint64_t vb = *(const uint64_t *)kth_db_get_data(*b);

    //// std::cout << "va: " << va << std::endl;
    //// std::cout << "vb: " << va << std::endl;

    return (va < vb) ? -1 : va > vb;
}

template <typename Clock>
bool internal_database_basis<Clock>::open_databases() {
    // std::cout << "********************** internal_database_basis::open_databases()\n";


    // KTH_DB_txn* db_txn;

    // auto res = kth_db_txn_begin(env_, NULL, KTH_DB_CONDITIONAL_READONLY, &db_txn);
    // if (res != KTH_DB_SUCCESS) {
    //     return false;
    // }

    // res = kth_db_dbi_open(db_txn, block_header_db_name, KTH_DB_CONDITIONAL_CREATE | KTH_DB_INTEGERKEY, &dbi_block_header_);
    // if (res != KTH_DB_SUCCESS) {
    //     return false;
    // }

    // res = kth_db_dbi_open(db_txn, block_header_by_hash_db_name, KTH_DB_CONDITIONAL_CREATE, &dbi_block_header_by_hash_);
    // if (res != KTH_DB_SUCCESS) {
    //     return false;
    // }

    // res = kth_db_dbi_open(db_txn, utxo_db_name, KTH_DB_CONDITIONAL_CREATE, &dbi_utxo_);
    // if (res != KTH_DB_SUCCESS) {
    //     return false;
    // }

    // res = kth_db_dbi_open(db_txn, reorg_pool_name, KTH_DB_CONDITIONAL_CREATE, &dbi_reorg_pool_);
    // if (res != KTH_DB_SUCCESS) {
    //     return false;
    // }

    // res = kth_db_dbi_open(db_txn, reorg_index_name, KTH_DB_CONDITIONAL_CREATE | KTH_DB_DUPSORT | KTH_DB_INTEGERKEY | KTH_DB_DUPFIXED, &dbi_reorg_index_);
    // if (res != KTH_DB_SUCCESS) {
    //     return false;
    // }

    // res = kth_db_dbi_open(db_txn, reorg_block_name, KTH_DB_CONDITIONAL_CREATE | KTH_DB_INTEGERKEY, &dbi_reorg_block_);
    // if (res != KTH_DB_SUCCESS) {
    //     return false;
    // }

    // res = kth_db_dbi_open(db_txn, db_properties_name, KTH_DB_CONDITIONAL_CREATE | KTH_DB_INTEGERKEY, &dbi_properties_);
    // if (res != KTH_DB_SUCCESS) {
    //     return false;
    // }

#if defined(KTH_DB_NEW_BLOCKS)
    res = kth_db_dbi_open(db_txn, block_db_name, KTH_DB_CONDITIONAL_CREATE | KTH_DB_INTEGERKEY, &dbi_block_db_);
    if (res != KTH_DB_SUCCESS) {
        return false;
    }
#endif

#if defined(KTH_DB_NEW_FULL)

    res = kth_db_dbi_open(db_txn, block_db_name, KTH_DB_CONDITIONAL_CREATE | KTH_DB_DUPSORT | KTH_DB_INTEGERKEY | KTH_DB_DUPFIXED  | MDB_INTEGERDUP, &dbi_block_db_);
    if (res != KTH_DB_SUCCESS) {
        return false;
    }

    res = kth_db_dbi_open(db_txn, transaction_db_name, KTH_DB_CONDITIONAL_CREATE | KTH_DB_INTEGERKEY, &dbi_transaction_db_);
    if (res != KTH_DB_SUCCESS) {
        return false;
    }

    res = kth_db_dbi_open(db_txn, transaction_hash_db_name, KTH_DB_CONDITIONAL_CREATE, &dbi_transaction_hash_db_);
    if (res != KTH_DB_SUCCESS) {
        return false;
    }

    res = kth_db_dbi_open(db_txn, history_db_name, KTH_DB_CONDITIONAL_CREATE | KTH_DB_DUPSORT | KTH_DB_DUPFIXED, &dbi_history_db_);
    if (res != KTH_DB_SUCCESS) {
        return false;
    }

    res = kth_db_dbi_open(db_txn, spend_db_name, KTH_DB_CONDITIONAL_CREATE, &dbi_spend_db_);
    if (res != KTH_DB_SUCCESS) {
        return false;
    }

    res = kth_db_dbi_open(db_txn, transaction_unconfirmed_db_name, KTH_DB_CONDITIONAL_CREATE, &dbi_transaction_unconfirmed_db_);
    if (res != KTH_DB_SUCCESS) {
        return false;
    }

    mdb_set_dupsort(db_txn, dbi_history_db_, compare_uint64);

#endif //KTH_DB_NEW_FULL

    // db_opened_ = kth_db_txn_commit(db_txn) == KTH_DB_SUCCESS;
    // return db_opened_;

    return true;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::remove_inputs(hash_digest const& tx_id, uint32_t height, domain::chain::input::list const& inputs, bool insert_reorg) {
    // std::cout << "********************** internal_database_basis::remove_inputs()\n";
    uint32_t pos = 0;
    for (auto const& input: inputs) {

        domain::chain::input_point const inpoint {tx_id, pos};
        auto const& prevout = input.previous_output();

#if defined(KTH_DB_NEW_FULL)
        auto res = insert_input_history(inpoint, height, input);
        if (res != result_code::success) {
            return res;
        }

        //set spender height in tx database
        //Note(Knuth): Commented because we don't validate transaction duplicates (BIP-30)
        /*res = set_spend(prevout, height);
        if (res != result_code::success) {
            return res;
        }*/

#else
    result_code res;
#endif

        // res = remove_utxo(height, prevout, insert_reorg);
        res = remove_utxo(height, prevout, insert_reorg);
        if (res != result_code::success) {
            return res;
        }

#if defined(KTH_DB_NEW_FULL)

        //insert in spend database
        res = insert_spend(prevout, inpoint);
        if (res != result_code::success) {
            return res;
        }

#endif

        ++pos;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_outputs(hash_digest const& tx_id, uint32_t height, domain::chain::output::list const& outputs, uint32_t median_time_past, bool coinbase) {
    // std::cout << "********************** internal_database_basis::insert_outputs()\n";
    uint32_t pos = 0;
    for (auto const& output: outputs) {
        // auto res = insert_utxo(domain::chain::point{tx_id, pos}, output, height, median_time_past, coinbase);
        auto res = insert_utxo(domain::chain::point{tx_id, pos}, output, height, median_time_past, coinbase);
        if (res != result_code::success) {
            return res;
        }

        #if defined(KTH_DB_NEW_FULL)

        res = insert_output_history(tx_id, height, pos, output);
        if (res != result_code::success) {
            return res;
        }

        #endif

        ++pos;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_outputs_error_treatment(uint32_t height, uint32_t median_time_past, bool coinbase, hash_digest const& txid, domain::chain::output::list const& outputs) {
    // std::cout << "********************** internal_database_basis::insert_outputs_error_treatment()\n";
    auto res = insert_outputs(txid, height, outputs, median_time_past, coinbase);

    if (res == result_code::duplicated_key) {
        //TODO(fernando): log and continue
        return result_code::success_duplicate_coinbase;
    }
    return res;
}

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::push_transactions_outputs_non_coinbase(uint32_t height, uint32_t median_time_past, bool coinbase, I f, I l) {
    // std::cout << "********************** internal_database_basis::push_transactions_outputs_non_coinbase()\n";
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        auto res = insert_outputs_error_treatment(height, median_time_past, coinbase, tx.hash(), tx.outputs());
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }
    return result_code::success;
}

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::remove_transactions_inputs_non_coinbase(uint32_t height, I f, I l, bool insert_reorg) {
    // std::cout << "********************** internal_database_basis::remove_transactions_inputs_non_coinbase()\n";
    while (f != l) {
        auto const& tx = *f;
        auto res = remove_inputs(tx.hash(), height, tx.inputs(), insert_reorg);
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }
    return result_code::success;
}

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::push_transactions_non_coinbase(uint32_t height, uint32_t median_time_past, bool coinbase, I f, I l, bool insert_reorg) {
    // std::cout << "********************** internal_database_basis::push_transactions_non_coinbase()\n";
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    auto res = push_transactions_outputs_non_coinbase(height, median_time_past, coinbase, f, l);
    if (res != result_code::success) {
        return res;
    }

    return remove_transactions_inputs_non_coinbase(height, f, l, insert_reorg);
}

template <typename Clock>
// result_code internal_database_basis<Clock>::push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg) {
result_code internal_database_basis<Clock>::push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg) {
    // std::cout << "********************** internal_database_basis::push_block()\n";
    //precondition: block.transactions().size() >= 1

    // auto res = push_block_header(block, height);
    auto res = push_block_header(block, height);
    if (res != result_code::success) {
        return res;
    }

    auto const& txs = block.transactions();

#if defined(KTH_DB_NEW_BLOCKS)
    res = insert_block(block, height);
    if (res != result_code::success) {
        // // std::cout << "22222222222222" << static_cast<uint32_t>(res) << "\n";
        return res;
    }

#elif defined(KTH_DB_NEW_FULL)

    auto tx_count = get_tx_count(db_txn);

    res = insert_block(block, height, tx_count);
    if (res != result_code::success) {
        // // std::cout << "22222222222222" << static_cast<uint32_t>(res) << "\n";
        return res;
    }

    res = insert_transactions(txs.begin(), txs.end(), height, median_time_past, tx_count);
    if (res == result_code::duplicated_key) {
        res = result_code::success_duplicate_coinbase;
    } else if (res != result_code::success) {
        return res;
    }

#endif

    if ( insert_reorg ) {
        res = push_block_reorg(block, height);
        if (res != result_code::success) {
            return res;
        }
    }

    auto const& coinbase = txs.front();

    // auto fixed = utxo_entry::to_data_fixed(height, median_time_past, true);                                     //TODO(fernando): podría estar afuera de la DBTx
    auto res0 = insert_outputs_error_treatment(height, median_time_past, true, coinbase.hash(), coinbase.outputs());     //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
    if ( ! succeed(res0)) {
        return res0;
    }

    // fixed.back() = 0;   //The last byte equal to 0 means NonCoinbaseTx
    res = push_transactions_non_coinbase(height, median_time_past, false, txs.begin() + 1, txs.end(), insert_reorg);
    if (res != result_code::success) {
        return res;
    }

    if (res == result_code::success_duplicate_coinbase)
        return res;

    return res0;
}

template <typename Clock>
// result_code internal_database_basis<Clock>::push_genesis(domain::chain::block const& block) {
result_code internal_database_basis<Clock>::push_genesis(domain::chain::block const& block) {
    // std::cout << "********************** internal_database_basis::push_genesis()\n";


//     auto res = push_block_header(block, 0);
//     if (res != result_code::success) {
//         return res;
//     }

// #if defined(KTH_DB_NEW_BLOCKS)
//     res = insert_block(block, 0);
// #elif defined(KTH_DB_NEW_FULL)
//     auto tx_count = get_tx_count(db_txn);
//     res = insert_block(block, 0, tx_count);

//     if (res != result_code::success) {
//         return res;
//     }

//     auto const& txs = block.transactions();
//     auto const& coinbase = txs.front();
//     auto const& hash = coinbase.hash();
//     auto const median_time_past = block.header().validation.median_time_past;

//     res = insert_transaction(tx_count, coinbase, 0, median_time_past, 0);
//     if (res != result_code::success && res != result_code::duplicated_key) {
//         return res;
//     }

//     res = insert_output_history(hash, 0, 0, coinbase.outputs()[0]);
//     if (res != result_code::success) {
//         return res;
//     }

// #endif

//     return res;

    auto res = push_block_header(block, 0);
    return res;

}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_outputs(hash_digest const& txid, domain::chain::output::list const& outputs) {
    // std::cout << "********************** internal_database_basis::remove_outputs()\n";
    uint32_t pos = outputs.size() - 1;
    for (auto const& output: outputs) {
        domain::chain::output_point const point {txid, pos};
        // auto res = remove_utxo(0, point, false);
        auto res = remove_utxo(0, point, false);
        if (res != result_code::success) {
            return res;
        }
        --pos;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_inputs(domain::chain::input::list const& inputs) {
    // std::cout << "********************** internal_database_basis::insert_inputs()\n";
    for (auto const& input: inputs) {
        auto const& point = input.previous_output();

        auto res = insert_output_from_reorg_and_remove(point);
        if (res != result_code::success) {
            return res;
        }
    }
    return result_code::success;
}

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::insert_transactions_inputs_non_coinbase(I f, I l) {
    // std::cout << "********************** internal_database_basis::insert_transactions_inputs_non_coinbase()\n";
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        auto res = insert_inputs(tx.inputs());
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }

    return result_code::success;
}

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::remove_transactions_outputs_non_coinbase(I f, I l) {
    // std::cout << "********************** internal_database_basis::remove_transactions_outputs_non_coinbase()\n";
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        auto res = remove_outputs(tx.hash(), tx.outputs());
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }

    return result_code::success;
}

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::remove_transactions_non_coinbase(I f, I l) {
    // std::cout << "********************** internal_database_basis::remove_transactions_non_coinbase()\n";
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    auto res = insert_transactions_inputs_non_coinbase(f, l);
    if (res != result_code::success) {
        return res;
    }
    return remove_transactions_outputs_non_coinbase(f, l);
}


template <typename Clock>
result_code internal_database_basis<Clock>::remove_block(domain::chain::block const& block, uint32_t height) {
    // std::cout << "********************** internal_database_basis::remove_block()\n";
    //precondition: block.transactions().size() >= 1

    auto const& txs = block.transactions();
    auto const& coinbase = txs.front();

    //UTXO
    auto res = remove_transactions_non_coinbase(txs.begin() + 1, txs.end());
    if (res != result_code::success) {
        return res;
    }

    //UTXO Coinbase
    //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
    res = remove_outputs(coinbase.hash(), coinbase.outputs());
    if (res != result_code::success) {
        return res;
    }

    //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
    res = remove_block_header(block.hash(), height);
    if (res != result_code::success) {
        return res;
    }

    res = remove_block_reorg(height);
    if (res != result_code::success) {
        return res;
    }

    res = remove_reorg_index(height);
    if (res != result_code::success && res != result_code::key_not_found) {
        return res;
    }

#if defined(KTH_DB_NEW_FULL)
    //Transaction Database
    res = remove_transactions(block, height);
    if (res != result_code::success) {
        return res;
    }
#endif //defined(KTH_DB_NEW_FULL)

#if defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)
    res = remove_blocks_db(height);
    if (res != result_code::success) {
        return res;
    }
#endif //defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_INTERNAL_DATABASE_IPP_

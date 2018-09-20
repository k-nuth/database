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
// #ifdef BITPRIM_DB_NEW

#include <bitcoin/database/databases/utxo_database.hpp>

#include <cstddef>
#include <cstdint>
// #include <bitcoin/bitcoin.hpp>

#ifdef BITPRIM_UTXO_4BYTES_INDEX
#define BITPRIM_UXTO_WIRE true
#else
#define BITPRIM_UXTO_WIRE false
#endif


namespace libbitcoin { namespace database {

// Transactions uses a hash table index, O(1).
utxo_database::utxo_database(path const& db_dir)
    : db_dir_(db_dir)
{}

utxo_database::~utxo_database() {
    close();
}

// Create.
// ----------------------------------------------------------------------------

bool utxo_database::create_and_open_environment() {

    if (mdb_env_create(&env_) != MDB_SUCCESS) {
        return false;
    }
    env_created_ = true;


    // E(mdb_env_set_maxreaders(env_, 1));
    // E(mdb_env_set_mapsize(env_, 10485760));
    auto res = mdb_env_set_mapsize(env_, size_t(10485760) * 1024);      //TODO(fernando): hardcoded
    if (res != MDB_SUCCESS) {
        return false;
    }

    auto res = mdb_env_set_maxdbs(env_, 2);      //TODO(fernando): hardcoded
    if (res != MDB_SUCCESS) {
        return false;
    }

    //TODO(fernando): use MDB_RDONLY for Read-only node
    //                  MDB_WRITEMAP ????
    //                  MDB_NOMETASYNC ????
    //                  MDB_NOSYNC ????

    //TODO(fernando): put the 0664 in the CFG file
    if (mdb_env_open(env_, db_dir_.c_str(), MDB_FIXEDMAP | MDB_NORDAHEAD | MDB_NOMEMINIT, 0664) != MDB_SUCCESS) {
        return false;
    }

    return true;
}

// Initialize files and start.
bool utxo_database::create() {
    boost::system::error_code ec;

    if ( ! create_directories(db_dir_, ec)) {
        // if (ec.value() == directory_exists) {
        //     LOG_ERROR(LOG_NODE) << format(BN_INITCHAIN_EXISTS) % directory;
        //     return false;
        // }

        // LOG_ERROR(LOG_NODE) << format(BN_INITCHAIN_NEW) % directory % ec.message();
        return false;
    }
    // LOG_INFO(LOG_NODE) << format(BN_INITIALIZING_CHAIN) % directory;

    return create_and_open_environment();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// Start files and primitives.
bool utxo_database::open() {
    if ( ! create_and_open_environment()) {
        return false;
    }

    MDB_txn* db_txn;
    if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
        return false;
    }

    if (mdb_dbi_open(db_txn, utxo_db_name, 0, &dbi_utxo_) != MDB_SUCCESS) {
        return false;
    }
    if (mdb_dbi_open(db_txn, reorg_pool_name, 0, &dbi_reorg_pool_) != MDB_SUCCESS) {
        return false;
    }
    db_created_ = true;

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return false;
    }

    return true;
}

// Close files.
bool utxo_database::close() {
    if (db_created_) {
        mdb_dbi_close(env_, dbi_utxo_);
        mdb_dbi_close(env_, dbi_reorg_pool_);
        db_created_ = false;
    }

    if (env_created_) {
        mdb_env_close(env_);
        env_created_ = false;
    }

    return true;
}



/*
Block 1
    Transaccion 1 (Coinbase)
        Sólo Outputs (1 o más)                      0 -> Insertar (Point, Output)

    Transaccion 2
        Inputs (1 o mas)                            1 -> Remover (Input.PreviousOutput)
        Outputs (1 o más)                           2 -> Insertar (Point, Output)

    Transaccion 3
        Inputs (1 o mas)                            3 -> Remover (Input.PreviousOutput)
        Outputs (1 o más)                           4 -> Insertar (Point, Output)

Block 2
    Transaccion 1 (Coinbase)
        Sólo Outputs (1 o más)                      5 -> Insertar (Point, Output)

    Transaccion 2
        Inputs (1 o mas)                            6 -> Remover (Input.PreviousOutput)
        Outputs (1 o más)                           7 -> Insertar (Point, Output)

    Transaccion 3
        Inputs (1 o mas)                            8 -> Remover (Input.PreviousOutput)
        Outputs (1 o más)                           9 -> Insertar (Point, Output)


Block 3
    Transaccion 1 (Coinbase)
        Sólo Outputs (1 o más)                      10 -> Insertar (Point, Output)

    Transaccion 2
        Inputs (1 o mas)                            11 -> Remover (Input.PreviousOutput)
        Outputs (1 o más)                           12 -> Insertar (Point, Output)

    Transaccion 3
        Inputs (1 o mas)                            13 -> Remover (Input.PreviousOutput)    -> Insert(Input.PreviousOutput, ????)
        Outputs (1 o más)                           14 -> Insertar (Point, Output)          -> Remove(Point)

*/

// private
utxo_code utxo_database::remove(size_t height, chain::output_point const& point, MDB_txn* db_txn) {
    auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
    MDB_val key {keyarr.size(), keyarr.data()};

    auto res = mdb_get(db_txn, dbi_utxo_, &key, &value);
    if (res == MDB_NOTFOUND) return utxo_code::key_not_found;
    if (res != MDB_SUCCESS) return utxo_code::other;

    std::vector<uint8_t> new_data(value.mv_size + sizeof(uint32_t));
    // data_chunk data {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};

        // uint32_t x = 0x0C7E;
        // uint8_t bytes[3];

        // bytes[0] = (x >> 0)  & 0xFF;
        // bytes[1] = (x >> 8)  & 0xFF;
        // bytes[2] = (x >> 16) & 0xFF;


        // /* Go back. */
        // x = (bytes[2] << 16) | (bytes[1] << 8) | (bytes[0] << 0);


    auto res = mdb_del(db_txn, dbi_utxo_, &key, NULL);
    if (res == MDB_NOTFOUND) return utxo_code::key_not_found;
    if (res != MDB_SUCCESS) return utxo_code::other;
    return utxo_code::success;
}

// private
utxo_code utxo_database::insert(chain::output_point const& point, chain::output const& output, MDB_txn* db_txn) {
    auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
    auto valuearr = output.to_data(false);

    MDB_val key   {keyarr.size(), keyarr.data()};
    MDB_val value {valuearr.size(), valuearr.data()};

    auto res = mdb_put(db_txn, dbi_utxo_, &key, &value, MDB_NOOVERWRITE);

    if (res == MDB_KEYEXIST) return utxo_code::duplicated_key;
    if (res != MDB_SUCCESS) return utxo_code::other;
    return utxo_code::success;
}


// private
utxo_code utxo_database::remove_inputs(chain::input::list const& inputs, MDB_txn* db_txn) {
    for (auto const& input: inputs) {
        auto res = remove(input.previous_output(), db_txn);
        if (res != utxo_code::success) {
            return res
        }
    }
    return utxo_code::success;
}

// private
utxo_code utxo_database::insert_outputs(hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
    uint32_t pos = 0;
    for (auto const& output: outputs) {
        auto res = insert(chain::point{txid, pos}, output, db_txn);
        if (res != utxo_code::success) {
            return res
        }
        ++pos;
    }
    return utxo_code::success;
}

// Push functions   ------------------------------------------------------

// private
utxo_code utxo_database::insert_outputs_error_treatment(hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
    auto res = insert_outputs(txid, outputs, db_txn);
    
    if (res == utxo_code::duplicated_key) {
        //TODO(fernando): log and continue
        return utxo_code::success;
    }
    return res;
}

// private
utxo_code utxo_database::push_transaction_non_coinbase(chain::transaction const& tx, MDB_txn* db_txn) {

    auto res = remove_inputs(tx.inputs(), db_txn);
    if (res != utxo_code::success) {
        return res;
    }

    return insert_outputs_error_treatment(tx, db_txn);
}

// private
template <typename I>
utxo_code utxo_database::push_transactions_non_coinbase(I f, I l, MDB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        auto res = push_transaction_non_coinbase(tx, db_txn);
        if (res != utxo_code::success) {
            return res;
        }
        ++f;
    }

    return utxo_code::success;
}

// private
utxo_code utxo_database::push_block(chain::block const& block, MDB_txn* db_txn) {
    //precondition: block.transactions().size() >= 1

    auto const& txs = block.transactions();
    auto const& coinbase = txs.front();

    auto res = push_transaction(coinbase, db_txn);
    if (res != utxo_code::success) {
        return res;
    }

    res = push_transactions_non_coinbase(txs.begin() + 1, txs.end(), db_txn);
    if (res != utxo_code::success) {
        return res;
    }
    return utxo_code::success;
}


// Remove functions ------------------------------------------------------

// private
utxo_code utxo_database::remove_transaction(chain::transaction const& tx, MDB_txn* db_txn) {
    auto res = remove_inputs(tx.inputs(), db_txn);
    
    if (res == utxo_code::key_not_found) {
        //TODO(fernando): log and continue
        return utxo_code::success;
    }
    return res;
}

// private
utxo_code utxo_database::remove_transaction_non_coinbase(chain::transaction const& tx, MDB_txn* db_txn) {

    auto res = insert_outputs(tx.hash(), tx.outputs(), db_txn);
    if (res != utxo_code::success) {
        return res;
    }

    return remove_transaction(tx, db_txn);
}

// private
template <typename I>
utxo_code utxo_database::remove_transactions_non_coinbase(I f, I l, MDB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
    
    if (f != l) return utxo_code::success;

    // reverse order
    while (f != --l) {
        auto const& tx = *l;
        auto res = remove_transaction_non_coinbase(tx, db_txn);
        if (res != utxo_code::success) {
            return res;
        }
    }

    return utxo_code::success;
}

// private
utxo_code utxo_database::remove_block(chain::block const& block, MDB_txn* db_txn) {
    //precondition: block.transactions().size() >= 1

    auto const& txs = block.transactions();
    auto const& coinbase = txs.front();

    res = remove_transactions_non_coinbase(txs.begin() + 1, txs.end(), db_txn);
    if (res != utxo_code::success) {
        return res;
    }

    auto res = remove_transaction(coinbase, db_txn);
    if (res != utxo_code::success) {
        return res;
    }
    return utxo_code::success;
}


// Public interface functions ---------------------------

utxo_code utxo_database::push_block(chain::block const& block) {
    MDB_txn* db_txn;
    auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
    if (res0 != MDB_SUCCESS) {
        return utxo_code::other;
    }

    auto res = push_block(block, db_txn);
    if (res != utxo_code::success) {
        mdb_txn_abort(db_txn);
        return res;
    }

    auto res2 = mdb_txn_commit(db_txn);
    if (res2 != MDB_SUCCESS) {
        return utxo_code::other;
    }
    return utxo_code::success;
}

utxo_code utxo_database::remove_block(chain::block const& block) {

    MDB_txn* db_txn;
    auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
    if (res0 != MDB_SUCCESS) {
        return utxo_code::other;
    }

    auto res = remove_block(block, db_txn);
    if (res != utxo_code::success) {
        mdb_txn_abort(db_txn);
        return res;
    }

    auto res2 = mdb_txn_commit(db_txn);
    if (res2 != MDB_SUCCESS) {
        return utxo_code::other;
    }
    return utxo_code::success;
}


// boost::optional<get_return_t> utxo_database::get(chain::output_point const& key) {
utxo_database::get_return_t utxo_database::get(chain::output_point const& point) {
    MDB_txn* db_txn;
    if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
        return utxo_database::get_return_t{};
    }

    auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
    MDB_val key {keyarr.size(), keyarr.data()};
    MDB_val value;

    if (mdb_get(db_txn, dbi_utxo_, &key, &value) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        // mdb_txn_abort(db_txn);
        return utxo_database::get_return_t{};
    }

    data_chunk data {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};
    auto res = chain::output::factory_from_data(data, false);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return utxo_database::get_return_t{};
    }

    return res;
}

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

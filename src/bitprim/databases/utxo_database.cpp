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

#include <bitprim/database/databases/utxo_database.hpp>

#include <cstddef>
#include <cstdint>
// #include <bitcoin/bitcoin.hpp>

#ifdef BITPRIM_UTXO_4BYTES_INDEX
#define BITPRIM_UXTO_WIRE true
#else
#define BITPRIM_UXTO_WIRE false
#endif


namespace libbitcoin { 
namespace database {

constexpr char utxo_database::block_header_db_name[];
constexpr char utxo_database::block_header_by_hash_db_name[];
constexpr char utxo_database::utxo_db_name[];
constexpr char utxo_database::reorg_pool_name[];
constexpr char utxo_database::reorg_index_name[];

// static
bool utxo_database::succeed(utxo_code code) {
    return code == utxo_code::success || code == utxo_code::success_duplicate_coinbase;
}

// Constructors.
// ----------------------------------------------------------------------------

utxo_database::utxo_database(path const& db_dir, std::chrono::seconds limit)
    : db_dir_(db_dir), limit_(limit)
{}

utxo_database::~utxo_database() {
    close();
}

bool utxo_database::is_old_block(chain::block const& block) const {
    return is_old_block_(block, limit_);
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

    auto res = mdb_env_set_mapsize(env_, size_t(10485760) * 1024 * 10);      //TODO(fernando): hardcoded
    // auto res = mdb_env_set_mapsize(env_, size_t(10485760) * 1024);      //TODO(fernando): hardcoded
    // auto res = mdb_env_set_mapsize(env_, size_t(10485760) * 10);      //TODO(fernando): hardcoded
    if (res != MDB_SUCCESS) {
        return false;
    }

    res = mdb_env_set_maxdbs(env_, 5);      //TODO(fernando): hardcoded
    if (res != MDB_SUCCESS) {
        return false;
    }

    //TODO(fernando): use MDB_RDONLY for Read-only node
    //                  MDB_WRITEMAP ????
    //                  MDB_NOMETASYNC ????
    //                  MDB_NOSYNC ????

    //TODO(fernando): put the 0664 in the CFG file
    //TODO(fernando): MDB_NOSYNC para IBD ??

    // res = mdb_env_open(env_, db_dir_.c_str(), MDB_FIXEDMAP | MDB_NORDAHEAD | MDB_NOMEMINIT, 0664);
    // res = mdb_env_open(env_, db_dir_.c_str(), MDB_FIXEDMAP, 0664);
    
    // res = mdb_env_open(env_, db_dir_.c_str(), 0, 0664);
    res = mdb_env_open(env_, db_dir_.c_str(), MDB_NORDAHEAD | MDB_NOSYNC, 0664);


    // std::cout << "utxo_database::create_and_open_environment() - mdb_env_open: " << res << std::endl;
    return res == MDB_SUCCESS;
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

    return open();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// private
bool utxo_database::open_databases() {
    MDB_txn* db_txn;
    if (mdb_txn_begin(env_, NULL, 0, &db_txn) != MDB_SUCCESS) {
        return false;
    }

    //TODO(fernando): check if we can use an optimization for fixed size keys and values
    auto res = mdb_dbi_open(db_txn, block_header_db_name, MDB_CREATE | MDB_INTEGERKEY, &dbi_block_header_);
    if (res != MDB_SUCCESS) {
        return false;
    }
    res = mdb_dbi_open(db_txn, block_header_by_hash_db_name, MDB_CREATE, &dbi_block_header_by_hash_);
    if (res != MDB_SUCCESS) {
        return false;
    }

    res = mdb_dbi_open(db_txn, utxo_db_name, MDB_CREATE, &dbi_utxo_);
    if (res != MDB_SUCCESS) {
        return false;
    }

    res = mdb_dbi_open(db_txn, reorg_pool_name, MDB_CREATE, &dbi_reorg_pool_);
    if (res != MDB_SUCCESS) {
        return false;
    }

    res = mdb_dbi_open(db_txn, reorg_index_name, MDB_CREATE | MDB_DUPSORT | MDB_INTEGERKEY | MDB_DUPFIXED, &dbi_reorg_index_);
    if (res != MDB_SUCCESS) {
        return false;
    }

    db_created_ = true;

    return mdb_txn_commit(db_txn) == MDB_SUCCESS;
}

// Start files and primitives.
bool utxo_database::open() {
    if ( ! create_and_open_environment()) {
        return false;
    }
    return open_databases();
}

// Close files.
bool utxo_database::close() {
    if (db_created_) {
        
        mdb_dbi_close(env_, dbi_block_header_);
        mdb_dbi_close(env_, dbi_block_header_by_hash_);

        mdb_dbi_close(env_, dbi_utxo_);
        mdb_dbi_close(env_, dbi_reorg_pool_);
        mdb_dbi_close(env_, dbi_reorg_index_);
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

// template <std::size_t N>
// std::size_t SizeOf(uint8_t(&)[N]) {
// 	return S;
// }


// std::array<uint8_t, size(uint32_t)> uint32_to_bytes(uint32_t x) {
// 	return { (x >> 0)  & 0xFF,
//              (x >> 8)  & 0xFF,
//              (x >> 16) & 0xFF,
//              (x >> 24) & 0xFF};
// }


// private
utxo_code utxo_database::insert_reorg_pool(uint32_t height, MDB_val& key, MDB_txn* db_txn) {
    MDB_val value;
    auto res = mdb_get(db_txn, dbi_utxo_, &key, &value);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::insert_reorg_pool - mdb_get: " << res;        
        return utxo_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::insert_reorg_pool - mdb_get: " << res;        
        return utxo_code::other;
    }
    

    res = mdb_put(db_txn, dbi_reorg_pool_, &key, &value, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::insert_reorg_pool - mdb_put(0): " << res;        
        return utxo_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::insert_reorg_pool - mdb_put(0): " << res;        
        return utxo_code::other;
    }

    MDB_val key_index {sizeof(height), &height};
    MDB_val value_index {key.mv_size, key.mv_data};
    // res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, MDB_APPENDDUP);
    // res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, MDB_APPEND);
    // res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, MDB_NODUPDATA);
    res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, 0);
    
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::insert_reorg_pool - mdb_put(1): " << res;
        return utxo_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::insert_reorg_pool - mdb_put(1): " << res;
        return utxo_code::other;
    }

    return utxo_code::success;
}

// private
utxo_code utxo_database::remove(uint32_t height, chain::output_point const& point, bool insert_reorg, MDB_txn* db_txn) {
    auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
    MDB_val key {keyarr.size(), keyarr.data()};

    if (insert_reorg) {
        auto res0 = insert_reorg_pool(height, key, db_txn);
        if (res0 != utxo_code::success) return res0;
    }

    auto res = mdb_del(db_txn, dbi_utxo_, &key, NULL);
    if (res == MDB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::remove - mdb_del: " << res;
        return utxo_code::key_not_found;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::remove - mdb_del: " << res;
        return utxo_code::other;
    }
    return utxo_code::success;
}

// private
utxo_code utxo_database::insert(chain::output_point const& point, chain::output const& output, data_chunk const& fixed_data, MDB_txn* db_txn) {
    auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
    // auto valuearr = entry.to_data();
    auto valuearr = utxo_entry::to_data_pepe(output, fixed_data);

    MDB_val key   {keyarr.size(), keyarr.data()};
    MDB_val value {valuearr.size(), valuearr.data()};
    auto res = mdb_put(db_txn, dbi_utxo_, &key, &value, MDB_NOOVERWRITE);

    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::insert - mdb_put: " << res;        
        return utxo_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::insert - mdb_put: " << res;        
        return utxo_code::other;
    }
    return utxo_code::success;
}

// private
utxo_code utxo_database::remove_inputs(uint32_t height, chain::input::list const& inputs, bool insert_reorg, MDB_txn* db_txn) {
    for (auto const& input: inputs) {
        auto res = remove(height, input.previous_output(), insert_reorg, db_txn);
        if (res != utxo_code::success) {
            return res;
        }
    }
    return utxo_code::success;
}

// private
utxo_code utxo_database::insert_outputs(hash_digest const& txid, chain::output::list const& outputs, data_chunk const& fixed_data, MDB_txn* db_txn) {
    uint32_t pos = 0;
    for (auto const& output: outputs) {
        auto res = insert(chain::point{txid, pos}, output, fixed_data, db_txn);
        if (res != utxo_code::success) {
            return res;
        }
        ++pos;
    }
    return utxo_code::success;
}

// Push functions   ------------------------------------------------------

// private
utxo_code utxo_database::insert_outputs_error_treatment(uint32_t height, data_chunk const& fixed_data, hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
    auto res = insert_outputs(txid, outputs, fixed_data, db_txn);
    
    if (res == utxo_code::duplicated_key) {
        //TODO(fernando): log and continue
        return utxo_code::success_duplicate_coinbase;
    }
    return res;
}

// private
utxo_code utxo_database::push_transaction_non_coinbase(uint32_t height, data_chunk const& fixed_data, chain::transaction const& tx, bool insert_reorg, MDB_txn* db_txn) {

    auto res = remove_inputs(height, tx.inputs(), insert_reorg, db_txn);
    if (res != utxo_code::success) {
        return res;
    }

    return insert_outputs_error_treatment(height, fixed_data, tx.hash(), tx.outputs(), db_txn);
}

// private
template <typename I>
utxo_code utxo_database::push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, MDB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        auto res = push_transaction_non_coinbase(height, fixed_data, tx, insert_reorg, db_txn);
        if (res != utxo_code::success) {
            return res;
        }
        ++f;
    }

    return utxo_code::success;
}

// private
utxo_code utxo_database::push_block_header(chain::block const& block, uint32_t height, MDB_txn* db_txn) {

    auto valuearr = block.header().to_data(true);
    MDB_val key {sizeof(height), &height};
    MDB_val value {valuearr.size(), valuearr.data()};
    auto res = mdb_put(db_txn, dbi_block_header_, &key, &value, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::push_block_header - mdb_put(0) " << res;
        return utxo_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::push_block_header - mdb_put(0) " << res;        
        return utxo_code::other;
    }

    auto key_by_hash_arr = block.hash();
    MDB_val key_by_hash {key_by_hash_arr.size(), key_by_hash_arr.data()};
    res = mdb_put(db_txn, dbi_block_header_by_hash_, &key_by_hash, &key, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::push_block_header - mdb_put(1) " << res;        
        return utxo_code::duplicated_key;
    }
    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::push_block_header - mdb_put(1) " << res;        
        return utxo_code::other;
    }

    return utxo_code::success;
}

// private
utxo_code utxo_database::push_block(chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg, MDB_txn* db_txn) {
    //precondition: block.transactions().size() >= 1

    auto res = push_block_header(block, height, db_txn);
    if (res != utxo_code::success) {
        return res;
    }

    auto const& txs = block.transactions();
    auto const& coinbase = txs.front();

    auto fixed = utxo_entry::to_data_fixed(height, median_time_past, true);
    auto res0 = insert_outputs_error_treatment(height, fixed, coinbase.hash(), coinbase.outputs(), db_txn);
    if ( ! succeed(res0)) {
        return res0;
    }

    fixed.back() = 0;   //TODO(fernando): comment this
    res = push_transactions_non_coinbase(height, fixed, txs.begin() + 1, txs.end(), insert_reorg, db_txn);
    if (res != utxo_code::success) {
        return res;
    }
    return res0;
}

// private
utxo_code utxo_database::push_genesis(chain::block const& block, MDB_txn* db_txn) {
    auto res = push_block_header(block, 0, db_txn);
    return res;
}


// Remove functions ------------------------------------------------------

// // private
// utxo_code utxo_database::remove_transaction(chain::transaction const& tx, MDB_txn* db_txn) {
//     auto res = remove_inputs(tx.inputs(), db_txn);
    
//     if (res == utxo_code::key_not_found) {
//         //TODO(fernando): log and continue
//         return utxo_code::success;
//     }
//     return res;
// }

// // private
// utxo_code utxo_database::remove_transaction_non_coinbase(chain::transaction const& tx, MDB_txn* db_txn) {

//     auto res = insert_outputs(tx.hash(), tx.outputs(), db_txn);
//     if (res != utxo_code::success) {
//         return res;
//     }

//     return remove_transaction(tx, db_txn);
// }

// // private
// template <typename I>
// utxo_code utxo_database::remove_transactions_non_coinbase(I f, I l, MDB_txn* db_txn) {
//     // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
    
//     if (f != l) return utxo_code::success;

//     // reverse order
//     while (f != --l) {
//         auto const& tx = *l;
//         auto res = remove_transaction_non_coinbase(tx, db_txn);
//         if (res != utxo_code::success) {
//             return res;
//         }
//     }

//     return utxo_code::success;
// }

// // private
// utxo_code utxo_database::remove_block(chain::block const& block, MDB_txn* db_txn) {
//     //precondition: block.transactions().size() >= 1

//     auto const& txs = block.transactions();
//     auto const& coinbase = txs.front();

//     res = remove_transactions_non_coinbase(txs.begin() + 1, txs.end(), db_txn);
//     if (res != utxo_code::success) {
//         return res;
//     }

//     auto res = remove_transaction(coinbase, db_txn);
//     if (res != utxo_code::success) {
//         return res;
//     }
//     return utxo_code::success;
// }


// Public interface functions ---------------------------

utxo_code utxo_database::push_block(chain::block const& block, uint32_t height, uint32_t median_time_past) {

    MDB_txn* db_txn;
    auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
    if (res0 != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::push_block - mdb_txn_begin " << res0;
        return utxo_code::other;
    }

    auto res = push_block(block, height, median_time_past, ! is_old_block(block), db_txn);
    if (! succeed(res)) {
        mdb_txn_abort(db_txn);
        return res;
    }

    auto res2 = mdb_txn_commit(db_txn);
    if (res2 != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "utxo_database::push_block - mdb_txn_commit " << res2;
        return utxo_code::other;
    }
    return res;
}

utxo_code utxo_database::push_genesis(chain::block const& block) {
    MDB_txn* db_txn;
    auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
    if (res0 != MDB_SUCCESS) {
        return utxo_code::other;
    }

    // auto hash = block.hash();
    // std::cout << encode_hash(hash) << std::endl;

    auto res = push_genesis(block, db_txn);
    if (! succeed(res)) {
        mdb_txn_abort(db_txn);
        return res;
    }

    auto res2 = mdb_txn_commit(db_txn);
    if (res2 != MDB_SUCCESS) {
        return utxo_code::other;
    }
    return res;
}

utxo_entry utxo_database::get(chain::output_point const& point) const {
    MDB_txn* db_txn;
    if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
        return utxo_entry{};
    }

    auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
    MDB_val key {keyarr.size(), keyarr.data()};
    MDB_val value;

    if (mdb_get(db_txn, dbi_utxo_, &key, &value) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        // mdb_txn_abort(db_txn);
        return utxo_entry{};
    }

    data_chunk data {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};
    auto res = utxo_entry::factory_from_data(data);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return utxo_entry{};
    }

    return res;
}

utxo_code utxo_database::get_last_height(uint32_t& out_height) const {
    MDB_txn* db_txn;
    if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
        return utxo_code::other;
    }

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_block_header_, &cursor) != MDB_SUCCESS) {
        return utxo_code::other;
    }

    MDB_val key;
    MDB_val value;
    int rc;

    // if ((rc = mdb_cursor_get(cursor, &key, &value, MDB_LAST)) != MDB_SUCCESS) {
    if ((rc = mdb_cursor_get(cursor, &key, nullptr, MDB_LAST)) != MDB_SUCCESS) {
        return utxo_code::db_empty;  
    }

    // assert key.mv_size == 4;
    out_height = *static_cast<uint32_t*>(key.mv_data);
    
    mdb_cursor_close(cursor);

    // mdb_txn_abort(db_txn);
    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return utxo_code::other;
    }

    return utxo_code::success;
}


std::pair<chain::header, uint32_t> utxo_database::get_header(hash_digest const& hash) const {
    MDB_txn* db_txn;
    if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
        return {};
    }

    MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};
    MDB_val value;

    if (mdb_get(db_txn, dbi_block_header_by_hash_, &key, &value) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        // mdb_txn_abort(db_txn);
        return {};
    }

    // assert value.mv_size == 4;
    auto height = *static_cast<uint32_t*>(value.mv_data);

    auto header = get_header(height, db_txn);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return {};
    }

    return {header, height};
}


// private
chain::header utxo_database::get_header(uint32_t height, MDB_txn* db_txn) const {
    MDB_val key {sizeof(height), &height};
    MDB_val value;

    if (mdb_get(db_txn, dbi_block_header_, &key, &value) != MDB_SUCCESS) {
        return chain::header{};
    }

    data_chunk data {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};
    auto res = chain::header::factory_from_data(data);
    return res;
}

chain::header utxo_database::get_header(uint32_t height) const {
    MDB_txn* db_txn;
    if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
        return chain::header{};
    }

    auto res = get_header(height, db_txn);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return chain::header{};
    }

    return res;
}

// utxo_code utxo_database::remove_block(chain::block const& block) {

//     MDB_txn* db_txn;
//     auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
//     if (res0 != MDB_SUCCESS) {
//         return utxo_code::other;
//     }

//     auto res = remove_block(block, db_txn);
//     if (res != utxo_code::success) {
//         mdb_txn_abort(db_txn);
//         return res;
//     }

//     auto res2 = mdb_txn_commit(db_txn);
//     if (res2 != MDB_SUCCESS) {
//         return utxo_code::other;
//     }
//     return utxo_code::success;
// }

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW











/*void lmdb_resized(MDB_env *env)
{
  mdb_txn_safe::prevent_new_txns();

  MGINFO("LMDB map resize detected.");

  MDB_envinfo mei;

  mdb_env_info(env, &mei);
  uint64_t old = mei.me_mapsize;

  mdb_txn_safe::wait_no_active_txns();

  int result = mdb_env_set_mapsize(env, 0);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to set new mapsize: ", result).c_str()));

  mdb_env_info(env, &mei);
  uint64_t new_mapsize = mei.me_mapsize;

  MGINFO("LMDB Mapsize increased." << "  Old: " << old / (1024 * 1024) << "MiB" << ", New: " << new_mapsize / (1024 * 1024) << "MiB");

  mdb_txn_safe::allow_new_txns();
}

inline int lmdb_txn_begin(MDB_env *env, MDB_txn *parent, unsigned int flags, MDB_txn **txn)
{
  int res = mdb_txn_begin(env, parent, flags, txn);
  if (res == MDB_MAP_RESIZED) {
    lmdb_resized(env);
    res = mdb_txn_begin(env, parent, flags, txn);
  }
  return res;
}

void BlockchainLMDB::do_resize(uint64_t increase_size)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  CRITICAL_REGION_LOCAL(m_synchronization_lock);
  const uint64_t add_size = 1LL << 30;

  // check disk capacity
  try
  {
    boost::filesystem::path path(m_folder);
    boost::filesystem::space_info si = boost::filesystem::space(path);
    if(si.available < add_size)
    {
      MERROR("!! WARNING: Insufficient free space to extend database !!: " <<
          (si.available >> 20L) << " MB available, " << (add_size >> 20L) << " MB needed");
      return;
    }
  }
  catch(...)
  {
    // print something but proceed.
    MWARNING("Unable to query free disk space.");
  }

  MDB_envinfo mei;

  mdb_env_info(m_env, &mei);

  MDB_stat mst;

  mdb_env_stat(m_env, &mst);

  // add 1Gb per resize, instead of doing a percentage increase
  uint64_t new_mapsize = (double) mei.me_mapsize + add_size;

  // If given, use increase_size instead of above way of resizing.
  // This is currently used for increasing by an estimated size at start of new
  // batch txn.
  if (increase_size > 0)
    new_mapsize = mei.me_mapsize + increase_size;

  new_mapsize += (new_mapsize % mst.ms_psize);

  mdb_txn_safe::prevent_new_txns();

  if (m_write_txn != nullptr)
  {
    if (m_batch_active)
    {
      throw0(DB_ERROR("lmdb resizing not yet supported when batch transactions enabled!"));
    }
    else
    {
      throw0(DB_ERROR("attempting resize with write transaction in progress, this should not happen!"));
    }
  }

  mdb_txn_safe::wait_no_active_txns();

  int result = mdb_env_set_mapsize(m_env, new_mapsize);
  if (result)
    throw0(DB_ERROR(lmdb_error("Failed to set new mapsize: ", result).c_str()));

  MGINFO("LMDB Mapsize increased." << "  Old: " << mei.me_mapsize / (1024 * 1024) << "MiB" << ", New: " << new_mapsize / (1024 * 1024) << "MiB");

  mdb_txn_safe::allow_new_txns();
}

// threshold_size is used for batch transactions
bool BlockchainLMDB::need_resize(uint64_t threshold_size) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
#if defined(ENABLE_AUTO_RESIZE)
  MDB_envinfo mei;

  mdb_env_info(m_env, &mei);

  MDB_stat mst;

  mdb_env_stat(m_env, &mst);

  // size_used doesn't include data yet to be committed, which can be
  // significant size during batch transactions. For that, we estimate the size
  // needed at the beginning of the batch transaction and pass in the
  // additional size needed.
  uint64_t size_used = mst.ms_psize * mei.me_last_pgno;

  LOG_PRINT_L1("DB map size:     " << mei.me_mapsize);
  LOG_PRINT_L1("Space used:      " << size_used);
  LOG_PRINT_L1("Space remaining: " << mei.me_mapsize - size_used);
  LOG_PRINT_L1("Size threshold:  " << threshold_size);
  float resize_percent = RESIZE_PERCENT;
  LOG_PRINT_L1(boost::format("Percent used: %.04f  Percent threshold: %.04f") % ((double)size_used/mei.me_mapsize) % resize_percent);

  if (threshold_size > 0)
  {
    if (mei.me_mapsize - size_used < threshold_size)
    {
      LOG_PRINT_L1("Threshold met (size-based)");
      return true;
    }
    else
      return false;
  }

  if ((double)size_used / mei.me_mapsize  > resize_percent)
  {
    LOG_PRINT_L1("Threshold met (percent-based)");
    return true;
  }
  return false;
#else
  return false;
#endif
}

void BlockchainLMDB::check_and_resize_for_batch(uint64_t batch_num_blocks, uint64_t batch_bytes)
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  LOG_PRINT_L1("[" << __func__ << "] " << "checking DB size");
  const uint64_t min_increase_size = 512 * (1 << 20);
  uint64_t threshold_size = 0;
  uint64_t increase_size = 0;
  if (batch_num_blocks > 0)
  {
    threshold_size = get_estimated_batch_size(batch_num_blocks, batch_bytes);
    MDEBUG("calculated batch size: " << threshold_size);

    // The increased DB size could be a multiple of threshold_size, a fixed
    // size increase (> threshold_size), or other variations.
    //
    // Currently we use the greater of threshold size and a minimum size. The
    // minimum size increase is used to avoid frequent resizes when the batch
    // size is set to a very small numbers of blocks.
    increase_size = (threshold_size > min_increase_size) ? threshold_size : min_increase_size;
    MDEBUG("increase size: " << increase_size);
  }

  // if threshold_size is 0 (i.e. number of blocks for batch not passed in), it
  // will fall back to the percent-based threshold check instead of the
  // size-based check
  if (need_resize(threshold_size))
  {
    MGINFO("[batch] DB resize needed");
    do_resize(increase_size);
  }
}

uint64_t BlockchainLMDB::get_estimated_batch_size(uint64_t batch_num_blocks, uint64_t batch_bytes) const
{
  LOG_PRINT_L3("BlockchainLMDB::" << __func__);
  uint64_t threshold_size = 0;

  // batch size estimate * batch safety factor = final size estimate
  // Takes into account "reasonable" block size increases in batch.
  float batch_safety_factor = 1.7f;
  float batch_fudge_factor = batch_safety_factor * batch_num_blocks;
  // estimate of stored block expanded from raw block, including denormalization and db overhead.
  // Note that this probably doesn't grow linearly with block size.
  float db_expand_factor = 4.5f;
  uint64_t num_prev_blocks = 500;
  // For resizing purposes, allow for at least 4k average block size.
  uint64_t min_block_size = 4 * 1024;

  uint64_t block_stop = 0;
  uint64_t m_height = height();
  if (m_height > 1)
    block_stop = m_height - 1;
  uint64_t block_start = 0;
  if (block_stop >= num_prev_blocks)
    block_start = block_stop - num_prev_blocks + 1;
  uint32_t num_blocks_used = 0;
  uint64_t total_block_size = 0;
  MDEBUG("[" << __func__ << "] " << "m_height: " << m_height << "  block_start: " << block_start << "  block_stop: " << block_stop);
  size_t avg_block_size = 0;
  if (batch_bytes)
  {
    avg_block_size = batch_bytes / batch_num_blocks;
    goto estim;
  }
  if (m_height == 0)
  {
    MDEBUG("No existing blocks to check for average block size");
  }
  else if (m_cum_count >= num_prev_blocks)
  {
    avg_block_size = m_cum_size / m_cum_count;
    MDEBUG("average block size across recent " << m_cum_count << " blocks: " << avg_block_size);
    m_cum_size = 0;
    m_cum_count = 0;
  }
  else
  {
    MDB_txn *rtxn;
    mdb_txn_cursors *rcurs;
    block_rtxn_start(&rtxn, &rcurs);
    for (uint64_t block_num = block_start; block_num <= block_stop; ++block_num)
    {
      // we have access to block weight, which will be greater or equal to block size,
      // so use this as a proxy. If it's too much off, we might have to check actual size,
      // which involves reading more data, so is not really wanted
      size_t block_weight = get_block_weight(block_num);
      total_block_size += block_weight;
      // Track number of blocks being totalled here instead of assuming, in case
      // some blocks were to be skipped for being outliers.
      ++num_blocks_used;
    }
    block_rtxn_stop();
    avg_block_size = total_block_size / num_blocks_used;
    MDEBUG("average block size across recent " << num_blocks_used << " blocks: " << avg_block_size);
  }
estim:
  if (avg_block_size < min_block_size)
    avg_block_size = min_block_size;
  MDEBUG("estimated average block size for batch: " << avg_block_size);

  // bigger safety margin on smaller block sizes
  if (batch_fudge_factor < 5000.0)
    batch_fudge_factor = 5000.0;
  threshold_size = avg_block_size * db_expand_factor * batch_fudge_factor;
  return threshold_size;
}
*/

// void BlockchainLMDB::open(const std::string& filename, const int db_flags)
// {
//   int result;
//   int mdb_flags = MDB_NORDAHEAD;

//   LOG_PRINT_L3("BlockchainLMDB::" << __func__);

//   if (m_open)
//     throw0(DB_OPEN_FAILURE("Attempted to open db, but it's already open"));

//   boost::filesystem::path direc(filename);
//   if (boost::filesystem::exists(direc))
//   {
//     if (!boost::filesystem::is_directory(direc))
//       throw0(DB_OPEN_FAILURE("LMDB needs a directory path, but a file was passed"));
//   }
//   else
//   {
//     if (!boost::filesystem::create_directories(direc))
//       throw0(DB_OPEN_FAILURE(std::string("Failed to create directory ").append(filename).c_str()));
//   }

//   // check for existing LMDB files in base directory
//   boost::filesystem::path old_files = direc.parent_path();
//   if (boost::filesystem::exists(old_files / CRYPTONOTE_BLOCKCHAINDATA_FILENAME)
//       || boost::filesystem::exists(old_files / CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME))
//   {
//     LOG_PRINT_L0("Found existing LMDB files in " << old_files.string());
//     LOG_PRINT_L0("Move " << CRYPTONOTE_BLOCKCHAINDATA_FILENAME << " and/or " << CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME << " to " << filename << ", or delete them, and then restart");
//     throw DB_ERROR("Database could not be opened");
//   }

//   boost::optional<bool> is_hdd_result = tools::is_hdd(filename.c_str());
//   if (is_hdd_result)
//   {
//     if (is_hdd_result.value())
//         MCLOG_RED(el::Level::Warning, "global", "The blockchain is on a rotating drive: this will be very slow, use a SSD if possible");
//   }

//   m_folder = filename;

// #ifdef __OpenBSD__
//   if ((mdb_flags & MDB_WRITEMAP) == 0) {
//     MCLOG_RED(el::Level::Info, "global", "Running on OpenBSD: forcing WRITEMAP");
//     mdb_flags |= MDB_WRITEMAP;
//   }
// #endif
//   // set up lmdb environment
//   if ((result = mdb_env_create(&m_env)))
//     throw0(DB_ERROR(lmdb_error("Failed to create lmdb environment: ", result).c_str()));
//   if ((result = mdb_env_set_maxdbs(m_env, 20)))
//     throw0(DB_ERROR(lmdb_error("Failed to set max number of dbs: ", result).c_str()));

//   int threads = tools::get_max_concurrency();
//   if (threads > 110 &&	/* maxreaders default is 126, leave some slots for other read processes */
//     (result = mdb_env_set_maxreaders(m_env, threads+16)))
//     throw0(DB_ERROR(lmdb_error("Failed to set max number of readers: ", result).c_str()));

//   size_t mapsize = DEFAULT_MAPSIZE;

//   if (db_flags & DBF_FAST)
//     mdb_flags |= MDB_NOSYNC;
//   if (db_flags & DBF_FASTEST)
//     mdb_flags |= MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC;
//   if (db_flags & DBF_RDONLY)
//     mdb_flags = MDB_RDONLY;
//   if (db_flags & DBF_SALVAGE)
//     mdb_flags |= MDB_PREVSNAPSHOT;

//   if (auto result = mdb_env_open(m_env, filename.c_str(), mdb_flags, 0644))
//     throw0(DB_ERROR(lmdb_error("Failed to open lmdb environment: ", result).c_str()));

//   MDB_envinfo mei;
//   mdb_env_info(m_env, &mei);
//   uint64_t cur_mapsize = (double)mei.me_mapsize;

//   if (cur_mapsize < mapsize)
//   {
//     if (auto result = mdb_env_set_mapsize(m_env, mapsize))
//       throw0(DB_ERROR(lmdb_error("Failed to set max memory map size: ", result).c_str()));
//     mdb_env_info(m_env, &mei);
//     cur_mapsize = (double)mei.me_mapsize;
//     LOG_PRINT_L1("LMDB memory map size: " << cur_mapsize);
//   }

//   if (need_resize())
//   {
//     LOG_PRINT_L0("LMDB memory map needs to be resized, doing that now.");
//     do_resize();
//   }

//   int txn_flags = 0;
//   if (mdb_flags & MDB_RDONLY)
//     txn_flags |= MDB_RDONLY;

//   // get a read/write MDB_txn, depending on mdb_flags
//   mdb_txn_safe txn;
//   if (auto mdb_res = mdb_txn_begin(m_env, NULL, txn_flags, txn))
//     throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", mdb_res).c_str()));

//   // open necessary databases, and set properties as needed
//   // uses macros to avoid having to change things too many places
//   lmdb_db_open(txn, LMDB_BLOCKS, MDB_INTEGERKEY | MDB_CREATE, m_blocks, "Failed to open db handle for m_blocks");

//   lmdb_db_open(txn, LMDB_BLOCK_INFO, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_info, "Failed to open db handle for m_block_info");
//   lmdb_db_open(txn, LMDB_BLOCK_HEIGHTS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_block_heights, "Failed to open db handle for m_block_heights");

//   lmdb_db_open(txn, LMDB_TXS, MDB_INTEGERKEY | MDB_CREATE, m_txs, "Failed to open db handle for m_txs");
//   lmdb_db_open(txn, LMDB_TXS_PRUNED, MDB_INTEGERKEY | MDB_CREATE, m_txs_pruned, "Failed to open db handle for m_txs_pruned");
//   lmdb_db_open(txn, LMDB_TXS_PRUNABLE, MDB_INTEGERKEY | MDB_CREATE, m_txs_prunable, "Failed to open db handle for m_txs_prunable");
//   lmdb_db_open(txn, LMDB_TXS_PRUNABLE_HASH, MDB_INTEGERKEY | MDB_CREATE, m_txs_prunable_hash, "Failed to open db handle for m_txs_prunable_hash");
//   lmdb_db_open(txn, LMDB_TX_INDICES, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_tx_indices, "Failed to open db handle for m_tx_indices");
//   lmdb_db_open(txn, LMDB_TX_OUTPUTS, MDB_INTEGERKEY | MDB_CREATE, m_tx_outputs, "Failed to open db handle for m_tx_outputs");

//   lmdb_db_open(txn, LMDB_OUTPUT_TXS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_output_txs, "Failed to open db handle for m_output_txs");
//   lmdb_db_open(txn, LMDB_OUTPUT_AMOUNTS, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, m_output_amounts, "Failed to open db handle for m_output_amounts");

//   lmdb_db_open(txn, LMDB_SPENT_KEYS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, m_spent_keys, "Failed to open db handle for m_spent_keys");

//   lmdb_db_open(txn, LMDB_TXPOOL_META, MDB_CREATE, m_txpool_meta, "Failed to open db handle for m_txpool_meta");
//   lmdb_db_open(txn, LMDB_TXPOOL_BLOB, MDB_CREATE, m_txpool_blob, "Failed to open db handle for m_txpool_blob");

//   // this subdb is dropped on sight, so it may not be present when we open the DB.
//   // Since we use MDB_CREATE, we'll get an exception if we open read-only and it does not exist.
//   // So we don't open for read-only, and also not drop below. It is not used elsewhere.
//   if (!(mdb_flags & MDB_RDONLY))
//     lmdb_db_open(txn, LMDB_HF_STARTING_HEIGHTS, MDB_CREATE, m_hf_starting_heights, "Failed to open db handle for m_hf_starting_heights");

//   lmdb_db_open(txn, LMDB_HF_VERSIONS, MDB_INTEGERKEY | MDB_CREATE, m_hf_versions, "Failed to open db handle for m_hf_versions");

//   lmdb_db_open(txn, LMDB_PROPERTIES, MDB_CREATE, m_properties, "Failed to open db handle for m_properties");

//   mdb_set_dupsort(txn, m_spent_keys, compare_hash32);
//   mdb_set_dupsort(txn, m_block_heights, compare_hash32);
//   mdb_set_dupsort(txn, m_tx_indices, compare_hash32);
//   mdb_set_dupsort(txn, m_output_amounts, compare_uint64);
//   mdb_set_dupsort(txn, m_output_txs, compare_uint64);
//   mdb_set_dupsort(txn, m_block_info, compare_uint64);

//   mdb_set_compare(txn, m_txpool_meta, compare_hash32);
//   mdb_set_compare(txn, m_txpool_blob, compare_hash32);
//   mdb_set_compare(txn, m_properties, compare_string);

//   if (!(mdb_flags & MDB_RDONLY))
//   {
//     result = mdb_drop(txn, m_hf_starting_heights, 1);
//     if (result && result != MDB_NOTFOUND)
//       throw0(DB_ERROR(lmdb_error("Failed to drop m_hf_starting_heights: ", result).c_str()));
//   }

//   // get and keep current height
//   MDB_stat db_stats;
//   if ((result = mdb_stat(txn, m_blocks, &db_stats)))
//     throw0(DB_ERROR(lmdb_error("Failed to query m_blocks: ", result).c_str()));
//   LOG_PRINT_L2("Setting m_height to: " << db_stats.ms_entries);
//   uint64_t m_height = db_stats.ms_entries;

//   bool compatible = true;

//   MDB_val_copy<const char*> k("version");
//   MDB_val v;
//   auto get_result = mdb_get(txn, m_properties, &k, &v);
//   if(get_result == MDB_SUCCESS)
//   {
//     const uint32_t db_version = *(const uint32_t*)v.mv_data;
//     if (db_version > VERSION)
//     {
//       MWARNING("Existing lmdb database was made by a later version (" << db_version << "). We don't know how it will change yet.");
//       compatible = false;
//     }
// #if VERSION > 0
//     else if (db_version < VERSION)
//     {
//       // Note that there was a schema change within version 0 as well.
//       // See commit e5d2680094ee15889934fe28901e4e133cda56f2 2015/07/10
//       // We don't handle the old format previous to that commit.
//       txn.commit();
//       m_open = true;
//       migrate(db_version);
//       return;
//     }
// #endif
//   }
//   else
//   {
//     // if not found, and the DB is non-empty, this is probably
//     // an "old" version 0, which we don't handle. If the DB is
//     // empty it's fine.
//     if (VERSION > 0 && m_height > 0)
//       compatible = false;
//   }

//   if (!compatible)
//   {
//     txn.abort();
//     mdb_env_close(m_env);
//     m_open = false;
//     MFATAL("Existing lmdb database is incompatible with this version.");
//     MFATAL("Please delete the existing database and resync.");
//     return;
//   }

//   if (!(mdb_flags & MDB_RDONLY))
//   {
//     // only write version on an empty DB
//     if (m_height == 0)
//     {
//       MDB_val_copy<const char*> k("version");
//       MDB_val_copy<uint32_t> v(VERSION);
//       auto put_result = mdb_put(txn, m_properties, &k, &v, 0);
//       if (put_result != MDB_SUCCESS)
//       {
//         txn.abort();
//         mdb_env_close(m_env);
//         m_open = false;
//         MERROR("Failed to write version to database.");
//         return;
//       }
//     }
//   }

//   // commit the transaction
//   txn.commit();

//   m_open = true;
//   // from here, init should be finished
// }

// void BlockchainLMDB::close()
// {
//   LOG_PRINT_L3("BlockchainLMDB::" << __func__);
//   if (m_batch_active)
//   {
//     LOG_PRINT_L3("close() first calling batch_abort() due to active batch transaction");
//     batch_abort();
//   }
//   this->sync();
//   m_tinfo.reset();

//   // FIXME: not yet thread safe!!!  Use with care.
//   mdb_env_close(m_env);
//   m_open = false;
// }

// void BlockchainLMDB::sync()
// {
//   LOG_PRINT_L3("BlockchainLMDB::" << __func__);
//   check_open();

//   if (is_read_only())
//     return;

//   // Does nothing unless LMDB environment was opened with MDB_NOSYNC or in part
//   // MDB_NOMETASYNC. Force flush to be synchronous.
//   if (auto result = mdb_env_sync(m_env, true))
//   {
//     throw0(DB_ERROR(lmdb_error("Failed to sync database: ", result).c_str()));
//   }
// }

// void BlockchainLMDB::safesyncmode(const bool onoff)
// {
//   MINFO("switching safe mode " << (onoff ? "on" : "off"));
//   mdb_env_set_flags(m_env, MDB_NOSYNC|MDB_MAPASYNC, !onoff);
// }

// void BlockchainLMDB::reset()
// {
//   LOG_PRINT_L3("BlockchainLMDB::" << __func__);
//   check_open();

//   mdb_txn_safe txn;
//   if (auto result = lmdb_txn_begin(m_env, NULL, 0, txn))
//     throw0(DB_ERROR(lmdb_error("Failed to create a transaction for the db: ", result).c_str()));

//   if (auto result = mdb_drop(txn, m_blocks, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_blocks: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_block_info, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_block_info: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_block_heights, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_block_heights: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_txs_pruned, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_txs_pruned: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_txs_prunable, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_txs_prunable: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_txs_prunable_hash, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_txs_prunable_hash: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_tx_indices, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_tx_indices: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_tx_outputs, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_tx_outputs: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_output_txs, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_output_txs: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_output_amounts, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_output_amounts: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_spent_keys, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_spent_keys: ", result).c_str()));
//   (void)mdb_drop(txn, m_hf_starting_heights, 0); // this one is dropped in new code
//   if (auto result = mdb_drop(txn, m_hf_versions, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_hf_versions: ", result).c_str()));
//   if (auto result = mdb_drop(txn, m_properties, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to drop m_properties: ", result).c_str()));

//   // init with current version
//   MDB_val_copy<const char*> k("version");
//   MDB_val_copy<uint32_t> v(VERSION);
//   if (auto result = mdb_put(txn, m_properties, &k, &v, 0))
//     throw0(DB_ERROR(lmdb_error("Failed to write version to database: ", result).c_str()));

//   txn.commit();
//   m_cum_size = 0;
//   m_cum_count = 0;
// }


/*
bool BlockchainLMDB::is_read_only() const
{
  unsigned int flags;
  auto result = mdb_env_get_flags(m_env, &flags);
  if (result)
    throw0(DB_ERROR(lmdb_error("Error getting database environment info: ", result).c_str()));

  if (flags & MDB_RDONLY)
    return true;

  return false;
}

uint64_t BlockchainLMDB::get_database_size() const
{
  uint64_t size = 0;
  boost::filesystem::path datafile(m_folder);
  datafile /= CRYPTONOTE_BLOCKCHAINDATA_FILENAME;
  if (!epee::file_io_utils::get_file_size(datafile.string(), size))
    size = 0;
  return size;
}
*/









/*


Some Optimization
If you frequently begin and abort read-only transactions, as an optimization, it is possible to only reset and renew a transaction.

mdb_txn_reset() releases any old copies of data kept around for a read-only transaction. To reuse this reset transaction, call mdb_txn_renew() on it. Any cursors in this transaction must also be renewed using mdb_cursor_renew().

Note that mdb_txn_reset() is similar to mdb_txn_abort() and will close any databases you opened within the transaction.

To permanently free a transaction, reset or not, use mdb_txn_abort().


*/

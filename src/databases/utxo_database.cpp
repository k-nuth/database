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
    auto res = mdb_env_set_mapsize(env_, size_t(10485760) * 1024);
    if (res != MDB_SUCCESS) {
        std::cout << "utxo_database::create_and_open_environment() - mdb_env_set_mapsize - res: " << res << std::endl;
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

    if (mdb_dbi_open(db_txn, NULL, 0, &dbi_) != MDB_SUCCESS) {
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
    if (env_created_) {
        mdb_env_close(env_);
        env_created_ = false;
    }

    if (db_created_) {
        mdb_dbi_close(env_, dbi_);
        db_created_ = false;
    }

    return true;
}

/*
key:        TxId (32 bytes) + Output Index (4 bytes) = 36 bytes or
key:        TxId (32 bytes) + Output Index (2 bytes) = 34 bytes
value:      Output Serialized (n bytes)
*/

// private
bool utxo_database::remove(chain::transaction const& tx, MDB_txn* db_txn) {
    for (const auto& input: tx.inputs()) {
        auto keyarr = input.previous_output().to_data(BITPRIM_UXTO_WIRE);

        // MDB_val key;
        // key.mv_size = keyarr.size();
		// key.mv_data = keyarr.data();

        MDB_val key {keyarr.size(), keyarr.data()};
        auto res = mdb_del(db_txn, dbi_, &key, NULL);
        if (res != MDB_SUCCESS) {
            std::cout << "utxo_database::remove - res: " << res << std::endl;
            return false;
        }
    }
    return true;
}

// private
bool utxo_database::insert(chain::transaction const& tx, MDB_txn* db_txn) {

    uint32_t pos = 0;
    for (const auto& output: tx.outputs()) {
        auto keyarr = chain::point{tx.hash(), pos}.to_data(BITPRIM_UXTO_WIRE);

#ifdef BITPRIM_UTXO_SERIALIZE_WHOLE_OUTPUT
        auto valuearr = output.to_data(false);
#else
        auto valuearr = output.script().to_data(true);
#endif // BITPRIM_UTXO_SERIALIZE_WHOLE_OUTPUT

        MDB_val key   {keyarr.size(), keyarr.data()};
        MDB_val value {valuearr.size(), valuearr.data()};

        auto res = mdb_put(db_txn, dbi_, &key, &value, MDB_NOOVERWRITE);
        if (res != MDB_SUCCESS) {
            std::cout << "utxo_database::insert - res: " << res << std::endl;
            std::cout << "utxo_database::insert - tx.hash(): " << encode_hash(tx.hash()) << std::endl;
            std::cout << "utxo_database::insert - pos:       " << pos << std::endl;
            return false;
        }

        ++pos;
    }
    return true;
}

// private
size_t utxo_database::push_block(chain::block const& block, MDB_txn* db_txn) {
    //precondition: block.transactions().size() >= 1

    auto const& txs = block.transactions();
    auto const& coinbase = txs.front();
    if ( ! insert(coinbase, db_txn)) {
        return 4;
    }

    // Skip coinbase as it has no previous output.
    for (auto it = txs.begin() + 1; it != txs.end(); ++it) {
        auto const& tx = *it;

        if ( ! remove(tx, db_txn)) {
            return 5;
        }

        if ( ! insert(tx, db_txn)) {
            return 6;
        }
    }
    return 0;
}

//TODO(fernando): to enum
size_t utxo_database::push_block(chain::block const& block) {
    // std::cout << "utxo_database::push_block - BEGIN" << std::endl;

    MDB_txn* db_txn;
    auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
    if (res0 != MDB_SUCCESS) {
        std::cout << "utxo_database::push_block - res0: " << res0 << std::endl;
        return 1;
    }

    // if ( ! push_block(block, db_txn)) {
    auto res = push_block(block, db_txn);
    if (res != 0) {
        std::cout << "utxo_database::push_block - before abort!" << std::endl;
        mdb_txn_abort(db_txn);
        return res;
    }

    auto res2 = mdb_txn_commit(db_txn);
    if (res2 != MDB_SUCCESS) {
        std::cout << "utxo_database::push_block - res2: " << res2 << std::endl;
        return 3;
    }
    return 0;
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

    if (mdb_get(db_txn, dbi_, &key, &value) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        // mdb_txn_abort(db_txn);
        return utxo_database::get_return_t{};
    }

    data_chunk data {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};
#ifdef BITPRIM_UTXO_SERIALIZE_WHOLE_OUTPUT
    auto res = chain::output::factory_from_data(data, false);
#else
    auto res = chain::script::factory_from_data(data, true);
#endif // BITPRIM_UTXO_SERIALIZE_WHOLE_OUTPUT

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return utxo_database::get_return_t{};
    }

    return res;
}

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

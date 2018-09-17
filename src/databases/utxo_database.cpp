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


    if (mdb_env_create(&env_) != MDB_SUCCESS) {
        return false;
    }

    // E(mdb_env_set_maxreaders(env_, 1));
    // E(mdb_env_set_mapsize(env_, 10485760));

    //TODO(fernando): use MDB_RDONLY for Read-only node
    //                  MDB_WRITEMAP ????
    //                  MDB_NOMETASYNC ????
    //                  MDB_NOSYNC ????

    // 

    //TODO(fernando): put the 0664 in the CFG file
    if (mdb_env_open(env_, db_dir_.c_str(), MDB_FIXEDMAP | MDB_NORDAHEAD | MDB_NOMEMINIT, 0664) != MDB_SUCCESS) {
        // mdb_env_close(env_);
        return false;
    }

    return true;
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// Start files and primitives.
bool utxo_database::open() {
    // E(mdb_txn_begin(env_, NULL, 0, &txn));
    // E(mdb_dbi_open(txn, NULL, 0, &dbi));
    // E(mdb_txn_commit(txn));

    return true;
}

// Close files.
bool utxo_database::close() {
    // mdb_dbi_close(env_, dbi);
    mdb_env_close(env_);
    return true;
}

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

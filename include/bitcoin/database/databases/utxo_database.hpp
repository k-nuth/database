/**
 * Copyright (c) 2016-2017 Bitprim Inc.
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
#ifndef BITPRIM_DATABASE_UTXO_DATABASE_HPP_
#define BITPRIM_DATABASE_UTXO_DATABASE_HPP_

// #ifdef BITPRIM_DB_NEW

#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

#include <lmdb.h>
// #include "lmdb.h"

namespace libbitcoin {
namespace database {

enum class uxto_code {
    success = 0,
    duplicated_key = 1,
    key_not_found = 2,
    other = 3
};

class BCD_API utxo_database {
public:
    using path = boost::filesystem::path;
    using get_return_t = chain::output;

    /// Construct the database.
    utxo_database(path const& db_dir);

    /// Close the database.
    ~utxo_database();

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Remove all the previous outputs and insert all the new outputs atomically.
    uxto_code push_block(chain::block const& block);

    /// TODO???
    // boost::optional<get_return_t> get(chain::output_point const& key);
    get_return_t get(chain::output_point const& key);

private:
    bool create_and_open_environment();
    uxto_code remove(chain::transaction const& tx, MDB_txn* db_txn);
    uxto_code insert(chain::transaction const& tx, MDB_txn* db_txn);
    uxto_code push_block(chain::block const& block, MDB_txn* db_txn);
    uxto_code push_transaction(chain::transaction const& tx, MDB_txn* db_txn);
    uxto_code push_transaction_non_coinbase(chain::transaction const& tx, MDB_txn* db_txn);
    uxto_code push_transactions_non_coinbase(chain::transaction::list const& txs, MDB_txn* db_txn);

    path db_dir_;
    bool env_created_ = false;
    bool db_created_ = false;

    MDB_env* env_;
    MDB_dbi dbi_;
};

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

#endif // BITPRIM_DATABASE_UTXO_DATABASE_HPP_

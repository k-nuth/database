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

// #include <cstddef>
// #include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
// #include <bitcoin/database/memory/memory_map.hpp>
// #include <bitcoin/database/result/transaction_result.hpp>
// #include <bitcoin/database/primitives/slab_hash_table.hpp>
// #include <bitcoin/database/primitives/slab_manager.hpp>

// #include <lmdb.h>
#include "lmdb.h"

namespace libbitcoin {
namespace database {

class BCD_API utxo_database {
public:
    using path = boost::filesystem::path;

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



    // /// Fetch transaction by its hash, at or below the specified block height.
    // transaction_result get(const hash_digest& hash, size_t fork_height, bool require_confirmed) const;

    // /// Get the output at the specified index within the transaction.
    // bool get_output(chain::output& out_output, size_t& out_height,
    //     uint32_t& out_median_time_past, bool& out_coinbase,
    //     const chain::output_point& point, size_t fork_height,
    //     bool require_confirmed) const;

    // /// Get the output at the specified index within the transaction.
    // bool get_output_is_confirmed(chain::output& out_output, size_t& out_height,
    //     bool& out_coinbase, bool& out_is_confirmed, const chain::output_point& point,
    //     size_t fork_height, bool require_confirmed) const;


    // /// Store a transaction in the database.
    // void store(const chain::transaction& tx, size_t height,
    //     uint32_t median_time_past, size_t position);

    // /// Update the spender height of the output in the tx store.
    // bool spend(const chain::output_point& point, size_t spender_height);

    // /// Update the spender height of the output in the tx store.
    // bool unspend(const chain::output_point& point);

    // /// Promote an unconfirmed tx (not including its indexes).
    // bool confirm(const hash_digest& hash, size_t height,
    //     uint32_t median_time_past, size_t position);

    // /// Demote the transaction (not including its indexes).
    // bool unconfirm(const hash_digest& hash);

    // /// Commit latest inserts.
    // void synchronize();

    // /// Flush the memory map to disk.
    // bool flush() const;

private:
    path db_dir_;
    MDB_env* env_;
    MDB_dbi dbi_;


};

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

#endif // BITPRIM_DATABASE_UTXO_DATABASE_HPP_

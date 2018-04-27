/**
 * Copyright (c) 2011-2018 Bitprim developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_DATABASE_HPP_
#define LIBBITCOIN_DATABASE_UNSPENT_DATABASE_HPP_

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

#include <bitcoin/database/memory/memory_map.hpp>
#include <bitcoin/database/result/transaction_result.hpp>
#include <bitcoin/database/primitives/slab_hash_table.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>
// #include <bitcoin/database/unspent_outputs.hpp>
// #include <bitcoin/bitcoin/chainv2/transaction.hpp>

namespace libbitcoin { namespace database {

/// This enables lookups of transactions by hash.
/// An alternative and faster method is lookup from a unique index
/// that is assigned upon storage.
/// This is so we can quickly reconstruct blocks given a list of tx indexes
/// belonging to that block. These are stored with the block.
class BCD_API unspent_database {
public:
    using path = boost::filesystem::path;
    using mutex_ptr = std::shared_ptr<shared_mutex>;

    /// Sentinel for use in tx position to indicate unconfirmed.
    static const size_t unconfirmed;

    /// Construct the database.
    unspent_database(path const& map_filename, size_t buckets, size_t expansion, mutex_ptr mutex = nullptr);

    /// Close the database (all threads must first be stopped).
    ~unspent_database();

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Fetch transaction by its hash, at or below the specified block height.
    transaction_result get(const hash_digest& hash, size_t fork_height, bool require_confirmed) const;

    /// Get the output at the specified index within the transaction.
    bool get_output(chain::output& out_output, size_t& out_height, uint32_t& out_median_time_past, bool& out_coinbase, const chain::output_point& point, size_t fork_height, bool require_confirmed) const;
    bool get_output(chainv2::output& out_output, size_t& out_height, uint32_t& out_median_time_past, bool& out_coinbase, const chainv2::output_point& point, size_t fork_height, bool require_confirmed) const;

    /// Get the output at the specified index within the transaction.
    bool get_output_is_confirmed(chain::output& out_output, size_t& out_height, bool& out_coinbase, bool& out_is_confirmed, const chain::output_point& point, size_t fork_height, bool require_confirmed) const;

    /// Store a transaction in the database.
    void store(const chain::transaction& tx, size_t height, uint32_t median_time_past, size_t position);

    /// Update the spender height of the output in the tx store.
    bool spend(const chain::output_point& point, size_t spender_height);

    /// Update the spender height of the output in the tx store.
    bool unspend(const chain::output_point& point);

    /// Promote an unconfirmed tx (not including its indexes).
    bool confirm(const hash_digest& hash, size_t height, uint32_t median_time_past, size_t position);

    /// Demote the transaction (not including its indexes).
    bool unconfirm(const hash_digest& hash);

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush() const;

private:
    using value_t = std::pair<size_t, chainv2::output>;
    std::unordered_map<chainv2::output_point, value_t> data_;
};

}} // namespace libbitcoin::database

#endif // LIBBITCOIN_DATABASE_UNSPENT_DATABASE_HPP_

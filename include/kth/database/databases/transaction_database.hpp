// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TRANSACTION_DATABASE_HPP_
#define KTH_DATABASE_TRANSACTION_DATABASE_HPP_

#ifdef KTH_DB_LEGACY

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory_map.hpp>
#include <bitcoin/database/result/transaction_result.hpp>
#include <bitcoin/database/primitives/slab_hash_table.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>

#ifdef KTH_DB_UNSPENT_LIBBITCOIN
#include <bitcoin/database/unspent_outputs.hpp>
#endif // KTH_DB_UNSPENT_LIBBITCOIN

namespace libbitcoin {
namespace database {

/// This enables lookups of transactions by hash.
/// An alternative and faster method is lookup from a unique index
/// that is assigned upon storage.
/// This is so we can quickly reconstruct blocks given a list of tx indexes
/// belonging to that block. These are stored with the block.
class BCD_API transaction_database
{
public:
    typedef boost::filesystem::path path;
    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    /// Sentinel for use in tx position to indicate unconfirmed.
    static const size_t unconfirmed;

    /// Construct the database.
    transaction_database(const path& map_filename, size_t buckets,
        size_t expansion, size_t cache_capacity, mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~transaction_database();

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Fetch transaction by its hash, at or below the specified block height.
    transaction_result get(const hash_digest& hash, size_t fork_height, bool require_confirmed) const;

    /// Get the output at the specified index within the transaction.
    bool get_output(chain::output& out_output, size_t& out_height,
        uint32_t& out_median_time_past, bool& out_coinbase,
        const chain::output_point& point, size_t fork_height,
        bool require_confirmed) const;

    /// Get the output at the specified index within the transaction.
    bool get_output_is_confirmed(chain::output& out_output, size_t& out_height,
        bool& out_coinbase, bool& out_is_confirmed, const chain::output_point& point,
        size_t fork_height, bool require_confirmed) const;


    /// Store a transaction in the database.
    void store(const chain::transaction& tx, size_t height,
        uint32_t median_time_past, size_t position);

    /// Update the spender height of the output in the tx store.
    bool spend(const chain::output_point& point, size_t spender_height);

    /// Update the spender height of the output in the tx store.
    bool unspend(const chain::output_point& point);

    /// Promote an unconfirmed tx (not including its indexes).
    bool confirm(const hash_digest& hash, size_t height,
        uint32_t median_time_past, size_t position);

    /// Demote the transaction (not including its indexes).
    bool unconfirm(const hash_digest& hash);

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush() const;

// private:
    typedef slab_hash_table<hash_digest> slab_map;

    memory_ptr find(const hash_digest& hash, size_t maximum_height,
        bool require_confirmed) const;

    // The starting size of the hash table, used by create.
    const size_t initial_map_file_size_;

    // Hash table used for looking up txs by hash.
    memory_map lookup_file_;
    slab_hash_table_header lookup_header_;
    slab_manager lookup_manager_;
    slab_map lookup_map_;

#ifdef KTH_DB_UNSPENT_LIBBITCOIN
    // This is thread safe, and as a cache is mutable.
    mutable unspent_outputs cache_;
#endif // KTH_DB_UNSPENT_LIBBITCOIN

    // This provides atomicity for height and position.
    mutable shared_mutex metadata_mutex_;
};

} // namespace database
} // namespace kth

#endif // KTH_DB_LEGACY

#endif // KTH_DATABASE_TRANSACTION_DATABASE_HPP_

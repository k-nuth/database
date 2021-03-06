// Copyright (c) 2016-2021 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_SPEND_DATABASE_HPP_
#define KTH_DATABASE_SPEND_DATABASE_HPP_

#ifdef KTH_DB_SPENDS

#include <cstddef>
#include <filesystem>
#include <memory>

// #include <boost/filesystem.hpp>

#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/legacy/primitives/record_hash_table.hpp>
#include <kth/database/legacy/memory/memory_map.hpp>

namespace kth::database {

struct KD_API spend_statinfo {
    /// Number of buckets used in the hashtable.
    /// load factor = rows / buckets
    size_t const buckets;

    /// Total number of spend rows.
    size_t const rows;
};

/// This enables you to lookup the spend of an output point, returning
/// the input point. It is a simple map.
class KD_API spend_database {
public:
    using path = kth::path;
    using mutex_ptr = std::shared_ptr<shared_mutex>;

    /// Construct the database.
    spend_database(const path& filename, size_t buckets, size_t expansion, mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~spend_database();

    /// Initialize a new spend database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Get inpoint that spent the given outpoint.
    domain::chain::input_point get(const domain::chain::output_point& outpoint) const;

    /// Store a spend in the database.
    void store(const domain::chain::output_point& outpoint, const domain::chain::input_point& spend);

    /// Delete outpoint spend item from database.
    bool unlink(const domain::chain::output_point& outpoint);

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush() const;

    /// Return statistical info about the database.
    spend_statinfo statinfo() const;

private:
    using record_map = record_hash_table<domain::chain::point>;

    // The starting size of the hash table, used by create.
    size_t const initial_map_file_size_;

    // Hash table used for looking up inpoint spends by outpoint.
    memory_map lookup_file_;
    record_hash_table_header lookup_header_;
    record_manager lookup_manager_;
    record_map lookup_map_;
};

} // namespace kth::database

#endif // KTH_DB_SPENDS

#endif // KTH_DATABASE_SPEND_DATABASE_HPP_

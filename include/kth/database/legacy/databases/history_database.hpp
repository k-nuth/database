// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_HISTORY_DATABASE_HPP_
#define KTH_DATABASE_HISTORY_DATABASE_HPP_

#ifdef KTH_DB_HISTORY

#include <filesystem>
#include <memory>

#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/legacy/memory/memory_map.hpp>
#include <kth/database/legacy/primitives/record_multimap.hpp>

namespace kth::database {

struct KD_API history_statinfo
{
    /// Number of buckets used in the hashtable.
    /// load factor = addrs / buckets
    size_t const buckets;

    /// Total number of unique addresses in the database.
    size_t const addrs;

    /// Total number of rows across all addresses.
    size_t const rows;
};

/// This is a multimap where the key is the Bitcoin address hash,
/// which returns several rows giving the history for that address.
class KD_API history_database
{
public:
    using path = kth::path;
    using mutex_ptr = std::shared_ptr<shared_mutex>;

    /// Construct the database.
    history_database(const path& lookup_filename, const path& rows_filename,
        size_t buckets, size_t expansion, mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~history_database();

    /// Initialize a new history database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Add an output row to the key. If key doesn't exist it will be created.
    void add_output(short_hash const& key, const domain::chain::output_point& outpoint,
        size_t output_height, uint64_t value);

    /// Add an input to the key. If key doesn't exist it will be created.
    void add_input(short_hash const& key, const domain::chain::output_point& inpoint,
        size_t input_height, const domain::chain::input_point& previous);

    /// Delete the last row that was added to key.
    bool delete_last_row(short_hash const& key);

    /// Get the output and input points associated with the address hash.
    domain::chain::history_compact::list get(short_hash const& key, size_t limit,
        size_t from_height) const;

    /// Get the txns associated with the address hash.
    std::vector<hash_digest> get_txns(short_hash const& key, size_t limit,
                                     size_t from_height) const;

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory maps to disk.
    bool flush() const;

    /// Return statistical info about the database.
    history_statinfo statinfo() const;

private:
    using record_map = record_hash_table<short_hash>;
    using record_multiple_map = record_multimap<short_hash>;

    // The starting size of the hash table, used by create.
    size_t const initial_map_file_size_;

    /// Hash table used for start index lookup for linked list by address hash.
    memory_map lookup_file_;
    record_hash_table_header lookup_header_;
    record_manager lookup_manager_;
    record_map lookup_map_;

    /// History rows.
    memory_map rows_file_;
    record_manager rows_manager_;
    record_multiple_map rows_multimap_;
};

} // namespace kth::database

#endif // KTH_DB_HISTORY

#endif // KTH_DATABASE_HISTORY_DATABASE_HPP_

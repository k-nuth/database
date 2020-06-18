// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_BLOCK_DATABASE_HPP_
#define KTH_DATABASE_BLOCK_DATABASE_HPP_

#ifdef KTH_DB_LEGACY

#include <cstddef>
#include <filesystem>
#include <memory>

// #include <boost/filesystem.hpp>

#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/legacy/memory/memory_map.hpp>
#include <kth/database/legacy/primitives/record_manager.hpp>
#include <kth/database/legacy/primitives/slab_hash_table.hpp>
#include <kth/database/legacy/result/block_result.hpp>


namespace kth::database {

/// Stores block_headers each with a list of transaction indexes.
/// Lookup possible by hash or height.
class KD_API block_database
{
public:
    typedef std::vector<size_t> heights;
    typedef std::filesystem::path path;
    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    static const file_offset empty;

    /// Construct the database.
    block_database(const path& map_filename, const path& index_filename,
        size_t buckets, size_t expansion, mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~block_database();

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Determine if a block exists at the given height.
    bool exists(size_t height) const;

    /// Fetch block by height using the index table.
    block_result get(size_t height) const;

    /// Fetch block by hash using the hashtable.
    block_result get(hash_digest const& hash) const;


    //NOTE: This is public interface, but apparently it is not used in Blockchain
    /// Store a block in the database.
    void store(domain::chain::block const& block, size_t height);

    /// The list of heights representing all chain gaps.
    bool gaps(heights& out_gaps) const;

    /// Unlink all blocks upwards from (and including) from_height.
    bool unlink(size_t from_height);

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory maps to disk.
    bool flush() const;

    /// The index of the highest existing block, independent of gaps.
    bool top(size_t& out_height) const;

private:
    typedef slab_hash_table<hash_digest> slab_map;

    /// Zeroize the specfied index positions.
    void zeroize(array_index first, array_index count);

    /// Write block hash table position into the block index.
    void write_position(file_offset position, array_index height);

    /// Use block index to get block hash table position from height.
    file_offset read_position(array_index height) const;

    // The starting size of the hash table, used by create.
    const size_t initial_map_file_size_;

    /// Hash table used for looking up blocks by hash.
    memory_map lookup_file_;
    slab_hash_table_header lookup_header_;
    slab_manager lookup_manager_;
    slab_map lookup_map_;

    /// Table used for looking up blocks by height.
    /// Resolves to a position within the slab.
    memory_map index_file_;
    record_manager index_manager_;

    // Guard against concurrent update of a range of block indexes.
    mutable upgrade_mutex mutex_;

    // This provides atomicity for height.
    mutable shared_mutex metadata_mutex_;
};

} // namespace kth::database

#endif // KTH_DB_LEGACY

#endif // KTH_DATABASE_BLOCK_DATABASE_HPP_


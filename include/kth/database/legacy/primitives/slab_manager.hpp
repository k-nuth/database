// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_SLAB_MANAGER_HPP
#define KTH_DATABASE_SLAB_MANAGER_HPP

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/memory_map.hpp>

namespace kth {
namespace database {

constexpr size_t minimum_slabs_size = sizeof(file_offset);
constexpr size_t slab_hash_table_header_size(size_t buckets)
{
    return sizeof(file_offset) + minimum_slabs_size * buckets;
}

/// The slab manager represents a growing collection of various sized
/// slabs of data on disk. It will resize the file accordingly and keep
/// track of the current end pointer so new slabs can be allocated.
class BCD_API slab_manager
{
public:
    slab_manager(memory_map& file, file_offset header_size);

    /// Create slab manager.
    bool create();

    /// Prepare manager for use.
    bool start();

    /// Synchronise the payload size to disk.
    void sync() const;

    /// Allocate a slab and return its position, sync() after writing.
    file_offset new_slab(size_t size);

    /// Return memory object for the slab at the specified position.
    const memory_ptr get(file_offset position) const;

protected:

    /// Get the size of all slabs and size prefix (excludes header).
    file_offset payload_size() const;

private:

    // Read the size of the data from the file.
    void read_size();

    // Write the size of the data from the file.
    void write_size() const;

    // This class is thread and remap safe.
    memory_map& file_;
    const file_offset header_size_;

    // Payload size is protected by mutex.
    file_offset payload_size_;
    mutable shared_mutex mutex_;
};

} // namespace database
} // namespace kth

#endif

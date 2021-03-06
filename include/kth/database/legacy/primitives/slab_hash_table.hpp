// Copyright (c) 2016-2021 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_SLAB_HASH_TABLE_HPP
#define KTH_DATABASE_SLAB_HASH_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include <kth/database/legacy/primitives/hash_table_header.hpp>
#include <kth/database/legacy/primitives/slab_manager.hpp>

namespace kth::database {

using slab_hash_table_header = hash_table_header<array_index, file_offset>;

/**
 * A hashtable mapping hashes to variable sized values (slabs).
 * Uses a combination of the hash_table and slab_manager.
 *
 * The hash_table is basically a bucket list containing the start
 * value for the slab_row.
 *
 * The slab_manager is used to create linked chains. A header
 * containing the hash of the item, and the next value is stored
 * with each slab.
 *
 *   [ KeyType  ]
 *   [ next:8   ]
 *   [ value... ]
 *
 * If we run manager.sync() before the link() step then we ensure
 * data can be lost but the hashtable is never corrupted.
 * Instead we prefer speed and batch that operation. The user should
 * call allocator.sync() after a series of store() calls.
 */
template <typename KeyType>
class slab_hash_table {
public:
    using write_function = serializer<uint8_t*>::functor;

    slab_hash_table(slab_hash_table_header& header, slab_manager& manager);

    /// Execute a write. value_size is the required size of the buffer.
    /// Returns the file offset of the new value.
    file_offset store(const KeyType& key, write_function write, size_t value_size);

    /// Execute a writer against a key's buffer if the key is found.
    /// Returns the file offset of the found value (or zero).
    file_offset update(const KeyType& key, write_function write);

    /// Find the slab for a given key. Returns a null pointer if not found.
    memory_ptr find(const KeyType& key) const;

    /// Delete a key-value pair from the hashtable by unlinking the node.
    bool unlink(const KeyType& key);

    template <typename UnaryFunction>
    void for_each(UnaryFunction f) const;

private:

    // What is the bucket given a hash.
    array_index bucket_index(const KeyType& key) const;

    // What is the slab start position for a chain.
    file_offset read_bucket_value(const KeyType& key) const;

    // Link a new chain into the bucket header.
    void link(const KeyType& key, file_offset begin);

    file_offset read_bucket_value_by_index(array_index index) const;

    slab_hash_table_header& header_;
    slab_manager& manager_;
    mutable shared_mutex create_mutex_;
    mutable shared_mutex update_mutex_;
};

} // namespace kth::database

#include <kth/database/legacy/impl/slab_hash_table.ipp>

#endif

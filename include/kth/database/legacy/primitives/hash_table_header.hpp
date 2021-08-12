// Copyright (c) 2016-2021 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_HASH_TABLE_HEADER_HPP
#define KTH_DATABASE_HASH_TABLE_HEADER_HPP

#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory_map.hpp>

namespace kth::database {

/**
 * Implements contigious memory array with a fixed size elements.
 *
 * File format looks like:
 *
 *  [   size:IndexType   ]
 *  [ [      ...       ] ]
 *  [ [ item:ValueType ] ]
 *  [ [      ...       ] ]
 *
 * Empty elements are represented by the value hash_table_header.empty
 */
template <typename IndexType, typename ValueType>
class hash_table_header
{
public:
    static const ValueType empty;

    hash_table_header(memory_map& file, IndexType buckets);

    /// Allocate the hash table and populate with empty values.
    bool create();

    /// Must be called before use. Loads the size from the file.
    bool start();

    /// Read item's value.
    ValueType read(IndexType index) const;

    /// Write value to item.
    void write(IndexType index, ValueType value);

    /// The hash table size (bucket count).
    IndexType size() const;

private:

    // Locate the item in the memory map.
    file_offset item_position(IndexType index) const;

    memory_map& file_;
    IndexType buckets_;
    mutable shared_mutex mutex_;
};

} // namespace kth::database

#include <kth/database/legacy/impl/hash_table_header.ipp>

#endif

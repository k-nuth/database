// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_RECORD_HASH_TABLE_HPP
#define KTH_DATABASE_RECORD_HASH_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include <kth/database/legacy/primitives/hash_table_header.hpp>
#include <kth/database/legacy/primitives/record_manager.hpp>

namespace kth::database {

template <typename KeyType>
constexpr size_t hash_table_record_size(size_t value_size)
{
    return std::tuple_size<KeyType>::value + sizeof(array_index) + value_size;
}

typedef hash_table_header<array_index, array_index> record_hash_table_header;

/**
 * A hashtable mapping hashes to fixed sized values (records).
 * Uses a combination of the hash_table and record_manager.
 *
 * The hash_table is basically a bucket list containing the start
 * value for the record_row.
 *
 * The record_manager is used to create linked chains. A header
 * containing the hash of the item, and the next value is stored
 * with each record.
 *
 *   [ KeyType ]
 *   [ next:4  ]
 *   [ record  ]
 *
 * By using the record_manager instead of slabs, we can have smaller
 * indexes avoiding reading/writing extra bytes to the file.
 * Using fixed size records is therefore faster.
 */
template <typename KeyType>
class record_hash_table
{
public:
    typedef serializer<uint8_t*>::functor write_function;

    record_hash_table(record_hash_table_header& header, record_manager& manager);

    /// Execute a write. The provided write() function must write the correct
    /// number of bytes (record_size - key_size - sizeof(array_index)).
    void store(const KeyType& key, write_function write);

    /// Execute a writer against a key's buffer if the key is found.
    void update(const KeyType& key, write_function write);

    /// Find the record for a given key.
    /// Returns a null pointer if not found.
    memory_ptr find(const KeyType& key) const;

    /// Delete a key-value pair from the hashtable by unlinking the node.
    bool unlink(const KeyType& key);

private:
    // What is the bucket given a hash.
    array_index bucket_index(const KeyType& key) const;

    // What is the record start index for a chain.
    array_index read_bucket_value(const KeyType& key) const;

    // Link a new chain into the bucket header.
    void link(const KeyType& key, array_index begin);

    record_hash_table_header& header_;
    record_manager& manager_;
    mutable shared_mutex create_mutex_;
    mutable shared_mutex update_mutex_;

};

} // namespace database
} // namespace kth

#include <kth/database/legacy/impl/record_hash_table.ipp>

#endif

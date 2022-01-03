// Copyright (c) 2016-2022 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_RECORD_MULTIMAP_HPP
#define KTH_DATABASE_RECORD_MULTIMAP_HPP

#include <cstdint>
#include <string>
#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include <kth/database/legacy/primitives/record_hash_table.hpp>
#include <kth/database/legacy/primitives/record_manager.hpp>

namespace kth::database {

inline 
size_t multimap_record_size(size_t value_size) {
    return sizeof(array_index) + value_size;
}

template <typename KeyType>
constexpr 
size_t hash_table_multimap_record_size() {
    // The hash table maps a key only to the first record index.
    return hash_table_record_size<KeyType>(sizeof(array_index));
}

/**
 * A multimap hashtable where each key maps to a set of fixed size
 * values.
 *
 * The database is abstracted on top of a record map, and linked records.
 * The map links keys to start indexes in the linked records.
 * The linked records are chains of records that can be iterated through
 * given a start index.
 */
template <typename KeyType>
class record_multimap {
public:
    using write_function = serializer<uint8_t*>::functor;
    using record_hash_table_type = record_hash_table<KeyType>;

    record_multimap(record_hash_table_type& map, record_manager& manager);

    /// Add a new row for a key.
    void store(const KeyType& key, write_function write);

    /// Lookup a key, returning a traversable index.
    array_index find(const KeyType& key) const;

    /// Get a remap safe address pointer to the indexed data.
    memory_ptr get(array_index index) const;

    /// Delete the last row that was added for the key.
    bool unlink(const KeyType& key);

private:
    record_hash_table_type& map_;
    record_manager& manager_;
    mutable shared_mutex create_mutex_;
    mutable shared_mutex update_mutex_;
};

} // namespace kth::database

#include <kth/database/legacy/impl/record_multimap.ipp>

#endif

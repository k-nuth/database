// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_RECORD_MULTIMAP_IPP
#define KTH_DATABASE_RECORD_MULTIMAP_IPP

#include <kth/database/legacy/memory/memory.hpp>
#include <kth/database/legacy/primitives/record_list.hpp>

namespace kth::database {

template <typename KeyType>
record_multimap<KeyType>::record_multimap(record_hash_table_type& map,
    record_manager& manager)
  : map_(map), manager_(manager)
{
}

template <typename KeyType>
void record_multimap<KeyType>::store(const KeyType& key,
    write_function write)
{
    // Allocate and populate new unlinked row.
    record_list record(manager_);
    auto const begin = record.create(write);

    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(create_mutex_);

    auto const old_begin = find(key);

    // Link the row to the previous first element (or terminator).
    record.link(old_begin);

    if (old_begin == record_list::empty)
    {
        map_.store(key, [=](serializer<uint8_t*>& serial)
        {
            //*****************************************************************
            serial.template write_little_endian<array_index>(begin);
            //*****************************************************************
        });
    }
    else
    {
        map_.update(key, [=](serializer<uint8_t*>& serial)
        {
            // Critical Section
            ///////////////////////////////////////////////////////////////////
            unique_lock lock(update_mutex_);
            serial.template write_little_endian<array_index>(begin);
            ///////////////////////////////////////////////////////////////////
        });
    }
    ///////////////////////////////////////////////////////////////////////////
}

template <typename KeyType>
array_index record_multimap<KeyType>::find(const KeyType& key) const
{
    auto const begin_address = map_.find(key);

    if ( ! begin_address)
        return record_list::empty;

    auto const memory = REMAP_ADDRESS(begin_address);

    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(update_mutex_);
    return from_little_endian_unsafe<array_index>(memory);
    ///////////////////////////////////////////////////////////////////////////
}

/// Get a remap safe address pointer to the indexed data.
template <typename KeyType>
memory_ptr record_multimap<KeyType>::get(array_index index) const
{
    const record_list record(manager_, index);
    return record.data();
}

// Unlink is not safe for concurrent write.
template <typename KeyType>
bool record_multimap<KeyType>::unlink(const KeyType& key)
{
    auto const begin = find(key);

    // No rows exist.
    if (begin == record_list::empty)
        return false;

    auto const next_index = record_list(manager_, begin).next_index();

    // Remove the hash table entry, which delinks the single row.
    if (next_index == record_list::empty)
        return map_.unlink(key);

    // Update the hash table entry, which skips the first of multiple rows.
    map_.update(key, [&](serializer<uint8_t*>& serial)
    {
        // Critical Section.
        ///////////////////////////////////////////////////////////////////////
        unique_lock lock(update_mutex_);
        serial.template write_little_endian<array_index>(next_index);
        ///////////////////////////////////////////////////////////////////////
    });

    return true;
}

} // namespace database
} // namespace kth

#endif
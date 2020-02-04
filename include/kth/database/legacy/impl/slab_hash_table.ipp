// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_SLAB_HASH_TABLE_IPP
#define KTH_DATABASE_SLAB_HASH_TABLE_IPP

#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include "../impl/remainder.ipp"
#include "../impl/slab_row.ipp"

namespace kth {
namespace database {

template <typename KeyType>
slab_hash_table<KeyType>::slab_hash_table(slab_hash_table_header& header,
    slab_manager& manager)
  : header_(header), manager_(manager)
{
    KTH_ASSERT(slab_hash_table_header::empty == slab_row<KeyType>::empty);
}

// This is not limited to storing unique key values. If duplicate keyed values
// are store then retrieval and unlinking will fail as these multiples cannot
// be differentiated except in the order written (used by bip30).
template <typename KeyType>
file_offset slab_hash_table<KeyType>::store(const KeyType& key,
    write_function write, size_t value_size)
{
    // Allocate and populate new unlinked record.
    slab_row<KeyType> slab(manager_);
    auto const position = slab.create(key, write, value_size);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    create_mutex_.lock();

    // Link new slab.next to current first slab.
    slab.link(read_bucket_value(key));

    // Link header to new slab as the new first.
    link(key, position);

    create_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Return the file offset of the slab data segment.
    return position + slab_row<KeyType>::prefix_size;
}

// Execute a writer against a key's buffer if the key is found.
// Return the file offset of the found value (or zero).
template <typename KeyType>
file_offset slab_hash_table<KeyType>::update(const KeyType& key,
    write_function write)
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != header_.empty)
    {
        const slab_row<KeyType> item(manager_, current);

        // Found.
        if (item.compare(key))
        {
            auto const memory = item.data();
            auto serial = make_unsafe_serializer(REMAP_ADDRESS(memory));
            write(serial);
            return item.offset();
        }

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next_position();
        ///////////////////////////////////////////////////////////////////////
    }

    return 0;
}

// This is limited to returning the first of multiple matching key values.
template <typename KeyType>
memory_ptr slab_hash_table<KeyType>::find(const KeyType& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != header_.empty)
    {
        const slab_row<KeyType> item(manager_, current);

        // Found, return data.
        if (item.compare(key))
            return item.data();

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next_position();
        ///////////////////////////////////////////////////////////////////////
    }

    return nullptr;
}

// This is limited to unlinking the first of multiple matching key values.
template <typename KeyType>
bool slab_hash_table<KeyType>::unlink(const KeyType& key)
{
    // Find start item...
    auto previous = read_bucket_value(key);
    const slab_row<KeyType> begin_item(manager_, previous);

    // If start item has the key then unlink from buckets.
    if (begin_item.compare(key))
    {
        //*********************************************************************
        auto const next = begin_item.next_position();
        //*********************************************************************

        link(key, next);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    update_mutex_.lock_shared();
    auto current = begin_item.next_position();
    update_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // Iterate through list...
    while (current != header_.empty)
    {
        const slab_row<KeyType> item(manager_, current);

        // Found, unlink current item from previous.
        if (item.compare(key))
        {
            slab_row<KeyType> previous_item(manager_, previous);

            // Critical Section
            ///////////////////////////////////////////////////////////////////
            update_mutex_.lock_upgrade();
            auto const next = item.next_position();
            update_mutex_.unlock_upgrade_and_lock();
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            previous_item.write_next_position(next);
            update_mutex_.unlock();
            ///////////////////////////////////////////////////////////////////
            return true;
        }

        previous = current;

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next_position();
        ///////////////////////////////////////////////////////////////////////
    }

    return false;
}

template <typename KeyType>
array_index slab_hash_table<KeyType>::bucket_index(const KeyType& key) const
{
    auto const bucket = remainder(key, header_.size());
    KTH_ASSERT(bucket < header_.size());
    return bucket;
}

template <typename KeyType>
file_offset slab_hash_table<KeyType>::read_bucket_value(
    const KeyType& key) const
{
    auto const value = header_.read(bucket_index(key));
    static_assert(sizeof(value) == sizeof(file_offset), "Invalid size");
    return value;
}

template <typename KeyType>
void slab_hash_table<KeyType>::link(const KeyType& key, file_offset begin)
{
    header_.write(bucket_index(key), begin);
}

template <typename KeyType>
file_offset slab_hash_table<KeyType>::read_bucket_value_by_index(array_index index) const
{
    auto const value = header_.read(index);
    static_assert(sizeof(value) == sizeof(file_offset), "Invalid size");
    return value;
}


template <typename KeyType>
template <typename UnaryFunction>
void slab_hash_table<KeyType>::for_each(UnaryFunction f) const {
    array_index index = 0;
    while (index < header_.size()) {

        // Find start item...
        auto current = read_bucket_value_by_index(index);

        // Iterate through list...
        while (current != header_.empty) {
            const slab_row<KeyType> item(manager_, current);

            //DO SOMETHING
            if (!f(item.data())) { //memory_ptr
                return;
            }

            auto const previous = current;
            current = item.next_position();

            // This may otherwise produce an infinite loop here.
            // It indicates that a write operation has interceded.
            // So we must return gracefully vs. looping forever.
            // A parallel write operation cannot safely use this call.
            if (previous == current)
                break;
        }

        ++index;
    }
}

} // namespace database
} // namespace kth

#endif

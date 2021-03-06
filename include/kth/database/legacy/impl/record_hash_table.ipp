// Copyright (c) 2016-2021 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_RECORD_HASH_TABLE_IPP
#define KTH_DATABASE_RECORD_HASH_TABLE_IPP

#include <string>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include "../impl/record_row.ipp"
#include "../impl/remainder.ipp"

namespace kth::database {

template <typename KeyType>
record_hash_table<KeyType>::record_hash_table(
    record_hash_table_header& header, record_manager& manager)
  : header_(header), manager_(manager)
{
    KTH_ASSERT(record_hash_table_header::empty ==
        record_row<KeyType>::empty);
}

// This is not limited to storing unique key values. If duplicate keyed values
// are store then retrieval and unlinking will fail as these multiples cannot
// be differentiated except in the order written.
template <typename KeyType>
void record_hash_table<KeyType>::store(const KeyType& key,
    write_function write)
{
    // Allocate and populate new unlinked record.
    record_row<KeyType> record(manager_);
    auto const position = record.create(key, write);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    create_mutex_.lock();

    // Link new record.next to current first record.
    record.link(read_bucket_value(key));

    // Link header to new record as the new first.
    link(key, position);

    create_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

// Execute a writer against a key's buffer if the key is found.
template <typename KeyType>
void record_hash_table<KeyType>::update(const KeyType& key,
    write_function write)
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != header_.empty) {
        const record_row<KeyType> item(manager_, current);

        // Found.
        if (item.compare(key))
        {
            auto const memory = item.data();
            auto serial = make_unsafe_serializer(REMAP_ADDRESS(memory));
            write(serial);
            return;
        }

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next_index();
        ///////////////////////////////////////////////////////////////////////
    }
}

// This is limited to returning the first of multiple matching key values.
template <typename KeyType>
memory_ptr record_hash_table<KeyType>::find(const KeyType& key) const {
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != header_.empty) {
        const record_row<KeyType> item(manager_, current);

        // Found, return data.
        if (item.compare(key))
            return item.data();

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next_index();
        ///////////////////////////////////////////////////////////////////////
    }

    return nullptr;
}

// This is limited to unlinking the first of multiple matching key values.
template <typename KeyType>
bool record_hash_table<KeyType>::unlink(const KeyType& key)
{
    // Find start item...
    auto previous = read_bucket_value(key);
    const record_row<KeyType> begin_item(manager_, previous);

    // If start item has the key then unlink from buckets.
    if (begin_item.compare(key)) {
        //*********************************************************************
        auto const next = begin_item.next_index();
        //*********************************************************************

        link(key, next);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    update_mutex_.lock_shared();
    auto current = begin_item.next_index();
    update_mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////

    // Iterate through list...
    while (current != header_.empty) {
        const record_row<KeyType> item(manager_, current);

        // Found, unlink current item from previous.
        if (item.compare(key))
        {
            record_row<KeyType> previous_item(manager_, previous);

            // Critical Section
            ///////////////////////////////////////////////////////////////////
            update_mutex_.lock_upgrade();
            auto const next = item.next_index();
            update_mutex_.unlock_upgrade_and_lock();
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            previous_item.write_next_index(next);
            update_mutex_.unlock();
            ///////////////////////////////////////////////////////////////////
            return true;
        }

        previous = current;

        // Critical Section
        ///////////////////////////////////////////////////////////////////////
        shared_lock lock(update_mutex_);
        current = item.next_index();
        ///////////////////////////////////////////////////////////////////////
    }

    return false;
}

template <typename KeyType>
array_index record_hash_table<KeyType>::bucket_index(const KeyType& key) const {
    auto const bucket = remainder(key, header_.size());
    KTH_ASSERT(bucket < header_.size());
    return bucket;
}

template <typename KeyType>
array_index record_hash_table<KeyType>::read_bucket_value(
    const KeyType& key) const {
    auto value = header_.read(bucket_index(key));
    static_assert(sizeof(value) == sizeof(array_index), "Invalid size");
    return value;
}

template <typename KeyType>
void record_hash_table<KeyType>::link(const KeyType& key, array_index begin)
{
    header_.write(bucket_index(key), begin);
}

} // namespace kth::database

#endif

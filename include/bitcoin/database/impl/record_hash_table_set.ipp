/**
 * Copyright (c) 2016 Bitprim developers (see AUTHORS)
 *
 * This file is part of Bitprim.
 *
 * Bitprim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_RECORD_HASH_TABLE_SET_IPP
#define LIBBITCOIN_DATABASE_RECORD_HASH_TABLE_SET_IPP

#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include "../impl/record_row_set.ipp"
#include "../impl/remainder.ipp"

namespace libbitcoin {
namespace database {

template <typename KeyType>
record_hash_table_set<KeyType>::record_hash_table_set(
    record_hash_table_header& header, record_manager& manager)
  : header_(header), manager_(manager)
{
}

// This is not limited to storing unique key values. If duplicate keyed values
// are store then retrieval and unlinking will fail as these multiples cannot
// be differentiated.
template <typename KeyType>
void record_hash_table_set<KeyType>::store(KeyType const& key)
{
    auto const index = bucket_index(key);

    // Store current bucket value.
    const auto old_begin = read_bucket_value_from_index(index);
    record_row_set<KeyType> item(manager_, 0);
    const auto new_begin = item.create(key, old_begin);

    // Link record to header.
    link_with_index(index, new_begin);




    // // Store current bucket value.
    // const auto old_begin = read_bucket_value(key);
    // record_row_set<KeyType> item(manager_, 0);
    // const auto new_begin = item.create(key, old_begin);

    // // Link record to header.
    // link(key, new_begin);
}

// This is limited to returning the first of multiple matching key values.
template <typename KeyType>
bool record_hash_table_set<KeyType>::contains(KeyType const& key) const
{
    // Find start item...
    auto current = read_bucket_value(key);

    // Iterate through list...
    while (current != header_.empty)
    {
        const record_row_set<KeyType> item(manager_, current);

        // Found!
        if (item.compare(key))
            return true;

        const auto previous = current;
        current = item.next_index();

        // This may otherwise produce an infinite loop here.
        // It indicates that a write operation has interceded.
        // So we must return gracefully vs. looping forever.
        if (previous == current)
            return false;
    }

    return false;
}

// This is limited to unlinking the first of multiple matching key values.
template <typename KeyType>
bool record_hash_table_set<KeyType>::unlink(const KeyType& key)
{
    // Find start item...
    // const auto begin = read_bucket_value(key);
    auto const index = bucket_index(key);
    auto const begin = read_bucket_value_from_index(index);

    if (begin == header_.empty) return false;
    
    const record_row_set<KeyType> begin_item(manager_, begin);

    // If start item has the key then unlink from buckets.
    if (begin_item.compare(key))
    {
        // link(key, begin_item.next_index());
        link_with_index(index, begin_item.next_index());
        return true;
    }

    // Continue on...
    auto previous = begin;
    auto current = begin_item.next_index();

    // Iterate through list...
    while (current != header_.empty)
    {
        const record_row_set<KeyType> item(manager_, current);

        // Found, unlink current item from previous.
        if (item.compare(key))
        {
            release(item, previous);
            return true;
        }

        previous = current;
        current = item.next_index();

        // This may otherwise produce an infinite loop here.
        // It indicates that a write operation has interceded.
        // So we must return gracefully vs. looping forever.
        if (previous == current)
            return false;
    }

    return false;
}

template <typename KeyType>
array_index record_hash_table_set<KeyType>::bucket_index(
    const KeyType& key) const
{
    const auto bucket = remainder(key, header_.size());
    BITCOIN_ASSERT(bucket < header_.size());
    return bucket;
}

template <typename KeyType>
array_index record_hash_table_set<KeyType>::read_bucket_value(
    const KeyType& key) const
{
    // auto value = header_.read(bucket_index(key));
    // static_assert(sizeof(value) == sizeof(array_index), "Invalid size");
    // return value;
    return read_bucket_value_from_index(bucket_index(key));
}

template <typename KeyType>
array_index record_hash_table_set<KeyType>::read_bucket_value_from_index(
    array_index index) const
{
    auto value = header_.read(index);
    static_assert(sizeof(value) == sizeof(array_index), "Invalid size");
    return value;
}

template <typename KeyType>
void record_hash_table_set<KeyType>::link(const KeyType& key,
    const array_index begin)
{
    // header_.write(bucket_index(key), begin);
    link_with_index(bucket_index(key), begin);
}

template <typename KeyType>
void record_hash_table_set<KeyType>::link_with_index(array_index index,
    const array_index begin)
{
    header_.write(index, begin);
}


template <typename KeyType>
template <typename ListItem>
void record_hash_table_set<KeyType>::release(const ListItem& item,
    const file_offset previous)
{
    ListItem previous_item(manager_, previous);
    previous_item.write_next_index(item.next_index());
}

} // namespace database
} // namespace libbitcoin

#endif /*LIBBITCOIN_DATABASE_RECORD_HASH_TABLE_SET_IPP*/

// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_RECORD_ROW_IPP
#define KTH_DATABASE_RECORD_ROW_IPP

#include <cstddef>
#include <cstdint>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth::database {

/**
 * Item for record_hash_table. A chained list with the key included.
 *
 * Stores the key, next index and user data.
 * With the starting item, we can iterate until the end using the
 * next_index() method.
 */
template <typename KeyType>
class record_row {
public:
    static constexpr array_index empty = kth::max_uint32; 
    static constexpr size_t index_size = sizeof(array_index);
    static constexpr size_t key_start = 0;
    static constexpr size_t key_size = std::tuple_size<KeyType>::value;
    static constexpr file_offset prefix_size = key_size + index_size;

    using write_function = serializer<uint8_t*>::functor;

    // Construct for a new or existing record.
    record_row(record_manager& manager, array_index index=empty);

    /// Allocate and populate a new record.
    array_index create(const KeyType& key, write_function write);

    /// Link allocated/populated record.
    void link(array_index next);

    /// Does this match?
    bool compare(const KeyType& key) const;

    /// The actual user data.
    memory_ptr data() const;

    /// The file offset of the user data.
    file_offset offset() const;

    /// Index of next record in the list.
    array_index next_index() const;

    /// Write the next index.
    void write_next_index(array_index next);

private:
    memory_ptr raw_data(file_offset offset) const;

    array_index index_;
    record_manager& manager_;
};

template <typename KeyType>
record_row<KeyType>::record_row(record_manager& manager, array_index index)
  : manager_(manager), index_(index)
{
}

template <typename KeyType>
array_index record_row<KeyType>::create(const KeyType& key, write_function write) {
    KTH_ASSERT(index_ == empty);

    // Create new record and populate its key.
    //   [ KeyType  ] <==
    //   [ next:4   ]
    //   [ value... ]
    index_ = manager_.new_records(1);

    auto const memory = raw_data(key_start);
    auto const record = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(record);
    serial.write_forward(key);
    serial.skip(index_size);
    serial.write_delegated(write);

    return index_;
}

template <typename KeyType>
void record_row<KeyType>::link(array_index next) {
    // Populate next pointer value.
    //   [ KeyType  ]
    //   [ next:4   ] <==
    //   [ value... ]

    // Write record.
    auto const memory = raw_data(key_size);
    auto const next_data = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(next_data);

    //*************************************************************************
    serial.template write_little_endian<array_index>(next);
    //*************************************************************************
}

template <typename KeyType>
bool record_row<KeyType>::compare(const KeyType& key) const
{
    // Key data is at the start.
    auto const memory = raw_data(key_start);
    return std::equal(key.begin(), key.end(), REMAP_ADDRESS(memory));
}

template <typename KeyType>
memory_ptr record_row<KeyType>::data() const
{
    // Get value pointer.
    //   [ KeyType  ]
    //   [ next:4   ]
    //   [ value... ] ==>

    // Value data is at the end.
    return raw_data(prefix_size);
}

template <typename KeyType>
file_offset record_row<KeyType>::offset() const
{
    // Value data is at the end.
    return index_ + prefix_size;
}

template <typename KeyType>
array_index record_row<KeyType>::next_index() const
{
    auto const memory = raw_data(key_size);
    auto const next_address = REMAP_ADDRESS(memory);

    //*************************************************************************
    return from_little_endian_unsafe<array_index>(next_address);
    //*************************************************************************
}

template <typename KeyType>
void record_row<KeyType>::write_next_index(array_index next)
{
    auto const memory = raw_data(key_size);
    auto serial = make_unsafe_serializer(REMAP_ADDRESS(memory));

    //*************************************************************************
    serial.template write_little_endian<array_index>(next);
    //*************************************************************************
}

template <typename KeyType>
memory_ptr record_row<KeyType>::raw_data(file_offset offset) const
{
    auto memory = manager_.get(index_);
    REMAP_INCREMENT(memory, offset);
    return memory;
}

} // namespace database
} // namespace kth

#endif

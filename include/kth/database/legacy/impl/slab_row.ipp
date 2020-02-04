// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_SLAB_LIST_IPP
#define KTH_DATABASE_SLAB_LIST_IPP

#include <cstddef>
#include <cstdint>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth {
namespace database {

/**
 * Item for slab_hash_table. A chained list with the key included.
 *
 * Stores the key, next position and user data.
 * With the starting item, we can iterate until the end using the
 * next_position() method.
 */
template <typename KeyType>
class slab_row
{
public:
    static constexpr file_offset empty = bc::max_uint64;
    static constexpr size_t position_size = sizeof(file_offset);
    static constexpr size_t key_start = 0;
    static constexpr size_t key_size = std::tuple_size<KeyType>::value;
    static constexpr file_offset prefix_size = key_size + position_size;

    typedef serializer<uint8_t*>::functor write_function;

    // Construct for a new or existing slab.
    slab_row(slab_manager& manager, file_offset position=empty);

    /// Allocate and populate a new slab.
    file_offset create(const KeyType& key, write_function write,
        size_t value_size);

    /// Link allocated/populated slab.
    void link(file_offset next);

    /// Does this match?
    bool compare(const KeyType& key) const;

    /// The actual user data.
    memory_ptr data() const;

    /// The file offset of the user data.
    file_offset offset() const;

    /// Position of next slab in the list.
    file_offset next_position() const;

    /// Write the next position.
    void write_next_position(file_offset next);

private:
    memory_ptr raw_data(file_offset offset) const;

    file_offset position_;
    slab_manager& manager_;
};

template <typename KeyType>
slab_row<KeyType>::slab_row(slab_manager& manager, file_offset position)
  : manager_(manager), position_(position)
{
    static_assert(position_size == 8, "Invalid file_offset size.");
}

template <typename KeyType>
file_offset slab_row<KeyType>::create(const KeyType& key, write_function write,
    size_t value_size)
{
    KTH_ASSERT(position_ == empty);

    // Create new slab and populate its key.
    //   [ KeyType  ] <==
    //   [ next:8   ]
    //   [ value... ]
    const size_t slab_size = prefix_size + value_size;
    position_ = manager_.new_slab(slab_size);

    auto const memory = raw_data(key_start);
    auto const key_data = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(key_data);
    serial.write_forward(key);
    serial.skip(position_size);
    serial.write_delegated(write);

    return position_;
}

template <typename KeyType>
void slab_row<KeyType>::link(file_offset next)
{
    // Populate next pointer value.
    //   [ KeyType  ]
    //   [ next:8   ] <==
    //   [ value... ]

    // Write next pointer after the key.
    auto const memory = raw_data(key_size);
    auto const next_data = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(next_data);

    //*************************************************************************
    serial.template write_little_endian<file_offset>(next);
    //*************************************************************************
}

template <typename KeyType>
bool slab_row<KeyType>::compare(const KeyType& key) const
{
    auto const memory = raw_data(key_start);
    return std::equal(key.begin(), key.end(), REMAP_ADDRESS(memory));
}

template <typename KeyType>
memory_ptr slab_row<KeyType>::data() const
{
    // Get value pointer.
    //   [ KeyType  ]
    //   [ next:8   ]
    //   [ value... ] ==>

    // Value data is at the end.
    return raw_data(prefix_size);
}

template <typename KeyType>
file_offset slab_row<KeyType>::offset() const
{
    // Value data is at the end.
    return position_ + prefix_size;
}

template <typename KeyType>
file_offset slab_row<KeyType>::next_position() const
{
    auto const memory = raw_data(key_size);
    auto const next_address = REMAP_ADDRESS(memory);

    //*************************************************************************
    return from_little_endian_unsafe<file_offset>(next_address);
    //*************************************************************************
}

template <typename KeyType>
void slab_row<KeyType>::write_next_position(file_offset next)
{
    auto const memory = raw_data(key_size);
    auto serial = make_unsafe_serializer(REMAP_ADDRESS(memory));

    //*************************************************************************
    serial.template write_little_endian<file_offset>(next);
    //*************************************************************************
}

template <typename KeyType>
memory_ptr slab_row<KeyType>::raw_data(file_offset offset) const
{
    auto memory = manager_.get(position_);
    REMAP_INCREMENT(memory, offset);
    return memory;
}

} // namespace database
} // namespace kth

#endif

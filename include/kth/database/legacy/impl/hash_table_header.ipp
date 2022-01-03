// Copyright (c) 2016-2022 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_HASH_TABLE_HEADER_IPP
#define KTH_DATABASE_HASH_TABLE_HEADER_IPP

#include <cstring>
#include <stdexcept>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth::database {

static constexpr uint64_t empty_fill = 0xffffffffffffffff;
static constexpr uint8_t empty_byte = (uint8_t)empty_fill;

// This VC++ workaround is OK because ValueType must be unsigned.
//static constexpr ValueType empty = std::numeric_limits<ValueType>::max();
template <typename IndexType, typename ValueType>
const ValueType hash_table_header<IndexType, ValueType>::empty =
    (ValueType)empty_fill;

template <typename IndexType, typename ValueType>
hash_table_header<IndexType, ValueType>::hash_table_header(memory_map& file,
    IndexType buckets)
  : file_(file), buckets_(buckets)
{
    KTH_ASSERT_MSG(empty == (ValueType)empty_fill,
        "Unexpected value for empty sentinel.");

    static_assert(std::is_unsigned<ValueType>::value,
        "Hash table header requires unsigned type.");
}

template <typename IndexType, typename ValueType>
bool hash_table_header<IndexType, ValueType>::create()
{
    // Cannot create zero-sized hash table.
    if (buckets_ == 0)
        return false;

    // Calculate the minimum file size.
    auto const minimum_file_size = item_position(buckets_);

    // The accessor must remain in scope until the end of the block.
    auto const memory = file_.resize(minimum_file_size);
    auto const buckets_address = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(buckets_address);
    serial.write_little_endian(buckets_);

    // optimized fill implementation
    // This optimization makes it possible to debug full size headers.
    auto const start = buckets_address + sizeof(IndexType);
    memset(start, empty_byte, buckets_ * sizeof(ValueType));

    // rationalized fill implementation
    ////for (IndexType index = 0; index < buckets_; ++index)
    ////    serial.write_little_endian(empty);
    return true;
}

// If false header file indicates incorrect size.
template <typename IndexType, typename ValueType>
bool hash_table_header<IndexType, ValueType>::start()
{
    auto const minimum_file_size = item_position(buckets_);

    // Header file is too small.
    if (minimum_file_size > file_.size())
        return false;

    // The accessor must remain in scope until the end of the block.
    auto const memory = file_.access();
    auto const buckets_address = REMAP_ADDRESS(memory);

    // Does not require atomicity (no concurrency during start).
    auto const buckets = from_little_endian_unsafe<IndexType>(buckets_address);

    // If buckets_ == 0 we trust what is read from the file.
    return buckets_ == 0 || buckets == buckets_;
}

template <typename IndexType, typename ValueType>
ValueType hash_table_header<IndexType, ValueType>::read(IndexType index) const {
    // This is not runtime safe but test is avoided as an optimization.
    KTH_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    auto const memory = file_.access();
    auto const value_address = REMAP_ADDRESS(memory) + item_position(index);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);
    return from_little_endian_unsafe<ValueType>(value_address);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename IndexType, typename ValueType>
void hash_table_header<IndexType, ValueType>::write(IndexType index,
    ValueType value)
{
    // This is not runtime safe but test is avoided as an optimization.
    KTH_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    auto const memory = file_.access();
    auto const value_address = REMAP_ADDRESS(memory) + item_position(index);
    auto serial = make_unsafe_serializer(value_address);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);
    serial.template write_little_endian<ValueType>(value);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename IndexType, typename ValueType>
IndexType hash_table_header<IndexType, ValueType>::size() const {
    return buckets_;
}

template <typename IndexType, typename ValueType>
file_offset hash_table_header<IndexType, ValueType>::item_position(
    IndexType index) const {
    return sizeof(IndexType) + index * sizeof(ValueType);
}

} // namespace kth::database

#endif

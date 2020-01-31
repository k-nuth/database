// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin/database/primitives/record_list.hpp>

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

record_list::record_list(record_manager& manager, array_index index)
  : manager_(manager), index_(index)
{
}

array_index record_list::create(write_function write)
{
    BITCOIN_ASSERT(index_ == empty);

    // Create new record without populating its next pointer.
    //   [ next:4   ]
    //   [ value... ] <==
    index_ = manager_.new_records(1);

    auto const memory = raw_data(index_size);
    auto const record = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(record);
    serial.write_delegated(write);

    return index_;
}

void record_list::link(array_index next)
{
    // Populate next pointer value.
    //   [ next:4   ] <==
    //   [ value... ]

    // Write record.
    auto const memory = raw_data(0);
    auto const next_data = REMAP_ADDRESS(memory);
    auto serial = make_unsafe_serializer(next_data);

    //*************************************************************************
    serial.template write_little_endian<array_index>(next);
    //*************************************************************************
}
memory_ptr record_list::data() const
{
    // Get value pointer.
    //   [ next:4   ]
    //   [ value... ] ==>

    // Value data is at the end.
    return raw_data(index_size);
}

array_index record_list::next_index() const
{
    auto const memory = raw_data(0);
    auto const next_address = REMAP_ADDRESS(memory);
    //*************************************************************************
    return from_little_endian_unsafe<array_index>(next_address);
    //*************************************************************************
}

memory_ptr record_list::raw_data(file_offset offset) const
{
    auto memory = manager_.get(index_);
    REMAP_INCREMENT(memory, offset);
    return memory;
}

} // namespace database
} // namespace kth

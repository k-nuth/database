// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_RECORD_LIST_HPP
#define KTH_DATABASE_RECORD_LIST_HPP

#include <cstddef>
#include <cstdint>
#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include <kth/database/legacy/primitives/record_manager.hpp>

namespace kth {
namespace database {

class BCD_API record_list
{
public:
    static constexpr array_index empty = bc::max_uint32;
    static constexpr size_t index_size = sizeof(array_index);

    typedef serializer<uint8_t*>::functor write_function;

    /// Construct for a new or existing record.
    record_list(record_manager& manager, array_index index=empty);

    /// Allocate and populate a new record.
    array_index create(write_function write);

    /// Allocate a record to the existing next record.
    void link(array_index next);

    /// The actual user data for this record.
    memory_ptr data() const;

    /// Index of the next record.
    array_index next_index() const;

private:
    memory_ptr raw_data(file_offset offset) const;

    array_index index_;
    record_manager& manager_;
};

} // namespace database
} // namespace kth

#endif

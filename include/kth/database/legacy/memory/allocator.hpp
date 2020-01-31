// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_ALLOCATOR_HPP
#define KTH_DATABASE_ALLOCATOR_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace kth {
namespace database {

#ifdef REMAP_SAFETY

/// This class provides remap safe access to file-mapped memory.
/// The memory size is unprotected and unmanaged.
class BCD_API allocator
  : public memory, noncopyable
{
public:
    allocator(shared_mutex& mutex);
    ~allocator();

    /// Get the address indicated by the pointer.
    uint8_t* buffer();

    /// Advance the pointer the specified number of bytes.
    void increment(size_t value);

protected:
    friend class memory_map;

    /// Set the pointer.
    void assign(uint8_t* data);

private:
    shared_mutex& mutex_;
    uint8_t* data_;
};

#endif // REMAP_SAFETY

} // namespace database
} // namespace kth

#endif

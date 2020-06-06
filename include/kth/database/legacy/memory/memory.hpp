// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_MEMORY_HPP
#define KTH_DATABASE_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <boost/thread.hpp>
#include <kth/domain.hpp>
#include <kth/database/define.hpp>

namespace kth::database {

#ifdef REMAP_SAFETY

/// This interface defines remap safe unrestricted access to a memory map.
class KD_API memory
{
public:
    typedef std::shared_ptr<memory> ptr;

    /// Get the address indicated by the pointer.
    virtual uint8_t* buffer() = 0;

    /// Increment the pointer the specified number of bytes within the record.
    virtual void increment(size_t value) = 0;
};

#endif // REMAP_SAFETY

#ifdef REMAP_SAFETY
    typedef memory::ptr memory_ptr;
    #define REMAP_ADDRESS(ptr) ptr->buffer()
    #define REMAP_ASSIGN(ptr, data) ptr->assign(data)
    #define REMAP_INCREMENT(ptr, offset) ptr->increment(offset)
    #define REMAP_ACCESSOR(ptr, mutex) std::make_shared<accessor>(mutex, ptr)
    #define REMAP_ALLOCATOR(mutex) std::make_shared<allocator>(mutex)
    #define REMAP_READ(mutex) shared_lock lock(mutex)
    #define REMAP_WRITE(mutex) unique_lock lock(mutex)
#else
    typedef uint8_t* memory_ptr;
    #define REMAP_ADDRESS(ptr) ptr
    #define REMAP_ASSIGN(ptr, data)
    #define REMAP_INCREMENT(ptr, offset) ptr += (offset)
    #define REMAP_ACCESSOR(ptr, mutex)
    #define REMAP_ALLOCATOR(mutex)
    #define REMAP_READ(mutex)
    #define REMAP_WRITE(mutex)
#endif // REMAP_SAFETY

#ifdef ALLOCATE_SAFETY
    #define ALLOCATE_READ(mutex) shared_lock lock(mutex)
    #define ALLOCATE_WRITE(mutex) unique_lock lock(mutex)
#else
    #define ALLOCATE_READ(mutex)
    #define ALLOCATE_WRITE(mutex)
#endif // ALLOCATE_SAFETY

} // namespace database
} // namespace kth

#endif

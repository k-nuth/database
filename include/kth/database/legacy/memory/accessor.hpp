// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_ACCESSOR_HPP
#define KTH_DATABASE_ACCESSOR_HPP

#include <cstddef>
#include <cstdint>
#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth {
namespace database {

#ifdef REMAP_SAFETY

/// This class provides shared remap safe access to file-mapped memory.
/// The memory size is unprotected and unmanaged.
class BCD_API accessor
  : public memory
{
public:
    accessor(shared_mutex& mutex, uint8_t*& data);
    ~accessor();

    /// This class is not copyable.
    accessor(const accessor& other) = delete;

    /// Get the address indicated by the pointer.
    uint8_t* buffer();

    /// Increment the pointer the specified number of bytes.
    void increment(size_t value);

private:
    shared_mutex& mutex_;
    uint8_t* data_;
};

#endif // REMAP_SAFETY

} // namespace database
} // namespace kth

#endif

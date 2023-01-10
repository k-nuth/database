// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/legacy/memory/accessor.hpp>

#include <cstdint>
#include <cstddef>
#include <kth/domain.hpp>
#include <kth/database/define.hpp>

namespace kth::database {

#ifdef REMAP_SAFETY

accessor::accessor(shared_mutex& mutex, uint8_t*& data)
  : mutex_(mutex)
{
    ///////////////////////////////////////////////////////////////////////////
    // Begin Critical Section

    // Acquire shared lock.
    mutex_.lock_shared();

    KTH_ASSERT_MSG(data != nullptr, "Invalid pointer value.");

    // Save protected pointer.
    data_ = data;
}

uint8_t* accessor::buffer()
{
    return data_;
}

void accessor::increment(size_t value)
{
    KTH_ASSERT((size_t)data_ <= kth::max_size_t - value);
    data_ += value;
}

accessor::~accessor()
{
    // Release shared lock.
    mutex_.unlock_shared();

    // End Critical Section
    ///////////////////////////////////////////////////////////////////////////
}

#endif // REMAP_SAFETY

} // namespace kth::database

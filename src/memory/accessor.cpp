// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin/database/memory/accessor.hpp>

#include <cstdint>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

#ifdef REMAP_SAFETY

accessor::accessor(shared_mutex& mutex, uint8_t*& data)
  : mutex_(mutex)
{
    ///////////////////////////////////////////////////////////////////////////
    // Begin Critical Section

    // Acquire shared lock.
    mutex_.lock_shared();

    BITCOIN_ASSERT_MSG(data != nullptr, "Invalid pointer value.");

    // Save protected pointer.
    data_ = data;
}

uint8_t* accessor::buffer()
{
    return data_;
}

void accessor::increment(size_t value)
{
    BITCOIN_ASSERT((size_t)data_ <= bc::max_size_t - value);
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

} // namespace database
} // namespace kth

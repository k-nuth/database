// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin/database/memory/allocator.hpp>

#include <cstdint>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

#ifdef REMAP_SAFETY

allocator::allocator(shared_mutex& mutex)
  : mutex_(mutex),
    data_(nullptr)
{
    // Begin Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();
}

// Get share-protected data pointer.
uint8_t* allocator::buffer()
{
    BITCOIN_ASSERT_MSG(data_ != nullptr, "Downgrade must be called.");
    return data_;
}

// Add an unsafe offset to share-protected pointer (convenience method).
void allocator::increment(size_t value)
{
    BITCOIN_ASSERT((size_t)data_ <= bc::max_size_t - value);
    data_ += value;
}

// protected/friend
void allocator::assign(uint8_t* data)
{
    // The caller has the option to upgrade/downgrade but must have left the
    // the mutex in the original (upgradeable) state or this will fail.
    mutex_.unlock_upgrade_and_lock_shared();

    BITCOIN_ASSERT_MSG(data != nullptr, "Invalid pointer value.");
    data_ = data;
}

allocator::~allocator()
{
    // End Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.unlock_shared();
}

#endif // REMAP_SAFETY

} // namespace database
} // namespace kth

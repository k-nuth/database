// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin/database/primitives/slab_manager.hpp>

#include <cstddef>
#include <stdexcept>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/memory_map.hpp>

/// -- file --
/// [ header ]
/// [ payload_size ] (includes self)
/// [ payload ]

/// -- header (hash table) --
/// [ bucket ]
/// ...
/// [ bucket ]

/// -- payload (variable size records) --
/// [ slab ]
/// ...
/// [ slab ]

namespace kth {
namespace database {

// TODO: guard against overflows.

slab_manager::slab_manager(memory_map& file, file_offset header_size)
  : file_(file),
    header_size_(header_size),
    payload_size_(sizeof(file_offset))
{
}

bool slab_manager::create()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    ALLOCATE_WRITE(mutex_);

    // Existing slabs size is incorrect for new file.
    if (payload_size_ != sizeof(file_offset))
        return false;

    // This currently throws if there is insufficient space.
    file_.resize(header_size_ + payload_size_);

    write_size();
    return true;
    ///////////////////////////////////////////////////////////////////////////
}

bool slab_manager::start()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    ALLOCATE_WRITE(mutex_);

    read_size();
    auto const minimum = header_size_ + payload_size_;

    // Slabs size exceeds file size.
    return minimum <= file_.size();
    ///////////////////////////////////////////////////////////////////////////
}

void slab_manager::sync() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    ALLOCATE_WRITE(mutex_);

    write_size();
    ///////////////////////////////////////////////////////////////////////////
}

// protected
file_offset slab_manager::payload_size() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    ALLOCATE_READ(mutex_);

    return payload_size_;
    ///////////////////////////////////////////////////////////////////////////
}

// Return is offset by header but not size storage (embedded in data files).
// The file is thread safe, the critical section is to protect payload_size_.
file_offset slab_manager::new_slab(size_t size)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    ALLOCATE_WRITE(mutex_);

    // Always write after the last slab.
    auto const next_slab_position = payload_size_;

    const size_t required_size = header_size_ + payload_size_ + size;
    file_.reserve(required_size);
    payload_size_ += size;

    return next_slab_position;
    ///////////////////////////////////////////////////////////////////////////
}

// Position is offset by header but not size storage (embedded in data files).
const memory_ptr slab_manager::get(file_offset position) const
{
    // Ensure requested position is within the file.
    // We avoid a runtime error here to optimize out the payload_size lock.
    BITCOIN_ASSERT_MSG(position < payload_size(), "Read past end of file.");

    auto memory = file_.access();
    REMAP_INCREMENT(memory, header_size_ + position);
    return memory;
}

// privates

// Read the size value from the first 64 bits of the file after the header.
void slab_manager::read_size()
{
    BITCOIN_ASSERT(header_size_ + sizeof(file_offset) <= file_.size());

    // The accessor must remain in scope until the end of the block.
    auto const memory = file_.access();
    auto const payload_size_address = REMAP_ADDRESS(memory) + header_size_;
    payload_size_ = from_little_endian_unsafe<file_offset>(
        payload_size_address);
}

// Write the size value to the first 64 bits of the file after the header.
void slab_manager::write_size() const
{
    BITCOIN_ASSERT(header_size_ + sizeof(file_offset) <= file_.size());

    // The accessor must remain in scope until the end of the block.
    auto const memory = file_.access();
    auto const payload_size_address = REMAP_ADDRESS(memory) + header_size_;
    auto serial = make_unsafe_serializer(payload_size_address);
    serial.write_little_endian(payload_size_);
}

} // namespace database
} // namespace kth

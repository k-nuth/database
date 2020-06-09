// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef KTH_DB_LEGACY
#include <kth/database/legacy/result/block_result.hpp>

#include <cstdint>
#include <cstddef>
#include <utility>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth::database {

using namespace kth::domain::chain;

static constexpr size_t version_size = sizeof(uint32_t);
static constexpr size_t previous_size = hash_size;
static constexpr size_t merkle_size = hash_size;
static constexpr size_t time_size = sizeof(uint32_t);
static constexpr size_t bits_size = sizeof(uint32_t);
static constexpr size_t nonce_size = sizeof(uint32_t);
static constexpr size_t median_time_past_size = sizeof(uint32_t);
static constexpr size_t height_size = sizeof(uint32_t);
static constexpr size_t serialized_size_size = sizeof(uint64_t);

static constexpr auto version_offset = 0u;
static constexpr auto time_offset = version_size + previous_size + merkle_size;
static constexpr auto bits_offset = time_offset + time_size;
static constexpr auto height_offset = bits_offset + bits_size + nonce_size +
    median_time_past_size;
static constexpr auto serialized_size_offset = height_offset + height_size;
static constexpr auto count_offset = serialized_size_offset + serialized_size_size;

block_result::block_result()
  : block_result(nullptr)
{
}

block_result::block_result(const memory_ptr slab)
  : slab_(slab), height_(0), hash_(null_hash)
{
}

block_result::block_result(const memory_ptr slab, hash_digest&& hash,
    uint32_t height)
  : slab_(slab), height_(height), hash_(std::move(hash))
{
}

block_result::block_result(const memory_ptr slab, hash_digest const& hash,
    uint32_t height)
  : slab_(slab), height_(height), hash_(hash)
{
}

block_result::operator bool() const
{
    return slab_ != nullptr;
}

void block_result::reset()
{
    slab_.reset();
}

hash_digest const& block_result::hash() const
{
    return hash_;
}

chain::header block_result::header() const
{
    KTH_ASSERT(slab_);
    auto const memory = REMAP_ADDRESS(slab_);
    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(slab_));

    // READ THE HEADER (including median_time_past metadata).
    chain::header header;
    header.from_data(deserial, false);
    header.validation.height = height_;

    // TODO: add hash param to deserialization to eliminate this move.
    return chain::header(std::move(header), hash_digest(hash_));
}

// TODO: block height is unguarded and will be inconsistent during write.
size_t block_result::height() const
{
    KTH_ASSERT(slab_);
    return height_;
}

uint32_t block_result::bits() const
{
    KTH_ASSERT(slab_);
    auto const memory = REMAP_ADDRESS(slab_);
    return from_little_endian_unsafe<uint32_t>(memory + bits_offset);
}

uint32_t block_result::timestamp() const
{
    KTH_ASSERT(slab_);
    auto const memory = REMAP_ADDRESS(slab_);
    return from_little_endian_unsafe<uint32_t>(memory + time_offset);
}

uint32_t block_result::version() const
{
    KTH_ASSERT(slab_);
    auto const memory = REMAP_ADDRESS(slab_);
    return from_little_endian_unsafe<uint32_t>(memory + version_offset);
}

size_t block_result::transaction_count() const
{
    KTH_ASSERT(slab_);
    auto const memory = REMAP_ADDRESS(slab_);
    auto deserial = make_unsafe_deserializer(memory + count_offset);
    return deserial.read_size_little_endian();
}

hash_digest block_result::transaction_hash(size_t index) const
{
    KTH_ASSERT(slab_);
    auto const memory = REMAP_ADDRESS(slab_);
    auto deserial = make_unsafe_deserializer(memory + count_offset);
    auto const tx_count = deserial.read_size_little_endian();

    KTH_ASSERT(index < tx_count);
    deserial.skip(index * hash_size);
    return deserial.read_hash();
}

hash_list block_result::transaction_hashes() const
{
    KTH_ASSERT(slab_);
    auto const memory = REMAP_ADDRESS(slab_);
    auto deserial = make_unsafe_deserializer(memory + count_offset);
    auto const tx_count = deserial.read_size_little_endian();
    hash_list hashes;
    hashes.reserve(tx_count);

    for (size_t position = 0; position < tx_count; ++position)
        hashes.push_back(deserial.read_hash());

    return hashes;
}

uint64_t block_result::serialized_size() const
{
    KTH_ASSERT(slab_);
    auto const memory = REMAP_ADDRESS(slab_);
    auto deserial = make_unsafe_deserializer(memory + serialized_size_offset);
    return deserial.read_8_bytes_little_endian();
}

} // namespace database
} // namespace kth

#endif // KTH_DB_LEGACY
// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef KTH_DB_LEGACY

#include <kth/database/legacy/result/transaction_result.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <kth/domain.hpp>
#include <kth/database/legacy/databases/transaction_database.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth::database {

using namespace kth::domain::chain;

static constexpr auto value_size = sizeof(uint64_t);
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(position_t);
static constexpr auto median_time_past_size = sizeof(uint32_t);
static constexpr auto metadata_size = height_size + position_size + median_time_past_size;

transaction_result::transaction_result()
    : transaction_result(nullptr)
{}

transaction_result::transaction_result(const memory_ptr slab)
    : slab_(slab), height_(0), median_time_past_(0), position_(0)
    , hash_(null_hash)
{}

transaction_result::transaction_result(const memory_ptr slab, hash_digest&& hash, uint32_t height, uint32_t median_time_past, position_t position)
    : slab_(slab), height_(height), median_time_past_(median_time_past)
    , position_(position), hash_(std::move(hash))
{}

transaction_result::transaction_result(const memory_ptr slab, hash_digest const& hash, uint32_t height, uint32_t median_time_past, position_t position)
    : slab_(slab), height_(height), median_time_past_(median_time_past), position_(position), hash_(hash)
{}

transaction_result::operator bool() const {
    return slab_ != nullptr;
}

void transaction_result::reset() {
    slab_.reset();
}

hash_digest const& transaction_result::hash() const {
    return hash_;
}

// Height is unguarded and will be inconsistent during write.
size_t transaction_result::height() const {
    KTH_ASSERT(slab_);
    return height_;
}

// Position is unguarded and will be inconsistent during write.
size_t transaction_result::position() const {
    KTH_ASSERT(slab_);
    return position_;
}

// Median time past is unguarded and will be inconsistent during write.
uint32_t transaction_result::median_time_past() const {
    KTH_ASSERT(slab_);
    return median_time_past_;
}

// Spentness is unguarded and will be inconsistent during write.
bool transaction_result::is_spent(size_t fork_height) const {
    // Cannot be spent if unconfirmed.
    if (position_ == transaction_database::unconfirmed)
        return false;

    KTH_ASSERT(slab_);
    auto const tx_start = REMAP_ADDRESS(slab_) + metadata_size;
    auto deserial = make_unsafe_deserializer(tx_start);
    auto const outputs = deserial.read_size_little_endian();
    KTH_ASSERT(deserial);

    // Search all outputs for an unspent indication.
    for (uint32_t output = 0; output < outputs; ++output) {
        auto const spender_height = deserial.read_4_bytes_little_endian();
        KTH_ASSERT(deserial);

        // A spend from above the fork height is not an actual spend.
        if (spender_height == output::validation::not_spent ||
            spender_height > fork_height)
            return false;

        deserial.skip(value_size);
        deserial.skip(deserial.read_size_little_endian());
        KTH_ASSERT(deserial);
    }

    return true;
}

// spender_heights are unguarded and will be inconsistent during write.
// If index is out of range returns default/invalid output (.value not_found).
domain::chain::output transaction_result::output(uint32_t index) const {
    KTH_ASSERT(slab_);
    auto const tx_start = REMAP_ADDRESS(slab_) + metadata_size;
    auto deserial = make_unsafe_deserializer(tx_start);
    auto const outputs = deserial.read_size_little_endian();
    KTH_ASSERT(deserial);

    if (index >= outputs) {
        return {};
    }

    // Skip outputs until the target output.
    for (uint32_t output = 0; output < index; ++output) {
        deserial.skip(height_size + value_size);
        deserial.skip(deserial.read_size_little_endian());
        KTH_ASSERT(deserial);
    }

    // Read and return the target output (including spender height).
    domain::chain::output out;
    out.from_data(deserial, false);
    return out;
}

// median_time_past added in v3.3, witness added in v3.4
// ----------------------------------------------------------------------------
// [ height:4 ]
// [ position:2 ]
// [ median_time_past:4 ]
// ----------------------------------------------------------------------------
// [ output_count:varint ]
// [ [ spender_height:4 ][ value:8 ][ script:varint ]...]
// [ input_count:varint ]
// [ [ hash:4 ][ index:2 ][ script:varint ][ witness:varint ][ sequence:4 ]...]
// [ locktime:varint ]
// [ version:varint ]
// ----------------------------------------------------------------------------

// spender_heights are unguarded and will be inconsistent during write.
domain::chain::transaction transaction_result::transaction(bool witness) const
{
#ifdef KTH_CURRENCY_BCH
    witness = false;
    bool from_data_witness = false;
#else 
    bool from_data_witness = true;
#endif
    KTH_ASSERT(slab_);
    auto const tx_start = REMAP_ADDRESS(slab_) + metadata_size;
    auto deserial = make_unsafe_deserializer(tx_start);

    // READ THE TX
    //TODO WITNESS
    domain::chain::transaction tx;
    tx.from_data(deserial, false, from_data_witness
    #ifdef KTH_CACHED_RPC_DATA
        , false
    #endif
    );

#ifndef KTH_CURRENCY_BCH
    //TODO(legacy): optimize so that witness reads are skipped.
    if ( ! witness) {
        tx.strip_witness();
    }
#endif

    //TODO(legacy): add hash param to deserialization to eliminate this construction.
    return domain::chain::transaction(std::move(tx), hash_digest(hash_));
}

}} // namespace kth::database

#endif // KTH_DB_LEGACY
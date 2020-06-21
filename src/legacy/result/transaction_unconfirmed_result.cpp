// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED

#include <kth/database/legacy/result/transaction_unconfirmed_result.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <kth/domain.hpp>
#include <kth/database/legacy/databases/transaction_unconfirmed_database.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth::database {

using namespace kth::domain::chain;

static constexpr auto value_size = sizeof(uint64_t);
static constexpr auto arrival_time_size = sizeof(uint32_t);
static constexpr auto metadata_size = arrival_time_size;

transaction_unconfirmed_result::transaction_unconfirmed_result()
  : transaction_unconfirmed_result(nullptr)
{
}

transaction_unconfirmed_result::transaction_unconfirmed_result(const memory_ptr slab)
  : slab_(slab), arrival_time_(0), hash_(null_hash)
{
}

transaction_unconfirmed_result::transaction_unconfirmed_result(const memory_ptr slab,
    hash_digest&& hash, uint32_t arrival_time)
  : slab_(slab), arrival_time_(arrival_time), hash_(std::move(hash))
{
}

transaction_unconfirmed_result::transaction_unconfirmed_result(const memory_ptr slab,
    hash_digest const& hash, uint32_t arrival_time)
  : slab_(slab), arrival_time_(arrival_time), hash_(hash)
{
}

transaction_unconfirmed_result::operator bool() const
{
    return slab_ != nullptr;
}

void transaction_unconfirmed_result::reset()
{
    slab_.reset();
}

hash_digest const& transaction_unconfirmed_result::hash() const
{
    return hash_;
}

// Height is unguarded and will be inconsistent during write.
uint32_t transaction_unconfirmed_result::arrival_time() const
{
    KTH_ASSERT(slab_);
    return arrival_time_;
}


// spender_heights are unguarded and will be inconsistent during write.
// If index is out of range returns default/invalid output (.value not_found).
domain::chain::output transaction_unconfirmed_result::output(uint32_t index) const
{
    KTH_ASSERT(slab_);
    auto const tx_start = REMAP_ADDRESS(slab_) + metadata_size;
    auto deserial = make_unsafe_deserializer(tx_start);
    auto const outputs = deserial.read_size_little_endian();
    KTH_ASSERT(deserial);

    if (index >= outputs)
        return{};

    // Skip outputs until the target output.
    for (uint32_t output = 0; output < index; ++output)
    {
        deserial.skip(arrival_time_size + value_size);
        deserial.skip(deserial.read_size_little_endian());
        KTH_ASSERT(deserial);
    }

    // Read and return the target output (including spender height).
    domain::chain::output out;
    out.from_data(deserial, false);
    return out;
}

// median_time_past added in v3.3
// ----------------------------------------------------------------------------
// [ height:4 ]
// [ position:2 ]
// [ median_time_past:4 ]
// ----------------------------------------------------------------------------
// [ output_count:varint ]
// [ [ spender_height:4 ][ value:8 ][ script:varint ]... ]
// [ input_count:varint ]
// [ [ hash:4 ][ index:2 ][ script:varint ][ sequence:4 ]... ]
// [ locktime:varint ]
// [ version:varint ]
// ----------------------------------------------------------------------------

// spender_heights are unguarded and will be inconsistent during write.
chain::transaction transaction_unconfirmed_result::transaction(bool witness) const {
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
    chain::transaction tx;
#if defined(KTH_CACHED_RPC_DATA)    
    tx.from_data(deserial, false, from_data_witness, true);
#else
    tx.from_data(deserial, false, from_data_witness);
#endif

#ifndef KTH_CURRENCY_BCH
    if ( ! witness) {
        tx.strip_witness();
    }
#endif

    //TODO(legacy): add hash param to deserialization to eliminate this construction.
    return chain::transaction(std::move(tx), hash_digest(hash_));
}
} // namespace database
} // namespace kth

#endif // KTH_DB_TRANSACTION_UNCONFIRMED
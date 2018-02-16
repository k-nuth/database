/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/result/transaction_unconfirmed_result.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/databases/transaction_unconfirmed_database.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

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
    const hash_digest& hash, uint32_t arrival_time)
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

const hash_digest& transaction_unconfirmed_result::hash() const
{
    return hash_;
}

// Height is unguarded and will be inconsistent during write.
uint32_t transaction_unconfirmed_result::arrival_time() const
{
    BITCOIN_ASSERT(slab_);
    return arrival_time_;
}


// spender_heights are unguarded and will be inconsistent during write.
// If index is out of range returns default/invalid output (.value not_found).
chain::output transaction_unconfirmed_result::output(uint32_t index) const
{
    BITCOIN_ASSERT(slab_);
    const auto tx_start = REMAP_ADDRESS(slab_) + metadata_size;
    auto deserial = make_unsafe_deserializer(tx_start);
    const auto outputs = deserial.read_size_little_endian();
    BITCOIN_ASSERT(deserial);

    if (index >= outputs)
        return{};

    // Skip outputs until the target output.
    for (uint32_t output = 0; output < index; ++output)
    {
        deserial.skip(arrival_time_size + value_size);
        deserial.skip(deserial.read_size_little_endian());
        BITCOIN_ASSERT(deserial);
    }

    // Read and return the target output (including spender height).
    chain::output out;
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
chain::transaction transaction_unconfirmed_result::transaction() const
{
    BITCOIN_ASSERT(slab_);
    const auto tx_start = REMAP_ADDRESS(slab_) + metadata_size;
    auto deserial = make_unsafe_deserializer(tx_start);

    // READ THE TX
    chain::transaction tx;
    tx.from_data(deserial, false, true);

    // TODO: add hash param to deserialization to eliminate this construction.
    return chain::transaction(std::move(tx), hash_digest(hash_));
}
} // namespace database
} // namespace libbitcoin

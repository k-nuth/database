// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TRANSACTION_RESULT_HPP_
#define KTH_DATABASE_TRANSACTION_RESULT_HPP_

#ifdef KTH_DB_LEGACY

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

#ifdef KTH_CURRENCY_BCH
using position_t = uint32_t;
#else 
using position_t = uint16_t;
#endif

/// Deferred read transaction result.
class BCD_API transaction_result {
public:
    transaction_result();
    transaction_result(const memory_ptr slab);
    transaction_result(const memory_ptr slab, hash_digest&& hash, uint32_t height, uint32_t median_time_past, position_t position);
    transaction_result(const memory_ptr slab, const hash_digest& hash, uint32_t height, uint32_t median_time_past, position_t position);

    /// True if this transaction result is valid (found).
    operator bool() const;

    /// Reset the slab pointer so that no lock is held.
    void reset();

    /// The transaction hash (from cache).
    const hash_digest& hash() const;

    /// The height of the block which includes the transaction.
    size_t height() const;

    /// The ordinal position of the transaction within its block.
    size_t position() const;

    /// The median time past of the block which includes the transaction.
    uint32_t median_time_past() const;

    /// True if all transaction outputs are spent at or below fork_height.
    bool is_spent(size_t fork_height) const;

    /// The output at the specified index within this transaction.
    chain::output output(uint32_t index) const;

    /// The transaction, optionally including witness.
    chain::transaction transaction(bool witness = true) const;

private:
    memory_ptr slab_;
    const uint32_t height_;
    const uint32_t median_time_past_;
    const position_t position_;
    const hash_digest hash_;
};

} // namespace database
} // namespace kth

#endif // KTH_DB_LEGACY

#endif // KTH_DATABASE_TRANSACTION_RESULT_HPP_

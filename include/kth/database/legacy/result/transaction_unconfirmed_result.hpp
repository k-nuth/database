// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TRANSACTION_UNCONFIRMED_RESULT_HPP
#define KTH_DATABASE_TRANSACTION_UNCONFIRMED_RESULT_HPP

#include <cstddef>
#include <cstdint>
#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth {
namespace database {

/// Deferred read transaction result.
class BCD_API transaction_unconfirmed_result
{
public:
    transaction_unconfirmed_result();
    transaction_unconfirmed_result(const memory_ptr slab);
    transaction_unconfirmed_result(const memory_ptr slab, hash_digest&& hash,
        uint32_t arrival_time);
    transaction_unconfirmed_result(const memory_ptr slab, const hash_digest& hash,
        uint32_t arrival_time);

    /// True if this transaction result is valid (found).
    operator bool() const;

    /// Reset the slab pointer so that no lock is held.
    void reset();

    /// The transaction hash (from cache).
    const hash_digest& hash() const;

    /// The time when the transaction arrive
    uint32_t arrival_time() const;

    /// The output at the specified index within this transaction.
    chain::output output(uint32_t index) const;

    /// The transaction.
    chain::transaction transaction(bool witness=true) const;

private:
    memory_ptr slab_;
    const uint32_t arrival_time_;
    const hash_digest hash_;
};

} // namespace database
} // namespace kth

#endif

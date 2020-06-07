// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_BLOCK_RESULT_HPP_
#define KTH_DATABASE_BLOCK_RESULT_HPP_

#ifdef KTH_DB_LEGACY

#include <cstdint>
#include <cstddef>
#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth::database {

/// Deferred read block result.
class KD_API block_result
{
public:
    block_result();
    block_result(const memory_ptr slab);
    block_result(const memory_ptr slab, hash_digest&& hash, uint32_t height);
    block_result(const memory_ptr slab, hash_digest const& hash,
        uint32_t height);

    /// True if this block result is valid (found).
    operator bool() const;

    /// Reset the slab pointer so that no lock is held.
    void reset();

    /// The block header hash (from cache).
    hash_digest const& hash() const;

    /// The block header.
    chain::header header() const;

    /// The height of this block in the chain.
    size_t height() const;

    /// The header.bits of this block.
    uint32_t bits() const;

    /// The header.timestamp of this block.
    uint32_t timestamp() const;

    /// The header.version of this block.
    uint32_t version() const;

    /// The number of transactions in this block.
    size_t transaction_count() const;

    /// A transaction hash where index < transaction_count.
    hash_digest transaction_hash(size_t index) const;

    /// An ordered set of all transaction hashes in the block.
    hash_list transaction_hashes() const;

    uint64_t serialized_size() const;

private:
    memory_ptr slab_;
    const uint32_t height_;
    const hash_digest hash_;
};

} // namespace database
} // namespace kth

#endif // KTH_DB_LEGACY

#endif // KTH_DATABASE_BLOCK_RESULT_HPP_

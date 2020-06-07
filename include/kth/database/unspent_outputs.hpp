// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_UNSPENT_OUTPUTS_HPP_
#define KTH_DATABASE_UNSPENT_OUTPUTS_HPP_

#ifdef KTH_DB_UNSPENT_LEGACY

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>

#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/unspent_transaction.hpp>

#include <kth/infrastructure/utility/noncopyable.hpp>

namespace kth::database {

/// This class is thread safe.
/// A circular-by-age hash table of [point, output].
class KD_API unspent_outputs : noncopyable {
public:
    // Construct a cache with the specified transaction count limit.
    unspent_outputs(size_t capacity);

    /// The cache capacity is zero.
    bool disabled() const;

    /// The cache has no elements.
    size_t empty() const;

    /// The number of elements in the cache.
    size_t size() const;

    /// The cache performance as a ratio of hits to accesses.
    float hit_rate() const;

    /// Add a set of outputs to the cache (purges older entry).
    void add(const chain::transaction& transaction, size_t height,
        uint32_t median_time_past, bool confirmed);

    /// Remove a set of outputs from the cache (has been reorganized out).
    void remove(hash_digest const& tx_hash);

    /// Remove an output from the cache (has been spent).
    void remove(const chain::output_point& point);

    /// Determine if the output is unspent (otherwise fall back to the store).
    bool get(chain::output& out_output, size_t& out_height,
        uint32_t& out_median_time_past, bool& out_coinbase,
        const chain::output_point& point, size_t fork_height,
        bool require_confirmed) const;

    bool get_is_confirmed(chain::output& out_output, size_t& out_height, bool& out_coinbase, bool& out_is_confirmed,
        const chain::output_point& point, size_t fork_height,
        bool require_confirmed) const;


private:
    // A bidirection map is used for efficient output and position retrieval.
    // This produces the effect of a circular buffer tx hash table of outputs.
    typedef boost::bimaps::bimap<
        boost::bimaps::unordered_set_of<unspent_transaction>,
        boost::bimaps::set_of<uint32_t>> unspent_transactions;

    // These are thread safe.
    const size_t capacity_;
    mutable std::atomic<size_t> hits_;
    mutable std::atomic<size_t> queries_;

    // These are protected by mutex.
    uint32_t sequence_;
    unspent_transactions unspent_;
    mutable upgrade_mutex mutex_;
};

} // namespace database
} // namespace kth

#endif // KTH_DB_UNSPENT_LEGACY

#endif // KTH_DATABASE_UNSPENT_OUTPUTS_HPP_

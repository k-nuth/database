// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_UNSPENT_TRANSACTION_HPP_
#define KTH_DATABASE_UNSPENT_TRANSACTION_HPP_

#ifdef KTH_DB_UNSPENT_LEGACY

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <boost/functional/hash_fwd.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// This class is not thread safe.
class BCD_API unspent_transaction
{
public:
    typedef std::unordered_map<uint32_t, chain::output> output_map;
    typedef std::shared_ptr<output_map> output_map_ptr;

    // Move/copy constructors.
    unspent_transaction(unspent_transaction&& other);
    unspent_transaction(const unspent_transaction& other);

    /// Constructors.
    explicit unspent_transaction(const hash_digest& hash);
    explicit unspent_transaction(const chain::output_point& point);
    explicit unspent_transaction(const chain::transaction& tx, size_t height,
        uint32_t median_time_past, bool confirmed);

    /// Properties.
    size_t height() const;
    uint32_t median_time_past() const;
    bool is_coinbase() const;
    bool is_confirmed() const;
    const hash_digest& hash() const;

    /// Access to outputs is mutable and unprotected (not thread safe).
    output_map_ptr outputs() const;

    /// Operators.
    bool operator==(const unspent_transaction& other) const;
    unspent_transaction& operator=(unspent_transaction&& other);
    unspent_transaction& operator=(const unspent_transaction& other);

private:

    // These are thread safe (non-const only for assignment operator).
    size_t height_;
    uint32_t median_time_past_;
    bool is_coinbase_;
    bool is_confirmed_;
    hash_digest hash_;

    // This is not thead safe and is publicly reachable.
    // The outputs can be changed without affecting the bimapping.
    mutable output_map_ptr outputs_;
};

} // namespace database
} // namespace kth

// Standard (boost) hash.
//-----------------------------------------------------------------------------

namespace boost
{

// Extend boost namespace with our unspent output wrapper hash function.
template <>
struct hash<bc::database::unspent_transaction>
{
    size_t operator()(const bc::database::unspent_transaction& unspent) const
    {
        return boost::hash<bc::hash_digest>()(unspent.hash());
    }
};

} // namespace boost

#endif // KTH_DB_UNSPENT_LEGACY

#endif // KTH_DATABASE_UNSPENT_TRANSACTION_HPP_

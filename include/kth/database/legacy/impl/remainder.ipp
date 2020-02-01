// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_REMAINDER_IPP
#define KTH_DATABASE_REMAINDER_IPP

#include <cstdint>
#include <functional>
#include <kth/domain.hpp>

namespace kth {
namespace database {

/// Return a hash of the key reduced to the domain of the divisor.
template <typename KeyType, typename Divisor>
Divisor remainder(const KeyType& key, const Divisor divisor)
{
    return divisor == 0 ? 0 : std::hash<KeyType>()(key) % divisor;
}

} // namespace database
} // namespace kth

#endif
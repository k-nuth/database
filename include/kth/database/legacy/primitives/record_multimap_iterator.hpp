// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_RECORD_MULTIMAP_ITERATOR_HPP
#define KTH_DATABASE_RECORD_MULTIMAP_ITERATOR_HPP

#include <cstdint>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/record_list.hpp>

namespace kth {
namespace database {

/// Forward iterator for multimap record values.
/// After performing key lookup iterate the multiple values in a for loop.
class BCD_API record_multimap_iterator
{
public:
    record_multimap_iterator(const record_manager& manager, array_index index);

    /// Next value in the chain.
    void operator++();

    /// The record index.
    array_index operator*() const;

    /// Comparison operators.
    bool operator==(record_multimap_iterator other) const;
    bool operator!=(record_multimap_iterator other) const;

private:
    array_index index_;
    const record_manager& manager_;
};

} // namespace database
} // namespace kth

#endif

// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin/database/primitives/record_multimap_iterable.hpp>

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/record_list.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/primitives/record_multimap_iterator.hpp>

namespace libbitcoin {
namespace database {
    
record_multimap_iterable::record_multimap_iterable(
    const record_manager& manager, array_index begin)
  : begin_(begin), manager_(manager)
{
}

record_multimap_iterator record_multimap_iterable::begin() const
{
    return record_multimap_iterator(manager_, begin_);
}

record_multimap_iterator record_multimap_iterable::end() const
{
    return record_multimap_iterator(manager_, record_list::empty);
}

} // namespace database
} // namespace kth

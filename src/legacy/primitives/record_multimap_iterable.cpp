// Copyright (c) 2016-2021 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/legacy/primitives/record_multimap_iterable.hpp>

#include <kth/database/define.hpp>
#include <kth/database/legacy/primitives/record_list.hpp>
#include <kth/database/legacy/primitives/record_manager.hpp>
#include <kth/database/legacy/primitives/record_multimap_iterator.hpp>

namespace kth::database {
    
record_multimap_iterable::record_multimap_iterable(
    const record_manager& manager, array_index begin)
  : begin_(begin), manager_(manager)
{
}

record_multimap_iterator record_multimap_iterable::begin() const {
    return record_multimap_iterator(manager_, begin_);
}

record_multimap_iterator record_multimap_iterable::end() const {
    return record_multimap_iterator(manager_, record_list::empty);
}

} // namespace kth::database

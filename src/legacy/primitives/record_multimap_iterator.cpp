// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/legacy/primitives/record_multimap_iterator.hpp>

#include <kth/database/legacy/primitives/record_list.hpp>
#include <kth/database/legacy/primitives/record_manager.hpp>

namespace kth {
namespace database {

record_multimap_iterator::record_multimap_iterator(
    const record_manager& manager, array_index index)
  : index_(index), manager_(manager)
{
}

void record_multimap_iterator::operator++()
{
    // HACK: next_index() is const, so this is safe despite being ugly.
    auto& manager = const_cast<record_manager&>(manager_);

    index_ = record_list(manager, index_).next_index();
}

array_index record_multimap_iterator::operator*() const
{
    return index_;
}

bool record_multimap_iterator::operator==(record_multimap_iterator other) const
{
    return this->index_ == other.index_;
}

bool record_multimap_iterator::operator!=(record_multimap_iterator other) const
{
    return this->index_ != other.index_;
}

} // namespace database
} // namespace kth

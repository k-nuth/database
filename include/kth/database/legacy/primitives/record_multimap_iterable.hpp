// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_RECORD_MULTIMAP_ITERABLE_HPP
#define KTH_DATABASE_RECORD_MULTIMAP_ITERABLE_HPP

#include <kth/database/define.hpp>
#include <kth/database/legacy/primitives/record_manager.hpp>
#include <kth/database/legacy/primitives/record_multimap_iterator.hpp>

namespace kth::database {

/// Result of a multimap database query. This is a container wrapper allowing
/// the values to be iterated.
class KD_API record_multimap_iterable
{
public:
    record_multimap_iterable(const record_manager& manager, array_index begin);

    record_multimap_iterator begin() const;
    record_multimap_iterator end() const;

private:
    array_index begin_;
    const record_manager& manager_;
};

} // namespace kth::database

#endif

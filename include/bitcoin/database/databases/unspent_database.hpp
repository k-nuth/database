/**
 * Copyright (c) 2016 Bitprim developers (see AUTHORS)
 *
 * This file is part of Bitprim.
 *
 * Bitprim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBBITCOIN_DATABASE_UNSPENT_DATABASE_HPP
#define LIBBITCOIN_DATABASE_UNSPENT_DATABASE_HPP

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/record_hash_table_set.hpp>
#include <bitcoin/database/memory/memory_map.hpp>

namespace libbitcoin {
namespace database {

struct BCD_API unspent_statinfo
{
    /// Number of buckets used in the hashtable.
    /// load factor = rows / buckets
    const size_t buckets;

    /// Total number of spend rows.
    const size_t rows;
};

/// This enables you to lookup the spend of an output point, returning
/// the input point. It is a simple map.
class BCD_API unspent_database
{
public:
    /// Construct the database.
    unspent_database(boost::filesystem::path const& filename,
        std::shared_ptr<shared_mutex> mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~unspent_database();

    /// Initialize a new spend database.
    bool create();

    /// Call before using the database.
    bool start();

    /// Call to signal a stop of current operations.
    bool stop();

    /// Call to unload the memory map.
    bool close();

    /// TODO Fer
    bool contains(chain::output_point const& outpoint) const;

    /// Store a spend in the database.
    void store(chain::output_point const& outpoint);

    /// Delete outpoint spend item from database.
    void remove(chain::output_point const& outpoint);

    /// Synchronise storage with disk so things are consistent.
    /// Should be done at the end of every block write.
    void sync();

    /// Return statistical info about the database.
    unspent_statinfo statinfo() const;

private:
    typedef record_hash_table_set<chain::point> record_map;

    // Hash table used for looking up inpoint spends by outpoint.
    memory_map lookup_file_;
    record_hash_table_header lookup_header_;
    record_manager lookup_manager_;
    record_map lookup_map_;
};

} // namespace database
} // namespace libbitcoin

#endif /*LIBBITCOIN_DATABASE_UNSPENT_DATABASE_HPP*/

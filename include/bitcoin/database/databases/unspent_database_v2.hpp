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

#ifndef LIBBITCOIN_DATABASE_UNSPENT_DATABASE_V2_HPP
#define LIBBITCOIN_DATABASE_UNSPENT_DATABASE_V2_HPP

#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
 #include <bitcoin/database/primitives/mem_hash_set.hpp>

namespace boost {

template <>
struct hash<bc::chain::point> {
    // Changes to this function invalidate existing database files.
    size_t operator()(const bc::chain::point& point) const {
        size_t seed = 0;
        boost::hash_combine(seed, point.hash());
        boost::hash_combine(seed, point.index());
        return seed;
    }
};

} // namespace boost

namespace libbitcoin {
namespace database {

struct BCD_API unspent_v2_statinfo
{
    /// Number of buckets used in the hashtable.
    /// load factor = rows / buckets
    const size_t buckets;

    /// Total number of spend rows.
    const size_t rows;
};

/// This enables you to lookup the spend of an output point, returning
/// the input point. It is a simple map.
class BCD_API unspent_database_v2
{
public:
    /// Construct the database.
    unspent_database_v2(boost::filesystem::path const& filename, 
        std::string const& mapname,
        std::shared_ptr<shared_mutex> mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    // ~unspent_database_v2();

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
    unspent_v2_statinfo statinfo() const;

private:
    using record_map = mem_hash_set<chain::point>;
    using record_map_ptr = std::unique_ptr<record_map>;

    std::string filename_;
    std::string mapname_;

    record_map_ptr lookup_map_;
    mutable shared_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif /*LIBBITCOIN_DATABASE_UNSPENT_DATABASE_V2_HPP*/

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
#include <bitcoin/database/databases/unspent_database.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;
using namespace bc::chain;

BC_CONSTEXPR size_t number_buckets = 45000000; //228110589;
BC_CONSTEXPR size_t header_size = record_hash_table_header_size(number_buckets);
BC_CONSTEXPR size_t initial_map_file_size = header_size + minimum_records_size;

// BC_CONSTEXPR size_t value_size = std::tuple_size<chain::point>::value;
// BC_CONSTEXPR size_t record_size = hash_table_record_size<chain::point>(value_size);
BC_CONSTEXPR size_t record_size = hash_table_set_record_size<chain::point>();

unspent_database::unspent_database(path const& filename,
    std::shared_ptr<shared_mutex> mutex)
  : lookup_file_(filename, mutex), 
    lookup_header_(lookup_file_, number_buckets),
    lookup_manager_(lookup_file_, header_size, record_size),
    lookup_map_(lookup_header_, lookup_manager_)
{
    //std::cout << "unspent_database::unspent_database(...)\n";
}

// Close does not call stop because there is no way to detect thread join.
unspent_database::~unspent_database()
{
    //std::cout << "unspent_database::~unspent_database()\n";
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool unspent_database::create()
{
    //std::cout << "bool unspent_database::create()\n";
    // Resize and create require a started file.
    if (!lookup_file_.start())
        return false;

    // This will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size);

    if (!lookup_header_.create() ||
        !lookup_manager_.create())
        return false;

    // Should not call start after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool unspent_database::start()
{
    //std::cout << "bool unspent_database::start()\n";
    return
        lookup_file_.start() &&
        lookup_header_.start() &&
        lookup_manager_.start();
}

bool unspent_database::stop()
{
    //std::cout << "bool unspent_database::stop()\n";
    return lookup_file_.stop();
}

bool unspent_database::close()
{
    //std::cout << "bool unspent_database::close()\n";
    return lookup_file_.close();
}

// ----------------------------------------------------------------------------

bool unspent_database::contains(output_point const& outpoint) const
{
    //std::cout << "bool unspent_database::contains(output_point const& outpoint) const\n";
    return lookup_map_.contains(outpoint);
}

void unspent_database::store(chain::output_point const& outpoint)
{

    if (encode_hash(outpoint.hash) == "b6af26bf360c141937de55a88ef8033852c372d188d38863a3e0ec1c71bd63eb" && 
        outpoint.index == 1) {

        std::cout << "unspent_database::store() "
                  << " - outpoint.hash:   " << encode_hash(outpoint.hash) << '\n'
                  << " - outpoint.index:  " << outpoint.index << '\n';
    }



    // std::cout << "void unspent_database::store(chain::output_point const& outpoint)\n";
    std::cout << "unspent_database::store  lookup_manager_.count(): " << lookup_manager_.count() << "\n";
    lookup_map_.store(outpoint);
    std::cout << "unspent_database::store  lookup_manager_.count(): " << lookup_manager_.count() << "\n";
}

void unspent_database::remove(output_point const& outpoint)
{

    if (encode_hash(outpoint.hash) == "b6af26bf360c141937de55a88ef8033852c372d188d38863a3e0ec1c71bd63eb" && 
        outpoint.index == 1) {

        std::cout << "unspent_database::remove() "
                  << " - outpoint.hash:   " << encode_hash(outpoint.hash) << '\n'
                  << " - outpoint.index:  " << outpoint.index << '\n';
    }

    // std::cout << "void unspent_database::remove(output_point const& outpoint)\n";
    // auto contains = lookup_map_.contains(outpoint);
    // std::cout << "contains: " << contains << "\n";

    std::cout << "unspent_database::remove lookup_manager_.count(): " << lookup_manager_.count() << "\n";
    bool success = lookup_map_.unlink(outpoint);
    std::cout << "success:                 " << success << "\n";
    // std::cout << "lookup_header_.size():   " << lookup_header_.size() << "\n";
    std::cout << "unspent_database::remove lookup_manager_.count(): " << lookup_manager_.count() << "\n";

    // DEBUG_ONLY(bool success =) lookup_map_.unlink(outpoint);
    // BITCOIN_ASSERT(success);
}

void unspent_database::sync()
{
    //std::cout << "void unspent_database::sync()\n";
    lookup_manager_.sync();
}

unspent_statinfo unspent_database::statinfo() const
{
    return
    {
        lookup_header_.size(),
        lookup_manager_.count()
    };
}

} // namespace database
} // namespace libbitcoin

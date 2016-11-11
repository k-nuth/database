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

#include <bitcoin/database/databases/unspent_database_v2.hpp>

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

// BC_CONSTEXPR size_t number_buckets = 45000000; //228110589;
// BC_CONSTEXPR size_t header_size = record_hash_table_header_size(number_buckets);
// BC_CONSTEXPR size_t initial_map_file_size = header_size + minimum_records_size;
// BC_CONSTEXPR size_t record_size = hash_table_set_record_size<chain::point>();

BC_CONSTEXPR size_t number_buckets = 45000000; //3;
BC_CONSTEXPR size_t filesize = number_buckets * 12; //65536;

unspent_database_v2::unspent_database_v2(path const& filename,
    std::string const& mapname, std::shared_ptr<shared_mutex> mutex)
    : filename_(filename.string())
    , mapname_(mapname)
    // : lookup_map_(new record_map(filename.string(), mapname, number_buckets, filesize))
{
    // std::cout << "unspent_database_v2::unspent_database_v2(...)" << std::endl;
    // std::cout << "filename.string(): " << filename.string() << std::endl;
    // lookup_map_.reset(new record_map(filename.string(), mapname, number_buckets, filesize));
    // std::cout << "unspent_database_v2::unspent_database_v2(...) -- END" << std::endl;
}

// // Close does not call stop because there is no way to detect thread join.
// unspent_database_v2::~unspent_database_v2() {
//     //std::cout << "unspent_database_v2::~unspent_database_v2()\n";
//     close();
// }

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool unspent_database_v2::create() {
    // std::cout << "unspent_database_v2::create()" << "\n";
    // std::cout << "filename_: " << filename_ << "\n";
    // std::cout << "mapname_: " << mapname_ << "\n";

    lookup_map_.reset(new record_map(filename_, mapname_, number_buckets, filesize));
    return true;

    // //std::cout << "bool unspent_database_v2::create()\n";
    // // Resize and create require a started file.
    // if (!lookup_file_.start())
    //     return false;

    // // This will throw if insufficient disk space.
    // lookup_file_.resize(initial_map_file_size);

    // if (!lookup_header_.create() ||
    //     !lookup_manager_.create())
    //     return false;

    // // Should not call start after create, already started.
    // return
    //     lookup_header_.start() &&
    //     lookup_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool unspent_database_v2::start() {
    std::cout << "unspent_database_v2::start()" << "\n";
    std::cout << "filename_: " << filename_ << "\n";
    std::cout << "mapname_: " << mapname_ << "\n";
    
    lookup_map_.reset(new record_map(filename_, mapname_, number_buckets, filesize));

    return true;
    // //std::cout << "bool unspent_database_v2::start()\n";
    // return
    //     lookup_file_.start() &&
    //     lookup_header_.start() &&
    //     lookup_manager_.start();
}

bool unspent_database_v2::stop() {
    return true;
    // //std::cout << "bool unspent_database_v2::stop()\n";
    // return lookup_file_.stop();
}

bool unspent_database_v2::close() {
    return true;
    // //std::cout << "bool unspent_database_v2::close()\n";
    // return lookup_file_.close();
}

// ----------------------------------------------------------------------------

bool unspent_database_v2::contains(output_point const& outpoint) const {
    boost::shared_lock<shared_mutex> lock(mutex_);
    //std::cout << "bool unspent_database_v2::contains(output_point const& outpoint) const\n";
    return lookup_map_->count(outpoint) > 0;
}

void unspent_database_v2::store(chain::output_point const& outpoint) {
    boost::unique_lock<shared_mutex> lock(mutex_);
    // std::cout << "void unspent_database_v2::store(chain::output_point const& outpoint)\n";
    // std::cout << "unspent_database_v2::store  lookup_map_->size(): " << lookup_map_->size() << "\n";
    lookup_map_->insert(outpoint);
    // std::cout << "unspent_database_v2::store  lookup_map_->size(): " << lookup_map_->size() << "\n";
}

void unspent_database_v2::remove(output_point const& outpoint) {
    boost::unique_lock<shared_mutex> lock(mutex_);

    // std::cout << "unspent_database_v2::remove lookup_map_->size(): " << lookup_map_->size() << "\n";
    bool success = lookup_map_->erase(outpoint) > 0;
    // std::cout << "success:                 " << success << "\n";
    // std::cout << "lookup_header_.size():   " << lookup_header_.size() << "\n";
    // std::cout << "unspent_database_v2::remove lookup_map_->size(): " << lookup_map_->size() << "\n";

    // DEBUG_ONLY(bool success =) lookup_map_->unlink(outpoint);
    // BITCOIN_ASSERT(success);
}

void unspent_database_v2::sync() {
    boost::unique_lock<shared_mutex> lock(mutex_);
    //std::cout << "void unspent_database_v2::sync()\n";
    lookup_map_->flush();
}

//TODO Fer
unspent_v2_statinfo unspent_database_v2::statinfo() const {
    return {0, 0};

    // return
    // {
    //     lookup_header_.size(),
    //     lookup_map_->size()
    // };
}

} // namespace database
} // namespace libbitcoin

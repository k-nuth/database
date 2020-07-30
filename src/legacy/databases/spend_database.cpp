// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef KTH_DB_SPENDS

#include <kth/database/legacy/databases/spend_database.hpp>


#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth::database {

using namespace kth::domain::chain;

// The spend database keys off of output point and has input point value.
static constexpr auto value_size = std::tuple_size<point>::value;
static constexpr auto record_size = hash_table_record_size<point>(value_size);

// Spends use a hash table index, O(1).
spend_database::spend_database(const path& filename, size_t buckets,
    size_t expansion, mutex_ptr mutex)
  : initial_map_file_size_(record_hash_table_header_size(buckets) +
        minimum_records_size),

    lookup_file_(filename, mutex, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_, record_hash_table_header_size(buckets),
        record_size),
    lookup_map_(lookup_header_, lookup_manager_)
{
}

spend_database::~spend_database()
{
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool spend_database::create()
{
    // Resize and create require an opened file.
    if ( ! lookup_file_.open())
        return false;

    // This will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size_);

    if ( ! lookup_header_.create() ||
        !lookup_manager_.create())
        return false;

    // Should not call start after create, already started.
    return
        lookup_header_.start() &&
        lookup_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool spend_database::open()
{
    return
        lookup_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start();
}

bool spend_database::close()
{
    return lookup_file_.close();
}

// Commit latest inserts.
void spend_database::synchronize()
{
    lookup_manager_.sync();
}

// Flush the memory map to disk.
bool spend_database::flush() const {
    return lookup_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------

input_point spend_database::get(const output_point& outpoint) const {
    input_point spend;
    auto const slab = lookup_map_.find(outpoint);

    if ( ! slab)
        return spend;

    // The order of properties in this serialization was changed in v3.
    // Previously it was { index, hash }, which was inconsistent with wire.
    auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(slab));
    spend.from_data(deserial, false);
    return spend;
}

void spend_database::store(const domain::chain::output_point& outpoint,
    const domain::chain::input_point& spend)
{
    //OLD before merge (Feb2017)
    //std::cout << "void spend_database::store(const domain::chain::output_point& outpoint, const domain::chain::input_point& spend)\n";
    // auto const write = [&spend](memory_ptr data)
    auto const write = [&](serializer<uint8_t*>& serial) {
        spend.to_data(serial, false);
    };

    lookup_map_.store(outpoint, write);
}

bool spend_database::unlink(const output_point& outpoint)
{
    // Spends are optional so do not assume present (otherwise will segfault).
    auto memory = lookup_map_.find(outpoint);
    if (memory == nullptr)
        return false;

    // Release lock.
    memory = nullptr;
    return lookup_map_.unlink(outpoint);
}

spend_statinfo spend_database::statinfo() const {
    return
    {
        lookup_header_.size(),
        lookup_manager_.count()
    };
}

} // namespace kth::database

#endif // KTH_DB_SPENDS

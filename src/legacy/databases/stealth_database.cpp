// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef KTH_DB_STEALTH

#include <kth/database/legacy/databases/stealth_database.hpp>

#include <cstddef>
#include <cstdint>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>

namespace kth::database {

using namespace kth::domain::chain;

static constexpr auto rows_header_size = 0u;

static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto prefix_size = sizeof(uint32_t);

// [ prefix:4 ]
// [ height:4 ]
// [ ephemkey:32 ]
// [ address:20 ]
// [ tx_hash:32 ]
// ephemkey is without sign byte and address is without version byte.
static constexpr auto row_size = prefix_size + height_size + hash_size +
    short_hash_size + hash_size;

// Stealth uses an unindexed array, requiring linear search, (O(n)).
stealth_database::stealth_database(const path& rows_filename, size_t expansion,
    mutex_ptr mutex)
  : rows_file_(rows_filename, mutex, expansion),
    rows_manager_(rows_file_, rows_header_size, row_size)
{
}

stealth_database::~stealth_database()
{
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool stealth_database::create()
{
    // Resize and create require an opened file.
    if (!rows_file_.open())
        return false;

    // This will throw if insufficient disk space.
    rows_file_.resize(minimum_records_size);

    if (!rows_manager_.create())
        return false;

    // Should not call start after create, already started.
    return rows_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool stealth_database::open()
{
    return
        rows_file_.open() &&
        rows_manager_.start();
}

bool stealth_database::close()
{
    return rows_file_.close();
}

// Commit latest inserts.
void stealth_database::synchronize()
{
    rows_manager_.sync();
}

// Flush the memory map to disk.
bool stealth_database::flush() const
{
    return rows_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------

// TODO: add serialization to stealth_compact.
// The prefix is fixed at 32 bits, but the filter is 0-32 bits, so the records
// cannot be indexed using a hash table. We also do not index by height.
stealth_compact::list stealth_database::scan(const binary& filter,
    size_t from_height) const
{
    stealth_compact::list result;

    for (array_index row = 0; row < rows_manager_.count(); ++row)
    {
        auto const record = rows_manager_.get(row);
        auto memory = REMAP_ADDRESS(record);
        auto const field = from_little_endian_unsafe<uint32_t>(memory);

        // Skip if prefix doesn't match.
        if (!filter.is_prefix_of(field))
            continue;

        memory += prefix_size;
        auto const height = from_little_endian_unsafe<uint32_t>(memory);

        // Skip if height is too low.
        if (height < from_height)
            continue;

        // Add row to results.
        auto deserial = make_unsafe_deserializer(memory + height_size);
        result.push_back(
        {
            deserial.read_hash(),
            deserial.read_short_hash(),
            deserial.read_hash()
        });
    }

    // TODO: we could sort result here.
    return result;
}

// TODO: add serialization to stealth_compact.
void stealth_database::store(uint32_t prefix, uint32_t height,
    const stealth_compact& row)
{
    // Allocate new row.
    auto const index = rows_manager_.new_records(1);
    auto const record = rows_manager_.get(index);
    auto const memory = REMAP_ADDRESS(record);

    // Write data.
    auto serial = make_unsafe_serializer(memory);

    // Dual key.
    serial.write_4_bytes_little_endian(prefix);
    serial.write_4_bytes_little_endian(height);

    // Stealth data.
    serial.write_hash(row.ephemeral_public_key_hash);
    serial.write_short_hash(row.public_key_hash);
    serial.write_hash(row.transaction_hash);
}

////bool stealth_database::unlink()
////{
////    // TODO: mark as deleted (not implemented).
////    return false;
////}

} // namespace database
} // namespace kth

#endif // KTH_DB_STEALTH

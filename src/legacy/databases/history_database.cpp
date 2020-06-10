// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/legacy/databases/history_database.hpp>

#ifdef KTH_DB_HISTORY

#include <cstdint>
#include <cstddef>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include <kth/database/legacy/primitives/record_hash_table.hpp>
#include <kth/database/legacy/primitives/record_multimap.hpp>
#include <kth/database/legacy/primitives/record_multimap_iterable.hpp>

namespace kth::database {

using namespace kth::domain::chain;

static constexpr auto rows_header_size = 0u;

static constexpr auto flag_size = sizeof(uint8_t);
static constexpr auto point_size = std::tuple_size<point>::value;
static constexpr auto height_position = flag_size + point_size;
static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto checksum_size = sizeof(uint64_t);
static constexpr auto value_size = flag_size + point_size + height_size +
    checksum_size;

static constexpr auto table_record_size = hash_table_multimap_record_size<short_hash>();
static auto const row_record_size = multimap_record_size(value_size);

// History uses a hash table index, O(1).
history_database::history_database(const path& lookup_filename, const path& rows_filename, size_t buckets, size_t expansion, mutex_ptr mutex)
    : initial_map_file_size_(record_hash_table_header_size(buckets) + minimum_records_size),
    lookup_file_(lookup_filename, mutex, expansion),
    lookup_header_(lookup_file_, buckets),
    lookup_manager_(lookup_file_, record_hash_table_header_size(buckets), table_record_size),
    lookup_map_(lookup_header_, lookup_manager_),
    rows_file_(rows_filename, mutex, expansion),
    rows_manager_(rows_file_, rows_header_size, row_record_size),
    rows_multimap_(lookup_map_, rows_manager_)
{}

history_database::~history_database() {
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool history_database::create() {
    // Resize and create require an opened file.
    if ( ! lookup_file_.open() || ! rows_file_.open()) {
        return false;
    }

    // These will throw if insufficient disk space.
    lookup_file_.resize(initial_map_file_size_);
    rows_file_.resize(minimum_records_size);

    if ( ! lookup_header_.create() || ! lookup_manager_.create() || ! rows_manager_.create()) {
        return false;
    }

    // Should not call start after create, already started.
    return lookup_header_.start() && lookup_manager_.start() && rows_manager_.start();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool history_database::open() {
    return
        lookup_file_.open() &&
        rows_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start() &&
        rows_manager_.start();
}

bool history_database::close() {
    return
        lookup_file_.close() &&
        rows_file_.close();
}

// Commit latest inserts.
void history_database::synchronize()
{
    lookup_manager_.sync();
    rows_manager_.sync();
}

// Flush the memory maps to disk.
bool history_database::flush() const {
    return
        lookup_file_.flush() &&
        rows_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------

void history_database::add_output(const short_hash& key, const output_point& outpoint, size_t output_height, uint64_t value) {
    auto const output_height32 = safe_unsigned<uint32_t>(output_height);

    auto const write = [&](serializer<uint8_t*>& serial) {
        serial.write_byte(static_cast<uint8_t>(point_kind::output));
        outpoint.to_data(serial, false);
        serial.write_4_bytes_little_endian(output_height32);
        serial.write_8_bytes_little_endian(value);
    };

    rows_multimap_.store(key, write);
}

void history_database::add_input(const short_hash& key, const output_point& inpoint, size_t input_height, const input_point& previous) {
    auto const input_height32 = safe_unsigned<uint32_t>(input_height);

    auto const write = [&](serializer<uint8_t*>& serial) {
        serial.write_byte(static_cast<uint8_t>(point_kind::spend));
        inpoint.to_data(serial, false);
        serial.write_4_bytes_little_endian(input_height32);
        serial.write_8_bytes_little_endian(previous.checksum());
    };

    rows_multimap_.store(key, write);
}

// This is the history unlink.
bool history_database::delete_last_row(const short_hash& key) {
    return rows_multimap_.unlink(key);
}

history_compact::list history_database::get(const short_hash& key, size_t limit, size_t from_height) const {
    // Read the height value from the row.
    auto const read_height = [](uint8_t* data) {
        return from_little_endian_unsafe<uint32_t>(data + height_position);
    };

    // TODO: add serialization to history_compact.
    // Read a row from the data for the history list.
    auto const read_row = [](uint8_t* data) {
        auto deserial = make_unsafe_deserializer(data);
        return history_compact {
            // output or spend?
            static_cast<point_kind>(deserial.read_byte()),

            // point
            domain::create<point>(deserial, false),

            // height
            deserial.read_4_bytes_little_endian(),

            // value or checksum
            { deserial.read_8_bytes_little_endian() }
        };
    };

    history_compact::list result;
    auto const start = rows_multimap_.find(key);
    auto const records = record_multimap_iterable(rows_manager_, start);

    for (auto const index : records) {
        // Stop once we reach the limit (if specified).
        if (limit > 0 && result.size() >= limit)
            break;

        // This obtains a remap safe address pointer against the rows file.
        auto const record = rows_multimap_.get(index);
        auto const address = REMAP_ADDRESS(record);

        // Skip rows below from_height.
        if (from_height == 0 || read_height(address) >= from_height)
            result.push_back(read_row(address));
    }

    // TODO: we could sort result here.
    return result;
}

std::vector<hash_digest> history_database::get_txns(const short_hash& key, size_t limit,
                                              size_t from_height) const
{

    // Read the height value from the row.
    auto const read_hash = [](uint8_t* data)
    {
        auto deserial = make_unsafe_deserializer(data+flag_size);
        return deserial.read_hash();
    };

    std::set<hash_digest> temp;
    std::vector<hash_digest> result;
    auto const start = rows_multimap_.find(key);
    auto const records = record_multimap_iterable(rows_manager_, start);

    for (auto const index: records)
    {
        // Stop once we reach the limit (if specified).
        if (limit > 0 && temp.size() >= limit)
            break;
        // This obtains a remap safe address pointer against the rows file.
        auto const record = rows_multimap_.get(index);
        auto const address = REMAP_ADDRESS(record);
        // Avoid inserting the same tx
        auto const & pair = temp.insert(read_hash(address));
        if (pair.second){
            // Add valid txns to the result vector
            result.push_back(*pair.first);
        }
    }
    return result;
}

history_statinfo history_database::statinfo() const
{
    return
    {
        lookup_header_.size(),
        lookup_manager_.count(),
        rows_manager_.count()
    };
}

} // namespace database
} // namespace kth

#endif // KTH_DB_HISTORY
// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifdef KTH_DB_TRANSACTION_UNCONFIRMED

#include <kth/database/legacy/databases/transaction_unconfirmed_database.hpp>

#include <cstddef>
#include <cstdint>
#include <kth/domain.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include <kth/database/legacy/result/transaction_result.hpp>

namespace kth::database {

using namespace kth::domain::chain;
using namespace kth::machine;

static constexpr auto value_size = sizeof(uint64_t);
static constexpr auto arrival_time_size = sizeof(uint32_t);
static constexpr auto metadata_size = arrival_time_size;

#if defined(KTH_CURRENCY_BCH)
size_t const transaction_unconfirmed_database::unconfirmed = max_uint32;
#else 
size_t const transaction_unconfirmed_database::unconfirmed = max_uint16;
#endif    

// Transactions uses a hash table index, O(1).
transaction_unconfirmed_database::transaction_unconfirmed_database(const path& map_filename, size_t buckets, size_t expansion, mutex_ptr mutex)
    : initial_map_file_size_(slab_hash_table_header_size(buckets) + minimum_slabs_size)
    , lookup_file_(map_filename, mutex, expansion)
    , lookup_header_(lookup_file_, buckets)
    , lookup_manager_(lookup_file_, slab_hash_table_header_size(buckets))
    , lookup_map_(lookup_header_, lookup_manager_)
{}

transaction_unconfirmed_database::~transaction_unconfirmed_database() {
    close();
}

// Create.
// ----------------------------------------------------------------------------

// Initialize files and start.
bool transaction_unconfirmed_database::create() {
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

// Start files and primitives.
bool transaction_unconfirmed_database::open() {
    return
        lookup_file_.open() &&
        lookup_header_.start() &&
        lookup_manager_.start();
}

// Close files.
bool transaction_unconfirmed_database::close() {
    return lookup_file_.close();
}

// Commit latest inserts.
void transaction_unconfirmed_database::synchronize() {
    lookup_manager_.sync();
}

// Flush the memory map to disk.
bool transaction_unconfirmed_database::flush() const {
    return lookup_file_.flush();
}

// Queries.
// ----------------------------------------------------------------------------

memory_ptr transaction_unconfirmed_database::find(hash_digest const& hash) const {
    //*************************************************************************
    // CONSENSUS: This simplified implementation does not allow the possibility
    // of a matching tx hash above the fork height or the existence of both
    // unconfirmed and confirmed transactions with the same hash. This is an
    // assumption of the impossibility of hash collisions, which is incorrect
    // but consistent with the current satoshi implementation. This method
    // encapsulates that assumption which can therefore be fixed in one place.
    //*************************************************************************
    auto slab = lookup_map_.find(hash);
    return slab;

}

transaction_unconfirmed_result transaction_unconfirmed_database::get(hash_digest const& hash) const {
    // Limit search to confirmed transactions at or below the fork height.
    // Caller should set fork height to max_size_t for unconfirmed search.
    auto const slab = find(hash);
    if (slab) {
        ///////////////////////////////////////////////////////////////////////
        metadata_mutex_.lock_shared();
        auto deserial = make_unsafe_deserializer(REMAP_ADDRESS(slab));
        auto const arrival_time = deserial.read_4_bytes_little_endian();
        metadata_mutex_.unlock_shared();
        ///////////////////////////////////////////////////////////////////////

        return transaction_unconfirmed_result(slab, hash, arrival_time);
    }

    return{};
}

uint32_t get_clock_now() {
    auto const now = std::chrono::high_resolution_clock::now();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

void transaction_unconfirmed_database::store(const domain::chain::transaction& tx) {
    uint32_t arrival_time = get_clock_now();
#if defined(KTH_CURRENCY_BCH)
        bool witness = false;
#else
        bool witness = true;
#endif
    auto const hash = tx.hash();

    // Unconfirmed txs: position is unconfirmed and height is validation forks.
    auto const write = [&](serializer<uint8_t*>& serial) {
        ///////////////////////////////////////////////////////////////////////
        // Critical Section
        metadata_mutex_.lock();
        serial.write_4_bytes_little_endian(static_cast<uint32_t>(arrival_time));
        metadata_mutex_.unlock();
        ///////////////////////////////////////////////////////////////////////

        // WRITE THE TX
        //TODO WITNESS
#if defined(KTH_CACHED_RPC_DATA)    
        tx.to_data(serial, false, witness, true);
#else
        tx.to_data(serial, false, witness);
#endif
    };


#if defined(KTH_CACHED_RPC_DATA)    
    auto const tx_size = tx.serialized_size(false, witness, true);
#else
    auto const tx_size = tx.serialized_size(false, witness);
#endif

    KTH_ASSERT(tx_size <= max_size_t - metadata_size);
    auto const total_size = metadata_size + static_cast<size_t>(tx_size);

    // Create slab for the new tx instance.
    lookup_map_.store(hash, write, total_size);
}

// bool transaction_unconfirmed_database::spend(const output_point& point, size_t spender_height)
// bool transaction_unconfirmed_database::unspend(const output_point& point)
// bool transaction_unconfirmed_database::confirm(hash_digest const& hash, size_t height, size_t position)
// bool transaction_unconfirmed_database::unconfirm(hash_digest const& hash)

bool transaction_unconfirmed_database::unlink(hash_digest const& hash) {
    return lookup_map_.unlink(hash);
}

bool transaction_unconfirmed_database::unlink_if_exists(hash_digest const& hash) {
    auto memory = lookup_map_.find(hash);
    if (memory == nullptr)
        return false;

    // Release lock.
    memory = nullptr;
    return unlink(hash);
}

}} // namespace kth::database

#endif // KTH_DB_TRANSACTION_UNCONFIRMED
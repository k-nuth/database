// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/store.hpp>

#include <cstddef>
#include <memory>
#include <kth/domain.hpp>

namespace kth::database {

using namespace kth::domain::chain;
using namespace kth::database;


#ifdef KTH_DB_LEGACY
// Database file names.
#define FLUSH_LOCK "flush_lock"
#define EXCLUSIVE_LOCK "exclusive_lock"
#define BLOCK_TABLE "block_table"
#define BLOCK_INDEX "block_index"
#define TRANSACTION_TABLE "transaction_table"
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
#define INTERNAL_DB_DIR "internal_db"
#endif // KTH_DB_NEW

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
#define TRANSACTION_UNCONFIRMED_TABLE "transaction_unconfirmed_table"
#endif // KTH_DB_TRANSACTION_UNCONFIRMED    

#ifdef KTH_DB_SPENDS
#define SPEND_TABLE "spend_table"
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
#define HISTORY_TABLE "history_table"
#define HISTORY_ROWS "history_rows"
#endif // KTH_DB_HISTORY    

#ifdef KTH_DB_STEALTH
#define STEALTH_ROWS "stealth_rows"
#endif // KTH_DB_STEALTH        



// The threashold max_uint32 is used to align with fixed-width config settings,
// and size_t is used to align with the database height domain.
const size_t store::without_indexes = max_uint32;

bool store::use_indexes() const {
#ifdef KTH_DB_WITH_INDEXES
    return use_indexes_;
#else
    return false;
#endif   
}


#ifdef KTH_DB_LEGACY
// static
bool store::create(const path& file_path) {
    kth::ofstream file(file_path.string());

    if (file.bad()) {
        return false;
    }

    // Write one byte so file is nonzero size (for memory map validation).
    file.put('x');
    return true;
}
#endif // KTH_DB_LEGACY

// Construct.
// ------------------------------------------------------------------------

store::store(const path& prefix, bool with_indexes, bool flush_each_write)
#ifdef KTH_DB_LEGACY
    : flush_each_write_(flush_each_write)
    , flush_lock_(prefix / FLUSH_LOCK)
    , exclusive_lock_(prefix / EXCLUSIVE_LOCK)
    // Content store.
    , block_table(prefix / BLOCK_TABLE)
    , block_index(prefix / BLOCK_INDEX)
    , transaction_table(prefix / TRANSACTION_TABLE)
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
    : internal_db_dir(prefix / INTERNAL_DB_DIR)
#endif // KTH_DB_NEW

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    , transaction_unconfirmed_table(prefix / TRANSACTION_UNCONFIRMED_TABLE)
#endif // KTH_DB_TRANSACTION_UNCONFIRMED    

    // Optional indexes.
#ifdef KTH_DB_SPENDS
    , spend_table(prefix / SPEND_TABLE)
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
    , history_table(prefix / HISTORY_TABLE)
    , history_rows(prefix / HISTORY_ROWS)
#endif // KTH_DB_HISTORY    

#ifdef KTH_DB_STEALTH
    , stealth_rows(prefix / STEALTH_ROWS)
#endif // KTH_DB_STEALTH

#ifdef KTH_DB_WITH_INDEXES
    , use_indexes_(with_indexes)
#endif
{}


#ifdef KTH_DB_LEGACY
// Open and close.
// ------------------------------------------------------------------------

// Create files.
bool store::create() {
    auto const created = create(block_table) 
                        && create(block_index) 
                        && create(transaction_table) 

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
                        && create(transaction_unconfirmed_table)
#endif // KTH_DB_TRANSACTION_UNCONFIRMED    
                        ;

    if ( ! use_indexes()) {
        return created;
    }

    return
        created 
#ifdef KTH_DB_SPENDS
            && create(spend_table) 
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
            && create(history_table)
            && create(history_rows) 
#endif // KTH_DB_HISTORY    

#ifdef KTH_DB_STEALTH
            && create(stealth_rows)
#endif // KTH_DB_STEALTH        
            ;
}

bool store::open() {
    return exclusive_lock_.lock() 
        && flush_lock_.try_lock() 
        && (flush_each_write_ || flush_lock_.lock_shared());
}

bool store::close() {
    return (flush_each_write_ || flush_lock_.unlock_shared())
           && exclusive_lock_.unlock();
}

store::handle store::begin_read() const {
    return sequential_lock_.begin_read();
}

bool store::is_read_valid(handle value) const {
    return sequential_lock_.is_read_valid(value);
}

bool store::is_write_locked(handle value) const {
    return sequential_lock_.is_write_locked(value);
}

bool store::begin_write() const {
    return flush_lock() && sequential_lock_.begin_write();
}

bool store::end_write() const {
    return sequential_lock_.end_write() && flush_unlock();
}

bool store::flush_lock() const {
    return ! flush_each_write_ || flush_lock_.lock_shared();
}

bool store::flush_unlock() const {
    return  ! flush_each_write_ || (flush() && flush_lock_.unlock_shared());
}
#endif // KTH_DB_LEGACY

} // namespace data_base
} // namespace kth

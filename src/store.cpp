/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/store.hpp>

#include <cstddef>
#include <memory>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace bc::database;


#ifdef BITPRIM_DB_LEGACY
// Database file names.
#define FLUSH_LOCK "flush_lock"
#define EXCLUSIVE_LOCK "exclusive_lock"
#define BLOCK_TABLE "block_table"
#define BLOCK_INDEX "block_index"
#define TRANSACTION_TABLE "transaction_table"
#endif // BITPRIM_DB_LEGACY

#ifdef BITPRIM_DB_NEW
#define UTXO_DIR "utxo_db"
#endif // BITPRIM_DB_NEW

#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
#define TRANSACTION_UNCONFIRMED_TABLE "transaction_unconfirmed_table"
#endif // BITPRIM_DB_TRANSACTION_UNCONFIRMED    

#ifdef BITPRIM_DB_SPENDS
#define SPEND_TABLE "spend_table"
#endif // BITPRIM_DB_SPENDS

#ifdef BITPRIM_DB_HISTORY
#define HISTORY_TABLE "history_table"
#define HISTORY_ROWS "history_rows"
#endif // BITPRIM_DB_HISTORY    

#ifdef BITPRIM_DB_STEALTH
#define STEALTH_ROWS "stealth_rows"
#endif // BITPRIM_DB_STEALTH        



// The threashold max_uint32 is used to align with fixed-width config settings,
// and size_t is used to align with the database height domain.
const size_t store::without_indexes = max_uint32;

// static
bool store::create(const path& file_path)
{
    bc::ofstream file(file_path.string());

    if (file.bad())
        return false;

    // Write one byte so file is nonzero size (for memory map validation).
    file.put('x');
    return true;
}

// Construct.
// ------------------------------------------------------------------------

store::store(const path& prefix, bool with_indexes, bool flush_each_write)
    : use_indexes(with_indexes)
    , flush_each_write_(flush_each_write)
#ifdef BITPRIM_DB_LEGACY
    , flush_lock_(prefix / FLUSH_LOCK)
    , exclusive_lock_(prefix / EXCLUSIVE_LOCK)
    // Content store.
    , block_table(prefix / BLOCK_TABLE)
    , block_index(prefix / BLOCK_INDEX)
    , transaction_table(prefix / TRANSACTION_TABLE)
#endif // BITPRIM_DB_LEGACY

#ifdef BITPRIM_DB_NEW
    , utxo_dir(prefix / UTXO_DIR)
#endif // BITPRIM_DB_NEW

#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
    , transaction_unconfirmed_table(prefix / TRANSACTION_UNCONFIRMED_TABLE)
#endif // BITPRIM_DB_TRANSACTION_UNCONFIRMED    

    // Optional indexes.

#ifdef BITPRIM_DB_SPENDS
    , spend_table(prefix / SPEND_TABLE)
#endif // BITPRIM_DB_SPENDS

#ifdef BITPRIM_DB_HISTORY
    , history_table(prefix / HISTORY_TABLE)
    , history_rows(prefix / HISTORY_ROWS)
#endif // BITPRIM_DB_HISTORY    

#ifdef BITPRIM_DB_STEALTH
    , stealth_rows(prefix / STEALTH_ROWS)
#endif // BITPRIM_DB_STEALTH        
{}

// Open and close.
// ------------------------------------------------------------------------

// Create files.
bool store::create() {
    const auto created = true
#ifdef BITPRIM_DB_LEGACY
                        && create(block_table) 
                        && create(block_index) 
                        && create(transaction_table) 
#endif // BITPRIM_DB_LEGACY

#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
                        && create(transaction_unconfirmed_table)
#endif // BITPRIM_DB_TRANSACTION_UNCONFIRMED    
                        ;

    if ( ! use_indexes) {
        return created;
    }

    return
        created 
#ifdef BITPRIM_DB_SPENDS
            && create(spend_table) 
#endif // BITPRIM_DB_SPENDS

#ifdef BITPRIM_DB_HISTORY
            && create(history_table)
            && create(history_rows) 
#endif // BITPRIM_DB_HISTORY    

#ifdef BITPRIM_DB_STEALTH
            && create(stealth_rows)
#endif // BITPRIM_DB_STEALTH        
            ;
}

bool store::open() {
    return true 
#ifdef BITPRIM_DB_LEGACY
        && exclusive_lock_.lock() 
        && flush_lock_.try_lock() 
#endif
        && (flush_each_write_ 
#ifdef BITPRIM_DB_LEGACY
        || flush_lock_.lock_shared()
#endif
        );
}

bool store::close() {
    return (flush_each_write_ 
#ifdef BITPRIM_DB_LEGACY
            || flush_lock_.unlock_shared()
#endif
           ) 
#ifdef BITPRIM_DB_LEGACY
           && exclusive_lock_.unlock()
#endif
           ;
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

#ifdef BITPRIM_DB_LEGACY
bool store::begin_write() const {
    return flush_lock() && sequential_lock_.begin_write();
}

bool store::end_write() const
{
    return sequential_lock_.end_write() && flush_unlock();
}
#endif // BITPRIM_DB_LEGACY

bool store::flush_lock() const {
    return ! flush_each_write_ 
#ifdef BITPRIM_DB_LEGACY
           || flush_lock_.lock_shared()
#endif
           ;
}

bool store::flush_unlock() const {
    return  ! flush_each_write_ 
            || (flush() 
#ifdef BITPRIM_DB_LEGACY
            && flush_lock_.unlock_shared()
#endif
            );
}

} // namespace data_base
} // namespace libbitcoin

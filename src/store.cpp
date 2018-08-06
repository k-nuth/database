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

// Database file names.
#define FLUSH_LOCK "flush_lock"
#define EXCLUSIVE_LOCK "exclusive_lock"
#define BLOCK_TABLE "block_table"
#define BLOCK_INDEX "block_index"
#define TRANSACTION_TABLE "transaction_table"
#define TRANSACTION_UNCONFIRMED_TABLE "transaction_unconfirmed_table"
#define SPEND_TABLE "spend_table"
#define HISTORY_TABLE "history_table"
#define HISTORY_ROWS "history_rows"
#define STEALTH_ROWS "stealth_rows"

// The threashold max_uint32 is used to align with fixed-width config settings,
// and size_t is used to align with the database height domain.
const size_t store::without_indexes = max_uint32;

#ifndef BITPRIM_READ_ONLY
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
#endif // BITPRIM_READ_ONLY

// Construct.
// ------------------------------------------------------------------------

store::store(const path& prefix, bool with_indexes, bool flush_each_write)
    : use_indexes(with_indexes)
    , flush_each_write_(flush_each_write)
#ifndef BITPRIM_READ_ONLY
    , flush_lock_(prefix / FLUSH_LOCK)
    , exclusive_lock_(prefix / EXCLUSIVE_LOCK)
#endif // BITPRIM_READ_ONLY

    // Content store.
    , block_table(prefix / BLOCK_TABLE)
    , block_index(prefix / BLOCK_INDEX)
    , transaction_table(prefix / TRANSACTION_TABLE)
    , transaction_unconfirmed_table(prefix / TRANSACTION_UNCONFIRMED_TABLE)

    // Optional indexes.
    , spend_table(prefix / SPEND_TABLE)
    , history_table(prefix / HISTORY_TABLE)
    , history_rows(prefix / HISTORY_ROWS)
    , stealth_rows(prefix / STEALTH_ROWS)
{}

// Open and close.
// ------------------------------------------------------------------------

#ifndef BITPRIM_READ_ONLY
// Create files.
bool store::create()
{
    const auto created =
        create(block_table) &&
        create(block_index) &&
        create(transaction_table) &&
        create(transaction_unconfirmed_table);

    if (!use_indexes)
        return created;

    return
        created &&
        create(spend_table) &&
        // create(unspent_table, false)  &&
        create(history_table) &&
        create(history_rows) &&
        create(stealth_rows);
}
#endif // BITPRIM_READ_ONLY

bool store::open()
{
#ifndef BITPRIM_READ_ONLY
    return exclusive_lock_.lock() && flush_lock_.try_lock() &&
        (flush_each_write_ || flush_lock_.lock_shared());
#else
    return true;
#endif // BITPRIM_READ_ONLY
}

bool store::close()
{
#ifndef BITPRIM_READ_ONLY
    return (flush_each_write_ || flush_lock_.unlock_shared()) &&
        exclusive_lock_.unlock();
#else
    return true;
#endif // BITPRIM_READ_ONLY
}

#ifndef BITPRIM_READ_ONLY

store::handle store::begin_read() const
{
    return sequential_lock_.begin_read();
}

bool store::is_read_valid(handle value) const
{
    return sequential_lock_.is_read_valid(value);
}

bool store::is_write_locked(handle value) const
{
    return sequential_lock_.is_write_locked(value);
}

bool store::begin_write() const
{
    return flush_lock() && sequential_lock_.begin_write();
}

bool store::end_write() const
{
    return sequential_lock_.end_write() && flush_unlock();
}

bool store::flush_lock() const
{
    return !flush_each_write_ || flush_lock_.lock_shared();
}

bool store::flush_unlock() const
{
    return !flush_each_write_ || (flush() && flush_lock_.unlock_shared());
}
#endif // BITPRIM_READ_ONLY

} // namespace data_base
} // namespace libbitcoin

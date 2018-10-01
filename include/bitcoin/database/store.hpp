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
#ifndef LIBBITCOIN_DATABASE_STORE_HPP
#define LIBBITCOIN_DATABASE_STORE_HPP

#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

#if defined(BITPRIM_DB_LEGACY) && (defined(BITPRIM_DB_SPENDS) || defined(BITPRIM_DB_HISTORY) || defined(BITPRIM_DB_STEALTH))
#define BITPRIM_DB_WITH_INDEXES
#endif

namespace libbitcoin {
namespace database {

class BCD_API store {
public:
    using path = boost::filesystem::path;
    using handle = sequential_lock::handle;

    static 
    size_t const without_indexes;


#ifdef BITPRIM_DB_LEGACY
    /// Create a single file with one byte of arbitrary data.
    static 
    bool create(path const& file_path);
#endif // BITPRIM_DB_LEGACY

    // Construct.
    // ------------------------------------------------------------------------

    store(path const& prefix, bool with_indexes, bool flush_each_write = false);


#ifdef BITPRIM_DB_LEGACY
    // Open and close.
    // ------------------------------------------------------------------------

    /// Create database files.
    virtual bool create();

    /// Acquire exclusive access.
    virtual bool open();

    /// Release exclusive access.
    virtual bool close();

    // Write with flush detection.
    // ------------------------------------------------------------------------

    /// Start a read sequence and obtain its handle.
    handle begin_read() const;

    /// Check read sequence result of the handle.
    bool is_read_valid(handle handle) const;

    /// Check the write state of the handle.
    bool is_write_locked(handle handle) const;


    /// Start sequence write with optional flush lock.
    bool begin_write() const;

    /// End sequence write with optional flush unlock.
    bool end_write() const;

    /// Optionally begin flush lock scope.
    bool flush_lock() const;

    /// Optionally end flush lock scope.
    bool flush_unlock() const;
#endif // BITPRIM_DB_LEGACY

    // File names.
    // ------------------------------------------------------------------------

    /// Content store.
#ifdef BITPRIM_DB_LEGACY
    path const block_table;
    path const block_index;
    path const transaction_table;
#endif // BITPRIM_DB_LEGACY

#ifdef BITPRIM_DB_NEW
    path const utxo_dir;
#endif // BITPRIM_DB_NEW

#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
    path const transaction_unconfirmed_table;
#endif // BITPRIM_DB_STEALTH        

    /// Optional indexes.
#ifdef BITPRIM_DB_SPENDS
    path const spend_table;
#endif // BITPRIM_DB_SPENDS

#ifdef BITPRIM_DB_HISTORY
    path const history_table;
    path const history_rows;
#endif // BITPRIM_DB_HISTORY

#ifdef BITPRIM_DB_STEALTH
    path const stealth_rows;
#endif // BITPRIM_DB_STEALTH

protected:
#ifdef BITPRIM_DB_LEGACY
    virtual bool flush() const = 0;
#endif

private:
#ifdef BITPRIM_DB_LEGACY
    bool const flush_each_write_;
    mutable bc::flush_lock flush_lock_;
    mutable interprocess_lock exclusive_lock_;
    mutable sequential_lock sequential_lock_;
#endif

protected:
#ifdef BITPRIM_DB_WITH_INDEXES
    bool const use_indexes_;
#endif    
    bool use_indexes() const;
};

} // namespace database
} // namespace libbitcoin

#endif

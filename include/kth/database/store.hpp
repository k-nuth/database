// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_STORE_HPP
#define KTH_DATABASE_STORE_HPP

#include <filesystem>
#include <memory>

#include <kth/domain.hpp>
#include <kth/database/define.hpp>

#include <kth/infrastructure/utility/sequential_lock.hpp>

#if defined(KTH_DB_LEGACY) && (defined(KTH_DB_SPENDS) || defined(KTH_DB_HISTORY) || defined(KTH_DB_STEALTH))
#define KTH_DB_WITH_INDEXES
#endif

namespace kth::database {

class KD_API store {
public:
    using path = std::filesystem::path;
    using handle = sequential_lock::handle;

    static 
    size_t const without_indexes;


#ifdef KTH_DB_LEGACY
    /// Create a single file with one byte of arbitrary data.
    static 
    bool create(path const& file_path);
#endif // KTH_DB_LEGACY

    // Construct.
    // ------------------------------------------------------------------------

    store(path const& prefix, bool with_indexes, bool flush_each_write = false);


#ifdef KTH_DB_LEGACY
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
#endif // KTH_DB_LEGACY

    // File names.
    // ------------------------------------------------------------------------

    /// Content store.
#ifdef KTH_DB_LEGACY
    path const block_table;
    path const block_index;
    path const transaction_table;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
    path const internal_db_dir;
#endif // KTH_DB_NEW

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    path const transaction_unconfirmed_table;
#endif // KTH_DB_STEALTH        

    /// Optional indexes.
#ifdef KTH_DB_SPENDS
    path const spend_table;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
    path const history_table;
    path const history_rows;
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_STEALTH
    path const stealth_rows;
#endif // KTH_DB_STEALTH

protected:
#ifdef KTH_DB_LEGACY
    virtual bool flush() const = 0;
#endif

private:
#ifdef KTH_DB_LEGACY
    bool const flush_each_write_;
    mutable bc::flush_lock flush_lock_;
    mutable interprocess_lock exclusive_lock_;
    mutable sequential_lock sequential_lock_;
#endif

protected:
#ifdef KTH_DB_WITH_INDEXES
    bool const use_indexes_;
#endif    
    bool use_indexes() const;
};

} // namespace database
} // namespace kth

#endif

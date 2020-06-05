// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_STEALTH_DATABASE_HPP
#define KTH_DATABASE_STEALTH_DATABASE_HPP

#ifdef KTH_DB_STEALTH

#include <cstdint>
#include <filesystem>
#include <memory>

// #include <boost/filesystem.hpp>

#include <kth/database/define.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include <kth/database/legacy/memory/memory_map.hpp>
#include <kth/database/legacy/primitives/record_manager.hpp>

namespace kth::database {

class KD_API stealth_database
{
public:
    typedef domain::chain::stealth_compact::list list;
    typedef std::filesystem::path path;
    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    /// Construct the database.
    stealth_database(const path& rows_filename, size_t expansion,
        mutex_ptr mutex=nullptr);

    /// Close the database (all threads must first be stopped).
    ~stealth_database();

    /// Initialize a new stealth database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// Linearly scan all entries, discarding those after from_height.
    list scan(const binary& filter, size_t from_height) const;

    /// Add a stealth row to the database.
    void store(uint32_t prefix, uint32_t height,
        const chain::stealth_compact& row);

    /////// Delete stealth row (not implemented).
    ////bool unlink();

    /// Commit latest inserts.
    void synchronize();

    /// Flush the memory map to disk.
    bool flush() const;

private:
    void write_index();
    array_index read_index(size_t from_height) const;

    // Row entries containing stealth tx data.
    memory_map rows_file_;
    record_manager rows_manager_;
};

} // namespace database
} // namespace kth

#endif // KTH_DB_STEALTH

#endif

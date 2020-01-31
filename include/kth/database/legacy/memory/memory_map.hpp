// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_MEMORY_MAP_HPP
#define KTH_DATABASE_MEMORY_MAP_HPP

#ifndef _WIN32
#include <sys/mman.h>
#endif
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace kth {
namespace database {

/// This class is thread safe, allowing concurent read and write.
/// A change to the size of the memory map waits on and locks read and write.
class BCD_API memory_map
{
public:
    typedef boost::filesystem::path path;
    typedef std::shared_ptr<shared_mutex> mutex_ptr;

    static const size_t default_expansion;

    /// Construct a database (start is currently called, may throw).
    memory_map(const path& filename);
    memory_map(const path& filename, mutex_ptr mutex);
    memory_map(const path& filename, mutex_ptr mutex, size_t expansion);

    /// Close the database.
    ~memory_map();

    /// This class is not copyable.
    memory_map(const memory_map&) = delete;
    void operator=(const memory_map&) = delete;

    /// Open and map database files.
    bool open();

    /// Flush the memory map to disk.
    bool flush() const;

    /// Unmap and release database files, can be restarted.
    bool close();

    /// Determine if the database is closed.
    bool closed() const;

    size_t size() const;
    memory_ptr access();
    memory_ptr resize(size_t size);
    memory_ptr reserve(size_t size);
    memory_ptr reserve(size_t size, size_t growth_ratio);

private:
    static size_t file_size(int file_handle);
    static int open_file(const boost::filesystem::path& filename);
    static bool handle_error(const std::string& context,
        const boost::filesystem::path& filename);

    size_t page() const;
    bool unmap();
    bool map(size_t size);
    bool remap(size_t size);
    bool truncate(size_t size);
    bool truncate_mapped(size_t size);
    bool validate(size_t size);

    void log_mapping() const;
    void log_resizing(size_t size) const;
    void log_flushed() const;
    void log_unmapping() const;
    void log_unmapped() const;

    // Optionally guard against concurrent remap.
    mutex_ptr remap_mutex_;

    // File system.
    int const file_handle_;
    const size_t expansion_;
    const boost::filesystem::path filename_;

    // Protected by internal mutex.
    uint8_t* data_;
    size_t file_size_;
    size_t logical_size_;
    std::atomic<bool> closed_;
    mutable upgrade_mutex mutex_;
};

} // namespace database
} // namespace kth

#endif

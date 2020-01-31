// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_SETTINGS_HPP
#define KTH_DATABASE_SETTINGS_HPP

#include <cstdint>
#include <boost/filesystem.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Common database configuration settings, properties not thread safe.
class BCD_API settings
{
public:
    settings();
    settings(config::settings context);

    /// Properties.
    boost::filesystem::path directory;
    bool flush_writes;
    uint16_t file_growth_rate;
    uint32_t index_start_height;

#ifdef KTH_DB_NEW
    uint32_t reorg_pool_limit;
    uint64_t db_max_size;
    bool safe_mode;
#endif // KTH_DB_NEW


#ifdef KTH_DB_LEGACY
    uint32_t block_table_buckets;
    uint32_t transaction_table_buckets;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    uint32_t transaction_unconfirmed_table_buckets;
#endif // KTH_DB_STEALTH        

#ifdef KTH_DB_SPENDS
    uint32_t spend_table_buckets;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
    uint32_t history_table_buckets;
#endif // KTH_DB_HISTORY

// #ifdef KTH_DB_UNSPENT_LEGACY
    uint32_t cache_capacity;
// #endif // KTH_DB_UNSPENT_LEGACY

#if defined(WITH_REMOTE_DATABASE)    
    config::endpoint replier;
#endif //defined(WITH_REMOTE_DATABASE)        

};

} // namespace database
} // namespace kth

#endif

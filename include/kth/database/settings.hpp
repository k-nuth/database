// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_SETTINGS_HPP
#define KTH_DATABASE_SETTINGS_HPP

#include <cstdint>
#include <filesystem>

#include <kth/database/define.hpp>
#include <kth/database/databases/property_code.hpp>

namespace kth::database {

/// Common database configuration settings, properties not thread safe.
class KD_API settings {
public:
    settings();
    settings(domain::config::network net);

    /// Properties.
    kth::path directory;
    db_mode_type db_mode;
    domain::config::network net;
    uint32_t reorg_pool_limit;

    // uint64_t db_max_size;
    uint64_t db_utxo_size;
    uint64_t db_header_size;
    uint64_t db_header_by_hash_size;
    uint64_t too_old;

    uint32_t cache_capacity;
};

} // namespace kth::database

#endif

// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/settings.hpp>

#include <filesystem>

namespace kth::database {

using namespace std::filesystem;

//TODO(fernando): look for good defaults
constexpr auto db_size_pruned_mainnet  = 100 * (uint64_t(1) << 30); //100 GiB
constexpr auto db_size_default_mainnet = 200 * (uint64_t(1) << 30); //200 GiB
constexpr auto db_size_full_mainnet    = 600 * (uint64_t(1) << 30); //600 GiB

constexpr auto db_size_pruned_testnet4  =  5 * (uint64_t(1) << 30); // 5 GiB
constexpr auto db_size_default_testnet4 = 20 * (uint64_t(1) << 30); //20 GiB
constexpr auto db_size_full_testnet4    = 50 * (uint64_t(1) << 30); //50 GiB

#if defined(KTH_DB_DYNAMIC)
constexpr
auto get_db_max_size_mainnet(db_mode_type mode) {
    return mode == db_mode_type::pruned
        ? db_size_pruned_mainnet
        : mode == db_mode_type::default
            ? db_size_default_mainnet
            : db_size_full_mainnet;
}

constexpr auto get_db_max_size_testnet4(db_mode_type mode) {
    return mode == db_mode_type::pruned
        ? db_size_pruned_testnet4
        : mode == db_mode_type::default
            ? db_size_default_testnet4
            : db_size_full_testnet4;
}
#endif // defined(KTH_DB_DYNAMIC)

settings::settings()
    : directory(u8"blockchain")
    , flush_writes(false)
    , file_growth_rate(50)
    , index_start_height(0)

#if defined(KTH_DB_DYNAMIC)
    , db_mode(db_mode_type::default)
#endif // defined(KTH_DB_DYNAMIC)

#if defined(KTH_DB_NEW) || defined(KTH_DB_DYNAMIC)
    , reorg_pool_limit(100)      //TODO(fernando): look for a good default

#if defined(KTH_DB_NEW_BLOCKS)
    , db_max_size(db_size_default_mainnet)
#elif defined(KTH_DB_NEW_FULL)
    , db_max_size(db_size_full_mainnet)
#elif defined(KTH_DB_DYNAMIC)
    , db_max_size(get_db_max_size_mainnet(db_mode))
#else
    , db_max_size(db_size_pruned_mainnet)
#endif // KTH_DB_NEW_BLOCKS

    , safe_mode(true)

#endif // defined(KTH_DB_NEW) || defined(KTH_DB_DYNAMIC)

    // Hash table sizes (must be configured).
#ifdef KTH_DB_LEGACY
    , block_table_buckets(0)
    , transaction_table_buckets(0)
#endif // KTH_DB_LEGACY
#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    , transaction_unconfirmed_table_buckets(0)
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_SPENDS
    , spend_table_buckets(0)
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
    , history_table_buckets(0)
#endif // KTH_DB_HISTORY

// #ifdef KTH_DB_UNSPENT_LEGACY
    , cache_capacity(0)
// #endif // KTH_DB_UNSPENT_LEGACY
{}

settings::settings(domain::config::network context)
    : settings()
{
    switch (context) {
        case domain::config::network::mainnet: {
#ifdef KTH_DB_LEGACY
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
            transaction_unconfirmed_table_buckets = 10000;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_SPENDS
            spend_table_buckets = 250000000;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
            history_table_buckets = 107000000;
#endif // KTH_DB_HISTORY
            break;
        }
        case domain::config::network::testnet: {
            // TODO(fernando): optimize for testnet.
#ifdef KTH_DB_LEGACY
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
            transaction_unconfirmed_table_buckets = 10000;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_SPENDS
            spend_table_buckets = 250000000;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
            history_table_buckets = 107000000;
#endif // KTH_DB_HISTORY

            break;
        }

#if defined(KTH_CURRENCY_BCH)
        case domain::config::network::testnet4: {
#if defined(KTH_DB_NEW_BLOCKS)
            db_max_size = db_size_default_testnet4;
#elif defined(KTH_DB_NEW_FULL)
            db_max_size = db_size_full_testnet4;
#elif defined(KTH_DB_DYNAMIC)
            db_max_size = get_db_max_size_tesnet4(db_mode);
#elif defined(KTH_DB_NEW)
            db_max_size = db_size_pruned_testnet4;
#endif // KTH_DB_NEW_BLOCKS

            // TODO(fernando): optimize for testnet4.
#ifdef KTH_DB_LEGACY
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
            transaction_unconfirmed_table_buckets = 10000;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_SPENDS
            spend_table_buckets = 250000000;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
            history_table_buckets = 107000000;
#endif // KTH_DB_HISTORY

            break;
        }
        case domain::config::network::scalenet: {
#if defined(KTH_DB_NEW_BLOCKS)
            db_max_size = db_size_default_tesnet4;
#elif defined(KTH_DB_NEW_FULL)
            db_max_size = db_size_full_tesnet4;
#elif defined(KTH_DB_DYNAMIC)
            db_max_size = get_db_max_size_tesnet4(db_mode);
#elif defined(KTH_DB_NEW)
            db_max_size = db_size_pruned_tesnet4;
#endif // KTH_DB_NEW_BLOCKS

            // TODO(fernando): optimize for scalenet.
#ifdef KTH_DB_LEGACY
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
            transaction_unconfirmed_table_buckets = 10000;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_SPENDS
            spend_table_buckets = 250000000;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
            history_table_buckets = 107000000;
#endif // KTH_DB_HISTORY

            break;
        }

        case domain::config::network::chipnet: {
#if defined(KTH_DB_NEW_BLOCKS)
            db_max_size = db_size_default_testnet4;
#elif defined(KTH_DB_NEW_FULL)
            db_max_size = db_size_full_testnet4;
#elif defined(KTH_DB_DYNAMIC)
            db_max_size = get_db_max_size_tesnet4(db_mode);
#elif defined(KTH_DB_NEW)
            db_max_size = db_size_pruned_testnet4;
#endif // KTH_DB_NEW_BLOCKS

            // TODO(fernando): optimize for testnet4.
#ifdef KTH_DB_LEGACY
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
            transaction_unconfirmed_table_buckets = 10000;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_SPENDS
            spend_table_buckets = 250000000;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
            history_table_buckets = 107000000;
#endif // KTH_DB_HISTORY

            break;
        }

#endif // defined(KTH_CURRENCY_BCH)
    }
}

} // namespace kth::database

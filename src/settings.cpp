// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/settings.hpp>

#include <filesystem>

namespace kth::database {

using namespace std::filesystem;

constexpr auto one_gigabyte     =  1ull * (uint64_t(1) << 30);
constexpr auto two_gigabytes    =  2ull * (uint64_t(1) << 30);
constexpr auto ten_gigabytes    = 10ull * (uint64_t(1) << 30);
constexpr auto twenty_gigabytes = 20ull * (uint64_t(1) << 30);
constexpr auto fifty_gigabytes  = 50ull * (uint64_t(1) << 30);
constexpr auto one_hundred_gigabytes  = 100ull * (uint64_t(1) << 30);
constexpr auto two_hundred_gigabytes  = 200ull * (uint64_t(1) << 30);
constexpr auto five_hundred_gigabytes = 500ull * (uint64_t(1) << 30);

constexpr
auto get_reorg_pool_limit(domain::config::network net, db_mode_type mode) {
    return 100; //TODO: look for a good default
}

constexpr
auto get_db_utxo_size(domain::config::network net, db_mode_type mode) {
    switch (net) {
        case domain::config::network::mainnet:
            return fifty_gigabytes;
        case domain::config::network::testnet:
            return ten_gigabytes;
#if defined(KTH_CURRENCY_BCH)
        case domain::config::network::testnet4:
            return ten_gigabytes;
        case domain::config::network::scalenet:
            return ten_gigabytes;
        case domain::config::network::chipnet:
            return ten_gigabytes;
#endif // defined(KTH_CURRENCY_BCH)
    }
    return fifty_gigabytes;
}

constexpr
auto get_db_header_size(domain::config::network net, db_mode_type mode) {
    switch (net) {
        case domain::config::network::mainnet:
            return twenty_gigabytes;
        case domain::config::network::testnet:
            return ten_gigabytes;
#if defined(KTH_CURRENCY_BCH)
        case domain::config::network::testnet4:
            return ten_gigabytes;
        case domain::config::network::scalenet:
            return ten_gigabytes;
        case domain::config::network::chipnet:
            return ten_gigabytes;
#endif // defined(KTH_CURRENCY_BCH)
    }
    return twenty_gigabytes;
}

constexpr
auto get_db_header_by_hash_size(domain::config::network net, db_mode_type mode) {
        switch (net) {
        case domain::config::network::mainnet:
            return twenty_gigabytes;
        case domain::config::network::testnet:
            return ten_gigabytes;
#if defined(KTH_CURRENCY_BCH)
        case domain::config::network::testnet4:
            return ten_gigabytes;
        case domain::config::network::scalenet:
            return ten_gigabytes;
        case domain::config::network::chipnet:
            return ten_gigabytes;
#endif // defined(KTH_CURRENCY_BCH)
    }
    return twenty_gigabytes;
}

constexpr
auto get_too_old(domain::config::network net, db_mode_type mode) {
    return 10000; //TODO: look for a good default
}

constexpr
auto get_cache_capacity(domain::config::network net, db_mode_type mode) {
    return 1'000'000; //TODO: look for a good default
}

settings::settings(domain::config::network net_)
    : directory(u8"blockchain")
    , db_mode(db_mode_type::blocks)
    , net(net_)
    , reorg_pool_limit(get_reorg_pool_limit(net, db_mode))
    // , db_max_size(get_db_max_size_mainnet(db_mode))
    , db_utxo_size(get_db_utxo_size(net, db_mode))
    , db_header_size(get_db_header_size(net, db_mode))
    , db_header_by_hash_size(get_db_header_by_hash_size(net, db_mode))
    , too_old(get_too_old(net, db_mode))
    , cache_capacity(get_cache_capacity(net, db_mode))
{}

settings::settings()
    : settings(domain::config::network::mainnet)
{}



} // namespace kth::database

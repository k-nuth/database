// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_HEADER_DATABASE_IPP_
#define KTH_DATABASE_HEADER_DATABASE_IPP_

#include <kth/infrastructure.hpp>

namespace kth::database {

#if ! defined(KTH_DB_READONLY)


template <typename Clock>
result_code internal_database_basis<Clock>::push_block_header(domain::chain::block const& block, uint32_t height) {

    // LOG_INFO(LOG_DATABASE, "push_block_header: ", height);

    if (height > last_height_) {
        last_height_ = height;
    }

    {
        // auto const inserted = header_cache_.emplace(height, block.header());            //concurrent
        auto const [_, inserted] = header_cache_.emplace(height, block.header());
        if ( ! inserted) {
            LOG_INFO(LOG_DATABASE, "Duplicate key inserting block header [push_block_header]");
            return result_code::duplicated_key;
        }
    }

    {
        // auto const inserted = header_by_hash_cache_.emplace(block.hash(), height);      // concurrent
        auto const [_, inserted] = header_by_hash_cache_.emplace(block.hash(), height);
        if ( ! inserted) {
            LOG_INFO(LOG_DATABASE, "Duplicate key inserting block header by hash [push_block_header]");
            return result_code::duplicated_key;
        }
    }

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
domain::chain::header internal_database_basis<Clock>::get_header(uint32_t height) const {
    if (height > last_height_) {
        LOG_ERROR(LOG_DATABASE, "Invalid height [get_header] - height: ", height, " last_height: ", last_height_);
        return domain::chain::header{};
    }

    // Concurrent
    // domain::chain::header ret;
    // size_t visited = header_cache_.cvisit(height, [&](auto const& x) {
    //     ret = x.second;
    // });
    // if (visited == 0) {
    //     LOG_ERROR(LOG_DATABASE, "Key not found [get_header] - height: ", height);
    //     return domain::chain::header{};
    // }
    // return ret;


    auto const it = header_cache_.find(height);
    if (it == header_cache_.end()) {
        LOG_ERROR(LOG_DATABASE, "Key not found [get_header] - height: ", height);
        return domain::chain::header{};
    }
    return it->second;
}

template <typename Clock>
std::pair<domain::chain::header, uint32_t> internal_database_basis<Clock>::get_header(hash_digest const& hash) const {
    auto const it = header_by_hash_cache_.find(hash);
    if (it == header_by_hash_cache_.end()) {
        return {};
    }
    auto const height = it->second;
    auto const header = get_header(height);
    return {header, height};

    // Concurrent
    // uint32_t height;
    // size_t visited = header_by_hash_cache_.cvisit(hash, [&](auto const& p) {
    //     height = p.second;
    // });
    // if (visited == 0) {
    //     return {};
    // }
    // auto const header = get_header(height);
    // return {header, height};
}

#if ! defined(KTH_DB_READONLY)



template <typename Clock>
result_code internal_database_basis<Clock>::remove_block_header(hash_digest const& hash, uint32_t height) {
    {
    auto const erased = header_cache_.erase(height);
    if (erased == 0) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting block header in LMDB [remove_block_header]");
        return result_code::key_not_found;
    }
    }

    auto const erased = header_by_hash_cache_.erase(hash);
    if (erased == 0) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting block header by hash in LMDB [remove_block_header]");
        return result_code::key_not_found;
    }

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)


} // namespace kth::database

#endif // KTH_DATABASE_HEADER_DATABASE_IPP_

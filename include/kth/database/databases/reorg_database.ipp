// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_REORG_DATABASE_HPP_
#define KTH_DATABASE_REORG_DATABASE_HPP_

namespace kth::database {

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::insert_reorg_pool(uint32_t height, domain::chain::output_point const& point) {

    // Buscamos la entrada UTXO en la base de datos
    auto it_utxo = utxo_set_cache_.find(point);
    if (it_utxo == utxo_set_cache_.end()) {
        LOG_INFO(LOG_DATABASE, "Key not found getting UTXO [insert_reorg_pool]");
        return result_code::key_not_found;
    }

    // Insertamos la entrada UTXO en el reorg_pool
    auto pair_res = reorg_pool_.insert({point, it_utxo->second});
    if (!pair_res.second) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg pool [insert_reorg_pool]");
        return result_code::duplicated_key;
    }

    // Insertamos la entrada en reorg_index
    auto& reorg_index_height = reorg_index_[height];  // crea la entrada si no existe aún
    reorg_index_height.push_back(point);

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_block_reorg(domain::chain::block const& block, uint32_t height) {
    auto res = reorg_block_map_.insert({height, block});
    if ( ! res.second) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in reorg block [push_block_reorg]");
        return result_code::duplicated_key;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_output_from_reorg_and_remove(domain::chain::output_point const& point) {
    auto it = reorg_pool_.find(point);
    if (it == reorg_pool_.end()) {
        LOG_INFO(LOG_DATABASE, "Key not found in reorg pool [insert_output_from_reorg_and_remove]");
        return result_code::key_not_found;
    }

    auto result = utxo_set_cache_.insert(*it);
    if (! result.second) {
        LOG_INFO(LOG_DATABASE, "Duplicate key inserting in UTXO [insert_output_from_reorg_and_remove]");
        return result_code::duplicated_key;
    }

    reorg_pool_.erase(it);
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_block_reorg(uint32_t height) {

    auto it = reorg_block_map_.find(height);
    if (it == reorg_block_map_.end()) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting reorg block [remove_block_reorg]");
        return result_code::key_not_found;
    }

    reorg_block_map_.erase(it);

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_reorg_index(uint32_t height) {

    auto it = reorg_index_.find(height);
    if (it == reorg_index_.end()) {
        LOG_DEBUG(LOG_DATABASE, "Key not found deleting reorg index [remove_reorg_index] - height: ", height);
        return result_code::key_not_found;
    }

    reorg_index_.erase(it);

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
domain::chain::block internal_database_basis<Clock>::get_block_from_reorg_pool(uint32_t height) const {
    auto it = reorg_block_map_.find(height);
    if (it == reorg_block_map_.end()) {
        return domain::chain::block{};
    }
    return it->second;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::prune_reorg_index(uint32_t remove_until) {
    // for (auto it = reorg_index_.begin(); it != reorg_index_.end();) {
    //     // uint32_t, std::vector<domain::chain::point>
    //     auto const& [height, points] = *it;
    //     // if (it->first < remove_until) {
    //     if (height < remove_until) {
    //         for (auto const& point : points) {
    //             auto output_it = reorg_pool_.find(point);
    //             if (output_it == reorg_pool_.end()) {
    //                 LOG_INFO(LOG_DATABASE, "Key not found deleting reorg pool [prune_reorg_index]");
    //                 return result_code::key_not_found;
    //             }
    //             reorg_pool_.erase(output_it);
    //         }
    //         it = reorg_index_.erase(it);
    //     } else {
    //         ++it;
    //     }
    // }

    erase_if(reorg_index_, [&](std::pair<uint32_t const, std::vector<domain::chain::point>>& v) {
        auto const& [height, points] = v;
        if (height >= remove_until) {
            return false;
        }

        for (auto const& point : points) {
            auto it = reorg_pool_.find(point);
            if (it == reorg_pool_.end()) {
                LOG_INFO(LOG_DATABASE, "Key not found deleting reorg pool [prune_reorg_index]");
                // return result_code::key_not_found;
                continue;
            }
            reorg_pool_.erase(it);
        }
        return true;
    });
    return result_code::success;
}

//TODO(fernando): amount is not a good name
template <typename Clock>
result_code internal_database_basis<Clock>::prune_reorg_block(uint32_t elements_to_delete) {
    //precondition: elements_to_delete >= 1
    //precondition: reorg_block_map_.size() >= elements_to_delete

    if (reorg_block_map_.empty()) {
        return result_code::other;
    }

    while (true) {
        auto it = std::min_element(reorg_block_map_.begin(), reorg_block_map_.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });
        reorg_block_map_.erase(it);
        if (--elements_to_delete == 0) break;
    }
    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::get_first_reorg_block_height(uint32_t& out_height) const {
    if (reorg_block_map_.empty()) {
        return result_code::db_empty;
    }

    auto it = std::min_element(reorg_block_map_.begin(), reorg_block_map_.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    out_height = it->first;

    return result_code::success;
}


} // namespace kth::database

#endif // KTH_DATABASE_REORG_DATABASE_HPP_

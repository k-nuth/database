// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_UTXO_DATABASE_HPP_
#define KTH_DATABASE_UTXO_DATABASE_HPP_

namespace kth::database {

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::remove_utxo(uint32_t height, domain::chain::output_point const& point, bool insert_reorg) {
    if (insert_reorg) {
        auto res0 = insert_reorg_pool(height, point);
        if (res0 != result_code::success) return res0;
    }

    auto it = utxo_set_cache_.find(point);
    if (it == utxo_set_cache_.end()) {
        // LOG_INFO(LOG_DATABASE, "Key not found deleting UTXO [remove_utxo] ");
        // return result_code::key_not_found;
        utxo_to_remove_cache_.emplace(point, height);
        ++remove_utxo_not_found_in_cache;
        return result_code::success;
    }

    utxo_set_cache_.erase(it);
    ++remove_uxto_found_in_cache;
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_utxo(domain::chain::output_point const& point, domain::chain::output const& output, uint32_t height, uint32_t median_time_past, bool coinbase) {
    auto [it, inserted] = utxo_set_cache_.emplace(point, utxo_entry{output, height, median_time_past, coinbase});

    if (!inserted) {
        LOG_DEBUG(LOG_DATABASE, "Duplicate Key inserting UTXO [insert_utxo]");
        return result_code::duplicated_key;
    }
    return result_code::success;
}


#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_UTXO_DATABASE_HPP_

// TODO:
//   - Remove the Clock template parameter, inverse the dependency, call the blockchain interface.
//   -

// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_INTERNAL_DATABASE_IPP_
#define KTH_DATABASE_INTERNAL_DATABASE_IPP_

#include <kth/infrastructure.hpp>

#include <functional>
#include <string_view>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/unordered_map.hpp>

#include <boost/container_hash/hash.hpp>

namespace kth::database {

// using utxo_pool_t = std::unordered_map<domain::chain::point, utxo_entry>;
using utxo_pool_t = boost::unordered_flat_map<domain::chain::point, utxo_entry_persistent>; //TODO(fernando): remove this typedefs

template <typename K, typename V>
boost::unordered_flat_map<K, V> to_flat_map(boost::concurrent_flat_map<K, V> const& map) {
    boost::unordered_flat_map<K, V> ret;
    ret.reserve(map.size());
    map.visit_all([&ret](auto const& x) {
        ret.insert(x);
    });
    return ret;
}

template <typename K, typename V>
boost::unordered_flat_map<K, V> to_flat_map_remove(boost::concurrent_flat_map<K, V>& map) {
    boost::unordered_flat_map<K, V> ret;
    ret.reserve(map.size());
    map.erase_if([&ret](auto& x){
        // ret.insert(std::move(x));
        ret.insert(x);
        return true; // erase unconditionally
    });
    return ret;
}

// //TODO(fernando): remove this
// template <typename K>
// boost::unordered_flat_set<K> to_flat_map_remove_fake(boost::concurrent_flat_map<K, int>& map) {
//     boost::unordered_flat_set<K> ret;
//     ret.reserve(map.size());
//     map.erase_if([&ret](auto& x){
//         ret.insert(std::move(x.first));
//         return true; // erase unconditionally
//     });
//     return ret;
// }

// template <typename K, typename V>
// std::vector<std::pair<K, V>> to_vector_remove(boost::concurrent_flat_map<K, V>& map) {
//     std::vector<std::pair<K, V>> ret;
//     ret.reserve(map.size());
//     map.erase_if([&ret](auto& x){
//         ret.emplace_back(std::move(x));
//         return true; // erase unconditionally
//     });
//     return ret;
// }


//TODO(fernando): remove Clock template parameter, use block height instead of timestamp
template <typename Clock>
internal_database_basis<Clock>::internal_database_basis(path const& db_dir, uint32_t reorg_pool_limit, uint64_t db_utxo_size, uint64_t db_header_size, uint64_t db_header_by_hash_size, uint64_t too_old, uint32_t cache_capacity)
    : db_dir_(db_dir)
    , reorg_pool_limit_(reorg_pool_limit)
    , limit_(blocks_to_seconds(reorg_pool_limit))
    // , db_max_size_(db_max_size)
    , db_utxo_size_(db_utxo_size)
    , db_header_size_(db_header_size)
    , db_header_by_hash_size_(db_header_by_hash_size)
    , too_old_(too_old)
    , cache_capacity_(cache_capacity)
{
    LOG_INFO(LOG_DATABASE, "DB UTXO size:           ", double(db_utxo_size_) / (uint64_t(1) << 30), " GB");
    LOG_INFO(LOG_DATABASE, "DB Header size:         ", double(db_header_size_) / (uint64_t(1) << 30), " GB");
    LOG_INFO(LOG_DATABASE, "DB Header by hash size: ", double(db_header_by_hash_size_) / (uint64_t(1) << 30), " GB");
    LOG_INFO(LOG_DATABASE, "Too old:                ", too_old_, " blocks");
    LOG_INFO(LOG_DATABASE, "Cache capacity:         ", cache_capacity_);
    LOG_INFO(LOG_DATABASE, "Reorg pool limit:       ", reorg_pool_limit_);

    persister_ = std::thread([this] {
        LOG_INFO(LOG_DATABASE, "Starting persister thread.");
        while (true) {
            // Wait until data is ready or the persister is stopped.
            LOG_INFO(LOG_DATABASE, "Persister thread is waiting for data.");
            while (!data_ready_.load(std::memory_order_acquire) && !stop_persister_);
            LOG_INFO(LOG_DATABASE, "Persister thread is awake.");

            if (stop_persister_) {
                LOG_INFO(LOG_DATABASE, "Stopping persister thread.");
                return;
            }
            LOG_INFO(LOG_DATABASE, "Persister thread is saving data.");

            // Reset the flag.
            data_ready_.store(false, std::memory_order_release);

            // Indicate that we are saving data.
            is_saving_.store(true, std::memory_order_release);

            auto header_cache_copy = to_flat_map_remove(cold_header_cache_);
            LOG_INFO(LOG_DATABASE, "cold_header_cache_ size: ", cold_header_cache_.size());
            LOG_INFO(LOG_DATABASE, "header_cache_copy size: ", header_cache_copy.size());

            auto header_by_hash_cache_copy = to_flat_map_remove(cold_header_by_hash_cache_);
            LOG_INFO(LOG_DATABASE, "cold_header_by_hash_cache_ size: ", cold_header_by_hash_cache_.size());
            LOG_INFO(LOG_DATABASE, "header_by_hash_cache_copy size: ", header_by_hash_cache_copy.size());

            uint32_t last_height_copy = cold_last_height_;
            // if (last_height_copy != 0) {
            //     last_height_copy -= too_old_;
            //     --last_height_copy;
            // }

            persist_headers_internal(last_height_copy, header_cache_copy, header_by_hash_cache_copy);

            auto utxo_db_copy = to_flat_map_remove(cold_utxo_set_cache_);
            LOG_INFO(LOG_DATABASE, "cold_utxo_set_cache_ size: ", cold_utxo_set_cache_.size());
            LOG_INFO(LOG_DATABASE, "utxo_db_copy size: ", utxo_db_copy.size());

            // auto utxo_to_remove_copy = to_flat_map_remove_fake(cold_utxo_to_remove_cache_);
            auto utxo_to_remove_copy = to_flat_map_remove(cold_utxo_to_remove_cache_);
            LOG_INFO(LOG_DATABASE, "cold_utxo_to_remove_cache_ size: ", cold_utxo_to_remove_cache_.size());
            LOG_INFO(LOG_DATABASE, "utxo_to_remove_copy size: ", utxo_to_remove_copy.size());

            persist_utxo_set_internal(utxo_db_copy, utxo_to_remove_copy);

            LOG_INFO(LOG_DATABASE, "AFTER persist_utxo_set_internal");
            LOG_INFO(LOG_DATABASE, "cold_utxo_set_cache_ size: ", cold_utxo_set_cache_.size());
            LOG_INFO(LOG_DATABASE, "cold_utxo_to_remove_cache_ size: ", cold_utxo_to_remove_cache_.size());

            if (cold_utxo_set_cache_.size() > 0) {
                LOG_ERROR(LOG_DATABASE, "cold_utxo_set_cache_.size() > 0 after persist_utxo_set_internal() - size: ", cold_utxo_set_cache_.size());
            }

            if (cold_utxo_to_remove_cache_.size() > 0) {
                LOG_ERROR(LOG_DATABASE, "cold_utxo_to_remove_cache_.size() > 0 after persist_utxo_set_internal() - size: ", cold_utxo_to_remove_cache_.size());
            }

            // Indicate that we are done saving data.
            is_saving_.store(false, std::memory_order_release);
        }
    });
}

template <typename Clock>
internal_database_basis<Clock>::~internal_database_basis() {
    LOG_INFO(LOG_DATABASE, "****************** ~internal_database_basis() ******************");
    if (db_opened_) {
        close();
    }
}

template <typename Clock>
void internal_database_basis<Clock>::wait_until_done_saving() {
    while (is_saving_.load(std::memory_order_acquire));
}


#if ! defined(KTH_DB_READONLY)

template <typename Clock>
bool internal_database_basis<Clock>::create() {
    std::error_code ec;

    if ( ! std::filesystem::create_directories(db_dir_, ec)) {
        if (ec.value() == directory_exists) {
            LOG_ERROR(LOG_DATABASE, "Failed because the directory ", db_dir_.string(), " already exists.");
            return false;
        }

        LOG_ERROR(LOG_DATABASE, "Failed to create directory ", db_dir_.string(), " with error, '", ec.message(), "'.");
        return false;
    }

    auto ret = open_internal();
    if ( ! ret ) {
        return false;
    }

    ret = create_db_mode_property();
    if ( ! ret ) {
        return false;
    }

    return true;
}

template <typename Clock>
bool internal_database_basis<Clock>::create_db_mode_property() {

    // property_code property_code_ = property_code::db_mode;
    // auto key = kth_db_make_value(sizeof(property_code_), &property_code_);
    // auto value = kth_db_make_value(sizeof(db_mode_), &db_mode_);

    // res = kth_db_put(db_txn, dbi_properties_, &key, &value, KTH_DB_NOOVERWRITE);
    // if (res != KTH_DB_SUCCESS) {
    //     LOG_ERROR(LOG_DATABASE, "Failed saving in DB Properties [create_db_mode_property] ", static_cast<int32_t>(res));
    //     kth_db_txn_abort();
    //     return false;
    // }


    return true;
}

#endif // ! defined(KTH_DB_READONLY)


template <typename Clock>
bool internal_database_basis<Clock>::open() {

    LOG_INFO(LOG_DATABASE, "Opening database at ", db_dir_.string());
    {
        auto ret = open_internal();
        if ( ! ret ) {
            LOG_ERROR(LOG_DATABASE, "Error opening database at ", db_dir_.string());
            return false;
        }
    }

    LOG_INFO(LOG_DATABASE, "Database opened at ", db_dir_.string());

    {
        auto ret = verify_db_mode_property();
        if ( ! ret ) {
            LOG_ERROR(LOG_DATABASE, "Error validating DB Mode.");
            return false;
        }
    }

    {
        auto ret = load_all();
        if (ret != result_code::success) {
            LOG_ERROR(LOG_DATABASE, "Error loading database at ", db_dir_.string());
            return false;
        }
    }

    return true;
}

template <typename Clock>
bool internal_database_basis<Clock>::open_internal() {
    return open_databases();
}

template <typename Clock>
bool internal_database_basis<Clock>::verify_db_mode_property() const {

    // property_code property_code_ = property_code::db_mode;

    // auto key = kth_db_make_value(sizeof(property_code_), &property_code_);
    // KTH_DB_val value;

    // res = kth_db_get(db_txn, dbi_properties_, &key, &value);
    // if (res != KTH_DB_SUCCESS) {
    //     LOG_ERROR(LOG_DATABASE, "Failed getting DB Properties [verify_db_mode_property] ", static_cast<int32_t>(res));
    //     return false;
    // }

    // auto const db_mode_db = *static_cast<db_mode_type*>(kth_db_get_data(value));

    // if (db_mode_ != db_mode_db) {
    //     LOG_ERROR(LOG_DATABASE, "Error validating DB Mode, the node is compiled for another DB mode. Node DB Mode: "
    //        , static_cast<uint32_t>(db_mode_)
    //        , ", Actual DB Mode: "
    //        , static_cast<uint32_t>(db_mode_db));
    //     return false;
    // }

    return true;
}

template <typename Clock>
bool internal_database_basis<Clock>::close() {
    LOG_INFO(LOG_DATABASE, "Closing database...");

    if (persister_.joinable()) {
        LOG_INFO(LOG_DATABASE, "Waiting for persister thread to finish...");
        // bool is_saving = is_saving_.load(std::memory_order_acquire);
        while (is_saving_.exchange(true, std::memory_order_acquire));
        LOG_INFO(LOG_DATABASE, "Persister thread is not saving, copying data to temporaries...");

        auto const ec = persist_all_background();
        if (ec != result_code::success) {
            LOG_ERROR(LOG_DATABASE, "Error copying data to temporaries, error code: ", int(ec));
        }
        LOG_INFO(LOG_DATABASE, "Data copied to temporaries, waking up persister thread...");

        LOG_INFO(LOG_DATABASE, "Saving data...");
        data_ready_.store(true, std::memory_order_release);
        LOG_INFO(LOG_DATABASE, "Waiting for persister thread to finish...");
        wait_until_done_saving();
        LOG_INFO(LOG_DATABASE, "Persister thread finished, joining...");
        stop_persister_.store(true, std::memory_order_release);
        persister_.join();
        LOG_INFO(LOG_DATABASE, "Persister thread joined.");
    } else {
        LOG_INFO(LOG_DATABASE, "Persister thread is not running. *************************************");
    }

    LOG_INFO(LOG_DATABASE, "Flushing data to disk...");
    segment_utxo_.flush();
    segment_header_.flush();
    segment_header_by_hash_.flush();
    segment_last_height_.flush();
    LOG_INFO(LOG_DATABASE, "Data flushed to disk.");
    LOG_INFO(LOG_DATABASE, "Database closed.");

    if (db_opened_) {
        db_opened_ = false;
    }

    return true;
}

#if ! defined(KTH_DB_READONLY)

//TODO(fernando): optimization: consider passing a list of outputs to insert and a list of inputs to delete instead of an entire Block.
//                  avoiding inserting and erasing internal spenders

template <typename Clock>
result_code internal_database_basis<Clock>::persist_utxo_set_internal(utxo_db_t const& utxo_to_insert, utxo_to_remove_t const& utxo_to_remove) {
    LOG_INFO(LOG_DATABASE, "persist_utxo_set_internal() START - utxo_to_insert.size(): ", utxo_to_insert.size(), " - db_utxo_.size(): ", db_utxo_->size());
    LOG_INFO(LOG_DATABASE, "persist_utxo_set_internal() - Removing UTXOs - utxo_to_remove.size(): ", utxo_to_remove.size());

    size_t removed = 0;
    for (auto const& [point, _]: utxo_to_remove) {
        auto const it = db_utxo_->find(point);
        if (it == db_utxo_->end()) {
            // LOG_ERROR(LOG_DATABASE, "Key not found deleting UTXO [remove_utxo] ", point.hash(), ":", point.index());
            // LOG_ERROR(LOG_DATABASE, "Key not found deleting UTXO [remove_utxo]");
            return result_code::key_not_found;
        }
        db_utxo_->erase(it);
        ++removed;
    }

    if (removed != utxo_to_remove.size()) {
        LOG_ERROR(LOG_DATABASE, "Error removing UTXOs [remove_utxo] ", removed, " != ", utxo_to_remove.size());
        return result_code::other;
    }

    LOG_INFO(LOG_DATABASE, "Finished removing UTXOs - removed: ", removed);


    for (auto&& [point, entry] : utxo_to_insert) {
        // auto const [it, inserted] = db_utxo_->emplace(std::move(point), std::move(entry));
        auto const [it, inserted] = db_utxo_->emplace(point, entry);
        if ( ! inserted) {
            LOG_ERROR(LOG_DATABASE, "Duplicate Key inserting UTXO [insert_utxo]");
            // LOG_ERROR(LOG_DATABASE, "Duplicate Key inserting UTXO [insert_utxo] ", point.hash(), ":", point.index());
            return result_code::duplicated_key;
        }
    }

    LOG_INFO(LOG_DATABASE, "persist_utxo_set_internal() END   - utxo_to_insert.size(): ", utxo_to_insert.size(), " - db_utxo_.size(): ", db_utxo_->size());

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::persist_headers_internal(
    uint32_t last_height,
    header_db_t& header_cache,
    header_by_hash_db_t& header_by_hash_cache
) {
    LOG_INFO(LOG_DATABASE, "persist_headers_internal() START ************");

    LOG_INFO(LOG_DATABASE, "persist_headers_internal() - Saving Last Height - *db_last_height_: ", *db_last_height_);
    LOG_INFO(LOG_DATABASE, "persist_headers_internal() - Saving Last Height - last_height:      ", last_height);
    *db_last_height_ = last_height;
    segment_last_height_.flush();
    LOG_INFO(LOG_DATABASE, "persist_headers_internal() - Saving Last Height - *db_last_height_: ", *db_last_height_);

    LOG_INFO(LOG_DATABASE, "persist_headers_internal() - Saving Headers - header_cache.size(): ", header_cache.size());

    bool exists = false;
    try {
        for (auto&& [height, header] : header_cache) {
            exists = db_header_->contains(height);
            auto const [it, ins] = db_header_->emplace(height, std::move(header));
        }
    } catch (boost::interprocess::bad_alloc const& ex) {
        LOG_ERROR(LOG_DATABASE, "Error inserting block header [persist_headers_internal] exists: ", exists);
        throw;
    } catch (std::exception const& ex) {
        LOG_ERROR(LOG_DATABASE, "Error inserting block header [persist_headers_internal] exists: ", exists);
        throw;
    } catch (...) {
        LOG_ERROR(LOG_DATABASE, "Error inserting block header [persist_headers_internal] exists: ", exists);
        throw;
    }

    LOG_INFO(LOG_DATABASE, "persist_headers_internal() - Saving Headers by Hash - header_by_hash_cache.size(): ", header_by_hash_cache.size());

    exists = false;
    try {
        for (auto const& [hash_const, height] : header_by_hash_cache) {
            exists = db_header_by_hash_->contains(hash_const);
            auto const [it, inserted] = db_header_by_hash_->emplace(hash_const, height);
        }
    } catch (boost::interprocess::bad_alloc const& ex) {
        LOG_ERROR(LOG_DATABASE, "Error inserting block header by hash [persist_headers_internal] exists: ", exists);
        throw;
    } catch (std::exception const& ex) {
        LOG_ERROR(LOG_DATABASE, "Error inserting block header by hash [persist_headers_internal] exists: ", exists);
        throw;
    } catch (...) {
        LOG_ERROR(LOG_DATABASE, "Error inserting block header by hash [persist_headers_internal] exists: ", exists);
        throw;
    }

    LOG_INFO(LOG_DATABASE, "persist_headers_internal() END");
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::persist_all_internal(
    uint32_t last_height,
    header_db_t& header_cache,
    header_by_hash_db_t& header_by_hash_cache,
    utxo_db_t& utxoset,
    utxo_to_remove_t& utxo_to_remove
) {
    // LOG_INFO(LOG_DATABASE, "persist_all_internal() START ************", " - ThreadId: ", std::this_thread::get_id());

    // auto res = persist_headers_internal(last_height, header_cache, header_by_hash_cache);
    // if (res != result_code::success) {
    //     return res;
    // }

    // res = persist_utxo_set_internal(utxoset, utxo_to_remove);
    // if (res != result_code::success) {
    //     return res;
    // }

    // LOG_INFO(LOG_DATABASE, "persist_all_internal() END ************", " - ThreadId: ", std::this_thread::get_id());
    return result_code::success;
}

constexpr
bool is_too_old(uint32_t last_height, uint32_t too_old, uint32_t height) {
    return height == 0 || (last_height >= too_old && height < last_height - too_old);
}

template <typename Clock>
result_code internal_database_basis<Clock>::persist_all_background() {
    LOG_INFO(LOG_DATABASE,
        "pab() - utxo_set_cache_.size(): " , utxo_set_cache_.size(),
        " - utxo_to_remove_cache_.size(): ", utxo_to_remove_cache_.size(),
        " - too_old_: ", too_old_,
        " - last_height_: ", last_height_,
        " - remove_utxo_not_found_in_cache: ", remove_utxo_not_found_in_cache,
        " - remove_uxto_found_in_cache: ", remove_uxto_found_in_cache);

    // header_db_t header_cache_;
    // header_by_hash_db_t header_by_hash_cache_;


    cold_last_height_ = last_height_.load();
    if (cold_last_height_ != 0) {
        cold_last_height_ -= too_old_;
        --cold_last_height_;
    }

    LOG_INFO(LOG_DATABASE, "pab() - cold_last_height_: ", cold_last_height_);

    for (auto const& [height, header] : header_cache_) {
        // if (height == 0 || (too_old_ >= last_height_ && height < last_height_ - too_old_)) {
        if (is_too_old(last_height_, too_old_, height)) {
            // LOG_INFO(LOG_DATABASE, "pab() - height: ", height, " is too old, moving to cold_header_cache_");
            cold_header_cache_.emplace(height, header);
            // header_cache_.erase(height);     //TODO: do not remove headers
        // } else {
        //     LOG_INFO(LOG_DATABASE, "pab() - height: ", height, " is not too old, so NOT copying to cold_header_cache_");
        }
    }

    for (auto const& [hash_const, height] : header_by_hash_cache_) {
        // if (height == 0 || (too_old_ >= last_height_ && height < last_height_ - too_old_)) {
        if (is_too_old(last_height_, too_old_, height)) {
            // LOG_INFO(LOG_DATABASE, "pab() - height: ", height, " is too old, moving to cold_header_by_hash_cache_");
            cold_header_by_hash_cache_.emplace(hash_const, height);
            // header_by_hash_cache_.erase(hash_const);     //TODO: do not remove headers
        }
    }

    for (auto const& [point, entry] : utxo_set_cache_) {
        // if (entry.height() == 0 || entry.height() < last_height_ - too_old_) {
        if (is_too_old(last_height_, too_old_, entry.height())) {
            // LOG_INFO(LOG_DATABASE, "pab() - entry.height(): ", entry.height(), " is too old, moving to cold_utxo_set_cache_");
            // cold_utxo_set_cache_.emplace(point, std::move(entry));
            cold_utxo_set_cache_.emplace(point, entry);
            utxo_set_cache_.erase(point);
        }
    }

    for (auto const& [point, height] : utxo_to_remove_cache_) {
        // if (height == 0 || height < last_height_ - too_old_) {
        if (is_too_old(last_height_, too_old_, height)) {
            // LOG_INFO(LOG_DATABASE, "pab() - height: ", height, " is too old, moving to cold_utxo_to_remove_cache_");
            cold_utxo_to_remove_cache_.emplace(point, height);
            utxo_to_remove_cache_.erase(point);
        }
    }

    LOG_INFO(LOG_DATABASE, "pab() ",
        " - TooOldElements: ", cold_utxo_set_cache_.size(),
        " - utxo_set_cache_.size(): ", utxo_set_cache_.size(),
        " - utxo_to_remove_cache_.size(): ", utxo_to_remove_cache_.size(),
        " - cold_utxo_to_remove_cache_.size(): ", cold_utxo_to_remove_cache_.size(),
        " - cold_utxo_set_cache_.size(): ", cold_utxo_set_cache_.size(),
        " - cold_header_cache_.size(): ", cold_header_cache_.size(),
        " - cold_header_by_hash_cache_.size(): ", cold_header_by_hash_cache_.size());

    data_ready_.store(true, std::memory_order_release);

    LOG_INFO(LOG_DATABASE, "pab() - AFTER data_ready_.store(true)");

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::load_utxo_set_internal() {
    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() START ************ - last_height_: ", last_height_);
    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - db_utxo_->size(): ", db_utxo_->size());

    // utxo_set_cache_.insert(db_utxo_->begin(), db_utxo_->end());
    size_t too_old_utxo = 0;
    size_t new_utxo = 0;
    int64_t min_height = std::numeric_limits<int64_t>::max();
    int64_t max_height = std::numeric_limits<int64_t>::min();

    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.size(): ", utxo_set_cache_.size());

    utxo_db_t utxo_set_cache_tmp;

    for (auto it = db_utxo_->begin(); it != db_utxo_->end(); ++it) {
        auto const& point = it->first;
        auto const& entry = it->second;

        // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - entry.output().script().serialized_size(): ", entry.output().script().serialized_size(true));

        // if (entry.height() < min_height) {
        //     min_height = entry.height();
        // }
        // if (entry.height() > max_height) {
        //     max_height = entry.height();
        // }

        if (last_height_ < too_old_ || entry.height() >= last_height_ - too_old_) {
            // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.emplace(point, entry)");
            // utxo_set_cache_.emplace(domain::chain::point{}, utxo_entry{});
            // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.emplace(point, entry)");
            auto point2 = point;
            auto entry2 = entry;
            utxo_set_cache_tmp.emplace(point2, entry2);

            // utxo_set_cache_.emplace(point, entry);

            // utxo_set_cache_.emplace(point, utxo_entry{});
            // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.emplace(point, entry)");
            // utxo_set_cache_.emplace(domain::chain::point{}, utxo_entry{});
            // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.emplace(point, entry)");
            ++new_utxo;
        } else {
            // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - not inserting into the cache");
            ++too_old_utxo;
        }
    }

    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_tmp.size(): ", utxo_set_cache_tmp.size());
    utxo_set_cache_ = utxo_set_cache_tmp;
    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.size(): ", utxo_set_cache_.size());

    // auto const& utxo_ref = *db_utxo_;
    // for (auto const& [point, entry] : utxo_ref) {
    //     // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - entry.height(): ", entry.height(), " - point.index(): ", point.index());

    //     // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - point.hash():   ", encode_hash(point.hash()));
    //     // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - point.index():  ", point.index());
    //     // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - entry.height(): ", entry.height());
    //     // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - entry.median_time_past(): ", entry.median_time_past());
    //     // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - entry.coinbase(): ", entry.coinbase());
    //     // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - entry.output().value(): ", entry.output().value());
    //     LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - entry.output().script().serialized_size(): ", entry.output().script().serialized_size(true));

    //     if (entry.height() < min_height) {
    //         min_height = entry.height();
    //     }
    //     if (entry.height() > max_height) {
    //         max_height = entry.height();
    //     }

    //     if (last_height_ < too_old_ || entry.height() >= last_height_ - too_old_) {
    //         // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.emplace(point, entry)");
    //         utxo_set_cache_.emplace(domain::chain::point{}, utxo_entry{});
    //         // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.emplace(point, entry)");
    //         utxo_set_cache_.emplace(point, entry);
    //         // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.emplace(point, entry)");
    //         // utxo_set_cache_.emplace(domain::chain::point{}, utxo_entry{});
    //         // LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.emplace(point, entry)");
    //         ++new_utxo;
    //     } else {
    //         LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - not inserting into the cache");
    //         ++too_old_utxo;
    //     }

    //     // utxo_set_cache_.emplace(point, entry);

    //     // // if (entry.height() == 0 || entry.height() >= last_height_ - too_old_) {
    //     // if (is_too_old(last_height_, too_old_, entry.height())) {
    //     //     utxo_set_cache_.emplace(point, entry);
    //     //     ++new_utxo;
    //     // } else {
    //     //     ++too_old_utxo;
    //     // }
    // }

    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - utxo_set_cache_.size(): ", utxo_set_cache_.size());
    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - too_old_utxo: ", too_old_utxo);
    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - new_utxo: ", new_utxo);
    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - min_height: ", min_height);
    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() - max_height: ", max_height);

    LOG_INFO(LOG_DATABASE, "load_utxo_set_internal() END ************");

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::load_headers_internal() {
    LOG_INFO(LOG_DATABASE, "load_headers_internal() START ************");

    last_height_ = *db_last_height_;

    LOG_INFO(LOG_DATABASE, "load_headers_internal() - Last Height: ", last_height_);


    auto const db_header_max_it = std::max_element(db_header_->begin(), db_header_->end(), [](auto const& a, auto const& b) {
        return a.first < b.first;
    });
    if (db_header_max_it != db_header_->end()) {
        auto const db_header_max_ = db_header_max_it->first;
        LOG_INFO(LOG_DATABASE, "load_headers_internal() - db_header_max_: ", db_header_max_);
    }

    auto const db_header_by_hash_max_it = std::max_element(db_header_by_hash_->begin(), db_header_by_hash_->end(), [](auto const& a, auto const& b) {
        return a.second < b.second;
    });
    if (db_header_by_hash_max_it != db_header_by_hash_->end()) {
        auto const db_header_by_hash_max_ = db_header_by_hash_max_it->second;
        LOG_INFO(LOG_DATABASE, "load_headers_internal() - db_header_by_hash_max_: ", db_header_by_hash_max_);
    }









    header_cache_.insert(db_header_->begin(), db_header_->end());

    LOG_INFO(LOG_DATABASE, "load_headers_internal() - header_cache_.size(): ", header_cache_.size());

    header_by_hash_cache_.insert(db_header_by_hash_->begin(), db_header_by_hash_->end());

    LOG_INFO(LOG_DATABASE, "load_headers_internal() - header_by_hash_cache_.size(): ", header_by_hash_cache_.size());


    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::load_all_internal() {
    {
        auto const res = load_headers_internal();
        if (res != result_code::success) {
            return res;
        }
    }

    {
        auto const res = load_utxo_set_internal();
        if (res != result_code::success) {
            return res;
        }
    }
    // return result_code::other;
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::load_all() {
    LOG_INFO(LOG_DATABASE, "load_all() START ************");

    auto res = load_all_internal();
    if (res != result_code::success) {
        return res;
    }

    LOG_INFO(LOG_DATABASE, "load_all() END ************");
    return result_code::success;
}

constexpr
bool has_to_persist(uint32_t height, uint32_t last_persisted_height) {
    if (height < 100'000) return false;
    if (height < 200'000) {
        return height - last_persisted_height >= 50'000;
    }
    if (height < 300'000) {
        return height - last_persisted_height >= 25'000;
    }
    return height - last_persisted_height >= 10'000;
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past) {

    // std::cout << "push_block() - Current Thread ID: " << std::this_thread::get_id() << std::endl;

    // TODO(fernando): use a proper metric to decide when to persist
    // if (utxo_set_cache_.size() >= cache_capacity_ || has_to_persist(height, last_persisted_height_)) {
    if (has_to_persist(height, last_persisted_height_)) {

        LOG_INFO(LOG_DATABASE, "push_block() ",
            " - utxo_set_cache_.size(): ", utxo_set_cache_.size(),
            " - cache_capacity_: ", cache_capacity_,
            " - height: ", height,
            " - last_persisted_height_: ", last_persisted_height_,
            utxo_set_cache_.size() >= cache_capacity_ ? " - UTXO Cache Full" : "");

        bool is_saving = is_saving_.load(std::memory_order_acquire);

        // if (is_saving) {
        //     LOG_WARNING(LOG_DATABASE, "push_block()  - persisting utxo set - trying to persist while persister thread is saving");
        // }
        // auto const ec = persist_all_background();
        // if (ec != result_code::success) {
        //     return ec;
        // }
        // last_persisted_height_ = height;
        // LOG_INFO(LOG_DATABASE, "push_block()  - persisting utxo set - END");

        if ( ! is_saving) {
            auto const ec = persist_all_background();
            if (ec != result_code::success) {
                return ec;
            }
            last_persisted_height_ = height;
            LOG_INFO(LOG_DATABASE, "push_block()  - persisting utxo set - END");
        } else {
            LOG_INFO(LOG_DATABASE, "push_block()  - persisting utxo set - SKIPPED because persister thread is saving");
        }
    }

    //TODO: save reorg blocks after the last checkpoint
    auto res = push_block(block, height, median_time_past, ! is_old_block(block));
    if ( !  succeed(res)) {
        return res;
    }

    return res;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
utxo_entry internal_database_basis<Clock>::get_utxo(domain::chain::output_point const& point) const {
    auto const it = utxo_set_cache_.find(point);
    if (it == utxo_set_cache_.end()) {
        return utxo_entry{};
    }

    auto const& tmp = it->second;
    return utxo_entry{
        from_persistent_data(tmp.output_bytes()),
        tmp.height(),
        tmp.median_time_past(),
        tmp.coinbase()
    };
}

template <typename Clock>
result_code internal_database_basis<Clock>::get_last_height(uint32_t& out_height) const {
    out_height = last_height_;
    return result_code::success;
}

template <typename Clock>
domain::chain::header::list internal_database_basis<Clock>::get_headers(uint32_t from, uint32_t to) const {
    // precondition: from <= to
    domain::chain::header::list list;

    for (auto const [height, header] : header_cache_) {
        if (height < from) continue;
        if (height > to) continue;
        list.push_back(header);
    }

    // Concurrent
    // header_cache_.cvisit_all([&list, from, to](auto const& p) {
    //     if (p.first < from) return;
    //     if (p.first > to) return;
    //     list.push_back(p.second);
    // });

    return list;
}
#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::pop_block(domain::chain::block& out_block) {
    uint32_t height;

    //TODO: (Mario) use only one transaction ?

    //TODO: (Mario) add overload with tx
    // The blockchain is empty (nothing to pop, not even genesis).
    auto res = get_last_height(height);
    if (res != result_code::success ) {
        return res;
    }

    //TODO: (Mario) add overload with tx
    // This should never become invalid if this call is protected.
    out_block = get_block_from_reorg_pool(height);
    if ( ! out_block.is_valid()) {
        return result_code::key_not_found;
    }

    res = remove_block(out_block, height);
    if (res != result_code::success) {
        return res;
    }

    return result_code::success;
}


template <typename Clock>
result_code internal_database_basis<Clock>::prune() {
    LOG_INFO(LOG_DATABASE, "prune() - START ************");

    uint32_t last_height;
    auto res = get_last_height(last_height);

    if (res == result_code::db_empty) return result_code::no_data_to_prune;
    if (res != result_code::success) return res;
    if (last_height < reorg_pool_limit_) return result_code::no_data_to_prune;

    uint32_t first_height;
    res = get_first_reorg_block_height(first_height);
    if (res == result_code::db_empty) return result_code::no_data_to_prune;
    if (res != result_code::success) return res;
    if (first_height > last_height) return result_code::db_corrupt;

    auto reorg_count = last_height - first_height + 1;
    if (reorg_count <= reorg_pool_limit_) return result_code::no_data_to_prune;

    //TODO(fernando): amount is not a good name
    auto amount_to_delete = reorg_count - reorg_pool_limit_;
    auto remove_until = first_height + amount_to_delete;

    res = prune_reorg_block(amount_to_delete);
    if (res != result_code::success) {
        return res;
    }

    res = prune_reorg_index(remove_until);
    if (res != result_code::success) {
        return res;
    }

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)


template <typename Clock>
result_code internal_database_basis<Clock>::insert_reorg_into_pool(utxo_pool_t& pool, domain::chain::output_point const& point) const {

    auto it = reorg_pool_.find(point);
    if (it == reorg_pool_.end()) {
        LOG_INFO(LOG_DATABASE, "Key not found in reorg pool [insert_reorg_into_pool]");
        return result_code::key_not_found;
    }
    pool[point] = it->second;
    return result_code::success;
}



template <typename Clock>
std::pair<result_code, utxo_pool_t> internal_database_basis<Clock>::get_utxo_pool_from(uint32_t from, uint32_t to) const {
    // precondition: from <= to
    utxo_pool_t pool;

    for(auto const& index_entry : reorg_index_) {
        if (index_entry.first < from || index_entry.first > to) {
            continue;
        }

        auto const& point_list = index_entry.second;
        for (auto const& point : point_list) {
            auto res = insert_reorg_into_pool(pool, point);
            if (res != result_code::success) {
                return {res, pool};
            }
        }
    }

    return {result_code::success, pool};
}


#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::push_transaction_unconfirmed(domain::chain::transaction const& tx, uint32_t height) {

    auto res = insert_transaction_unconfirmed(tx, height);
    if (res != result_code::success) {
        return res;
    }

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

// Private functions
// ------------------------------------------------------------------------------------------------------

template <typename Clock>
bool internal_database_basis<Clock>::is_old_block(domain::chain::block const& block) const {
    return is_old_block_<Clock>(block, limit_);
}

template <typename Clock>
size_t internal_database_basis<Clock>::get_db_page_size() const {
    return boost::interprocess::mapped_region::get_page_size();
}

inline
std::filesystem::path get_db_path(std::filesystem::path const& db_dir, std::string const& db_name) {
    std::filesystem::path db_path(db_dir);
    db_path /= db_name;
    db_path.replace_extension(".bin");
    return db_path;
}

template <typename Clock>
bool internal_database_basis<Clock>::open_databases() {
    using namespace boost::interprocess;
    using namespace std::literals;

    auto open_db = [&](auto const& db_name, size_t size, size_t bucket_count, auto*& db_pointer, managed_mapped_file& segment) {
        using MapType = std::remove_pointer_t<std::remove_reference_t<decltype(db_pointer)>>;

        auto const db_path = get_db_path(db_dir_, db_name);

        LOG_INFO(LOG_DATABASE, "Opening database: ", db_path.string(), " [open_databases] ");

        segment = managed_mapped_file(open_or_create, db_path.c_str(), size);
        auto alloc = segment.get_segment_manager();
        auto* db = segment.find_or_construct<MapType>(db_name)(bucket_count, boost::hash<typename MapType::key_type>(), std::equal_to<typename MapType::key_type>(), alloc);
        if ( ! db) {
            LOG_ERROR(LOG_DATABASE, "Error opening database: ", db_name, " [open_databases] ");
            return false;
        }

        db_pointer = db;

        return true;
    };

    //TODO(fernando): parametrize the bucket count
    if ( ! open_db("utxo",           db_utxo_size_,       100'000'000, db_utxo_, segment_utxo_)) return false;
    if ( ! open_db("header",         db_header_size_,         900'000, db_header_, segment_header_)) return false;
    if ( ! open_db("header_by_hash", db_header_by_hash_size_, 900'000, db_header_by_hash_, segment_header_by_hash_)) return false;

    auto const last_height_path = get_db_path(db_dir_, "last_height");
    // segment_last_height_ = managed_mapped_file(open_or_create, last_height_path.c_str(), sizeof(uint32_t));
    segment_last_height_ = managed_mapped_file(open_or_create, last_height_path.c_str(), 65'536);
    db_last_height_ = segment_last_height_.find_or_construct<uint32_t>("lastHeight")();

    return true;
}

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::remove_inputs(hash_digest const& tx_id, uint32_t height, domain::chain::input::list const& inputs, bool insert_reorg) {
    uint32_t pos = 0;
    for (auto const& input: inputs) {
        domain::chain::input_point const inpoint {tx_id, pos};
        auto const& prevout = input.previous_output();

        if (db_mode_ == db_mode_type::full) {
            auto res = insert_input_history(inpoint, height, input);
            if (res != result_code::success) {
                return res;
            }
        }

        auto res = remove_utxo(height, prevout, insert_reorg);
        if (res != result_code::success) {
            return res;
        }

        if (db_mode_ == db_mode_type::full) {
            res = insert_spend(prevout, inpoint);
            if (res != result_code::success) {
                return res;
            }
        }

        ++pos;
    }
    return result_code::success;
}

template <typename Clock>
// result_code internal_database_basis<Clock>::insert_outputs(hash_digest const& tx_id, uint32_t height, domain::chain::output::list const& outputs, data_chunk const& fixed_data) {
result_code internal_database_basis<Clock>::insert_outputs(hash_digest const& tx_id, uint32_t height, domain::chain::output::list const& outputs, uint32_t median_time_past, bool coinbase) {
    uint32_t pos = 0;
    for (auto const& output: outputs) {
        // auto res = insert_utxo(domain::chain::point{tx_id, pos}, output, fixed_data);
        auto res = insert_utxo(domain::chain::point{tx_id, pos}, output, height, median_time_past, coinbase);
        if (res != result_code::success) {
            return res;
        }

        if (db_mode_ == db_mode_type::full) {
            res = insert_output_history(tx_id, height, pos, output);
            if (res != result_code::success) {
                return res;
            }
        }
        ++pos;
    }
    return result_code::success;
}

template <typename Clock>
// result_code internal_database_basis<Clock>::insert_outputs_error_treatment(uint32_t height, data_chunk const& fixed_data, hash_digest const& txid, domain::chain::output::list const& outputs) {
result_code internal_database_basis<Clock>::insert_outputs_error_treatment(uint32_t height, uint32_t median_time_past, bool coinbase, hash_digest const& txid, domain::chain::output::list const& outputs) {
    // auto res = insert_outputs(txid, height, outputs, fixed_data);
    auto res = insert_outputs(txid, height, outputs, median_time_past, coinbase);
    if (res == result_code::duplicated_key) {
        //TODO(fernando): log and continue
        return result_code::success_duplicate_coinbase;
    }
    return res;
}

template <typename Clock>
template <typename I>
//result_code internal_database_basis<Clock>::push_transactions_outputs_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l) {
result_code internal_database_basis<Clock>::push_transactions_outputs_non_coinbase(uint32_t height, uint32_t median_time_past, I f, I l) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        // auto res = insert_outputs_error_treatment(height, fixed_data, tx.hash(), tx.outputs());
        auto res = insert_outputs_error_treatment(height, median_time_past, false, tx.hash(), tx.outputs());
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }
    return result_code::success;
}

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::remove_transactions_inputs_non_coinbase(uint32_t height, I f, I l, bool insert_reorg) {
    while (f != l) {
        auto const& tx = *f;
        auto res = remove_inputs(tx.hash(), height, tx.inputs(), insert_reorg);
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }
    return result_code::success;
}

template <typename Clock>
template <typename I>
// result_code internal_database_basis<Clock>::push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg) {
result_code internal_database_basis<Clock>::push_transactions_non_coinbase(uint32_t height, uint32_t median_time_past, I f, I l, bool insert_reorg) {

    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    // auto res = push_transactions_outputs_non_coinbase(height, fixed_data, f, l);
    auto res = push_transactions_outputs_non_coinbase(height, median_time_past, f, l);
    if (res != result_code::success) {
        return res;
    }

    return remove_transactions_inputs_non_coinbase(height, f, l, insert_reorg);
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg) {
    //precondition: block.transactions().size() >= 1

    auto res = push_block_header(block, height);
    if (res != result_code::success) {
        return res;
    }

    auto const& txs = block.transactions();

    if (db_mode_ == db_mode_type::full) {
        auto tx_count = get_tx_count();

        res = insert_block(block, height, tx_count);
        if (res != result_code::success) {
            return res;
        }

        res = insert_transactions(txs.begin(), txs.end(), height, median_time_past, tx_count);
        if (res == result_code::duplicated_key) {
            res = result_code::success_duplicate_coinbase;
        } else if (res != result_code::success) {
            return res;
        }
    } else if (db_mode_ == db_mode_type::blocks) {
        res = insert_block(block, height, 0);
        if (res != result_code::success) {
            return res;
        }
    }

    if (insert_reorg) {
        res = push_block_reorg(block, height);
        if (res != result_code::success) {
            return res;
        }
    }

    auto const& coinbase = txs.front();

    // auto fixed = utxo_entry::to_data_fixed(height, median_time_past, true);
    // auto res0 = insert_outputs_error_treatment(height, fixed, coinbase.hash(), coinbase.outputs());
    auto res0 = insert_outputs_error_treatment(height, median_time_past, true, coinbase.hash(), coinbase.outputs());
    if ( ! succeed(res0)) {
        return res0;
    }

    // fixed.back() = 0;
    // res = push_transactions_non_coinbase(height, fixed, txs.begin() + 1, txs.end(), insert_reorg);

    res = push_transactions_non_coinbase(height, median_time_past, txs.begin() + 1, txs.end(), insert_reorg);
    if (res != result_code::success) {
        return res;
    }

    if (res == result_code::success_duplicate_coinbase)
        return res;

    return res0;
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_genesis(domain::chain::block const& block) {

    LOG_INFO(LOG_DATABASE, "Pushing genesis block [push_genesis]");


    auto res = push_block_header(block, 0);
    if (res != result_code::success) {
        return res;
    }

    auto tmp = get_header(0);
    if ( ! tmp.is_valid()) {
        LOG_ERROR(LOG_DATABASE, "Error pushing genesis block in Temporal code [push_genesis]");
    }

    if (db_mode_ == db_mode_type::full) {
        auto tx_count = get_tx_count();
        res = insert_block(block, 0, tx_count);

        if (res != result_code::success) {
            return res;
        }

        auto const& txs = block.transactions();
        auto const& coinbase = txs.front();
        auto const& hash = coinbase.hash();
        auto const median_time_past = block.header().validation.median_time_past;

        res = insert_transaction(tx_count, coinbase, 0, median_time_past, 0);
        if (res != result_code::success && res != result_code::duplicated_key) {
            return res;
        }

        res = insert_output_history(hash, 0, 0, coinbase.outputs()[0]);
        if (res != result_code::success) {
            return res;
        }
    } else if (db_mode_ == db_mode_type::blocks) {
        res = insert_block(block, 0, 0);
    }

    return res;
}

template <typename Clock>
// result_code internal_database_basis<Clock>::remove_outputs(hash_digest const& txid, domain::chain::output::list const& outputs) {
result_code internal_database_basis<Clock>::remove_outputs(hash_digest const& txid, domain::chain::output::list const& outputs) {
    uint32_t pos = outputs.size() - 1;
    for (auto const& output: outputs) {
        domain::chain::output_point const point {txid, pos};
        auto res = remove_utxo(0, point, false);
        if (res != result_code::success) {
            return res;
        }
        --pos;
    }
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_inputs(domain::chain::input::list const& inputs) {
    for (auto const& input: inputs) {
        auto const& point = input.previous_output();
        auto res = insert_output_from_reorg_and_remove(point);
        if (res != result_code::success) {
            return res;
        }
    }
    return result_code::success;
}


template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::insert_transactions_inputs_non_coinbase(I f, I l) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        auto res = insert_inputs(tx.inputs());
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }

    return result_code::success;
}



template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::remove_transactions_outputs_non_coinbase(I f, I l) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        auto res = remove_outputs(tx.hash(), tx.outputs());
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }

    return result_code::success;
}

template <typename Clock>
template <typename I>
// result_code internal_database_basis<Clock>::remove_transactions_non_coinbase(I f, I l) {
result_code internal_database_basis<Clock>::remove_transactions_non_coinbase(I f, I l) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    auto res = insert_transactions_inputs_non_coinbase(f, l);
    if (res != result_code::success) {
        return res;
    }
    return remove_transactions_outputs_non_coinbase(f, l);
}


template <typename Clock>
result_code internal_database_basis<Clock>::remove_block(domain::chain::block const& block, uint32_t height) {
    //precondition: block.transactions().size() >= 1

    auto const& txs = block.transactions();
    auto const& coinbase = txs.front();

    auto res = remove_transactions_non_coinbase(txs.begin() + 1, txs.end());
    if (res != result_code::success) {
        return res;
    }

    //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
    res = remove_outputs(coinbase.hash(), coinbase.outputs());
    if (res != result_code::success) {
        return res;
    }

    //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
    res = remove_block_header(block.hash(), height);
    if (res != result_code::success) {
        return res;
    }

    res = remove_block_reorg(height);
    if (res != result_code::success) {
        return res;
    }

    res = remove_reorg_index(height);
    if (res != result_code::success && res != result_code::key_not_found) {
        return res;
    }

    if (db_mode_ == db_mode_type::full) {
        //Transaction Database
        res = remove_transactions(block, height);
        if (res != result_code::success) {
            return res;
        }
    }

    if (db_mode_ == db_mode_type::full || db_mode_ == db_mode_type::blocks) {
        res = remove_blocks_db(height);
        if (res != result_code::success) {
            return res;
        }
    }

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_INTERNAL_DATABASE_IPP_

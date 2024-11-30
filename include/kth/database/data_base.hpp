// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_DATA_BASE_HPP
#define KTH_DATABASE_DATA_BASE_HPP

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <memory>

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/databases/internal_database.hpp>
#include <kth/database/define.hpp>
#include <kth/database/settings.hpp>
#include <kth/database/store.hpp>

#include <kth/infrastructure/handlers.hpp>
#include <kth/infrastructure/utility/noncopyable.hpp>
#include <kth/infrastructure/utility/dispatcher.hpp>

#include <kth/infrastructure/utility/timer.hpp>

namespace kth::database {

//TODO: remove from here
inline
time_t floor_subtract_times(time_t left, time_t right) {
    static auto const floor = (std::numeric_limits<time_t>::min)();
    return right >= left ? floor : left - right;
}

/// This class is thread safe and implements the sequential locking pattern.
class KD_API data_base : public store, noncopyable {
public:
    using handle = store::handle;
    using result_handler = handle0;
    using path = kth::path;

    using height_t = uint32_t;
    using header_t = domain::chain::header;
    using header_with_height_t = std::pair<header_t, uint32_t>;
    using block_t = domain::chain::block;
    using block_with_height_t = std::pair<block_t, uint32_t>;
    using header_with_abla_state_t = std::tuple<header_t, uint64_t, uint64_t, uint64_t>;

    struct block_cache_t {
        using transaction_t = domain::chain::transaction;
        using blocks_t = std::vector<block_t>;
        using hash_to_height_map_t = boost::unordered_flat_map<hash_digest, size_t>;
        // using point_t = domain::chain::output_point;
        using point_t = domain::chain::point;
        using utxo_map_t = boost::unordered_flat_map<point_t, utxo_entry>;
        using utxo_set_t = boost::unordered_flat_set<point_t>;

        constexpr static auto max_height = std::numeric_limits<height_t>::max();

        block_cache_t(size_t max_size, internal_database* db)
            : max_size_(max_size)
            , db_(db)
        {
            // LOG_INFO(LOG_DATABASE, "block_cache_t - ", __func__);
        }

        ~block_cache_t() {
            // LOG_INFO(LOG_DATABASE, "block_cache_t - ", __func__);
            flush_to_db();
            db_ = nullptr;
        }

        // Queries
        // ------------------------------------------------------------------------------

        expect<height_t> last_height() const {
            if (start_height_ == max_height) {
                return make_unexpected(error::empty_cache);
            }
            return start_height_ + blocks_.size() - 1;
        }

        //TODO: test it
        bool is_height_in_cache(height_t height) const {
            return height >= start_height_ && height < start_height_ + blocks_.size();
        }

        expect<block_t> get_block(height_t height) const {
            if ( ! is_height_in_cache(height)) {
                return make_unexpected(error::height_not_found);
            }
            return blocks_[height_to_position(height)];
        }

        expect<block_with_height_t> get_block(hash_digest const& hash) const {
            auto const it = hash_to_height_map_.find(hash);
            if (it == hash_to_height_map_.end()) {
                return make_unexpected(error::hash_not_found);
            }
            auto const pos = it->second;
            return block_with_height_t{blocks_[pos], position_to_height(pos)};
        }

        expect<header_t> get_header(height_t height) const {
            if ( ! is_height_in_cache(height)) {
                return make_unexpected(error::height_not_found);
            }
            return blocks_[height_to_position(height)].header();
        }

       expect<header_with_abla_state_t> get_header_and_abla_state(height_t height) const {
            if ( ! is_height_in_cache(height)) {
                return make_unexpected(error::height_not_found);
            }
            auto const& blk = blocks_[height_to_position(height)];
            if ( ! blk.validation.state) {
                return header_with_abla_state_t{blk.header(), 0, 0, 0};
            }
            return header_with_abla_state_t{
                blk.header(),
                blk.validation.state->abla_state().block_size,
                blk.validation.state->abla_state().control_block_size,
                blk.validation.state->abla_state().elastic_buffer_size
            };
        }

        expect<header_with_height_t> get_header(hash_digest const& hash) const {
            auto const it = hash_to_height_map_.find(hash);
            if (it == hash_to_height_map_.end()) {
                return make_unexpected(error::hash_not_found);
            }
            auto const pos = it->second;
            return header_with_height_t{blocks_[pos].header(), position_to_height(pos)};
        }

        expect<utxo_entry> get_utxo(point_t const& point) const {
            auto const it = utxo_map_.find(point);
            if (it == utxo_map_.end()) {
                return make_unexpected(error::utxo_not_found);
            }
            return it->second;
        }

        // Commands
        // ------------------------------------------------------------------------------
        void add_block(block_t const& blk, height_t height) {
            blocks_.emplace_back(blk);
            hash_to_height_map_.emplace(blk.hash(), blocks_.size() - 1);
            process_utxos(blk, height);

            if (start_height_ == max_height) {
                start_height_ = height;
            }

            if (blocks_.size() >= max_size_) {
                flush_to_db();
            }
        }

        void add_block(block_t&& blk, height_t height) {
            blocks_.emplace_back(std::move(blk));
            hash_to_height_map_.emplace(blk.hash(), blocks_.size() - 1);
            process_utxos(blocks_.back(), height);

            if (start_height_ == max_height) {
                start_height_ = height;
            }

            if (blocks_.size() >= max_size_) {
                flush_to_db();
            }
        }

        // void add_blocks(blocks_t const& blocks, height_t height) {
        //     // LOG_INFO(LOG_DATABASE, "block_cache_t - ", __func__);
        //     blocks_.insert(blocks_.end(), blocks.begin(), blocks.end());

        //     for (size_t le = 0; i < blocks.size(); ++i) {
        //         hash_to_height_map_.emplace(blocks[i].hash(), blocks_.size() - 1 - i);
        //     }

        //     if (start_height_ == max_height) {
        //         start_height_ = height;
        //     }

        //     if (blocks_.size() >= max_size_) {
        //         flush_to_db();
        //     }
        // }

        // void add_blocks(blocks_t&& blocks, height_t height) {
        //     // LOG_INFO(LOG_DATABASE, "block_cache_t - ", __func__);
        //     blocks_.insert(
        //         blocks_.end(),
        //         std::make_move_iterator(blocks.begin()),
        //         std::make_move_iterator(blocks.end())
        //     );

        //     for (size_t i = 0; i < blocks.size(); ++i) {
        //         hash_to_height_map_.emplace(blocks[i].hash(), blocks_.size() - 1 - i);

        //     }

        //     if (start_height_ == max_height) {
        //         start_height_ = height;
        //     }

        //     if (blocks_.size() >= max_size_) {
        //         flush_to_db();
        //     }
        // }



        bool is_stale() const {
            //TODO: Should be a config value
            constexpr time_t notify_limit_seconds = 6 * 60 * 60; // 6 hours
            //TODO: cache the last header

            uint32_t timestamp = 0;
            auto last_height_exp = last_height();
            if ( ! last_height_exp) {
                return true;
            }
            auto last_height = *last_height_exp;

            auto last_header_exp = get_header(last_height);
            if ( ! last_header_exp) {
                return true;
            }
            auto last_header = *last_header_exp;
            timestamp = last_header.timestamp();

            return timestamp < floor_subtract_times(zulu_time(), notify_limit_seconds);
        }

        void flush_to_db() {
            // LOG_INFO(LOG_DATABASE, "block_cache_t - ", __func__);
            if (blocks_.empty()) {
                return;
            }
            if ( ! db_) {
                LOG_ERROR(LOG_DATABASE, "Internal database not set, when trying to flush block cache");
                return;
            }
            // db_->push_blocks(blocks_, start_height_);

            // result_code push_blocks_and_utxos(
            //     std::vector<domain::chain::block> const& blocks,
            //     uint32_t height,
            //     utxo_pool_t const& utxos,
            //     utxos_to_remove_t const& utxos_to_remove,
            //     bool is_stale
            // );

            db_->push_blocks_and_utxos(blocks_, start_height_, utxo_map_, utxo_to_remove_, is_stale());

            reset();
        }

        void reset() {
            start_height_ = max_height;
            blocks_.clear();
            hash_to_height_map_.clear();
            utxo_to_remove_.clear();
            utxo_map_.clear();
        }

    private:
        size_t height_to_position(height_t height) const {
            return height - start_height_;
        }

        height_t position_to_height(size_t position) const {
            return start_height_ + position;
        }

        void insert_utxos_from_transaction(transaction_t const& tx, height_t height, uint32_t median_time_past, utxo_map_t& utxos) {
            for (size_t i = 0; i < tx.outputs().size(); ++i) {
                auto const& output = tx.outputs()[i];
                point_t point {tx.hash(), uint32_t(i)};
                utxos.emplace(point, utxo_entry{output, height, median_time_past, false});
            }
        }

        void process_utxos(block_t const& blk, height_t height) {
            // precondition: blk.transactions().size() > 0

            utxo_map_t block_utxos;

            // first insert the utxos, then remove the spent
            // utxo_entry(domain::chain::output output, uint32_t height, uint32_t median_time_past, bool coinbase);

            auto const median_time_past = blk.header().validation.median_time_past;
            auto const& coinbase_tx = blk.transactions().front();
            insert_utxos_from_transaction(coinbase_tx, height, median_time_past, block_utxos);

            for (size_t i = 1; i < blk.transactions().size(); ++i) {
                auto const& tx = blk.transactions()[i];
                insert_utxos_from_transaction(tx, height, median_time_past, block_utxos);
            }

            // Remove the spent utxos, try first in the block_utxos
            // If not found, try in the utxo_map_, in this case they also have
            // to be removed from the DB

            // The coinbase tx does not spend coins, so it is skipped
            for (size_t i = 1; i < blk.transactions().size(); ++i) {
                auto const& tx = blk.transactions()[i];

                // if ( ! tx.inputs().empty()) {
                    // LOG_INFO(LOG_DATABASE, "process_utxos() - height: ", height, " - spending: ", tx.inputs().size());
                // }
                for (auto const& input : tx.inputs()) {
                    auto const& point = input.previous_output();
                    auto count = block_utxos.erase(point);
                    // LOG_INFO(LOG_DATABASE, "process_utxos() - height: ", height, " - erasing utxo: ", encode_hash(point.hash()), ":", point.index(), " - count: ", count);
                    if (count == 0) {
                        count = utxo_map_.erase(point);
                        // LOG_INFO(LOG_DATABASE, "process_utxos() - height: ", height, " - erasing utxo: ", encode_hash(point.hash()), ":", point.index(), " - count: ", count);
                        if (count == 0) {
                            // LOG_INFO(LOG_DATABASE, "UTXO not found in block neither in cache, have to be in DB");
                            utxo_to_remove_.insert(point);
                        }
                    }
                }
            }

            utxo_map_.insert(block_utxos.begin(), block_utxos.end());
        }

        size_t max_size_;
        internal_database* db_ = nullptr;

        height_t start_height_ = max_height;
        blocks_t blocks_;
        hash_to_height_map_t hash_to_height_map_;
        utxo_map_t utxo_map_;
        utxo_set_t utxo_to_remove_;
    };


    // Construct.
    // ----------------------------------------------------------------------------

    data_base(settings const& settings);

    // Open and close.
    // ------------------------------------------------------------------------

#if ! defined(KTH_DB_READONLY)
    /// Create and open all databases.
    bool create(domain::chain::block const& genesis);
#endif

    /// Open all databases.
    bool open();

    /// Close all databases.
    bool close();

    /// Call close on destruct.
    ~data_base();


    bool is_stale() const;

    // Readers.
    // ------------------------------------------------------------------------

    expect<domain::chain::header> get_header(size_t height) const;
    expect<header_with_height_t> get_header(hash_digest const& hash) const;
    expect<header_with_abla_state_t> get_header_and_abla_state(size_t height) const;
    expect<block_t> get_block(size_t height) const;
    expect<block_with_height_t> get_block(hash_digest const& hash) const;
    expect<height_t> get_last_height() const;
    expect<utxo_entry> get_utxo(domain::chain::output_point const& point) const;

    internal_database const& internal_db() const;

    // Synchronous writers.
    // ------------------------------------------------------------------------

#if ! defined(KTH_DB_READONLY)
    /// Store a block in the database.
    /// Returns store_block_duplicate if a block already exists at height.
    code insert(domain::chain::block const& block, size_t height);

    /// Add an unconfirmed tx to the store (without indexing).
    /// Returns unspent_duplicate if existing unspent hash duplicate exists.
    code push(domain::chain::transaction const& tx, uint32_t forks);

    /// Returns store_block_missing_parent if not linked.
    /// Returns store_block_invalid_height if height is not the current top + 1.
    code push(domain::chain::block const& block, size_t height);

    code prune_reorg();

    //bool set_database_flags(bool fast);

    // Asynchronous writers.
    // ------------------------------------------------------------------------

    /// Invoke pop_all and then push_all under a common lock.
    void reorganize(infrastructure::config::checkpoint const& fork_point, block_const_ptr_list_const_ptr incoming_blocks, block_const_ptr_list_ptr outgoing_blocks, dispatcher& dispatch, result_handler handler);
#endif // ! defined(KTH_DB_READONLY)

protected:
    void start();

#if ! defined(KTH_DB_READONLY)

    // Sets error if first_height is not the current top + 1 or not linked.
    void push_all(block_const_ptr_list_const_ptr in_blocks, size_t first_height, dispatcher& dispatch, result_handler handler);

    // Pop the set of blocks above the given hash.
    // Sets error if the database is corrupt or the hash doesn't exist.
    // Any blocks returned were successfully popped prior to any failure.
    void pop_above(block_const_ptr_list_ptr out_blocks, hash_digest const& fork_hash, dispatcher& dispatch, result_handler handler);
#endif // ! defined(KTH_DB_READONLY)


    std::shared_ptr<internal_database> internal_db_;

private:
    using inputs = domain::chain::input::list;
    using outputs = domain::chain::output::list;

#if ! defined(KTH_DB_READONLY)
    code push_genesis(domain::chain::block const& block);

    // Synchronous writers.
    // ------------------------------------------------------------------------
    bool pop(domain::chain::block& out_block);
    bool pop_inputs(const inputs& inputs, size_t height);
    bool pop_outputs(const outputs& outputs, size_t height);

#endif // ! defined(KTH_DB_READONLY)

    code verify_insert(domain::chain::block const& block, size_t height);
    code verify_push(domain::chain::block const& block, size_t height) const;

    // Asynchronous writers.
    // ------------------------------------------------------------------------
#if ! defined(KTH_DB_READONLY)
    void push_next(code const& ec, block_const_ptr_list_const_ptr blocks, size_t index, size_t height, dispatcher& dispatch, result_handler handler);
    void do_push(block_const_ptr block, size_t height, uint32_t median_time_past, dispatcher& dispatch, result_handler handler);


    void handle_pop(code const& ec, block_const_ptr_list_const_ptr incoming_blocks, size_t first_height, dispatcher& dispatch, result_handler handler);
    void handle_push(code const& ec, result_handler handler) const;
#endif // ! defined(KTH_DB_READONLY)


    std::atomic<bool> closed_;
    settings const& settings_;
    std::optional<block_cache_t> block_cache_;
};

} // namespace kth::database

#endif

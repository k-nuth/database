// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/data_base.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <boost/range/adaptor/reversed.hpp>

#include <kth/database/define.hpp>
#include <kth/database/settings.hpp>
#include <kth/database/store.hpp>
#include <kth/domain.hpp>
// #include <kth/domain/math/limits.hpp>


namespace kth::database {

using namespace kth::domain::chain;
using namespace kth::config;
using namespace kth::domain::wallet;
using namespace boost::adaptors;
using namespace std::filesystem;
using namespace std::placeholders;

#define NAME "data_base"

// A failure after begin_write is returned without calling end_write.
// This purposely leaves the local flush lock (as enabled) and inverts the
// sequence lock. The former prevents usagage after restart and the latter
// prevents continuation of reads and writes while running. Ideally we
// want to exit without clearing the global flush lock (as enabled).

// Construct.
// ----------------------------------------------------------------------------

data_base::data_base(settings const& settings)
    : closed_(true)
    , settings_(settings)
    , store(settings.directory)
{}

data_base::~data_base() {
    close();

}

// Open and close.
// ----------------------------------------------------------------------------

#if ! defined(KTH_DB_READONLY)
// Throws if there is insufficient disk space, not idempotent.
bool data_base::create(block const& genesis) {
    start();

    // These leave the databases open.
    auto created = true
#if ! defined(KTH_DB_READONLY)
                && internal_db_->create()
#endif
    ;

    if ( ! created) {
        return false;
    }

    push_genesis(genesis);

    closed_ = false;
    return true;
}
#endif // ! defined(KTH_DB_READONLY)

// Must be called before performing queries, not idempotent.
// May be called after stop and/or after close in order to reopen.
bool data_base::open() {
    start();
    auto const opened = internal_db_->open();
    closed_ = false;
    return opened;
}

// Close is idempotent and thread safe.
// Optional as the database will close on destruct.
bool data_base::close() {
    if (closed_) {
        return true;
    }

    if (block_cache_) {
        LOG_INFO(LOG_DATABASE, "Flushing the block cache to DB...");
        block_cache_->flush_to_db();
        LOG_INFO(LOG_DATABASE, "Block cache flushed to DB.");

        LOG_INFO(LOG_DATABASE, "Flushing the block cache to DB (again)...");
        block_cache_->flush_to_db();
        LOG_INFO(LOG_DATABASE, "Block cache flushed to DB.");
    }
    closed_ = true;
    auto const closed = internal_db_->close();
    return closed;
}

// protected
void data_base::start() {
    internal_db_ = std::make_shared<internal_database>(
        internal_db_dir,
        settings_.db_mode,
        settings_.reorg_pool_limit,
        settings_.db_max_size, settings_.safe_mode);

    LOG_INFO(LOG_DATABASE, "Starting a block cache with ", settings_.block_cache_capacity, " blocks capacity");
    block_cache_.emplace(settings_.block_cache_capacity, internal_db_.get());
    LOG_INFO(LOG_DATABASE, "Block cache initialized.");
    // LOG_INFO(LOG_DATABASE, "block_cache_.has_value(): ", block_cache_.has_value());
}

// Readers.
// ----------------------------------------------------------------------------

expect<domain::chain::header> data_base::get_header(size_t height) const {
    if (block_cache_->is_height_in_cache(height)) {
        return block_cache_->get_header(height);
    }
    auto const res = internal_db_->get_header(height);
    if (  ! res.is_valid()) {
        return make_unexpected(error::height_not_found);
    }
    return res;
}

expect<std::pair<domain::chain::header, uint32_t>> data_base::get_header(hash_digest const& hash) const {
    auto const head = block_cache_->get_header(hash);
    if (head) {
        return head;
    }
    auto const res = internal_db_->get_header(hash);
    if ( ! res.first.is_valid()) {
        return make_unexpected(error::hash_not_found);
    }
    return res;
}

// std::optional<database::header_with_abla_state_t> block_chain::get_header_and_abla_state(size_t height) const {
//     return database_.get_header_and_abla_state(height);
// }

expect<header_with_abla_state_t> data_base::get_header_and_abla_state(size_t height) const {
    if (block_cache_->is_height_in_cache(height)) {
        return block_cache_->get_header_and_abla_state(height);
    }
    auto const res = internal_db_->get_header_and_abla_state(height);
    if ( ! res.has_value()) {
        return make_unexpected(error::height_not_found);
    }
    return *res;


    // auto header = domain::chain::header::from_data(reader, true);
    // if ( ! header) {
    //     return make_unexpected(header.error());
    // }

    // auto const block_size = reader.read_little_endian<uint64_t>();
    // if ( ! block_size) {
    //     return std::make_tuple(std::move(*header), 0, 0, 0);
    // }

    // auto const control_block_size = reader.read_little_endian<uint64_t>();
    // if ( ! control_block_size) {
    //     return std::make_tuple(std::move(*header), 0, 0, 0);
    // }

    // auto const elastic_buffer_size = reader.read_little_endian<uint64_t>();
    // if ( ! elastic_buffer_size) {
    //     return std::make_tuple(std::move(*header), 0, 0, 0);
    // }

    // return std::make_tuple(std::move(*header), *block_size, *control_block_size, *elastic_buffer_size);
}

expect<domain::chain::block> data_base::get_block(size_t height) const {
    if (block_cache_->is_height_in_cache(height)) {
        return block_cache_->get_block(height);
    }
    auto const res = internal_db_->get_block(height);
    if ( ! res.is_valid()) {
        return make_unexpected(error::height_not_found);
    }
    return res;
}

expect<std::pair<domain::chain::block, uint32_t>> data_base::get_block(hash_digest const& hash) const {
    auto blk = block_cache_->get_block(hash);
    if (blk) {
        return blk;
    }
    auto const res = internal_db_->get_block(hash);
    if ( ! res.first.is_valid()) {
        return make_unexpected(error::hash_not_found);
    }
    return res;
}

expect<data_base::height_t> data_base::get_last_height() const {

    uint32_t cache_height = 0;
    auto const cache_height_exp = block_cache_->last_height();
    if (cache_height_exp) {
        cache_height = *cache_height_exp;
    }

    uint32_t db_height = 0;
    auto const res = internal_db_->get_last_height(db_height);
    if (res != result_code::success) {
        db_height = 0;
    }
    return std::max(cache_height, db_height);
}

expect<utxo_entry> data_base::get_utxo(domain::chain::output_point const& point) const {
    auto utxo = block_cache_->get_utxo(point);
    if (utxo) {
        return utxo;
    }
    auto const res = internal_db_->get_utxo(point);
    if ( ! res.is_valid()) {
        return make_unexpected(error::utxo_not_found);
    }
    return res;
}

internal_database const& data_base::internal_db() const {
    return *internal_db_;
}

// Others
// ----------------------------------------------------------------------------

bool data_base::is_stale() const {
    //TODO: Should be a config value
    constexpr time_t notify_limit_seconds = 6 * 60 * 60; // 6 hours
    //TODO: cache the last header

    uint32_t timestamp = 0;
    auto last_height_exp = get_last_height();
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

// Synchronous writers.
// ----------------------------------------------------------------------------

static inline
hash_digest get_previous_hash(internal_database const& db, size_t height) {
    return height == 0 ? null_hash : db.get_header(height - 1).hash();
}

//TODO(fernando): const?
code data_base::verify_insert(block const& block, size_t height) {
    if (block.transactions().empty()) {
        return error::empty_block;
    }

    auto res = internal_db_->get_header(height);

    if (res.is_valid()) {
        return error::store_block_duplicate;
    }

    return error::success;
}

code data_base::verify_push(block const& block, size_t height) const {
    if (block.transactions().empty()) {
        return error::empty_block;
    }

    if (settings_.db_mode == db_mode_type::pruned) {
        return error::success;
    }
    auto last_height_exp = get_last_height();
    if ( ! last_height_exp) {
        return error::height_not_found;
    }
    auto last_height = *last_height_exp;
    if (last_height + 1 != height) {
        return error::store_block_invalid_height;
    }

    if (block.header().previous_block_hash() != get_previous_hash(internal_db(), height)) {
        return error::store_block_missing_parent;
    }

    return error::success;
}


#if ! defined(KTH_DB_READONLY)

// Add block to the database at the given height (gaps allowed/created).
// This is designed for write concurrency but only with itself.
code data_base::insert(domain::chain::block const& block, size_t height) {

    auto const median_time_past = block.header().validation.median_time_past;

    auto const ec = verify_insert(block, height);

    if (ec) return ec;

    auto res = internal_db_->push_block(block, height, median_time_past);
    if ( ! succeed(res)) {
        return error::operation_failed_1;   //TODO(fernando): create a new operation_failed
    }

    return error::success;
}

// This is designed for write exclusivity and read concurrency.
code data_base::push(domain::chain::transaction const& tx, uint32_t forks) {
    if (settings_.db_mode != db_mode_type::full) {
        return error::success;
    }

    //We insert only in transaction unconfirmed here
    internal_db_->push_transaction_unconfirmed(tx, forks);
    return error::success;  //TODO(fernando): store the transactions in a new mempool
}

#endif // ! defined(KTH_DB_READONLY)

#if ! defined(KTH_DB_READONLY)

// Add a block in order (creates no gaps, must be at top).
// This is designed for write exclusivity and read concurrency.
code data_base::push(block const& block, size_t height) {
    auto const median_time_past = block.header().validation.median_time_past;
    auto res = internal_db_->push_block(block, height, median_time_past);
    if ( ! succeed(res)) {
        return error::operation_failed_6;   //TODO(fernando): create a new operation_failed
    }
    return error::success;
}

// private
// Add the Genesis block
code data_base::push_genesis(block const& block) {
    auto res = internal_db_->push_genesis(block);
    if ( ! succeed(res)) {
        return error::operation_failed_6;   //TODO(fernando): create a new operation_failed
    }

    return error::success;
}
#endif // ! defined(KTH_DB_READONLY)



#if defined(KTH_CURRENCY_BCH)

#if ! defined(KTH_DB_READONLY)
// A false return implies store corruption.
bool data_base::pop(block& out_block) {

    auto const start_time = asio::steady_clock::now();

    if (internal_db_->pop_block(out_block) != result_code::success) {
        return false;
    }

    out_block.validation.error = error::success;
    out_block.validation.start_pop = start_time;
    return true;
}
#endif // ! defined(KTH_DB_READONLY)

#else // KTH_CURRENCY_BCH

#if ! defined(KTH_DB_READONLY)
// A false return implies store corruption.
bool data_base::pop(block& out_block) {

    auto const start_time = asio::steady_clock::now();

    if (internal_db_->pop_block(out_block) != result_code::success) {
        return false;
    }

    out_block.validation.error = error::success;
    out_block.validation.start_pop = start_time;
    return true;
}
#endif //! defined(KTH_DB_READONLY)
#endif // KTH_CURRENCY_BCH


#if ! defined(KTH_DB_READONLY)
// A false return implies store corruption.
bool data_base::pop_inputs(const input::list& inputs, size_t height) {
    return true;
}

// A false return implies store corruption.
bool data_base::pop_outputs(const output::list& outputs, size_t height) {
    return true;
}
#endif //! defined(KTH_DB_READONLY)

// Asynchronous writers.
// ----------------------------------------------------------------------------

#if ! defined(KTH_DB_READONLY)
// Add a list of blocks in order.
// If the dispatch threadpool is shut down when this is running the handler
// will never be invoked, resulting in a threadpool.join indefinite hang.
void data_base::push_all(block_const_ptr_list_const_ptr in_blocks, size_t first_height, dispatcher& dispatch, result_handler handler) {
    DEBUG_ONLY(*safe_add(in_blocks->size(), first_height));

    // Parallel version (disabled at the moment)
    // This is the beginning of the push_all sequence.
    // push_next(error::success, in_blocks, 0, first_height, dispatch, handler);

    // // Sequential version
    // for (size_t i = 0; i < in_blocks->size(); ++i) {
    //     auto const block_ptr = (*in_blocks)[i];
    //     auto const median_time_past = block_ptr->header().validation.median_time_past;

    //     block_ptr->validation.start_push = asio::steady_clock::now();

    //     auto res = internal_db_->push_block(*block_ptr, first_height + i, median_time_past);
    //     if ( ! succeed(res)) {
    //         handler(error::operation_failed_7); //TODO(fernando): create a new operation_failed
    //         return;
    //     }

    //     block_ptr->validation.end_push = asio::steady_clock::now();
    // }
    // handler(error::success);

    // Cached version

    // block_cache_->add_blocks(in_blocks, first_height);

    for (size_t i = 0; i < in_blocks->size(); ++i) {
        auto const& block_ptr = (*in_blocks)[i];
        block_ptr->validation.start_push = asio::steady_clock::now();
        // LOG_INFO(LOG_DATABASE, "push_all() - before adding block to the cache, height: ", first_height + i);
        block_cache_->add_block(*block_ptr, first_height + i);  // Llamada a add_block por cada bloque
        // LOG_INFO(LOG_DATABASE, "push_all() - after adding block to the cache, height: ", first_height + i);

        block_ptr->validation.end_push = asio::steady_clock::now();
    }

    // LOG_INFO(LOG_DATABASE, "push_all() - returning with success");
    handler(error::success);
}

// TODO(legacy): resolve inconsistency with height and median_time_past passing.
void data_base::push_next(code const& ec, block_const_ptr_list_const_ptr blocks, size_t index, size_t height, dispatcher& dispatch, result_handler handler) {
    if (ec || index >= blocks->size()) {
        // This ends the loop.
        handler(ec);
        return;
    }

    auto const block = (*blocks)[index];
    auto const median_time_past = block->header().validation.median_time_past;

    // Set push start time for the block.
    block->validation.start_push = asio::steady_clock::now();

    result_handler const next = std::bind(&data_base::push_next, this, _1, blocks, index + 1, height + 1, std::ref(dispatch), handler);

    // This is the beginning of the block sub-sequence.
    dispatch.concurrent(&data_base::do_push, this, block, height, median_time_past, std::ref(dispatch), next);
}

void data_base::do_push(block_const_ptr block, size_t height, uint32_t median_time_past, dispatcher& dispatch, result_handler handler) {
    // LOG_DEBUG(LOG_DATABASE, "Write flushed to disk: ", ec.message());
    auto res = internal_db_->push_block(*block, height, median_time_past);
    if ( ! succeed(res)) {
        handler(error::operation_failed_7); //TODO(fernando): create a new operation_failed
        return;
    }
    block->validation.end_push = asio::steady_clock::now();
    // This is the end of the block sub-sequence.
    handler(error::success);
}

// TODO(legacy): make async and concurrency as appropriate.
// This precludes popping the genesis block.
void data_base::pop_above(block_const_ptr_list_ptr out_blocks, hash_digest const& fork_hash, dispatcher&, result_handler handler) {
    out_blocks->clear();

    auto fork_hash_hex = encode_hash(fork_hash);
    auto const header_result = get_header(fork_hash);

    if ( ! header_result) {
        handler(error::operation_failed_9);
        return;
    }

    // The fork point does not exist or failed to get it or the top, fail.
    if ( ! header_result->first.is_valid()) {
        handler(error::operation_failed_9);
        return;
    }
    // LOG_INFO(LOG_DATABASE, "pop_above() - header_result->first.is_valid() is true. Continuing");
    auto const top_exp = get_last_height();
    if ( ! top_exp) {
        handler(error::height_not_found);
        return;
    }
    auto const top = *top_exp;
    auto const fork = header_result->second;
    auto const size = top - fork;

    // The fork is at the top of the chain, nothing to pop.
    if (size == 0) {
        // LOG_INFO(LOG_DATABASE, "pop_above() - size is 0, returning with success");
        handler(error::success);
        return;
    }

    // If the fork is at the top there is one block to pop, and so on.
    out_blocks->reserve(size);

    // Enqueue blocks so .front() is fork + 1 and .back() is top.
    for (size_t height = top; height > fork; --height) {
        domain::message::block next;

        // TODO(legacy): parallelize pop of transactions within each block.
        if ( ! pop(next)) {
            // LOG_INFO(LOG_DATABASE, "pop_above() - pop failed. Returning with error");
            handler(error::operation_failed_10);
            return;
        }

        KTH_ASSERT(next.is_valid());
        auto block = std::make_shared<domain::message::block const>(std::move(next));
        out_blocks->insert(out_blocks->begin(), block);
    }

    // LOG_INFO(LOG_DATABASE, "pop_above() - returning with success");
    handler(error::success);
}

code data_base::prune_reorg() {
    auto res = internal_db_->prune();
    if ( ! succeed_prune(res)) {
        LOG_ERROR(LOG_DATABASE, "Error pruning the reorganization pool, code: ", static_cast<std::underlying_type<result_code>::type>(res));
        return error::unknown;
    }
    return error::success;
}
#endif // ! defined(KTH_DB_READONLY)

#if ! defined(KTH_DB_READONLY)
// This is designed for write exclusivity and read concurrency.
void data_base::reorganize(
    infrastructure::config::checkpoint const& fork_point,
    block_const_ptr_list_const_ptr incoming_blocks,
    block_const_ptr_list_ptr outgoing_blocks,
    dispatcher& dispatch,
    result_handler handler) {

    // LOG_INFO(LOG_DATABASE, "reorganize");
    auto const next_height = *safe_add(fork_point.height(), size_t(1));
    // TODO: remove std::bind, use lambda instead.
    // TOOD: Even better use C++20 coroutines.
    result_handler const pop_handler = std::bind(&data_base::handle_pop, this, _1, incoming_blocks, next_height, std::ref(dispatch), handler);
    pop_above(outgoing_blocks, fork_point.hash(), dispatch, pop_handler);
}

void data_base::handle_pop(code const& ec, block_const_ptr_list_const_ptr incoming_blocks, size_t first_height, dispatcher& dispatch, result_handler handler) {
    result_handler const push_handler = std::bind(&data_base::handle_push, this, _1, handler);

    if (ec) {
        LOG_INFO(LOG_DATABASE, "handle_pop() - ec is not success. Returning with error");
        push_handler(ec);
        return;
    }

    push_all(incoming_blocks, first_height, std::ref(dispatch), push_handler);
}

// We never invoke the caller's handler under the mutex, we never fail to clear
// the mutex, and we always invoke the caller's handler exactly once.
void data_base::handle_push(code const& ec, result_handler handler) const {
    if (ec) {
        handler(ec);
        return;
    }
    handler(error::success);
}
#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database
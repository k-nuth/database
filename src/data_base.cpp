// Copyright (c) 2016-2022 Knuth Project developers.
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

block global_genesis = {};

data_base::data_base(settings const& settings)
    : closed_(true)
    , settings_(settings)
    , store(settings.directory, settings.index_start_height < without_indexes, settings.flush_writes)
{
#ifdef KTH_DB_SPENDS
       , "spend [", settings.spend_table_buckets, "], "
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
       , "history [", settings.history_table_buckets, "]"
#endif // KTH_DB_HISTORY
}

data_base::~data_base() {
    std::cout << "********************** data_base::~data_base()\n";
    close();
}

// Open and close.
// ----------------------------------------------------------------------------

#if ! defined(KTH_DB_READONLY)
// Throws if there is insufficient disk space, not idempotent.
bool data_base::create(block const& genesis) {
    std::cout << "********************** data_base::create()\n";

    start();

    auto created = true
                && internal_db_->create()
                ;

    if ( ! created) {
        return false;
    }

    // Store the first block.
    // push(genesis, 0);
    push_genesis(genesis);
    global_genesis = genesis;

    closed_ = false;
    return true;
}
#endif // ! defined(KTH_DB_READONLY)

// Must be called before performing queries, not idempotent.
// May be called after stop and/or after close in order to reopen.
bool data_base::open() {
    std::cout << "********************** data_base::open()\n";

    start();

    auto opened = true
                && internal_db_->open()
                ;

    if ( ! opened) {
        return false;
    }

    // Store the first block.
    push_genesis(global_genesis);

    closed_ = false;

    std::cout << "********************** data_base::open() - opened: " << opened << "\n";
    return opened;
}

// Close is idempotent and thread safe.
// Optional as the database will close on destruct.
bool data_base::close() {
    std::cout << "********************** data_base::close()\n";

    if (closed_) {
        return true;
    }

    closed_ = true;

    auto closed = true
                && internal_db_->close()
                ;

    return closed
            ;
}

// protected
void data_base::start() {
    std::cout << "********************** data_base::start()\n";

#ifdef KTH_DB_NEW
    internal_db_ = std::make_shared<internal_database>(internal_db_dir, settings_.reorg_pool_limit, settings_.db_max_size, settings_.safe_mode);
    printf("-*-*-*-*-*-*-*--*-* internal_db_: %x\n", internal_db_.get());
    std::cout << "********************** data_base::start() after make_shared\n";
#endif // KTH_DB_NEW
}

// Readers.
// ----------------------------------------------------------------------------

#ifdef KTH_DB_NEW
internal_database const& data_base::internal_db() const {
    std::cout << "********************** data_base::internal_db()\n";
    printf("-*-*-*-*-*-*-*--*-* data_base::internal_db() -- internal_db_: %x\n", internal_db_.get());
    auto const& res = *internal_db_;
    printf("-*-*-*-*-*-*-*--*-* data_base::internal_db() -- internal_db_: %x\n", internal_db_.get());
    return res;


    // return *internal_db_;
}
#endif // KTH_DB_NEW


// Synchronous writers.
// ----------------------------------------------------------------------------

//TODO(fernando): const?
code data_base::verify_insert(block const& block, size_t height) {
    std::cout << "********************** data_base::verify_insert()\n";
    if (block.transactions().empty()) {
        return error::empty_block;
    }

#if defined(KTH_DB_LEGACY)
#elif defined(KTH_DB_NEW)
    auto res = internal_db_->get_header(height);

    if (res.is_valid()) {
        return error::store_block_duplicate;
    }
#endif // KTH_DB_LEGACY

    return error::success;
}

code data_base::verify_push(block const& block, size_t height) const {
    std::cout << "********************** data_base::verify_push()\n";
    if (block.transactions().empty()) {
        return error::empty_block;
    }

#if defined(KTH_DB_LEGACY)
#elif defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)

    if (get_next_height(internal_db()) != height) {
        return error::store_block_invalid_height;
    }

    if (block.header().previous_block_hash() != get_previous_hash(internal_db(), height)) {
        return error::store_block_missing_parent;
    }
#endif // KTH_DB_LEGACY

    return error::success;
}

#if ! defined(KTH_DB_READONLY)

// Add block to the database at the given height (gaps allowed/created).
// This is designed for write concurrency but only with itself.
code data_base::insert(domain::chain::block const& block, size_t height) {
    std::cout << "********************** data_base::insert()\n";

    auto const median_time_past = block.header().validation.median_time_past;

    auto const ec = verify_insert(block, height);

    if (ec) return ec;

#ifdef KTH_DB_NEW
    auto res = internal_db_->push_block(block, height, median_time_past);
    if ( ! succeed(res)) {
        return error::operation_failed_1;   //TODO(fernando): create a new operation_failed
    }
#endif // KTH_DB_NEW

    return error::success;
}
#endif //! defined(KTH_DB_READONLY)

#if ! defined(KTH_DB_READONLY)

// This is designed for write exclusivity and read concurrency.
code data_base::push(domain::chain::transaction const& tx, uint32_t forks) {
    std::cout << "********************** data_base::push()\n";
#ifdef KTH_DB_LEGACY
#elif defined(KTH_DB_NEW_FULL)
    //We insert only in transaction unconfirmed here
    internal_db_->push_transaction_unconfirmed(tx, forks);
    return error::success;  //TODO(fernando): store the transactions in a new mempool

#else
    return error::success;
#endif // KTH_DB_LEGACY
}
#endif // ! defined(KTH_DB_READONLY)

#if ! defined(KTH_DB_READONLY)
// Add a block in order (creates no gaps, must be at top).
// This is designed for write exclusivity and read concurrency.
code data_base::push(block const& block, size_t height) {
    std::cout << "********************** data_base::push()\n";

    auto const median_time_past = block.header().validation.median_time_past;

#ifdef KTH_DB_NEW
    auto res = internal_db_->push_block(block, height, median_time_past);
    if ( ! succeed(res)) {
        return error::operation_failed_6;   //TODO(fernando): create a new operation_failed
    }
#endif // KTH_DB_NEW


    return error::success;
}

// private
// Add the Genesis block
code data_base::push_genesis(block const& block) {
    std::cout << "********************** data_base::push_genesis()\n";

#ifdef KTH_DB_NEW
    auto res = internal_db_->push_genesis(block);
    if ( ! succeed(res)) {
        return error::operation_failed_6;   //TODO(fernando): create a new operation_failed
    }
#endif // KTH_DB_NEW

    return error::success;
}
#endif // ! defined(KTH_DB_READONLY)


#if defined(KTH_CURRENCY_BCH)

#if ! defined(KTH_DB_READONLY)
// A false return implies store corruption.
bool data_base::pop(block& out_block) {
    std::cout << "********************** data_base::pop()\n";

    auto const start_time = asio::steady_clock::now();

#ifdef KTH_DB_NEW
    if (internal_db_->pop_block(out_block) != result_code::success) {
        return false;
    }
#endif

    out_block.validation.error = error::success;
    out_block.validation.start_pop = start_time;
    return true;
}
#endif // ! defined(KTH_DB_READONLY)

#else // KTH_CURRENCY_BCH

#if ! defined(KTH_DB_READONLY)
// A false return implies store corruption.
bool data_base::pop(block& out_block) {
    std::cout << "********************** data_base::pop()\n";

    auto const start_time = asio::steady_clock::now();

#ifdef KTH_DB_NEW
    if (internal_db_->pop_block(out_block) != result_code::success) {
        return false;
    }
#endif

    out_block.validation.error = error::success;
    out_block.validation.start_pop = start_time;
    return true;
}
#endif //! defined(KTH_DB_READONLY)
#endif // KTH_CURRENCY_BCH


#if ! defined(KTH_DB_READONLY)
// A false return implies store corruption.
bool data_base::pop_inputs(const input::list& inputs, size_t height) {
    std::cout << "********************** data_base::pop_inputs()\n";
    // Loop in reverse.
    for (auto const& input: reverse(inputs)) {

        if (height < settings_.index_start_height) continue;

#ifdef KTH_DB_SPENDS
        // All spends are confirmed.
        // This can fail if index start has been changed between restarts.
        // So ignore the error here and succeed even if not found.
        /* bool */ spends_->unlink(input.previous_output());
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
        // Delete can fail if index start has been changed between restarts.
        for (auto const& address : input.addresses()) {
            /* bool */ history_->delete_last_row(address.hash());
        }
#endif // KTH_DB_HISTORY
    }

    return true;
}

// A false return implies store corruption.
bool data_base::pop_outputs(const output::list& outputs, size_t height) {
    std::cout << "********************** data_base::pop_outputs()\n";
    if (height < settings_.index_start_height) {
        return true;
    }

#ifdef KTH_DB_HISTORY
    // Loop in reverse.
    for (auto const output : reverse(outputs)) {
        // Delete can fail if index start has been changed between restarts.
        for (auto const& address : output.addresses()) {
            /* bool */ history_->delete_last_row(address.hash());
        }

        // All stealth entries are confirmed.
        // Stealth unlink is not implemented as there is no way to correlate.
    }
#endif // KTH_DB_HISTORY

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
    std::cout << "********************** data_base::push_all()\n";
    DEBUG_ONLY(safe_add(in_blocks->size(), first_height));

    // This is the beginning of the push_all sequence.
    push_next(error::success, in_blocks, 0, first_height, dispatch, handler);
}

// TODO(legacy): resolve inconsistency with height and median_time_past passing.
void data_base::push_next(code const& ec, block_const_ptr_list_const_ptr blocks, size_t index, size_t height, dispatcher& dispatch, result_handler handler) {
    std::cout << "********************** data_base::push_next()\n";
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

#ifdef KTH_DB_NEW
    // LOG_DEBUG(LOG_DATABASE, "Write flushed to disk: ", ec.message());
    auto res = internal_db_->push_block(*block, height, median_time_past);
    if ( ! succeed(res)) {
        handler(error::operation_failed_7); //TODO(fernando): create a new operation_failed
        return;
    }
    block->validation.end_push = asio::steady_clock::now();
    // This is the end of the block sub-sequence.
    handler(error::success);
#endif // KTH_DB_NEW

}

// TODO(legacy): make async and concurrency as appropriate.
// This precludes popping the genesis block.
void data_base::pop_above(block_const_ptr_list_ptr out_blocks, hash_digest const& fork_hash, dispatcher&, result_handler handler) {
    std::cout << "********************** data_base::pop_above()\n";
    out_blocks->clear();

#ifdef KTH_DB_NEW
    auto const header_result = internal_db_->get_header(fork_hash);

    uint32_t top;
    // The fork point does not exist or failed to get it or the top, fail.
    if ( ! header_result.first.is_valid() ||  internal_db_->get_last_height(top) != result_code::success) {
        //**--**
        handler(error::operation_failed_9);
        return;
    }

    auto const fork = header_result.second;

#endif


    auto const size = top - fork;

    // The fork is at the top of the chain, nothing to pop.
    if (size == 0) {
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
            //**--**
            handler(error::operation_failed_10);
            return;
        }

        KTH_ASSERT(next.is_valid());
        auto block = std::make_shared<domain::message::block const>(std::move(next));
        out_blocks->insert(out_blocks->begin(), block);
    }

    handler(error::success);
}

code data_base::prune_reorg() {
#ifdef KTH_DB_NEW
    auto res = internal_db_->prune();
    if ( ! succeed_prune(res)) {
        LOG_ERROR(LOG_DATABASE, "Error pruning the reorganization pool, code: ", static_cast<std::underlying_type<result_code>::type>(res));
        return error::unknown;
    }
#endif // KTH_DB_NEW
    return error::success;
}
#endif // ! defined(KTH_DB_READONLY)


#if ! defined(KTH_DB_READONLY)
// This is designed for write exclusivity and read concurrency.
void data_base::reorganize(infrastructure::config::checkpoint const& fork_point, block_const_ptr_list_const_ptr incoming_blocks, block_const_ptr_list_ptr outgoing_blocks, dispatcher& dispatch, result_handler handler) {
    std::cout << "********************** data_base::reorganize()\n";
    auto const next_height = safe_add(fork_point.height(), size_t(1));
    result_handler const pop_handler = std::bind(&data_base::handle_pop, this, _1, incoming_blocks, next_height, std::ref(dispatch), handler);


    pop_above(outgoing_blocks, fork_point.hash(), dispatch, pop_handler);
}

void data_base::handle_pop(code const& ec, block_const_ptr_list_const_ptr incoming_blocks, size_t first_height, dispatcher& dispatch, result_handler handler) {
    std::cout << "********************** data_base::handle_pop()\n";
    result_handler const push_handler = std::bind(&data_base::handle_push, this, _1, handler);

    if (ec) {
        push_handler(ec);
        return;
    }

    push_all(incoming_blocks, first_height, std::ref(dispatch), push_handler);
}

// We never invoke the caller's handler under the mutex, we never fail to clear
// the mutex, and we always invoke the caller's handler exactly once.
void data_base::handle_push(code const& ec, result_handler handler) const {
    std::cout << "********************** data_base::handle_push()\n";

    if (ec) {
        handler(ec);
        return;
    }

#if defined(KTH_DB_LEGACY)
#elif defined(KTH_DB_NEW)
    handler(error::success);
#else
#error You must define KTH_DB_LEGACY or KTH_DB_NEW
#endif
}
#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database
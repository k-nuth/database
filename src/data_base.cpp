// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/database/data_base.hpp>

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

// #include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <kth/domain.hpp>
#include <kth/database/define.hpp>
#include <kth/database/settings.hpp>
#include <kth/database/store.hpp>

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

data_base::data_base(const settings& settings)
    : closed_(true)
    , settings_(settings)
#ifdef KTH_DB_LEGACY    
    , remap_mutex_(std::make_shared<shared_mutex>())
#endif
    , store(settings.directory, settings.index_start_height < without_indexes, settings.flush_writes)
{
   
#ifdef KTH_DB_LEGACY
        LOG_DEBUG(LOG_DATABASE)
       , "Buckets: "
       , "block [", settings.block_table_buckets, "], "
       , "transaction [", settings.transaction_table_buckets, "], "
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_SPENDS
       , "spend [", settings.spend_table_buckets, "], "
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
       , "history [", settings.history_table_buckets, "]"
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_LEGACY
        ;
#endif // KTH_DB_LEGACY        
}

data_base::~data_base() {
    close();
}

// Open and close.
// ----------------------------------------------------------------------------

#if ! defined(KTH_DB_READONLY)
// Throws if there is insufficient disk space, not idempotent.
bool data_base::create(const block& genesis) {

#ifdef KTH_DB_LEGACY
    ///////////////////////////////////////////////////////////////////////////
    // Lock exclusive file access.
    if ( ! store::open()) {
        return false;
    }

    // Create files.
    if ( ! store::create()) {
        return false;
    }
#endif // KTH_DB_LEGACY

    start();

    // These leave the databases open.
    auto created = true
#ifdef KTH_DB_LEGACY
                && blocks_->create()
                && transactions_->create()
#endif

#if defined(KTH_DB_NEW) && ! defined(KTH_DB_READONLY)
                && internal_db_->create()
#endif

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
                && transactions_unconfirmed_->create()
#endif
                ;

#ifdef KTH_DB_WITH_INDEXES
    if (use_indexes()) {
        created = created 
#ifdef KTH_DB_SPENDS
                && spends_->create() 
#endif // KTH_DB_SPENDS
#ifdef KTH_DB_HISTORY
                && history_->create() 
#endif // KTH_DB_HISTORY
#ifdef KTH_DB_STEALTH                
                && stealth_->create()
#endif // KTH_DB_STEALTH                
                ;
    }
#endif // KTH_DB_WITH_INDEXES    

    if ( ! created) {
        return false;
    }

    // Store the first block.
    // push(genesis, 0);
    push_genesis(genesis);

    closed_ = false;
    return true;
}
#endif // ! defined(KTH_DB_READONLY)

// Must be called before performing queries, not idempotent.
// May be called after stop and/or after close in order to reopen.
bool data_base::open() {

#ifdef KTH_DB_LEGACY
    ///////////////////////////////////////////////////////////////////////////
    // Lock exclusive file access and conditionally the global flush lock.
    if ( ! store::open()) {
        return false;
    }
#endif // KTH_DB_LEGACY

    start();

    auto opened = true
#ifdef KTH_DB_LEGACY
                && blocks_->open() 
                && transactions_->open() 
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
                && internal_db_->open() 
#endif // KTH_DB_NEW

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
                && transactions_unconfirmed_->open()
#endif // KTH_DB_TRANSACTION_UNCONFIRMED
                ;

#ifdef KTH_DB_WITH_INDEXES
    if (use_indexes()) {
        opened = opened 
#ifdef KTH_DB_SPENDS
                && spends_->open()
#endif // KTH_DB_SPENDS
#ifdef KTH_DB_HISTORY
                && history_->open() 
#endif // KTH_DB_HISTORY
#ifdef KTH_DB_STEALTH                
                && stealth_->open()
#endif // KTH_DB_STEALTH                
                ;
    }
#endif // KTH_DB_WITH_INDEXES


    closed_ = false;
    return opened;
}

// Close is idempotent and thread safe.
// Optional as the database will close on destruct.
bool data_base::close() {

    if (closed_) {
        return true;
    }

    closed_ = true;

    auto closed = true
#ifdef KTH_DB_LEGACY
                && blocks_->close() 
                && transactions_->close() 
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
                && internal_db_->close() 
#endif // KTH_DB_NEW

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
                && transactions_unconfirmed_->close()
#endif // KTH_DB_TRANSACTION_UNCONFIRMED
                ;

#ifdef KTH_DB_WITH_INDEXES
    if (use_indexes()) {
        closed = closed 
#ifdef KTH_DB_SPENDS
                && spends_->close()
#endif // KTH_DB_SPENDS
#ifdef KTH_DB_HISTORY
                && history_->close() 
#endif // KTH_DB_HISTORY
#ifdef KTH_DB_STEALTH                
                && stealth_->close()
#endif // KTH_DB_STEALTH                
                ;
    }
#endif // KTH_DB_WITH_INDEXES


    return closed 
#ifdef KTH_DB_LEGACY    
            && store::close()
    // Unlock exclusive file access and conditionally the global flush lock.
    ///////////////////////////////////////////////////////////////////////////
#endif // KTH_DB_LEGACY
            ;
}

// protected
void data_base::start() {
    // TODO: parameterize initial file sizes as record count or slab bytes?

#ifdef KTH_DB_LEGACY
    blocks_ = std::make_shared<block_database>(block_table, block_index,
        settings_.block_table_buckets, settings_.file_growth_rate,
        remap_mutex_);

    transactions_ = std::make_shared<transaction_database>(transaction_table,
        settings_.transaction_table_buckets, settings_.file_growth_rate,
        settings_.cache_capacity, remap_mutex_);
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
    internal_db_ = std::make_shared<internal_database>(internal_db_dir, settings_.reorg_pool_limit, settings_.db_max_size, settings_.safe_mode);
#endif // KTH_DB_NEW

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    //TODO(fernando): transaction_table_buckets and file_growth_rate
    transactions_unconfirmed_ = std::make_shared<transaction_unconfirmed_database>(transaction_unconfirmed_table,
        settings_.transaction_unconfirmed_table_buckets, settings_.file_growth_rate, remap_mutex_);
#endif // KTH_DB_TRANSACTION_UNCONFIRMED


#ifdef KTH_DB_WITH_INDEXES
    if (use_indexes()) {

#ifdef KTH_DB_SPENDS
        spends_ = std::make_shared<spend_database>(spend_table,
            settings_.spend_table_buckets, settings_.file_growth_rate,
            remap_mutex_);
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
        history_ = std::make_shared<history_database>(history_table,
            history_rows, settings_.history_table_buckets,
            settings_.file_growth_rate, remap_mutex_);
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_STEALTH
        stealth_ = std::make_shared<stealth_database>(stealth_rows,
            settings_.file_growth_rate, remap_mutex_);
#endif // KTH_DB_STEALTH
    }
#endif // KTH_DB_WITH_INDEXES

}



#ifdef KTH_DB_LEGACY

#if ! defined(KTH_DB_READONLY)

// protected
bool data_base::flush() const {
    // Avoid a race between flush and close whereby flush is skipped because
    // close is true and therefore the flush lock file is deleted before close
    // fails. This would leave the database corrupted and undetected. The flush
    // call must execute and successfully flush or the lock must remain.
    ////if (closed_)
    ////    return true;

    auto flushed = true
                && blocks_->flush() 
                && transactions_->flush() 

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
                && transactions_unconfirmed_->flush()
#endif // KTH_DB_TRANSACTION_UNCONFIRMED
                ;

#ifdef KTH_DB_WITH_INDEXES
    if (use_indexes()) {
        flushed = flushed 
#ifdef KTH_DB_SPENDS
                && spends_->flush() 
#endif // KTH_DB_SPENDS
#ifdef KTH_DB_HISTORY
                && history_->flush() 
#endif // KTH_DB_HISTORY
#ifdef KTH_DB_STEALTH                
                && stealth_->flush()
#endif // KTH_DB_STEALTH                
                ;
    }
#endif // KTH_DB_WITH_INDEXES

    // Just for the log.
    code ec(flushed ? error::success : error::operation_failed_0);

    LOG_DEBUG(LOG_DATABASE, "Write flushed to disk: ", ec.message());

    return flushed;
}

#endif // ! defined(KTH_DB_READONLY)


// protected
void data_base::synchronize() {
#ifdef KTH_DB_WITH_INDEXES
    if (use_indexes()) {
#ifdef KTH_DB_SPENDS
        spends_->synchronize();
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
        history_->synchronize();
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_STEALTH                
        stealth_->synchronize();
#endif // KTH_DB_STEALTH                
    }
#endif // KTH_DB_WITH_INDEXES

    transactions_->synchronize();

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    transactions_unconfirmed_->synchronize();
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

    blocks_->synchronize();
}
#endif // KTH_DB_LEGACY

// Readers.
// ----------------------------------------------------------------------------

#ifdef KTH_DB_LEGACY
block_database const& data_base::blocks() const {
    return *blocks_;
}

transaction_database const& data_base::transactions() const {
    return *transactions_;
}
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
internal_database const& data_base::internal_db() const {
    return *internal_db_;
}
#endif // KTH_DB_NEW


#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
transaction_unconfirmed_database const& data_base::transactions_unconfirmed() const {
    return *transactions_unconfirmed_;
}
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_SPENDS
// Invalid if indexes not initialized.
spend_database const& data_base::spends() const {
    return *spends_;
}
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
// Invalid if indexes not initialized.
history_database const& data_base::history() const {
    return *history_;
}
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_STEALTH
// Invalid if indexes not initialized.
stealth_database const& data_base::stealth() const {
    return *stealth_;
}
#endif // KTH_DB_STEALTH

// Synchronous writers.
// ----------------------------------------------------------------------------

#ifdef KTH_DB_LEGACY
static inline 
size_t get_next_height(const block_database& blocks) {
    size_t current_height;
    auto const empty_chain = !blocks.top(current_height);
    return empty_chain ? 0 : current_height + 1;
}

static inline 
hash_digest get_previous_hash(const block_database& blocks, size_t height) {
    return height == 0 ? null_hash : blocks.get(height - 1).header().hash();
}
#endif // KTH_DB_LEGACY


#if defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)
static inline 
uint32_t get_next_height(internal_database const& db) {
    uint32_t current_height;
    auto res = db.get_last_height(current_height);

    if (res != result_code::success) {
        return 0;
    }

    return current_height + 1;
}

static inline 
hash_digest get_previous_hash(internal_database const& db, size_t height) {
    return height == 0 ? null_hash : db.get_header(height - 1).hash();
}
#endif // defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)


//TODO(fernando): const?
code data_base::verify_insert(const block& block, size_t height) {
    if (block.transactions().empty()) {
        return error::empty_block;
    }

#if defined(KTH_DB_LEGACY)
    if (blocks_->exists(height)) {
        return error::store_block_duplicate;
    }

#elif defined(KTH_DB_NEW)

    auto res = internal_db_->get_header(height);

    if (res.is_valid()) {
        return error::store_block_duplicate;
    }

#endif // KTH_DB_LEGACY

    return error::success;
}

//TODO(fernando): const?
code data_base::verify_push(const block& block, size_t height) {
    if (block.transactions().empty()) {
        return error::empty_block;
    }

#if defined(KTH_DB_LEGACY)
    if (get_next_height(blocks()) != height) {
        return error::store_block_invalid_height;
    }

    if (block.header().previous_block_hash() != get_previous_hash(blocks(), height)) {
        return error::store_block_missing_parent;
    }

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


//Note(Knuth): We don't store spend information
#if defined(KTH_DB_LEGACY)
//TODO(fernando): const?
code data_base::verify_push(const transaction& tx) {
    auto const result = transactions_->get(tx.hash(), max_size_t, false);
    return result && ! result.is_spent(max_size_t) ? error::unspent_duplicate : error::success;
}
#endif // defined(KTH_DB_LEGACY)


#ifdef KTH_DB_LEGACY
#if ! defined(KTH_DB_READONLY)
bool data_base::begin_insert() const {
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    write_mutex_.lock();

    return begin_write();
}

bool data_base::end_insert() const {
    write_mutex_.unlock();
    // End Critical Section
    ///////////////////////////////////////////////////////////////////////////

    return end_write();
}
#endif //! defined(KTH_DB_READONLY)
#endif // KTH_DB_LEGACY

#if ! defined(KTH_DB_READONLY)

// Add block to the database at the given height (gaps allowed/created).
// This is designed for write concurrency but only with itself.
code data_base::insert(domain::chain::block const& block, size_t height) {

    auto const median_time_past = block.header().validation.median_time_past;
    
    auto const ec = verify_insert(block, height);

    if (ec) return ec;

#ifdef KTH_DB_NEW
    auto res = internal_db_->push_block(block, height, median_time_past);
    if ( ! succeed(res)) {
        return error::operation_failed_1;   //TODO(fernando): create a new operation_failed
    }
#endif // KTH_DB_NEW

#ifdef KTH_DB_LEGACY
    
    if ( ! push_transactions(block, height, median_time_past) || ! push_heights(block, height)) {
        return error::operation_failed_1;
    }

    blocks_->store(block, height);

    synchronize();
#endif // KTH_DB_LEGACY

    return error::success;
}
#endif //! defined(KTH_DB_READONLY)

#if ! defined(KTH_DB_READONLY)

// This is designed for write exclusivity and read concurrency.
code data_base::push(domain::chain::transaction const& tx, uint32_t forks) {
#ifdef KTH_DB_LEGACY

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    // Returns error::unspent_duplicate if an unspent tx with same hash exists.
    auto const ec = verify_push(tx);

    if (ec) {
        return ec;
    }

    // Begin Flush Lock and Sequential Lock
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if ( ! begin_write()) {
        return error::operation_failed_2;
    }

    // When position is unconfirmed, height is used to store validation forks.
    transactions_->store(tx, forks, 0, transaction_database::unconfirmed);

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    transactions_unconfirmed_->store(tx); //, forks, transaction_unconfirmed_database::unconfirmed);
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

    transactions_->synchronize();

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    transactions_unconfirmed_->synchronize();
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

    return end_write() ? error::success : error::operation_failed_3;
    // End Sequential Lock and Flush Lock
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////

#elif defined(KTH_DB_NEW_FULL)
    //We insert only in transaction unconfirmed here
    internal_db_->push_transaction_unconfirmed(tx, forks);
    return error::success;  //TODO(fernando): store the transactions in a new mempool

#else 
    return error::success;
#endif // KTH_DB_LEGACY
}
#endif // ! defined(KTH_DB_READONLY)


#ifdef KTH_DB_LEGACY
#if ! defined(KTH_DB_READONLY)
/// TODO comment
code data_base::push_legacy(block const& block, size_t height) {
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(write_mutex_);

    auto const ec = verify_push(block, height);
    if (ec != error::success) {
        return ec;
    }

    // Begin Flush Lock and Sequential Lock
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if ( ! begin_write()) {
        return error::operation_failed_4;
    }

    auto const median_time_past = block.header().validation.median_time_past;

    if ( ! push_transactions(block, height, median_time_past) || ! push_heights(block, height)) {
        return error::operation_failed_5;
    }

    blocks_->store(block, height);

    synchronize();

    return end_write() ? error::success : error::operation_failed_6;
    // End Sequential Lock and Flush Lock
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ///////////////////////////////////////////////////////////////////////////

    return error::success;
}
#endif // ! defined(KTH_DB_READONLY)
#endif // KTH_DB_LEGACY

#if ! defined(KTH_DB_READONLY)
// Add a block in order (creates no gaps, must be at top).
// This is designed for write exclusivity and read concurrency.
code data_base::push(block const& block, size_t height) {

    auto const median_time_past = block.header().validation.median_time_past;

#ifdef KTH_DB_NEW
    auto res = internal_db_->push_block(block, height, median_time_past);
    if ( ! succeed(res)) {
        return error::operation_failed_6;   //TODO(fernando): create a new operation_failed
    }
#endif // KTH_DB_NEW

#ifdef KTH_DB_LEGACY
    return push_legacy(block, height);
#endif // KTH_DB_LEGACY

    return error::success;
}

// private
// Add the Genesis block
code data_base::push_genesis(block const& block) {

#ifdef KTH_DB_NEW
    auto res = internal_db_->push_genesis(block);
    if ( ! succeed(res)) {
        return error::operation_failed_6;   //TODO(fernando): create a new operation_failed
    }
#endif // KTH_DB_NEW

#ifdef KTH_DB_LEGACY
    size_t const height = 0;
    return push_legacy(block, height);
#endif // KTH_DB_LEGACY

    return error::success;
}
#endif // ! defined(KTH_DB_READONLY)

#if ! defined(KTH_DB_READONLY)
// To push in order call with bucket = 0 and buckets = 1 (defaults).
bool data_base::push_transactions(domain::chain::block const& block, size_t height, uint32_t median_time_past, size_t bucket /*= 0*/, size_t buckets/*= 1*/) {
    KTH_ASSERT(bucket < buckets);
    auto const& txs = block.transactions();
    auto const count = txs.size();

    for (auto position = bucket; position < count; position = ceiling_add(position, buckets)) {
        auto const& tx = txs[position];

#ifdef KTH_DB_LEGACY
        transactions_->store(tx, height, median_time_past, position);
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
        transactions_unconfirmed_->unlink_if_exists(tx.hash());
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

        if (height < settings_.index_start_height) {
            continue;
        }

        auto const tx_hash = tx.hash();

#if defined(KTH_DB_SPENDS) || defined(KTH_DB_HISTORY)
        if (position != 0) {
            push_inputs(tx_hash, height, tx.inputs());
        }
#endif // defined(KTH_DB_SPENDS) || defined(KTH_DB_HISTORY)        

#ifdef KTH_DB_HISTORY
        push_outputs(tx_hash, height, tx.outputs());
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_STEALTH
        push_stealth(tx_hash, height, tx.outputs());
#endif // KTH_DB_STEALTH        
    }

    return true;
}
#endif //! defined(KTH_DB_READONLY)

#ifdef KTH_DB_LEGACY
#if ! defined(KTH_DB_READONLY)
bool data_base::push_heights(domain::chain::block const& block, size_t height) {
    transactions_->synchronize();
    auto const& txs = block.transactions();

    // Skip coinbase as it has no previous output.
    for (auto tx = txs.begin() + 1; tx != txs.end(); ++tx) {
        for (auto const& input: tx->inputs()) {
            if ( ! transactions_->spend(input.previous_output(), height)) {
                return false;
            }
        }
    }
    return true;
}
#endif //! defined(KTH_DB_READONLY)
#endif // KTH_DB_LEGACY


#if defined(KTH_DB_LEGACY) && (defined(KTH_DB_SPENDS) || defined(KTH_DB_HISTORY))
#if ! defined(KTH_DB_READONLY)
void data_base::push_inputs(hash_digest const& tx_hash, size_t height, const input::list& inputs) {
    
    for (uint32_t index = 0; index < inputs.size(); ++index) {
        auto const& input = inputs[index];
        const input_point inpoint {tx_hash, index};
        auto const& prevout = input.previous_output();

#ifdef KTH_DB_SPENDS
        spends_->store(prevout, inpoint);
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
        if (prevout.validation.cache.is_valid()) {
            // This results in a complete and unambiguous history for the
            // address since standard outputs contain unambiguous address data.
            for (auto const& address : prevout.validation.cache.addresses()) {
                history_->add_input(address.hash(), inpoint, height, prevout);
            }
        } else {
            // For any p2pk spend this creates no record (insufficient data).
            // For any p2kh spend this creates the ambiguous p2sh address,
            // which significantly expands the size of the history store.
            // These are tradeoffs when no prevout is cached (checkpoint sync).
            bool valid = true;
            for (auto const& address : input.addresses()) {
                if ( ! address) {
                    valid = false;
                    break;
                }
            }
            
            if (valid) {
                for (auto const& address : input.addresses()) {
                    history_->add_input(address.hash(), inpoint, height, prevout);
                }
            } else {
                //During an IBD with checkpoints some previous output info is missing.
                //We can recover it by accessing the database
                domain::chain::output prev_output;
                size_t output_height;
                uint32_t output_median_time_past;
                bool output_is_coinbase;

                if (transactions_->get_output(prev_output, output_height, output_median_time_past, output_is_coinbase, prevout, MAX_UINT64, false)) {
                    for (auto const& address : prev_output.addresses()) {
                        history_->add_input(address.hash(), inpoint, height, prevout);
                    }
                }
            }
        }
#endif // KTH_DB_HISTORY
    }
}
#endif // ! defined(KTH_DB_READONLY)
#endif // defined(KTH_DB_SPENDS) || defined(KTH_DB_HISTORY)


#ifdef KTH_DB_HISTORY
#if ! defined(KTH_DB_READONLY)
void data_base::push_outputs(hash_digest const& tx_hash, size_t height, const output::list& outputs) {
    for (uint32_t index = 0; index < outputs.size(); ++index) {
        auto const outpoint = output_point {tx_hash, index};
        auto const& output = outputs[index];
        auto const value = output.value();

        // Standard outputs contain unambiguous address data.
        for (auto const& address : output.addresses()) {
            history_->add_output(address.hash(), outpoint, height, value);
        }
    }
}
#endif // ! defined(KTH_DB_READONLY)
#endif // KTH_DB_HISTORY    

#ifdef KTH_DB_STEALTH
#if ! defined(KTH_DB_READONLY)
void data_base::push_stealth(hash_digest const& tx_hash, size_t height, const output::list& outputs) {
    if (outputs.empty())
        return;

    // Stealth outputs are paired by convention.
    for (size_t index = 0; index < (outputs.size() - 1); ++index) {
        auto const& ephemeral_script = outputs[index].script();
        auto const& payment_output = outputs[index + 1];

        // Try to extract the payment address from the second output.
        auto const address = payment_output.address();
        if ( ! address)
            continue;

        // Try to extract an unsigned ephemeral key from the first output.
        hash_digest unsigned_ephemeral_key;
        if ( ! extract_ephemeral_key(unsigned_ephemeral_key, ephemeral_script))
            continue;

        // Try to extract a stealth prefix from the first output.
        uint32_t prefix;
        if ( ! to_stealth_prefix(prefix, ephemeral_script))
            continue;

        // The payment address versions are arbitrary and unused here.
        const stealth_compact row {
            unsigned_ephemeral_key,
            address.hash(),
            tx_hash
        };

        stealth_->store(prefix, height, row);
    }
}
#endif // ! defined(KTH_DB_READONLY)
#endif // KTH_DB_STEALTH


#ifdef KTH_CURRENCY_BCH

#ifdef KTH_DB_LEGACY

// bool data_base::pop_input_and_unconfirm(size_t height, domain::chain::transaction const& tx) {
   
//     if ( ! pop_inputs(tx.inputs(), height)) {
//         return false;
//     }

// // #ifdef KTH_DB_TRANSACTION_UNCONFIRMED
// //     // if ( ! tx.is_coinbase()) {       //Note(fernando): satisfied by the precondition
// //     transactions_unconfirmed_->store(tx);
// //     // }
// // #endif // KTH_DB_TRANSACTION_UNCONFIRMED

// //     if ( ! transactions_->unconfirm(tx.hash())) {
// //         return false;
// //     }

//     return true;
// }

#if ! defined(KTH_DB_READONLY)
bool data_base::pop_output_and_unconfirm(size_t height, domain::chain::transaction const& tx) {
   
    if ( ! pop_outputs(tx.outputs(), height)) {
        return false;
    }

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    // if ( ! tx.is_coinbase()) {       //Note(fernando): satisfied by the precondition
    transactions_unconfirmed_->store(tx);
    // }
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

    if ( ! transactions_->unconfirm(tx.hash())) {
        return false;
    }

    return true;
}

template <typename I>
bool data_base::pop_transactions_inputs_unconfirm_non_coinbase(size_t height, I f, I l) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
    
    while (f != l) {
        auto const& tx = *f;
        if ( ! pop_inputs(tx.inputs(), height)) {
            return false;
        }

        // if ( ! pop_input_and_unconfirm(height, tx)) {
        //     return false;
        // }
        ++f;
    } 

    return true;
}

template <typename I>
bool data_base::pop_transactions_outputs_non_coinbase(size_t height, I f, I l) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
    
    while (f != l) {
        auto const& tx = *f;
        if ( ! pop_output_and_unconfirm(height, tx)) {
            return false;
        }
        ++f;
    } 
    return true;
}

template <typename I>
bool data_base::pop_transactions_non_coinbase(size_t height, I f, I l) {

    if ( ! pop_transactions_inputs_unconfirm_non_coinbase(height, f, l)) {
        return false;
    }
    return pop_transactions_outputs_non_coinbase(height, f, l);
}
#endif //! defined(KTH_DB_READONLY)
#endif // KTH_DB_LEGACY

#if ! defined(KTH_DB_READONLY)
// A false return implies store corruption.
bool data_base::pop(block& out_block) {
    
    auto const start_time = asio::steady_clock::now();

#ifdef KTH_DB_NEW
    if (internal_db_->pop_block(out_block) != result_code::success) {
        return false;
    }
#endif

#ifdef KTH_DB_LEGACY
    size_t height;
    
    // The blockchain is empty (nothing to pop, not even genesis).
    if ( ! blocks_->top(height)) {
        return false;
    }

    // This should never become invalid if this call is protected.
    auto const block = blocks_->get(height);
    if ( ! block) {
        return false;
    }

    auto const count = block.transaction_count();
    transaction::list transactions;
    transactions.reserve(count);

    for (size_t position = 0; position < count; ++position) {
        auto tx_hash = block.transaction_hash(position);
        auto const tx = transactions_->get(tx_hash, height, true);

        if ( ! tx || (tx.height() != height) || (tx.position() != position))
            return false;

        // Deserialize transaction and move it to the block.
        // The tx move/copy constructors do not currently transfer cache.
        transactions.emplace_back(tx.transaction(), std::move(tx_hash));

        // std::cout << encode_hash(tx_hash) << std::endl;
    }

    if (count > 1) {
        if ( ! pop_transactions_non_coinbase(height, transactions.begin() + 1, transactions.end())) {
            return false;
        }
    }

    auto const& coinbase = transactions.front();

    if ( ! pop_outputs(coinbase.outputs(), height)) {
        return false;
    }

    if ( ! transactions_->unconfirm(coinbase.hash())) {
        return false;
    }

    if ( ! blocks_->unlink(height)) {
        return false;
    }

    // Synchronise everything that was changed.
    synchronize();
   
    // Return the block (with header/block metadata and pop start time). 
    out_block = domain::chain::block(block.header(), std::move(transactions));

#endif // KTH_DB_LEGACY

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

#ifdef KTH_DB_NEW
    if (internal_db_->pop_block(out_block) != result_code::success) {
        return false;
    }
#endif

#ifdef KTH_DB_LEGACY
    size_t height;
    
    // The blockchain is empty (nothing to pop, not even genesis).
    if ( ! blocks_->top(height)) {
        return false;
    }

    // This should never become invalid if this call is protected.
    auto const block = blocks_->get(height);
    if ( ! block) {
        return false;
    }

    auto const count = block.transaction_count();
    transaction::list transactions;
    transactions.reserve(count);

    for (size_t position = 0; position < count; ++position) {
        auto tx_hash = block.transaction_hash(position);
        auto const tx = transactions_->get(tx_hash, height, true);

        if ( ! tx || (tx.height() != height) || (tx.position() != position))
            return false;

        // Deserialize transaction and move it to the block.
        // The tx move/copy constructors do not currently transfer cache.
        transactions.emplace_back(tx.transaction(), std::move(tx_hash));
    }

    // Remove txs, then outputs, then inputs (also reverse order).
    // Loops txs backwards even though they may have been added asynchronously.
    for (auto const& tx: reverse(transactions)) {
        if ( ! transactions_->unconfirm(tx.hash())) {
            return false;
        }

        if ( ! tx.is_coinbase()) {
#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
            transactions_unconfirmed_->store(tx);
#endif // KTH_DB_TRANSACTION_UNCONFIRMED
        }

        if ( ! pop_outputs(tx.outputs(), height)) {
            return false;
        }

        if ( ! tx.is_coinbase() && ! pop_inputs(tx.inputs(), height)) {
            return false;
        }
    }

    if ( ! blocks_->unlink(height)) {
        return false;
    }

    // Synchronise everything that was changed.
    synchronize();
   
    // Return the block (with header/block metadata and pop start time). 
    out_block = domain::chain::block(block.header(), std::move(transactions));

#endif // KTH_DB_LEGACY

    out_block.validation.error = error::success;
    out_block.validation.start_pop = start_time;
    return true;
}
#endif //! defined(KTH_DB_READONLY)
#endif // KTH_CURRENCY_BCH


#if ! defined(KTH_DB_READONLY)
// A false return implies store corruption.
bool data_base::pop_inputs(const input::list& inputs, size_t height) {
    // Loop in reverse.
    for (auto const& input: reverse(inputs)) {

#ifdef KTH_DB_LEGACY
        if ( ! transactions_->unspend(input.previous_output())) {
            return false;
        }
#endif // KTH_DB_LEGACY

        if (height < settings_.index_start_height)
            continue;

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
    DEBUG_ONLY(safe_add(in_blocks->size(), first_height));

    // This is the beginning of the push_all sequence.
    push_next(error::success, in_blocks, 0, first_height, dispatch, handler);
}

// TODO: resolve inconsistency with height and median_time_past passing.
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

    const result_handler next = std::bind(&data_base::push_next, this, _1, blocks, index + 1, height + 1, std::ref(dispatch), handler);

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

#ifdef KTH_DB_LEGACY
    result_handler block_complete = std::bind(&data_base::handle_push_transactions, this, _1, block, height, handler);

    // This ensures linkage and that the there is at least one tx.
    auto const ec = verify_push(*block, height);

    if (ec) {
        block_complete(ec);
        return;
    }

    auto const threads = dispatch.size();
    auto const buckets = std::min(threads, block->transactions().size());
    KTH_ASSERT(buckets != 0);

    auto const join_handler = kth::synchronize(std::move(block_complete), buckets, NAME "_do_push");

    for (size_t bucket = 0; bucket < buckets; ++bucket) {
        dispatch.concurrent(&data_base::do_push_transactions, this, block, height, median_time_past, bucket, buckets, join_handler);
    }
#endif // KTH_DB_LEGACY
}

void data_base::do_push_transactions(block_const_ptr block, size_t height, uint32_t median_time_past, size_t bucket, size_t buckets, result_handler handler) {
    auto const result = push_transactions(*block, height, median_time_past, bucket, buckets);
    handler(result ? error::success : error::operation_failed_7);
}

void data_base::handle_push_transactions(code const& ec, block_const_ptr block, size_t height, result_handler handler) {
    if (ec) {
        handler(ec);
        return;
    }
#ifdef KTH_DB_LEGACY
    if ( ! push_heights(*block, height)) {
        handler(error::operation_failed_8);
        return;
    }

    // Push the block header and synchronize to complete the block.
    blocks_->store(*block, height);

    // Synchronize tx updates, indexes and block.
    synchronize();
#endif // KTH_DB_LEGACY

    // Set push end time for the block.
    block->validation.end_push = asio::steady_clock::now();

    // This is the end of the block sub-sequence.
    handler(error::success);
}

// TODO: make async and concurrency as appropriate.
// This precludes popping the genesis block.
void data_base::pop_above(block_const_ptr_list_ptr out_blocks, hash_digest const& fork_hash, dispatcher&, result_handler handler) {
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

#ifdef KTH_DB_LEGACY
    size_t top;
    auto const result = blocks_->get(fork_hash);

    // The fork point does not exist or failed to get it or the top, fail.
    if ( ! result || ! blocks_->top(top)) {
        //**--**
        handler(error::operation_failed_9);
        return;
    }

    auto const fork = result.height();

#endif // KTH_DB_LEGACY

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

        // TODO: parallelize pop of transactions within each block.
        if ( ! pop(next)) {
            //**--**
            handler(error::operation_failed_10);
            return;
        }

        KTH_ASSERT(next.is_valid());
        auto block = std::make_shared<const message::block>(std::move(next));
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

/*
bool data_base::set_database_flags(bool fast) {
#ifdef KTH_DB_NEW
    return internal_db_->set_fast_flags_environment(fast);
#endif // KTH_DB_NEW
    return true;
}
*/


#if ! defined(KTH_DB_READONLY)
// This is designed for write exclusivity and read concurrency.
void data_base::reorganize(const checkpoint& fork_point, block_const_ptr_list_const_ptr incoming_blocks, block_const_ptr_list_ptr outgoing_blocks, dispatcher& dispatch, result_handler handler) {
    auto const next_height = safe_add(fork_point.height(), size_t(1));
    const result_handler pop_handler = std::bind(&data_base::handle_pop, this, _1, incoming_blocks, next_height, std::ref(dispatch), handler);


#ifdef KTH_DB_LEGACY
    // Critical Section.
    ///////////////////////////////////////////////////////////////////////////
    write_mutex_.lock();

    //**--**
    // Begin Flush Lock and Sequential Lock
    //vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    if ( ! begin_write()) {
        pop_handler(error::operation_failed_11);
        return;
    }
#endif // KTH_DB_LEGACY

    pop_above(outgoing_blocks, fork_point.hash(), dispatch, pop_handler);
}

void data_base::handle_pop(code const& ec, block_const_ptr_list_const_ptr incoming_blocks, size_t first_height, dispatcher& dispatch, result_handler handler) {
    const result_handler push_handler = std::bind(&data_base::handle_push, this, _1, handler);

    if (ec) {
        push_handler(ec);
        return;
    }

    push_all(incoming_blocks, first_height, std::ref(dispatch), push_handler);
}

// We never invoke the caller's handler under the mutex, we never fail to clear
// the mutex, and we always invoke the caller's handler exactly once.
void data_base::handle_push(code const& ec, result_handler handler) const {

#ifdef KTH_DB_LEGACY
    write_mutex_.unlock();
    // End Critical Section.
    ///////////////////////////////////////////////////////////////////////////
#endif // KTH_DB_LEGACY

    if (ec) {
        handler(ec);
        return;
    }

#if defined(KTH_DB_LEGACY)
    handler(end_write() ? error::success : error::operation_failed_12);
    // End Sequential Lock and Flush Lock
    //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#elif defined(KTH_DB_NEW)
    handler(error::success);
#else
#error You must define KTH_DB_LEGACY or KTH_DB_NEW
#endif
}
#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database
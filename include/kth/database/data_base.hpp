// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_DATA_BASE_HPP
#define KTH_DATABASE_DATA_BASE_HPP

#include <atomic>
#include <cstddef>
#include <memory>
#include <boost/filesystem.hpp>
#include <kth/domain.hpp>
#include <kth/database/define.hpp>

#ifdef KTH_DB_LEGACY
#include <kth/database/legacy/databases/block_database.hpp>
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
#include <kth/database/databases/internal_database.hpp>
#endif // KTH_DB_NEW

#ifdef KTH_DB_SPENDS
#include <kth/database/legacy/databases/spend_database.hpp>
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_LEGACY
#include <kth/database/legacy/databases/transaction_database.hpp>
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
#include <kth/database/legacy/databases/transaction_unconfirmed_database.hpp>
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_HISTORY
#include <kth/database/legacy/databases/history_database.hpp>
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_STEALTH
#include <kth/database/legacy/databases/stealth_database.hpp>
#endif // KTH_DB_STEALTH

#include <kth/database/define.hpp>
#include <kth/database/settings.hpp>
#include <kth/database/store.hpp>

namespace kth::database {

/// This class is thread safe and implements the sequential locking pattern.
class BCD_API data_base : public store, noncopyable {
public:
    using handle = store::handle;
    using result_handler = handle0;
    using path = boost::filesystem::path;

    // Construct.
    // ----------------------------------------------------------------------------

    data_base(settings const& settings);

    // Open and close.
    // ------------------------------------------------------------------------

#if ! defined(KTH_DB_READONLY)
    /// Create and open all databases.
    bool create(chain::block const& genesis);
#endif

#ifdef KTH_DB_LEGACY
    /// Open all databases.
    bool open() override;

    /// Close all databases.
    bool close() override;
#else
    /// Open all databases.
    bool open();

    /// Close all databases.
    bool close();
#endif// KTH_DB_LEGACY

    /// Call close on destruct.
    ~data_base();

    // Readers.
    // ------------------------------------------------------------------------

#ifdef KTH_DB_LEGACY
    const block_database& blocks() const;
    const transaction_database& transactions() const;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
    internal_database const& internal_db() const;
#endif // KTH_DB_NEW

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    const transaction_unconfirmed_database& transactions_unconfirmed() const;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_SPENDS
    /// Invalid if indexes not initialized.
    const spend_database& spends() const;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
    /// Invalid if indexes not initialized.
    const history_database& history() const;
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_STEALTH
    /// Invalid if indexes not initialized.
    const stealth_database& stealth() const;
#endif // KTH_DB_STEALTH

    // Synchronous writers.
    // ------------------------------------------------------------------------

#if ! defined(KTH_DB_READONLY)

#ifdef KTH_DB_LEGACY
    /// Create flush lock if flush_writes is true, and set sequential lock.
    bool begin_insert() const;

    /// Clear flush lock if flush_writes is true, and clear sequential lock.
    bool end_insert() const;
#endif // KTH_DB_LEGACY

    /// Store a block in the database.
    /// Returns store_block_duplicate if a block already exists at height.
    code insert(chain::block const& block, size_t height);

    /// Add an unconfirmed tx to the store (without indexing).
    /// Returns unspent_duplicate if existing unspent hash duplicate exists.
    code push(chain::transaction const& tx, uint32_t forks);

    /// Returns store_block_missing_parent if not linked.
    /// Returns store_block_invalid_height if height is not the current top + 1.
    code push(chain::block const& block, size_t height);

    code prune_reorg();

    //bool set_database_flags(bool fast);

    // Asynchronous writers.
    // ------------------------------------------------------------------------

    /// Invoke pop_all and then push_all under a common lock.
    void reorganize(config::checkpoint const& fork_point, block_const_ptr_list_const_ptr incoming_blocks, block_const_ptr_list_ptr outgoing_blocks, dispatcher& dispatch, result_handler handler);
#endif // ! defined(KTH_DB_READONLY)

protected:
    void start();

#if ! defined(KTH_DB_READONLY)
#ifdef KTH_DB_LEGACY
    void synchronize();
    bool flush() const override;
#endif // KTH_DB_LEGACY

    // Sets error if first_height is not the current top + 1 or not linked.
    void push_all(block_const_ptr_list_const_ptr in_blocks, size_t first_height, dispatcher& dispatch, result_handler handler);

    // Pop the set of blocks above the given hash.
    // Sets error if the database is corrupt or the hash doesn't exist.
    // Any blocks returned were successfully popped prior to any failure.
    void pop_above(block_const_ptr_list_ptr out_blocks, hash_digest const& fork_hash, dispatcher& dispatch, result_handler handler);
#endif // ! defined(KTH_DB_READONLY)

#ifdef KTH_DB_LEGACY
    std::shared_ptr<block_database> blocks_;
    std::shared_ptr<transaction_database> transactions_;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
    std::shared_ptr<internal_database> internal_db_;
#endif // KTH_DB_NEW

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    std::shared_ptr<transaction_unconfirmed_database> transactions_unconfirmed_;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#ifdef KTH_DB_SPENDS
    std::shared_ptr<spend_database> spends_;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
    std::shared_ptr<history_database> history_;
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_STEALTH
    std::shared_ptr<stealth_database> stealth_;
#endif // KTH_DB_STEALTH


private:
    using inputs = chain::input::list;
    using outputs = chain::output::list;

#if ! defined(KTH_DB_READONLY)
    /// TODO comment
    code push_genesis(chain::block const& block);

#ifdef KTH_DB_LEGACY
    code push_legacy(chain::block const& block, size_t height);
#endif

    // Synchronous writers.
    // ------------------------------------------------------------------------

    bool push_transactions(chain::block const& block, size_t height, uint32_t median_time_past, size_t bucket = 0, size_t buckets = 1);

#ifdef KTH_DB_LEGACY    
    bool push_heights(chain::block const& block, size_t height);
#endif

#if defined(KTH_DB_LEGACY) && (defined(KTH_DB_SPENDS) || defined(KTH_DB_HISTORY))
    void push_inputs(hash_digest const& tx_hash, size_t height, const inputs& inputs);
#endif // defined(KTH_DB_SPENDS) || defined(KTH_DB_HISTORY)    

#ifdef KTH_DB_HISTORY
    void push_outputs(hash_digest const& tx_hash, size_t height, const outputs& outputs);
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_STEALTH
    void push_stealth(hash_digest const& tx_hash, size_t height, const outputs& outputs);
#endif // KTH_DB_STEALTH

#if defined(KTH_CURRENCY_BCH) && defined(KTH_DB_LEGACY)
    // bool pop_input_and_unconfirm(size_t height, chain::transaction const& tx);
    bool pop_output_and_unconfirm(size_t height, chain::transaction const& tx);

    template <typename I>
    bool pop_transactions_inputs_unconfirm_non_coinbase(size_t height, I f, I l);

    template <typename I>
    bool pop_transactions_outputs_non_coinbase(size_t height, I f, I l);

    template <typename I>
    bool pop_transactions_non_coinbase(size_t height, I f, I l);
#endif 

    bool pop(chain::block& out_block);
    bool pop_inputs(const inputs& inputs, size_t height);
    bool pop_outputs(const outputs& outputs, size_t height);

#endif // ! defined(KTH_DB_READONLY)


    code verify_insert(chain::block const& block, size_t height);
    code verify_push(chain::block const& block, size_t height);

#if defined(KTH_DB_LEGACY)
    code verify_push(const chain::transaction& tx);
#endif

    // Asynchronous writers.
    // ------------------------------------------------------------------------
#if ! defined(KTH_DB_READONLY)
    void push_next(code const& ec, block_const_ptr_list_const_ptr blocks, size_t index, size_t height, dispatcher& dispatch, result_handler handler);
    void do_push(block_const_ptr block, size_t height, uint32_t median_time_past, dispatcher& dispatch, result_handler handler);
    void do_push_transactions(block_const_ptr block, size_t height, uint32_t median_time_past, size_t bucket, size_t buckets, result_handler handler);
    void handle_push_transactions(code const& ec, block_const_ptr block, size_t height, result_handler handler);

    void handle_pop(code const& ec, block_const_ptr_list_const_ptr incoming_blocks, size_t first_height, dispatcher& dispatch, result_handler handler);
    void handle_push(code const& ec, result_handler handler) const;
#endif // ! defined(KTH_DB_READONLY)





    std::atomic<bool> closed_;
    const settings& settings_;

#ifdef KTH_DB_LEGACY
    // Used to prevent concurrent unsafe writes.
    mutable shared_mutex write_mutex_;

    // Used to prevent concurrent file remapping.
    std::shared_ptr<shared_mutex> remap_mutex_;
#endif // KTH_DB_LEGACY

};

} // namespace kth::database

#endif

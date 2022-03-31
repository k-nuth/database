// Copyright (c) 2016-2022 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_DATA_BASE_HPP
#define KTH_DATABASE_DATA_BASE_HPP

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <memory>

#include <kth/domain.hpp>
#include <kth/database/define.hpp>

#ifdef KTH_DB_NEW
#include <kth/database/databases/internal_database.hpp>
#endif // KTH_DB_NEW

#ifdef KTH_DB_SPENDS
#include <kth/database/legacy/databases/spend_database.hpp>
#endif // KTH_DB_SPENDS


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

#include <kth/infrastructure/utility/noncopyable.hpp>

namespace kth::database {

/// This class is thread safe and implements the sequential locking pattern.
class KD_API data_base : public store, noncopyable {
public:
    using handle = store::handle;
    using result_handler = handle0;
    using path = kth::path;

    // Construct.
    // ----------------------------------------------------------------------------

    data_base(settings const& settings);

    // Open and close.
    // ------------------------------------------------------------------------

#if ! defined(KTH_DB_READONLY)
    /// Create and open all databases.
    bool create(domain::chain::block const& genesis);
#endif

#ifdef KTH_DB_LEGACY
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
    using inputs = domain::chain::input::list;
    using outputs = domain::chain::output::list;

#if ! defined(KTH_DB_READONLY)
    /// TODO comment
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


};

} // namespace kth::database

#endif

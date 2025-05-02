// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_INTERNAL_DATABASE_HPP_
#define KTH_DATABASE_INTERNAL_DATABASE_HPP_

#include <filesystem>
#include <string>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/cache.h>
#include <leveldb/filter_policy.h>
#include <leveldb/status.h>

#include <kth/database/databases/generic_db.hpp>
#include <kth/database/databases/property_code.hpp>

#include <kth/domain.hpp>
#include <kth/domain/chain/input_point.hpp>

#include <kth/database/define.hpp>

#include <kth/database/databases/header_abla_entry.hpp>
#include <kth/database/databases/result_code.hpp>
#include <kth/database/databases/property_code.hpp>
#include <kth/database/databases/tools.hpp>
#include <kth/database/databases/utxo_entry.hpp>
#include <kth/database/databases/history_entry.hpp>
#include <kth/database/databases/transaction_entry.hpp>
#include <kth/database/databases/transaction_unconfirmed_entry.hpp>

#include <leveldb/db.h>

// #include <kth/infrastructure.hpp>

#ifdef KTH_INTERNAL_DB_4BYTES_INDEX
#define KTH_INTERNAL_DB_WIRE true
#else
#define KTH_INTERNAL_DB_WIRE false
#endif

// Since LevelDB doesn't have the KTH_DB_CREATE flag, we just define this to avoid errors
#if defined(KTH_DB_READONLY)
#define KTH_DB_CONDITIONAL_CREATE 0
#else
#define KTH_DB_CONDITIONAL_CREATE 1
#endif

// For compatibility with existing code
#if defined(KTH_DB_READONLY)
#define KTH_DB_CONDITIONAL_READONLY 1
#else
#define KTH_DB_CONDITIONAL_READONLY 0
#endif

// For compatibility with LMDB transaction parameters
#define KTH_DB_txn void

namespace kth::database {

constexpr size_t max_dbs_full_ = 13;        // KTH_DB_NEW_FULL
constexpr size_t max_dbs_blocks_ = 8;      // KTH_DB_NEW_BLOCKS
constexpr size_t max_dbs_pruned_ = 7;       // KTH_DB_NEW_PRUNED

constexpr size_t env_open_mode_ = 0664;
constexpr int directory_exists = 0;

template <typename Clock = std::chrono::system_clock>
class KD_API internal_database_basis {
public:
    using path = kth::path;
    using utxo_pool_t = std::unordered_map<domain::chain::point, utxo_entry>;

    constexpr static char block_header_db_name[] = "block_header";
    constexpr static char block_header_by_hash_db_name[] = "block_header_by_hash";
    constexpr static char utxo_db_name[] = "utxo_db";
    constexpr static char reorg_pool_name[] = "reorg_pool";
    constexpr static char reorg_index_name[] = "reorg_index";
    constexpr static char reorg_block_name[] = "reorg_block";
    constexpr static char db_properties_name[] = "properties";

    //Blocks DB
    constexpr static char block_db_name[] = "blocks";

    //Transactions
    constexpr static char transaction_db_name[] = "transactions";
    constexpr static char transaction_hash_db_name[] = "transactions_hash";
    constexpr static char history_db_name[] = "history";
    constexpr static char spend_db_name[] = "spend";
    constexpr static char transaction_unconfirmed_db_name[] = "transaction_unconfirmed";

    internal_database_basis(path const& db_dir, db_mode_type mode, uint32_t reorg_pool_limit, uint64_t db_max_size, bool safe_mode);
    ~internal_database_basis();

    // Non-copyable, non-movable
    internal_database_basis(internal_database_basis const&) = delete;
    internal_database_basis& operator=(internal_database_basis const&) = delete;

#if ! defined(KTH_DB_READONLY)
    bool create();
#endif

    bool open();
    bool close();

#if ! defined(KTH_DB_READONLY)
    result_code push_genesis(domain::chain::block const& block);

    //TODO(fernando): optimization: consider passing a list of outputs to insert and a list of inputs to delete instead of an entire Block.
    //                  avoiding inserting and erasing internal spenders
    result_code push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past);
#endif

    utxo_entry get_utxo(domain::chain::output_point const& point) const;

    result_code get_last_height(uint32_t& out_height) const;

    std::pair<domain::chain::header, uint32_t> get_header(hash_digest const& hash) const;
    domain::chain::header get_header(uint32_t height) const;
    domain::chain::header::list get_headers(uint32_t from, uint32_t to) const;
    std::optional<header_with_abla_state_t> get_header_and_abla_state(uint32_t height) const;

#if ! defined(KTH_DB_READONLY)
    result_code pop_block(domain::chain::block& out_block);

    result_code prune();
#endif

    std::pair<result_code, utxo_pool_t> get_utxo_pool_from(uint32_t from, uint32_t to) const;

    std::pair<domain::chain::block, uint32_t> get_block(hash_digest const& hash) const;
    domain::chain::block get_block(uint32_t height) const;

    transaction_entry get_transaction(hash_digest const& hash, size_t fork_height) const;

    domain::chain::history_compact::list get_history(short_hash const& key, size_t limit, size_t from_height) const;
    std::vector<hash_digest> get_history_txns(short_hash const& key, size_t limit, size_t from_height) const;

    domain::chain::input_point get_spend(domain::chain::output_point const& point) const;

    std::vector<transaction_unconfirmed_entry> get_all_transaction_unconfirmed() const;

    transaction_unconfirmed_entry get_transaction_unconfirmed(hash_digest const& hash) const;

#if ! defined(KTH_DB_READONLY)
    result_code push_transaction_unconfirmed(domain::chain::transaction const& tx, uint32_t height);
#endif // ! defined(KTH_DB_READONLY)

private:

#if ! defined(KTH_DB_READONLY)
    bool create_db_mode_property();
#endif

    bool verify_db_mode_property() const;

    bool open_internal();

    bool is_old_block(domain::chain::block const& block) const;

    size_t get_db_page_size() const;

    size_t adjust_db_size(size_t size) const;

    bool create_and_open_environment();

    bool open_databases();

    utxo_entry get_utxo(domain::chain::output_point const& point, KTH_DB_txn* /*unused*/) const;

#if ! defined(KTH_DB_READONLY)
    result_code insert_reorg_pool(uint32_t height, domain::chain::output_point const& point, leveldb::WriteBatch& batch);

    result_code remove_utxo(uint32_t height, domain::chain::output_point const& point, bool insert_reorg, leveldb::WriteBatch& batch);

    result_code insert_utxo(domain::chain::output_point const& point, domain::chain::output const& output, data_chunk const& fixed_data, leveldb::WriteBatch& batch);

    result_code remove_inputs(hash_digest const& tx_id, uint32_t height, domain::chain::input::list const& inputs, bool insert_reorg, leveldb::WriteBatch& batch);

    result_code insert_outputs(hash_digest const& tx_id, uint32_t height, domain::chain::output::list const& outputs, data_chunk const& fixed_data, leveldb::WriteBatch& batch);

    result_code insert_outputs_error_treatment(uint32_t height, data_chunk const& fixed_data, hash_digest const& txid, domain::chain::output::list const& outputs, leveldb::WriteBatch& batch);

    template <typename I>
    result_code push_transactions_outputs_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, leveldb::WriteBatch& batch);

    template <typename I>
    result_code remove_transactions_inputs_non_coinbase(uint32_t height, I f, I l, bool insert_reorg, leveldb::WriteBatch& batch);

    template <typename I>
    result_code push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, leveldb::WriteBatch& batch);

    result_code push_block_header(domain::chain::block const& block, uint32_t height, leveldb::WriteBatch& batch);

    result_code push_block_reorg(domain::chain::block const& block, uint32_t height, leveldb::WriteBatch& batch);

    result_code push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg, leveldb::WriteBatch& batch);

    result_code push_genesis(domain::chain::block const& block, leveldb::WriteBatch& batch);

    result_code remove_outputs(hash_digest const& txid, domain::chain::output::list const& outputs, leveldb::WriteBatch& batch);

    result_code insert_output_from_reorg_and_remove(domain::chain::output_point const& point, leveldb::WriteBatch& batch);

    result_code insert_inputs(domain::chain::input::list const& inputs, leveldb::WriteBatch& batch);

    template <typename I>
    result_code insert_transactions_inputs_non_coinbase(I f, I l, leveldb::WriteBatch& batch);

    template <typename I>
    result_code remove_transactions_outputs_non_coinbase(I f, I l, leveldb::WriteBatch& batch);

    template <typename I>
    result_code remove_transactions_non_coinbase(I f, I l, leveldb::WriteBatch& batch);

    result_code remove_block_header(hash_digest const& hash, uint32_t height, leveldb::WriteBatch& batch);

    result_code remove_block_reorg(uint32_t height, leveldb::WriteBatch& batch);

    result_code remove_reorg_index(uint32_t height, leveldb::WriteBatch& batch);

    result_code remove_block(domain::chain::block const& block, uint32_t height, leveldb::WriteBatch& batch);
#endif

    domain::chain::header get_header(uint32_t height, KTH_DB_txn* /*unused*/) const;
    std::optional<header_with_abla_state_t> get_header_and_abla_state(uint32_t height, KTH_DB_txn* /*unused*/) const;

    domain::chain::block get_block_reorg(uint32_t height, KTH_DB_txn* /*unused*/) const;
    domain::chain::block get_block_reorg(uint32_t height) const;

#if ! defined(KTH_DB_READONLY)
    result_code remove_block(domain::chain::block const& block, uint32_t height);
    result_code prune_reorg_index(uint32_t remove_until, leveldb::WriteBatch& batch);
    result_code prune_reorg_block(uint32_t amount_to_delete, leveldb::WriteBatch& batch);
#endif

    result_code get_first_reorg_block_height(uint32_t& out_height) const;

    // Helper function to insert reorg into pool
    result_code insert_reorg_into_pool(utxo_pool_t& pool, const leveldb::Slice& key_point) const;

#if ! defined(KTH_DB_READONLY)
    result_code remove_blocks_db(uint32_t height, leveldb::WriteBatch& batch);
#endif

    domain::chain::block get_block(uint32_t height, KTH_DB_txn* /*unused*/) const;

    domain::chain::block get_block(hash_digest const& hash, KTH_DB_txn* /*unused*/) const;

#if ! defined(KTH_DB_READONLY)
    result_code insert_block(domain::chain::block const& block, uint32_t height, uint64_t tx_count, leveldb::WriteBatch& batch);

    result_code remove_transactions(domain::chain::block const& block, uint32_t height, leveldb::WriteBatch& batch);

    result_code insert_transaction(uint64_t id, domain::chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position, leveldb::WriteBatch& batch);

    template <typename I>
    result_code insert_transactions(I f, I l, uint32_t height, uint32_t median_time_past, uint64_t tx_count, leveldb::WriteBatch& batch);
#endif // ! defined(KTH_DB_READONLY)

    transaction_entry get_transaction(hash_digest const& hash, size_t fork_height, KTH_DB_txn* /*unused*/) const;
    transaction_entry get_transaction(uint64_t id, KTH_DB_txn* /*unused*/) const;

#if ! defined(KTH_DB_READONLY)
    result_code insert_input_history(domain::chain::input_point const& inpoint, uint32_t height, domain::chain::input const& input, leveldb::WriteBatch& batch);

    result_code insert_output_history(hash_digest const& tx_hash, uint32_t height, uint32_t index, domain::chain::output const& output, leveldb::WriteBatch& batch);

    result_code insert_history_db(domain::wallet::payment_address const& address, data_chunk const& entry, leveldb::WriteBatch& batch);
#endif // ! defined(KTH_DB_READONLY)

    static
    domain::chain::history_compact history_entry_to_history_compact(history_entry const& entry);

#if ! defined(KTH_DB_READONLY)
    result_code remove_history_db(short_hash const& key, size_t height, leveldb::WriteBatch& batch);

    result_code remove_transaction_history_db(domain::chain::transaction const& tx, size_t height, leveldb::WriteBatch& batch);

    result_code insert_spend(domain::chain::output_point const& out_point, domain::chain::input_point const& in_point, leveldb::WriteBatch& batch);

    result_code remove_spend(domain::chain::output_point const& out_point, leveldb::WriteBatch& batch);

    result_code remove_transaction_spend_db(domain::chain::transaction const& tx, leveldb::WriteBatch& batch);

    result_code insert_transaction_unconfirmed(domain::chain::transaction const& tx, uint32_t height, leveldb::WriteBatch& batch);

    result_code remove_transaction_unconfirmed(hash_digest const& tx_id, leveldb::WriteBatch& batch);
#endif

    transaction_unconfirmed_entry get_transaction_unconfirmed(hash_digest const& hash, KTH_DB_txn* /*unused*/) const;

#if ! defined(KTH_DB_READONLY)
    result_code update_transaction(domain::chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position, leveldb::WriteBatch& batch);

    result_code set_spend(domain::chain::output_point const& point, uint32_t spender_height, leveldb::WriteBatch& batch);

    result_code set_unspend(domain::chain::output_point const& point, leveldb::WriteBatch& batch);
#endif // ! defined(KTH_DB_READONLY)

    uint32_t get_clock_now() const;

    uint64_t get_tx_count(KTH_DB_txn* /*unused*/) const;

    uint64_t get_history_count() const;

// Data members ----------------------------
    path const db_dir_;
    std::string db_path_;  // String version of db_dir_ for LevelDB
    uint32_t reorg_pool_limit_;
    std::chrono::seconds const limit_;
    bool db_opened_ = false;
    db_mode_type db_mode_;
    uint64_t db_max_size_;
    bool safe_mode_;

    // LevelDB database handle
    leveldb::DB* db_ = nullptr;

    // Column family handles
    leveldb::ColumnFamilyHandle* cf_default_ = nullptr;
    leveldb::ColumnFamilyHandle* cf_block_header_ = nullptr;
    leveldb::ColumnFamilyHandle* cf_block_header_by_hash_ = nullptr;
    leveldb::ColumnFamilyHandle* cf_utxo_db_ = nullptr;
    
    leveldb::ColumnFamilyHandle* cf_reorg_pool_ = nullptr;
    leveldb::ColumnFamilyHandle* cf_reorg_index_ = nullptr;
    leveldb::ColumnFamilyHandle* cf_reorg_block_ = nullptr;
    
    leveldb::ColumnFamilyHandle* cf_properties_ = nullptr;
    
    leveldb::ColumnFamilyHandle* cf_block_db_ = nullptr;
    
    leveldb::ColumnFamilyHandle* cf_transaction_db_ = nullptr;
    leveldb::ColumnFamilyHandle* cf_transaction_hash_db_ = nullptr;
    leveldb::ColumnFamilyHandle* cf_history_db_ = nullptr;
    leveldb::ColumnFamilyHandle* cf_spend_db_ = nullptr;
    leveldb::ColumnFamilyHandle* cf_transaction_unconfirmed_db_ = nullptr;
};

template <typename Clock>
constexpr char internal_database_basis<Clock>::block_header_db_name[];           //key: block height, value: block header

template <typename Clock>
constexpr char internal_database_basis<Clock>::block_header_by_hash_db_name[];   //key: block hash, value: block height

template <typename Clock>
constexpr char internal_database_basis<Clock>::utxo_db_name[];                   //key: point, value: output

template <typename Clock>
constexpr char internal_database_basis<Clock>::reorg_pool_name[];                //key: key: point, value: output

template <typename Clock>
constexpr char internal_database_basis<Clock>::reorg_index_name[];               //key: block height, value: point list

template <typename Clock>
constexpr char internal_database_basis<Clock>::reorg_block_name[];               //key: block height, value: block

template <typename Clock>
constexpr char internal_database_basis<Clock>::db_properties_name[];             //key: propery, value: data

template <typename Clock>
constexpr char internal_database_basis<Clock>::block_db_name[];                  //key: block height, value: block
                                                                                 //key: block height, value: tx hashes
template <typename Clock>
constexpr char internal_database_basis<Clock>::transaction_db_name[];            //key: tx hash, value: tx

template <typename Clock>
constexpr char internal_database_basis<Clock>::transaction_hash_db_name[];            //key: tx hash, value: tx

template <typename Clock>
constexpr char internal_database_basis<Clock>::history_db_name[];            //key: tx hash, value: tx

template <typename Clock>
constexpr char internal_database_basis<Clock>::spend_db_name[];            //key: output_point, value: input_point

template <typename Clock>
constexpr char internal_database_basis<Clock>::transaction_unconfirmed_db_name[];     //key: tx hash, value: tx

using internal_database = internal_database_basis<std::chrono::system_clock>;

} // namespace kth::database


#include <kth/database/databases/block_database.ipp>
#include <kth/database/databases/header_database.ipp>
#include <kth/database/databases/history_database.ipp>
#include <kth/database/databases/spend_database.ipp>
#include <kth/database/databases/transaction_unconfirmed_database.ipp>
#include <kth/database/databases/internal_database.ipp>
#include <kth/database/databases/reorg_database.ipp>
#include <kth/database/databases/transaction_database.ipp>
#include <kth/database/databases/utxo_database.ipp>

#endif // KTH_DATABASE_INTERNAL_DATABASE_HPP_

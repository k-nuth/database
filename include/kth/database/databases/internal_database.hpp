// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_INTERNAL_DATABASE_HPP_
#define KTH_DATABASE_INTERNAL_DATABASE_HPP_

#include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <lmdb.h>

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/bitcoin/chain/input_point.hpp>

#include <bitcoin/database/define.hpp>

#include <knuth/database/databases/result_code.hpp>
#include <knuth/database/databases/property_code.hpp>
#include <knuth/database/databases/tools.hpp>
#include <knuth/database/databases/utxo_entry.hpp>
#include <knuth/database/databases/history_entry.hpp>
#include <knuth/database/databases/transaction_entry.hpp>
#include <knuth/database/databases/transaction_unconfirmed_entry.hpp>

#ifdef KTH_INTERNAL_DB_4BYTES_INDEX
#define KTH_INTERNAL_DB_WIRE true
#else
#define KTH_INTERNAL_DB_WIRE false
#endif

namespace kth {
namespace database {

#if defined(KTH_DB_NEW_BLOCKS)
constexpr size_t max_dbs_ = 8;
#elif defined(KTH_DB_NEW_FULL)
constexpr size_t max_dbs_ = 13;
#else
constexpr size_t max_dbs_ = 7;
#endif

constexpr size_t env_open_mode_ = 0664;
constexpr int directory_exists = 0;

template <typename Clock = std::chrono::system_clock>
class BCD_API internal_database_basis {
public:
    using path = boost::filesystem::path;
    using utxo_pool_t = std::unordered_map<chain::point, utxo_entry>;

    constexpr static char block_header_db_name[] = "block_header";
    constexpr static char block_header_by_hash_db_name[] = "block_header_by_hash";
    constexpr static char utxo_db_name[] = "utxo_db";
    constexpr static char reorg_pool_name[] = "reorg_pool";
    constexpr static char reorg_index_name[] = "reorg_index";
    constexpr static char reorg_block_name[] = "reorg_block";
    constexpr static char db_properties_name[] = "properties";
  
#if defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)
    //Blocks DB
    constexpr static char block_db_name[] = "blocks";
#endif //defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)
    
#if defined(KTH_DB_NEW_FULL)
    //Transactions
    constexpr static char transaction_db_name[] = "transactions";
    constexpr static char transaction_hash_db_name[] = "transactions_hash";
    constexpr static char history_db_name[] = "history";
    constexpr static char spend_db_name[] = "spend";
    constexpr static char transaction_unconfirmed_db_name[] = "transaction_unconfirmed";
#endif  //KTH_DB_NEW_FULL

    internal_database_basis(path const& db_dir, uint32_t reorg_pool_limit, uint64_t db_max_size, bool safe_mode);
    ~internal_database_basis();

    // Non-copyable, non-movable
    internal_database_basis(internal_database_basis const&) = delete;
    internal_database_basis& operator=(internal_database_basis const&) = delete;

    bool create();
    bool open();
    bool close();

    result_code push_genesis(chain::block const& block);

    //TODO(fernando): optimization: consider passing a list of outputs to insert and a list of inputs to delete instead of an entire Block.
    //                  avoiding inserting and erasing internal spenders
    result_code push_block(chain::block const& block, uint32_t height, uint32_t median_time_past);
    
    utxo_entry get_utxo(chain::output_point const& point) const;
    
    result_code get_last_height(uint32_t& out_height) const;
    
    std::pair<chain::header, uint32_t> get_header(hash_digest const& hash) const;
    chain::header get_header(uint32_t height) const;
    
    result_code pop_block(chain::block& out_block);
    
    result_code prune();
    
    std::pair<result_code, utxo_pool_t> get_utxo_pool_from(uint32_t from, uint32_t to) const;

    //bool set_fast_flags_environment(bool enabled);
    
// #if defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)
    std::pair<chain::block, uint32_t> get_block(hash_digest const& hash) const;
    
    chain::block get_block(uint32_t height) const;
// #endif //KTH_DB_NEW_BLOCKS || KTH_DB_NEW_FULL

#if defined(KTH_DB_NEW_FULL)
    transaction_entry get_transaction(hash_digest const& hash, size_t fork_height) const;
    
    chain::history_compact::list get_history(short_hash const& key, size_t limit, size_t from_height) const;
    std::vector<hash_digest> get_history_txns(short_hash const& key, size_t limit, size_t from_height) const;
    
    chain::input_point get_spend(chain::output_point const& point) const;

    std::vector<transaction_unconfirmed_entry> get_all_transaction_unconfirmed() const;

    transaction_unconfirmed_entry get_transaction_unconfirmed(hash_digest const& hash) const;

    result_code push_transaction_unconfirmed(chain::transaction const& tx, uint32_t height);
#endif

private:
    
    bool create_db_mode_property();

    bool verify_db_mode_property();

    bool open_internal();
    
    bool is_old_block(chain::block const& block) const;

    size_t get_db_page_size() const;

    size_t adjust_db_size(size_t size) const;

    bool create_and_open_environment();

    bool open_databases();

    utxo_entry get_utxo(chain::output_point const& point, MDB_txn* db_txn) const;

    result_code insert_reorg_pool(uint32_t height, MDB_val& key, MDB_txn* db_txn);
    
    result_code remove_utxo(uint32_t height, chain::output_point const& point, bool insert_reorg, MDB_txn* db_txn);
    
    result_code insert_utxo(chain::output_point const& point, chain::output const& output, data_chunk const& fixed_data, MDB_txn* db_txn);

    result_code remove_inputs(hash_digest const& tx_id, uint32_t height, chain::input::list const& inputs, bool insert_reorg, MDB_txn* db_txn);

    result_code insert_outputs(hash_digest const& tx_id, uint32_t height, chain::output::list const& outputs, data_chunk const& fixed_data, MDB_txn* db_txn);

    result_code insert_outputs_error_treatment(uint32_t height, data_chunk const& fixed_data, hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn);

    template <typename I>
    result_code push_transactions_outputs_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, MDB_txn* db_txn);

    template <typename I>
    result_code remove_transactions_inputs_non_coinbase(uint32_t height, I f, I l, bool insert_reorg, MDB_txn* db_txn);

    template <typename I>
    result_code push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, MDB_txn* db_txn);

    result_code push_block_header(chain::block const& block, uint32_t height, MDB_txn* db_txn);
    
    result_code push_block_reorg(chain::block const& block, uint32_t height, MDB_txn* db_txn);

    result_code push_block(chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg, MDB_txn* db_txn);

    result_code push_genesis(chain::block const& block, MDB_txn* db_txn);

    result_code remove_outputs(hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn);

    result_code insert_output_from_reorg_and_remove(chain::output_point const& point, MDB_txn* db_txn);

    result_code insert_inputs(chain::input::list const& inputs, MDB_txn* db_txn);

    template <typename I>
    result_code insert_transactions_inputs_non_coinbase(I f, I l, MDB_txn* db_txn);

    template <typename I>
    result_code remove_transactions_outputs_non_coinbase(I f, I l, MDB_txn* db_txn);

    template <typename I>
    result_code remove_transactions_non_coinbase(I f, I l, MDB_txn* db_txn);

    result_code remove_block_header(hash_digest const& hash, uint32_t height, MDB_txn* db_txn);

    result_code remove_block_reorg(uint32_t height, MDB_txn* db_txn);
    
    result_code remove_reorg_index(uint32_t height, MDB_txn* db_txn);
    
    result_code remove_block(chain::block const& block, uint32_t height, MDB_txn* db_txn);

    chain::header get_header(uint32_t height, MDB_txn* db_txn) const;

    chain::block get_block_reorg(uint32_t height, MDB_txn* db_txn) const;
       
    chain::block get_block_reorg(uint32_t height) const;

    result_code remove_block(chain::block const& block, uint32_t height);
    
    result_code prune_reorg_index(uint32_t remove_until, MDB_txn* db_txn);
    
    result_code prune_reorg_block(uint32_t amount_to_delete, MDB_txn* db_txn);
    
    result_code get_first_reorg_block_height(uint32_t& out_height) const;

    result_code insert_reorg_into_pool(utxo_pool_t& pool, MDB_val key_point, MDB_txn* db_txn) const;

#if defined(KTH_DB_NEW_BLOCKS)
    result_code insert_block(chain::block const& block, uint32_t height, MDB_txn* db_txn);
#endif //defined(KTH_DB_NEW_BLOCKS)


#if defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)
    result_code remove_blocks_db(uint32_t height, MDB_txn* db_txn);
#endif //defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)

    chain::block get_block(uint32_t height, MDB_txn* db_txn) const;

#if defined(KTH_DB_NEW_FULL)
    
    result_code insert_block(chain::block const& block, uint32_t height, uint64_t tx_count, MDB_txn* db_txn);

    result_code remove_transactions(chain::block const& block, uint32_t height, MDB_txn* db_txn);
    
    result_code insert_transaction(uint64_t id, chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position , MDB_txn* db_txn);
    
    //data_chunk serialize_txs(chain::block const& block);
    
    template <typename I>
    result_code insert_transactions(I f, I l, uint32_t height, uint32_t median_time_past,uint64_t tx_count, MDB_txn* db_txn);
    
    transaction_entry get_transaction(hash_digest const& hash, size_t fork_height, MDB_txn* db_txn) const;
    transaction_entry get_transaction(uint64_t id, MDB_txn* db_txn) const;
    
    result_code insert_input_history(chain::input_point const& inpoint, uint32_t height, chain::input const& input, MDB_txn* db_txn);

    result_code insert_output_history(hash_digest const& tx_hash,uint32_t height, uint32_t index, chain::output const& output, MDB_txn* db_txn);
    
    result_code insert_history_db (wallet::payment_address const& address, data_chunk const& entry, MDB_txn* db_txn); 
    
    static
    chain::history_compact history_entry_to_history_compact(history_entry const& entry);
    
    result_code remove_history_db(const short_hash& key, size_t height, MDB_txn* db_txn);
    
    result_code remove_transaction_history_db(chain::transaction const& tx, size_t height, MDB_txn* db_txn);
    
    result_code insert_spend(chain::output_point const& out_point, chain::input_point const& in_point, MDB_txn* db_txn);
    
    result_code remove_spend(chain::output_point const& out_point, MDB_txn* db_txn);
    
    result_code remove_transaction_spend_db(chain::transaction const& tx, MDB_txn* db_txn);

    result_code insert_transaction_unconfirmed(chain::transaction const& tx, uint32_t height, MDB_txn* db_txn);

    result_code remove_transaction_unconfirmed(hash_digest const& tx_id,  MDB_txn* db_txn);

    transaction_unconfirmed_entry get_transaction_unconfirmed(hash_digest const& hash, MDB_txn* db_txn) const;

    result_code update_transaction(chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position, MDB_txn* db_txn);

    result_code set_spend(chain::output_point const& point, uint32_t spender_height, MDB_txn* db_txn);

    result_code set_unspend(chain::output_point const& point, MDB_txn* db_txn);

    uint32_t get_clock_now();

    uint64_t get_tx_count(MDB_txn* db_txn);

    uint64_t get_history_count(MDB_txn* db_txn);

    

#endif //defined(KTH_DB_NEW_FULL)

// Data members ----------------------------
    path const db_dir_;
    uint32_t reorg_pool_limit_;                 //TODO(fernando): check if uint32_max is needed for NO-LIMIT???
    std::chrono::seconds const limit_;
    bool env_created_ = false;
    bool db_opened_ = false;
    uint64_t db_max_size_;
    bool safe_mode_;
    //bool fast_mode = false;

    MDB_env* env_;
    MDB_dbi dbi_block_header_;
    MDB_dbi dbi_block_header_by_hash_;
    MDB_dbi dbi_utxo_;
    MDB_dbi dbi_reorg_pool_;
    MDB_dbi dbi_reorg_index_;
    MDB_dbi dbi_reorg_block_;
    MDB_dbi dbi_properties_;

#if defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)
    //Blocks DB
    MDB_dbi dbi_block_db_;
#endif 

#ifdef KTH_DB_NEW_FULL
    MDB_dbi dbi_transaction_db_;
    MDB_dbi dbi_transaction_hash_db_;
    MDB_dbi dbi_history_db_;
    MDB_dbi dbi_spend_db_;
    MDB_dbi dbi_transaction_unconfirmed_db_;
#endif
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
constexpr char internal_database_basis<Clock>::db_properties_name[];                   //key: propery, value: data

#if defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)
template <typename Clock>
constexpr char internal_database_basis<Clock>::block_db_name[];                  //key: block height, value: block
                                                                                 //key: block height, value: tx hashes   
#endif

#ifdef KTH_DB_NEW_FULL
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


#endif

using internal_database = internal_database_basis<std::chrono::system_clock>;

} // namespace database
} // namespace kth


#include <knuth/database/databases/block_database.ipp>
#include <knuth/database/databases/header_database.ipp>
#include <knuth/database/databases/history_database.ipp>
#include <knuth/database/databases/spend_database.ipp>
#include <knuth/database/databases/transaction_unconfirmed_database.ipp>
#include <knuth/database/databases/internal_database.ipp>
#include <knuth/database/databases/reorg_database.ipp>
#include <knuth/database/databases/transaction_database.ipp>
#include <knuth/database/databases/utxo_database.ipp>

#endif // KTH_DATABASE_INTERNAL_DATABASE_HPP_

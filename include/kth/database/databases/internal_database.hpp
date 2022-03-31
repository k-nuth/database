// Copyright (c) 2016-2022 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_INTERNAL_DATABASE_HPP_
#define KTH_DATABASE_INTERNAL_DATABASE_HPP_

#include <filesystem>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <kth/database/databases/generic_db.hpp>

#include <kth/domain.hpp>
#include <kth/domain/chain/input_point.hpp>

#include <kth/database/define.hpp>

#include <kth/database/databases/result_code.hpp>
#include <kth/database/databases/property_code.hpp>
#include <kth/database/databases/tools.hpp>
#include <kth/database/databases/utxo_entry.hpp>
#include <kth/database/databases/history_entry.hpp>
#include <kth/database/databases/transaction_entry.hpp>
#include <kth/database/databases/transaction_unconfirmed_entry.hpp>

#include <kth/infrastructure.hpp>

#ifdef KTH_INTERNAL_DB_4BYTES_INDEX
#define KTH_INTERNAL_DB_WIRE true
#else
#define KTH_INTERNAL_DB_WIRE false
#endif

#if defined(KTH_DB_READONLY)
#define KTH_DB_CONDITIONAL_CREATE 0
#else
#define KTH_DB_CONDITIONAL_CREATE KTH_DB_CREATE
#endif

#if defined(KTH_DB_READONLY)
#define KTH_DB_CONDITIONAL_READONLY KTH_DB_RDONLY
#else
#define KTH_DB_CONDITIONAL_READONLY 0
#endif


// Standard hash.
//-----------------------------------------------------------------------------

#include <boost/functional/hash.hpp>


namespace std {

template <>
struct hash<kth::domain::chain::output_point> {
    size_t operator()(kth::domain::chain::output_point const& point) const {
        size_t seed = 0;
        boost::hash_combine(seed, point.hash());
        boost::hash_combine(seed, point.index());
        return seed;
    }
};

} // namespace std



namespace kth::database {

constexpr int directory_exists = 0;

template <typename Clock = std::chrono::system_clock>
class KD_API internal_database_basis {
public:
    using path = kth::path;
    using utxo_pool_t = std::unordered_map<domain::chain::point, utxo_entry>;

    internal_database_basis(path const& db_dir, uint32_t reorg_pool_limit, uint64_t db_max_size, bool safe_mode);
    ~internal_database_basis();

    // Non-copyable, non-movable
    internal_database_basis(internal_database_basis const&) = delete;
    internal_database_basis& operator=(internal_database_basis const&) = delete;

    bool create();
    bool open();
    bool close();

    result_code push_genesis(domain::chain::block const& block);
    result_code push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past);
    utxo_entry get_utxo(domain::chain::output_point const& point) const;
    result_code get_last_height(uint32_t& out_height) const;
    std::pair<domain::chain::header, uint32_t> get_header(hash_digest const& hash) const;
    domain::chain::header get_header(uint32_t height) const;
    domain::chain::header::list get_headers(uint32_t from, uint32_t to) const;
    result_code pop_block(domain::chain::block& out_block);
    result_code prune();
    std::pair<result_code, utxo_pool_t> get_utxo_pool_from(uint32_t from, uint32_t to) const;
    std::pair<domain::chain::block, uint32_t> get_block(hash_digest const& hash) const;
    domain::chain::block get_block(uint32_t height) const;

private:
    bool create_db_mode_property();
    bool verify_db_mode_property() const;
    bool open_internal();
    bool is_old_block(domain::chain::block const& block) const;
    bool create_and_open_environment();
    bool open_databases();
    result_code insert_reorg_pool(uint32_t height, KTH_DB_val& key);
    result_code remove_utxo(uint32_t height, domain::chain::output_point const& point, bool insert_reorg);
    result_code insert_utxo(domain::chain::output_point const& point, domain::chain::output const& output, uint32_t height, uint32_t median_time_past, bool coinbase);
    result_code remove_inputs(hash_digest const& tx_id, uint32_t height, domain::chain::input::list const& inputs, bool insert_reorg);
    result_code insert_outputs(hash_digest const& tx_id, uint32_t height, domain::chain::output::list const& outputs, uint32_t median_time_past, bool coinbase);
    result_code insert_outputs_error_treatment(uint32_t height, uint32_t median_time_past, bool coinbase, hash_digest const& txid, domain::chain::output::list const& outputs);

    template <typename I>
    result_code push_transactions_outputs_non_coinbase(uint32_t height, uint32_t median_time_past, bool coinbase, I f, I l);

    template <typename I>
    result_code remove_transactions_inputs_non_coinbase(uint32_t height, I f, I l, bool insert_reorg);

    template <typename I>
    result_code push_transactions_non_coinbase(uint32_t height, uint32_t median_time_past, bool coinbase, I f, I l, bool insert_reorg);

    result_code push_block_header(domain::chain::block const& block, uint32_t height);
    result_code push_block_reorg(domain::chain::block const& block, uint32_t height);
    result_code push_block(domain::chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg);
    result_code remove_outputs(hash_digest const& txid, domain::chain::output::list const& outputs);
    result_code insert_output_from_reorg_and_remove(domain::chain::output_point const& point);
    result_code insert_inputs(domain::chain::input::list const& inputs);

    template <typename I>
    result_code insert_transactions_inputs_non_coinbase(I f, I l);

    template <typename I>
    result_code remove_transactions_outputs_non_coinbase(I f, I l);

    template <typename I>
    result_code remove_transactions_non_coinbase(I f, I l);

    result_code remove_block_header(hash_digest const& hash, uint32_t height);
    result_code remove_block_reorg(uint32_t height);
    result_code remove_reorg_index(uint32_t height);
    result_code remove_block(domain::chain::block const& block, uint32_t height);
    domain::chain::block get_block_reorg(uint32_t height) const;
    result_code prune_reorg_index(uint32_t remove_until);
    result_code prune_reorg_block(uint32_t amount_to_delete);
    result_code get_first_reorg_block_height(uint32_t& out_height) const;

    // //TODO(fernando): is taking KTH_DB_val by value, is that Ok?
    // result_code insert_reorg_into_pool(utxo_pool_t& pool, KTH_DB_val key_point) const;


// Data members ----------------------------
    // path const db_dir_;
    // uint32_t reorg_pool_limit_;                 //TODO(fernando): check if uint32_max is needed for NO-LIMIT???
    std::chrono::seconds const limit_;
    // bool env_created_ = false;
    bool db_opened_ = false;
    // uint64_t db_max_size_;
    // bool safe_mode_;

    std::unordered_map<size_t, domain::chain::header> headers_;
    std::unordered_map<hash_digest, size_t> headers_by_hash_;
    uint32_t last_height_ = 0;
    std::unordered_map<domain::chain::output_point, utxo_entry> utxo_set_;
};

using internal_database = internal_database_basis<std::chrono::system_clock>;

} // namespace kth::database


#include <kth/database/databases/block_database.ipp>
#include <kth/database/databases/header_database.ipp>
// #include <kth/database/databases/history_database.ipp>
// #include <kth/database/databases/spend_database.ipp>
// #include <kth/database/databases/transaction_unconfirmed_database.ipp>
#include <kth/database/databases/internal_database.ipp>
#include <kth/database/databases/reorg_database.ipp>
// #include <kth/database/databases/transaction_database.ipp>
#include <kth/database/databases/utxo_database.ipp>

#endif // KTH_DATABASE_INTERNAL_DATABASE_HPP_

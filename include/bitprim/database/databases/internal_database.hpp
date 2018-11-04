/**
 * Copyright (c) 2016-2017 Bitprim Inc.
 *
 * This file is part of Bitprim.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef BITPRIM_DATABASE_INTERNAL_DATABASE_HPP_
#define BITPRIM_DATABASE_INTERNAL_DATABASE_HPP_

#include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <lmdb.h>

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

#include <bitprim/database/databases/result_code.hpp>
#include <bitprim/database/databases/tools.hpp>
#include <bitprim/database/databases/utxo_entry.hpp>

#ifdef BITPRIM_INTERNAL_DB_4BYTES_INDEX
#define BITPRIM_INTERNAL_DB_WIRE true
#else
#define BITPRIM_INTERNAL_DB_WIRE false
#endif

namespace libbitcoin {
namespace database {

#ifdef BITPRIM_DB_NEW_BLOCKS
constexpr size_t max_dbs_ = 7;
#else
constexpr size_t max_dbs_ = 6;
#endif
constexpr size_t env_open_mode_ = 0664;
constexpr int directory_exists = 0;

template <typename Clock = std::chrono::system_clock>
class BCD_API internal_database_basis {
public:
    using path = boost::filesystem::path;

    constexpr static char block_header_db_name[] = "block_header";
    constexpr static char block_header_by_hash_db_name[] = "block_header_by_hash";
    constexpr static char utxo_db_name[] = "utxo_db";
    constexpr static char reorg_pool_name[] = "reorg_pool";
    constexpr static char reorg_index_name[] = "reorg_index";
    constexpr static char reorg_block_name[] = "reorg_block";
  
    #ifdef BITPRIM_DB_NEW_BLOCKS
    //Blocks DB
    constexpr static char block_db_name[] = "blocks";
    #endif

    internal_database_basis(path const& db_dir, uint32_t reorg_pool_limit, uint64_t db_max_size)
        : db_dir_(db_dir)
        , reorg_pool_limit_(reorg_pool_limit)
        , limit_(blocks_to_seconds(reorg_pool_limit))
        , db_max_size_(db_max_size)
    {}
    
    ~internal_database_basis() {
        close();
    }

    // Non-copyable, non-movable
    internal_database_basis(internal_database_basis const&) = delete;
    internal_database_basis& operator=(internal_database_basis const&) = delete;

    bool create() {
        boost::system::error_code ec;

        if ( ! create_directories(db_dir_, ec)) {
            if (ec.value() == directory_exists) {
                LOG_ERROR(LOG_DATABASE) << "Failed because the directory " << db_dir_ << " already exists.";
                return false;
            }

            LOG_ERROR(LOG_DATABASE) << "Failed to create directory " << db_dir_ << " with error, '" << ec.message() << "'.";
            return false;
        }

        return open();
    }    

    bool open() {
        if ( ! create_and_open_environment()) {
            return false;
        }

        return open_databases();
    }

    bool close() {
        if (db_opened_) {
            mdb_dbi_close(env_, dbi_block_header_);
            mdb_dbi_close(env_, dbi_block_header_by_hash_);
            mdb_dbi_close(env_, dbi_utxo_);
            mdb_dbi_close(env_, dbi_reorg_pool_);
            mdb_dbi_close(env_, dbi_reorg_index_);
            mdb_dbi_close(env_, dbi_reorg_block_);

            #ifdef BITPRIM_DB_NEW_BLOCKS 
            mdb_dbi_close(env_, dbi_block_db_);
            #endif

            db_opened_ = false;
        }

        if (env_created_) {
            mdb_env_close(env_);
            
            env_created_ = false;
        }

        return true;
    }

    result_code push_genesis(chain::block const& block) {
        
        #ifdef BITPRIM_DB_NEW_BLOCKS
        uint32_t height = 0;
        auto valuearr = block.to_data(false);               
        MDB_val key {sizeof(height), &height};         
        MDB_val value {valuearr.size(), valuearr.data()};
        #endif

        MDB_txn* db_txn;
        auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
        if (res0 != MDB_SUCCESS) {
            return result_code::other;
        }

        auto res = push_genesis(block, db_txn);
        if (! succeed(res)) {
            mdb_txn_abort(db_txn);
            return res;
        }

        #ifdef BITPRIM_DB_NEW_BLOCKS
        auto res_block = mdb_put(db_txn, dbi_block_db_, &key, &value, MDB_NOOVERWRITE);
        if (res_block == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate key in LMDB Block [push_genesis] " << res_block;
            return result_code::duplicated_key;
        }
        #endif

        auto res2 = mdb_txn_commit(db_txn);
        if (res2 != MDB_SUCCESS) {
            return result_code::other;
        }
        return res;
    }

    //TODO(fernando): optimization: consider passing a list of outputs to insert and a list of inputs to delete instead of an entire Block.
    //                  avoiding inserting and erasing internal spenders
    result_code push_block(chain::block const& block, uint32_t height, uint32_t median_time_past) {

        #ifdef BITPRIM_DB_NEW_BLOCKS
        auto valuearr = block.to_data(false);               
        MDB_val key {sizeof(height), &height};         
        MDB_val value {valuearr.size(), valuearr.data()};
        #endif

        MDB_txn* db_txn;
        auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
        if (res0 != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error begining LMDB Transaction [push_block] " << res0;
            return result_code::other;
        }

        auto res = push_block(block, height, median_time_past, ! is_old_block(block), db_txn);
        if (! succeed(res)) {
            mdb_txn_abort(db_txn);
            return res;
        }
       
        #ifdef BITPRIM_DB_NEW_BLOCKS
        auto res_block = mdb_put(db_txn, dbi_block_db_, &key, &value, MDB_NOOVERWRITE);
        if (res_block == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate key in LMDB Block [push_block] " << res_block;
            return result_code::duplicated_key;
        }
        #endif


        auto res2 = mdb_txn_commit(db_txn);
        if (res2 != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error commiting LMDB Transaction [push_block] " << res2;
            return result_code::other;
        }

        return res;
    }

    utxo_entry get_utxo(chain::output_point const& point) const {
        
        auto keyarr = point.to_data(BITPRIM_INTERNAL_DB_WIRE);
        MDB_val key {keyarr.size(), keyarr.data()};
        MDB_val value;

        MDB_txn* db_txn;
        auto res0 = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
        if (res0 != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error begining LMDB Transaction [get_utxo] " << res0;
            return utxo_entry{};
        }

        res0 = mdb_get(db_txn, dbi_utxo_, &key, &value);
        if (res0 != MDB_SUCCESS) {
          
            mdb_txn_commit(db_txn);
            // mdb_txn_abort(db_txn);  
            return utxo_entry{};
        }

        auto data = db_value_to_data_chunk(value);

        res0 = mdb_txn_commit(db_txn);
        if (res0 != MDB_SUCCESS) {
            LOG_DEBUG(LOG_DATABASE) << "Error commiting LMDB Transaction [get_utxo] " << res0;        
            return utxo_entry{};
        }

        auto res = utxo_entry::factory_from_data(data);
        return res;
    }
    
    result_code get_last_height(uint32_t& out_height) const {
        MDB_txn* db_txn;
        auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
        if (res != MDB_SUCCESS) {
            return result_code::other;
        }

        MDB_cursor* cursor;
        if (mdb_cursor_open(db_txn, dbi_block_header_, &cursor) != MDB_SUCCESS) {
            return result_code::other;
        }

        MDB_val key;
        int rc;
        if ((rc = mdb_cursor_get(cursor, &key, nullptr, MDB_LAST)) != MDB_SUCCESS) {
            return result_code::db_empty;  
        }

        // assert key.mv_size == 4;
        out_height = *static_cast<uint32_t*>(key.mv_data);
        
        mdb_cursor_close(cursor);

        // mdb_txn_abort(db_txn);
        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return result_code::other;
        }

        return result_code::success;
    }
    
    std::pair<chain::header, uint32_t> get_header(hash_digest const& hash) const {
        MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};

        MDB_txn* db_txn;
        auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
        if (res != MDB_SUCCESS) {
            return {};
        }

        MDB_val value;
        if (mdb_get(db_txn, dbi_block_header_by_hash_, &key, &value) != MDB_SUCCESS) {
            mdb_txn_commit(db_txn);
            // mdb_txn_abort(db_txn);
            return {};
        }

        // assert value.mv_size == 4;
        auto height = *static_cast<uint32_t*>(value.mv_data);

        auto header = get_header(height, db_txn);

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return {};
        }

        return {header, height};
    }
    
    chain::header get_header(uint32_t height) const {
        MDB_txn* db_txn;
        auto zzz = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
        if (zzz != MDB_SUCCESS) {
            return chain::header{};
        }

        auto res = get_header(height, db_txn);

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return chain::header{};
        }

        return res;
    }
    
    result_code pop_block(chain::block& out_block) {
        uint32_t height;

        //TODO: (Mario) add overload with tx
        // The blockchain is empty (nothing to pop, not even genesis).
        auto res = get_last_height(height);
        if (res != result_code::success ) {
            return res;
        }

        //TODO: (Mario) add overload with tx
        // This should never become invalid if this call is protected.
        out_block = get_block_reorg(height);
        if ( ! out_block.is_valid()) {
            return result_code::key_not_found;
        }

        res = remove_block(out_block, height);
        if (res != result_code::success) {
            return res;
        }

        return result_code::success;
    }

    result_code prune() {
        //TODO: (Mario) add overload with tx
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

        auto amount_to_delete = reorg_count - reorg_pool_limit_;
        auto remove_until = first_height + amount_to_delete;

        MDB_txn* db_txn;
        auto zzz = mdb_txn_begin(env_, NULL, 0, &db_txn);
        if (zzz != MDB_SUCCESS) {
            return result_code::other;
        }

        res = prune_reorg_block(amount_to_delete, db_txn);
        if (res != result_code::success) {
            mdb_txn_abort(db_txn);
            return res;
        }

        res = prune_reorg_index(remove_until, db_txn);
        if (res != result_code::success) {
            mdb_txn_abort(db_txn);
            return res;
        }

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return result_code::other;
        }

        return result_code::success;
    }

    #ifdef BITPRIM_DB_NEW_BLOCKS
    std::pair<chain::block, uint32_t> get_block(hash_digest const& hash) const {
        MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};

        MDB_txn* db_txn;
        auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
        if (res != MDB_SUCCESS) {
            return {};
        }

        MDB_val value;
        if (mdb_get(db_txn, dbi_block_header_by_hash_, &key, &value) != MDB_SUCCESS) {
            mdb_txn_commit(db_txn);
            // mdb_txn_abort(db_txn);
            return {};
        }

        // assert value.mv_size == 4;
        auto height = *static_cast<uint32_t*>(value.mv_data);

        auto block = get_block(height, db_txn);

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return {};
        }

        return {block, height};
    }

    chain::block get_block(uint32_t height) const { 
        
        MDB_txn* db_txn;
        auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
        if (res != MDB_SUCCESS) {
            return chain::block{};
        }

        auto block = get_block(height, db_txn);

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return chain::block{};
        }

        return block;
    }
    #endif //BITPRIM_DB_NEW_BLOCKS

private:
    bool is_old_block(chain::block const& block) const {
        return is_old_block_<Clock>(block, limit_);
    }

    size_t get_db_page_size() const {
        return boost::interprocess::mapped_region::get_page_size();
    }

    size_t adjust_db_size(size_t size) const {
        // precondition: env_ have to be created (mdb_env_create)
        // The mdb_env_set_mapsize should be a multiple of the OS page size.
        size_t const page_size = get_db_page_size();
        auto res = size_t(std::ceil(double(size) / page_size)) * page_size;
        return res;

        // size_t const mod = size % page_size;
        // return size + (mod != 0) ? (page_size - mod) : 0;
    }    

    bool create_and_open_environment() {

        if (mdb_env_create(&env_) != MDB_SUCCESS) {
            return false;
        }
        env_created_ = true;

        // TODO(fernando): see what to do with mdb_env_set_maxreaders ----------------------------------------------
        // int threads = tools::get_max_concurrency();
        // if (threads > 110 &&	/* maxreaders default is 126, leave some slots for other read processes */
        //     (result = mdb_env_set_maxreaders(m_env, threads+16)))
        //     throw0(DB_ERROR(lmdb_error("Failed to set max number of readers: ", result).c_str()));
        // ----------------------------------------------------------------------------------------------------------------

        auto res = mdb_env_set_mapsize(env_, adjust_db_size(db_max_size_));
        if (res != MDB_SUCCESS) {
            return false;
        }


        res = mdb_env_set_maxdbs(env_, max_dbs_);
        if (res != MDB_SUCCESS) {
            return false;
        }

        //TODO(fernando): check if we can apply the following flags
        // if (db_flags & DBF_FASTEST)
        //     mdb_flags |= MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC;

        res = mdb_env_open(env_, db_dir_.string().c_str() , MDB_NORDAHEAD | MDB_NOSYNC | MDB_NOTLS, env_open_mode_);
        return res == MDB_SUCCESS;
    }

    bool open_databases() {
        MDB_txn* db_txn;
        auto res = mdb_txn_begin(env_, NULL, 0, &db_txn);
        if (res != MDB_SUCCESS) {
            return false;
        }

        res = mdb_dbi_open(db_txn, block_header_db_name, MDB_CREATE | MDB_INTEGERKEY, &dbi_block_header_);
        if (res != MDB_SUCCESS) {
            return false;
        }
        res = mdb_dbi_open(db_txn, block_header_by_hash_db_name, MDB_CREATE, &dbi_block_header_by_hash_);
        if (res != MDB_SUCCESS) {
            return false;
        }

        res = mdb_dbi_open(db_txn, utxo_db_name, MDB_CREATE, &dbi_utxo_);
        if (res != MDB_SUCCESS) {
            return false;
        }

        res = mdb_dbi_open(db_txn, reorg_pool_name, MDB_CREATE, &dbi_reorg_pool_);
        if (res != MDB_SUCCESS) {
            return false;
        }

        res = mdb_dbi_open(db_txn, reorg_index_name, MDB_CREATE | MDB_DUPSORT | MDB_INTEGERKEY | MDB_DUPFIXED, &dbi_reorg_index_);
        if (res != MDB_SUCCESS) {
            return false;
        }

        res = mdb_dbi_open(db_txn, reorg_block_name, MDB_CREATE | MDB_INTEGERKEY, &dbi_reorg_block_);
        if (res != MDB_SUCCESS) {
            return false;
        }

        #ifdef BITPRIM_DB_NEW_BLOCKS
        res = mdb_dbi_open(db_txn, block_db_name, MDB_CREATE | MDB_INTEGERKEY, &dbi_block_db_);
        if (res != MDB_SUCCESS) {
            return false;
        }
        #endif


        db_opened_ = mdb_txn_commit(db_txn) == MDB_SUCCESS;
        return db_opened_;
    }

    result_code insert_reorg_pool(uint32_t height, MDB_val& key, MDB_txn* db_txn) {
        MDB_val value;
        auto res = mdb_get(db_txn, dbi_utxo_, &key, &value);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found getting UTXO [insert_reorg_pool] " << res;        
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error getting UTXO [insert_reorg_pool] " << res;        
            return result_code::other;
        }

        res = mdb_put(db_txn, dbi_reorg_pool_, &key, &value, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate key inserting in reorg pool [insert_reorg_pool] " << res;        
            return result_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error inseting in reorg pool [insert_reorg_pool] " << res;        
            return result_code::other;
        }

        MDB_val key_index {sizeof(height), &height};        //TODO(fernando): podría estar afuera de la DBTx
        MDB_val value_index {key.mv_size, key.mv_data};     //TODO(fernando): podría estar afuera de la DBTx
        res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, 0);
        
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate key inserting in reorg index [insert_reorg_pool] " << res;
            return result_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error inserting in reorg index [insert_reorg_pool] " << res;
            return result_code::other;
        }

        return result_code::success;
    }

    result_code remove_utxo(uint32_t height, chain::output_point const& point, bool insert_reorg, MDB_txn* db_txn) {
        auto keyarr = point.to_data(BITPRIM_INTERNAL_DB_WIRE);      //TODO(fernando): podría estar afuera de la DBTx
        MDB_val key {keyarr.size(), keyarr.data()};                 //TODO(fernando): podría estar afuera de la DBTx

        if (insert_reorg) {
            auto res0 = insert_reorg_pool(height, key, db_txn);
            if (res0 != result_code::success) return res0;
        }

        auto res = mdb_del(db_txn, dbi_utxo_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting UTXO [remove_utxo] " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting UTXO [remove_utxo] " << res;
            return result_code::other;
        }
        return result_code::success;
    }

    
    result_code insert_utxo(chain::output_point const& point, chain::output const& output, data_chunk const& fixed_data, MDB_txn* db_txn) {
        auto keyarr = point.to_data(BITPRIM_INTERNAL_DB_WIRE);                  //TODO(fernando): podría estar afuera de la DBTx
        auto valuearr = utxo_entry::to_data_with_fixed(output, fixed_data);     //TODO(fernando): podría estar afuera de la DBTx

        MDB_val key   {keyarr.size(), keyarr.data()};                           //TODO(fernando): podría estar afuera de la DBTx
        MDB_val value {valuearr.size(), valuearr.data()};                       //TODO(fernando): podría estar afuera de la DBTx
        auto res = mdb_put(db_txn, dbi_utxo_, &key, &value, MDB_NOOVERWRITE);

        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate Key inserting UTXO [insert_utxo] " << res;        
            return result_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error inserting UTXO [insert_utxo] " << res;        
            return result_code::other;
        }
        return result_code::success;
    }

    result_code remove_inputs(uint32_t height, chain::input::list const& inputs, bool insert_reorg, MDB_txn* db_txn) {
        for (auto const& input: inputs) {
            auto res = remove_utxo(height, input.previous_output(), insert_reorg, db_txn);
            if (res != result_code::success) {
                return res;
            }
        }
        return result_code::success;
    }

    result_code insert_outputs(hash_digest const& txid, chain::output::list const& outputs, data_chunk const& fixed_data, MDB_txn* db_txn) {
        uint32_t pos = 0;
        for (auto const& output: outputs) {
            auto res = insert_utxo(chain::point{txid, pos}, output, fixed_data, db_txn);
            if (res != result_code::success) {
                return res;
            }
            ++pos;
        }
        return result_code::success;
    }

    result_code insert_outputs_error_treatment(uint32_t height, data_chunk const& fixed_data, hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
        auto res = insert_outputs(txid, outputs, fixed_data, db_txn);
        
        if (res == result_code::duplicated_key) {
            //TODO(fernando): log and continue
            return result_code::success_duplicate_coinbase;
        }
        return res;
    }

    // result_code push_transaction_non_coinbase(uint32_t height, data_chunk const& fixed_data, chain::transaction const& tx, bool insert_reorg, MDB_txn* db_txn) {
    //     auto res = remove_inputs(height, tx.inputs(), insert_reorg, db_txn);
    //     if (res != result_code::success) {
    //         return res;
    //     }
    //     //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
    //     return insert_outputs_error_treatment(height, fixed_data, tx.hash(), tx.outputs(), db_txn);
    // }

    // template <typename I>
    // result_code push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, MDB_txn* db_txn) {
    //     // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    //     while (f != l) {
    //         auto const& tx = *f;
    //         auto res = push_transaction_non_coinbase(height, fixed_data, tx, insert_reorg, db_txn);
    //         if (res != result_code::success) {
    //             return res;
    //         }
    //         ++f;
    //     }
    //     return result_code::success;
    // }


    template <typename I>
    result_code push_transactions_outputs_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, MDB_txn* db_txn) {
        // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

        while (f != l) {
            auto const& tx = *f;
            auto res = insert_outputs_error_treatment(height, fixed_data, tx.hash(), tx.outputs(), db_txn);
            if (res != result_code::success) {
                return res;
            }
            ++f;
        }
        return result_code::success;
    }

    template <typename I>
    result_code remove_transactions_inputs_non_coinbase(uint32_t height, I f, I l, bool insert_reorg, MDB_txn* db_txn) {
        while (f != l) {
            auto const& tx = *f;
            auto res = remove_inputs(height, tx.inputs(), insert_reorg, db_txn);
            if (res != result_code::success) {
                return res;
            }
            ++f;
        }
        return result_code::success;
    }

    template <typename I>
    result_code push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, MDB_txn* db_txn) {
        // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

        auto res = push_transactions_outputs_non_coinbase(height, fixed_data, f, l, db_txn);
        if (res != result_code::success) {
            return res;
        }

        return remove_transactions_inputs_non_coinbase(height, f, l, insert_reorg, db_txn);
    }

    result_code push_block_header(chain::block const& block, uint32_t height, MDB_txn* db_txn) {

        auto valuearr = block.header().to_data(true);               //TODO(fernando): podría estar afuera de la DBTx
        MDB_val key {sizeof(height), &height};                      //TODO(fernando): podría estar afuera de la DBTx
        MDB_val value {valuearr.size(), valuearr.data()};           //TODO(fernando): podría estar afuera de la DBTx
        auto res = mdb_put(db_txn, dbi_block_header_, &key, &value, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            //TODO(fernando): El logging en general no está bueno que esté en la DbTx.
            LOG_INFO(LOG_DATABASE) << "Duplicate key inserting block header [push_block_header] " << res;        //TODO(fernando): podría estar afuera de la DBTx. 
            return result_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error inserting block header  [push_block_header] " << res;        
            return result_code::other;
        }
        
        auto key_by_hash_arr = block.hash();                                    //TODO(fernando): podría estar afuera de la DBTx
        MDB_val key_by_hash {key_by_hash_arr.size(), key_by_hash_arr.data()};   //TODO(fernando): podría estar afuera de la DBTx
        res = mdb_put(db_txn, dbi_block_header_by_hash_, &key_by_hash, &key, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate key inserting block header by hash [push_block_header] " << res;        
            return result_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error inserting block header by hash [push_block_header] " << res;        
            return result_code::other;
        }

        return result_code::success;
    }

    result_code push_block_reorg(chain::block const& block, uint32_t height, MDB_txn* db_txn) {

        auto valuearr = block.to_data(false);               //TODO(fernando): podría estar afuera de la DBTx
        MDB_val key {sizeof(height), &height};              //TODO(fernando): podría estar afuera de la DBTx
        MDB_val value {valuearr.size(), valuearr.data()};   //TODO(fernando): podría estar afuera de la DBTx
        auto res = mdb_put(db_txn, dbi_reorg_block_, &key, &value, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate key inserting in reorg block [push_block_reorg] " << res;
            return result_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error inserting in reorg block [push_block_reorg] " << res;        
            return result_code::other;
        }

        return result_code::success;
    }

    result_code push_block(chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg, MDB_txn* db_txn) {
        //precondition: block.transactions().size() >= 1

        auto res = push_block_header(block, height, db_txn);
        if (res != result_code::success) {
            return res;
        }

        if ( insert_reorg ) {
            res = push_block_reorg(block, height, db_txn);
            if (res != result_code::success) {
                return res;
            }
        }
        
        auto const& txs = block.transactions();
        auto const& coinbase = txs.front();

        auto fixed = utxo_entry::to_data_fixed(height, median_time_past, true);                                     //TODO(fernando): podría estar afuera de la DBTx
        auto res0 = insert_outputs_error_treatment(height, fixed, coinbase.hash(), coinbase.outputs(), db_txn);     //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
        if ( ! succeed(res0)) {
            return res0;
        }

        fixed.back() = 0;   //The last byte equal to 0 means NonCoinbaseTx
        res = push_transactions_non_coinbase(height, fixed, txs.begin() + 1, txs.end(), insert_reorg, db_txn);
        if (res != result_code::success) {
            return res;
        }
        return res0;
    }

    result_code push_genesis(chain::block const& block, MDB_txn* db_txn) {
        auto res = push_block_header(block, 0, db_txn);
        return res;
    }

    // result_code remove_outputs(hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
    //     uint32_t pos = outputs.size() - 1;
    //     for (auto const& output: boost::adaptors::reverse(outputs)) {
    //         chain::output_point const point {txid, pos};
    //         auto res = remove_utxo(0, point, false, db_txn);
    //         if (res != result_code::success) {
    //             return res;
    //         }
    //         --pos;
    //     }
    //     return result_code::success;
    // }

    result_code remove_outputs(hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
        uint32_t pos = outputs.size() - 1;
        for (auto const& output: outputs) {
            chain::output_point const point {txid, pos};
            auto res = remove_utxo(0, point, false, db_txn);
            if (res != result_code::success) {
                return res;
            }
            --pos;
        }
        return result_code::success;
    }


    result_code insert_output_from_reorg_and_remove(chain::output_point const& point, MDB_txn* db_txn) {
        auto keyarr = point.to_data(BITPRIM_INTERNAL_DB_WIRE);
        MDB_val key {keyarr.size(), keyarr.data()};

        MDB_val value;
        auto res = mdb_get(db_txn, dbi_reorg_pool_, &key, &value);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found in reorg pool [insert_output_from_reorg_and_remove] " << res;        
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error in reorg pool [insert_output_from_reorg_and_remove] " << res;        
            return result_code::other;
        }

        res = mdb_put(db_txn, dbi_utxo_, &key, &value, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "Duplicate key inserting in UTXO [insert_output_from_reorg_and_remove] " << res;        
            return result_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error inserting in UTXO [insert_output_from_reorg_and_remove] " << res;        
            return result_code::other;
        }

        res = mdb_del(db_txn, dbi_reorg_pool_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting in reorg pool [insert_output_from_reorg_and_remove] " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting in reorg pool [insert_output_from_reorg_and_remove] " << res;
            return result_code::other;
        }
        return result_code::success;
    }

    // result_code insert_inputs(chain::input::list const& inputs, MDB_txn* db_txn) {
    //     for (auto const& input: boost::adaptors::reverse(inputs)) {
    //         auto const& point = input.previous_output();

    //         auto res = insert_output_from_reorg_and_remove(point, db_txn);
    //         if (res != result_code::success) {
    //             return res;
    //         }
    //     }
    //     return result_code::success;
    // }

    result_code insert_inputs(chain::input::list const& inputs, MDB_txn* db_txn) {
        for (auto const& input: inputs) {
            auto const& point = input.previous_output();

            auto res = insert_output_from_reorg_and_remove(point, db_txn);
            if (res != result_code::success) {
                return res;
            }
        }
        return result_code::success;
    }

    // result_code remove_transaction_non_coinbase(chain::transaction const& tx, MDB_txn* db_txn) {
    //     //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
    //     auto res = remove_outputs(tx.hash(), tx.outputs(), db_txn);
    //     if (res != result_code::success) {    
    //         return res;
    //     }

    //     return insert_inputs(tx.inputs(), db_txn);
    // }

    template <typename I>
    result_code insert_transactions_inputs_non_coinbase(I f, I l, MDB_txn* db_txn) {
        // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
        
        while (f != l) {
            auto const& tx = *f;
            auto res = insert_inputs(tx.inputs(), db_txn);
            if (res != result_code::success) {
                return res;
            }
            ++f;
        } 

        return result_code::success;
    }

    template <typename I>
    result_code remove_transactions_outputs_non_coinbase(I f, I l, MDB_txn* db_txn) {
        // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
        
        while (f != l) {
            auto const& tx = *f;
            auto res = remove_outputs(tx.hash(), tx.outputs(), db_txn);
            if (res != result_code::success) {
                return res;
            }
            ++f;
        } 

        return result_code::success;
    }


    // template <typename I>
    // result_code remove_transactions_non_coinbase(I f, I l, MDB_txn* db_txn) {
    //     // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
        
    //     // reverse order
    //     while (f != l) {
    //         --l;
    //         auto const& tx = *l;
    //         auto res = remove_transaction_non_coinbase(tx, db_txn);
    //         if (res != result_code::success) {
    //             return res;
    //         }
    //     } 

    //     return result_code::success;
    // }


    template <typename I>
    result_code remove_transactions_non_coinbase(I f, I l, MDB_txn* db_txn) {
        // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

        auto res = insert_transactions_inputs_non_coinbase(f, l, db_txn);
        if (res != result_code::success) {
            return res;
        }
        return remove_transactions_outputs_non_coinbase(f, l, db_txn);
    }

    result_code remove_block_header(hash_digest const& hash, uint32_t height, MDB_txn* db_txn) {

        MDB_val key {sizeof(height), &height};
        auto res = mdb_del(db_txn, dbi_block_header_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting block header in LMDB [remove_block_header] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Erro deleting block header in LMDB [remove_block_header] - mdb_del: " << res;
            return result_code::other;
        }

        MDB_val key_hash {hash.size(), const_cast<hash_digest&>(hash).data()};
        res = mdb_del(db_txn, dbi_block_header_by_hash_, &key_hash, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting block header by hash in LMDB [remove_block_header] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Erro deleting block header by hash in LMDB [remove_block_header] - mdb_del: " << res;
            return result_code::other;
        }

        return result_code::success;
    }

    result_code remove_block_reorg(uint32_t height, MDB_txn* db_txn) {

        MDB_val key {sizeof(height), &height};
        auto res = mdb_del(db_txn, dbi_reorg_block_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting reorg block in LMDB [remove_block_reorg] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting reorg block in LMDB [remove_block_reorg] - mdb_del: " << res;
            return result_code::other;
        }
        return result_code::success;
    }

    result_code remove_reorg_index(uint32_t height, MDB_txn* db_txn) {

        MDB_val key {sizeof(height), &height};
        auto res = mdb_del(db_txn, dbi_reorg_index_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting reorg index in LMDB [remove_reorg_index] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting reorg index in LMDB [remove_reorg_index] - mdb_del: " << res;
            return result_code::other;
        }
        return result_code::success;
    }

    #ifdef BITPRIM_DB_NEW_BLOCKS
    result_code remove_blocks_db(uint32_t height, MDB_txn* db_txn) {

        MDB_val key {sizeof(height), &height};
        auto res = mdb_del(db_txn, dbi_block_db_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting blocks DB in LMDB [remove_blocks_db] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting blocks DB in LMDB [remove_blocks_db] - mdb_del: " << res;
            return result_code::other;
        }
        return result_code::success;
    }
    #endif

    result_code remove_block(chain::block const& block, uint32_t height, MDB_txn* db_txn) {
        //precondition: block.transactions().size() >= 1

        auto const& txs = block.transactions();
        auto const& coinbase = txs.front();

        auto res = remove_transactions_non_coinbase(txs.begin() + 1, txs.end(), db_txn);
        if (res != result_code::success) {
            return res;
        }

        //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
        res = remove_outputs(coinbase.hash(), coinbase.outputs(), db_txn);
        if (res != result_code::success) {
            return res;
        }

        //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx
        res = remove_block_header(block.hash(), height, db_txn);
        if (res != result_code::success) {
            return res;
        }

        res = remove_block_reorg(height, db_txn);
        if (res != result_code::success) {
            return res;
        }

        res = remove_reorg_index(height, db_txn);
        if (res != result_code::success) {
            return res;
        }

        #ifdef BITPRIM_DB_NEW_BLOCKS
        res = remove_blocks_db(height, db_txn);
        if (res != result_code::success) {
            return res;
        }
        #endif

        return result_code::success;
    }

    chain::header get_header(uint32_t height, MDB_txn* db_txn) const {
        MDB_val key {sizeof(height), &height};
        MDB_val value;

        if (mdb_get(db_txn, dbi_block_header_, &key, &value) != MDB_SUCCESS) {
            return chain::header{};
        }

        auto data = db_value_to_data_chunk(value);
        auto res = chain::header::factory_from_data(data);
        return res;
    }

    #ifdef BITPRIM_DB_NEW_BLOCKS
    chain::block get_block(uint32_t height, MDB_txn* db_txn) const {
        MDB_val key {sizeof(height), &height};
        MDB_val value;

        if (mdb_get(db_txn, dbi_block_db_, &key, &value) != MDB_SUCCESS) {
            return chain::block{};
        }

        auto data = db_value_to_data_chunk(value);
        auto res = chain::block::factory_from_data(data);
        return res;
    }
    #endif

    chain::block get_block_reorg(uint32_t height, MDB_txn* db_txn) const {
        MDB_val key {sizeof(height), &height};
        MDB_val value;

        if (mdb_get(db_txn, dbi_reorg_block_, &key, &value) != MDB_SUCCESS) {
            return chain::block{};
        }

        auto data = db_value_to_data_chunk(value);
        auto res = chain::block::factory_from_data(data);       //TODO(fernando): mover fuera de la DbTx
        return res;
    }

    chain::block get_block_reorg(uint32_t height) const {
        MDB_txn* db_txn;
        auto zzz = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
        if (zzz != MDB_SUCCESS) {
            return chain::block{};
        }

        auto res = get_block_reorg(height, db_txn);

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return chain::block{};
        }

        return res;
    }
    
    result_code remove_block(chain::block const& block, uint32_t height) {
        MDB_txn* db_txn;
        auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
        if (res0 != MDB_SUCCESS) {
            return result_code::other;
        }

        auto res = remove_block(block, height, db_txn);
        if (res != result_code::success) {
            mdb_txn_abort(db_txn);
            return res;
        }

        auto res2 = mdb_txn_commit(db_txn);
        if (res2 != MDB_SUCCESS) {
            return result_code::other;
        }
        return result_code::success;
    }

    result_code prune_reorg_index(uint32_t remove_until, MDB_txn* db_txn) {
        MDB_cursor* cursor;
        if (mdb_cursor_open(db_txn, dbi_reorg_index_, &cursor) != MDB_SUCCESS) {
            return result_code::other;
        }

        MDB_val key;
        MDB_val value;
        int rc;
        while ((rc = mdb_cursor_get(cursor, &key, &value, MDB_NEXT)) == MDB_SUCCESS) {
            auto current_height = *static_cast<uint32_t*>(key.mv_data);
            if (current_height < remove_until) {

                auto res = mdb_del(db_txn, dbi_reorg_pool_, &value, NULL);
                if (res == MDB_NOTFOUND) {
                    LOG_INFO(LOG_DATABASE) << "Key not found deleting reorg pool in LMDB [prune_reorg_index] - mdb_del: " << res;
                    return result_code::key_not_found;
                }
                if (res != MDB_SUCCESS) {
                    LOG_INFO(LOG_DATABASE) << "Error deleting reorg pool in LMDB [prune_reorg_index] - mdb_del: " << res;
                    return result_code::other;
                }

                if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
                    mdb_cursor_close(cursor);
                    return result_code::other;
                }
            } else {
                break;
            }
        }
        
        mdb_cursor_close(cursor);
        return result_code::success;
    }

    result_code prune_reorg_block(uint32_t amount_to_delete, MDB_txn* db_txn) {
        //precondition: amount_to_delete >= 1

        MDB_cursor* cursor;
        if (mdb_cursor_open(db_txn, dbi_reorg_block_, &cursor) != MDB_SUCCESS) {
            return result_code::other;
        }

        int rc;
        while ((rc = mdb_cursor_get(cursor, nullptr, nullptr, MDB_NEXT)) == MDB_SUCCESS) {
            if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
                mdb_cursor_close(cursor);
                return result_code::other;
            }
            if (--amount_to_delete == 0) break;
        }
        
        mdb_cursor_close(cursor);
        return result_code::success;
    }

    result_code get_first_reorg_block_height(uint32_t& out_height) const {
        MDB_txn* db_txn;
        auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
        if (res != MDB_SUCCESS) {
            return result_code::other;
        }

        MDB_cursor* cursor;
        if (mdb_cursor_open(db_txn, dbi_reorg_block_, &cursor) != MDB_SUCCESS) {
            return result_code::other;
        }

        MDB_val key;
        int rc;
        if ((rc = mdb_cursor_get(cursor, &key, nullptr, MDB_FIRST)) != MDB_SUCCESS) {
            return result_code::db_empty;  
        }

        // assert key.mv_size == 4;
        out_height = *static_cast<uint32_t*>(key.mv_data);
        
        mdb_cursor_close(cursor);

        // mdb_txn_abort(db_txn); 
        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return result_code::other;
        }

        return result_code::success;
    }    


    // Data members ----------------------------
    path const db_dir_;
    uint32_t reorg_pool_limit_;                 //TODO(fernando): check if uint32_max is needed for NO-LIMIT???
    std::chrono::seconds const limit_;
    bool env_created_ = false;
    bool db_opened_ = false;
    uint64_t db_max_size_;

    MDB_env* env_;
    MDB_dbi dbi_block_header_;
    MDB_dbi dbi_block_header_by_hash_;
    MDB_dbi dbi_utxo_;
    MDB_dbi dbi_reorg_pool_;
    MDB_dbi dbi_reorg_index_;
    MDB_dbi dbi_reorg_block_;

    #ifdef BITPRIM_DB_NEW_BLOCKS
    //Blocks DB
    MDB_dbi dbi_block_db_;
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

#ifdef BITPRIM_DB_NEW_BLOCKS
template <typename Clock>
constexpr char internal_database_basis<Clock>::block_db_name[];               //key: block height, value: block
#endif


using internal_database = internal_database_basis<std::chrono::system_clock>;

} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_INTERNAL_DATABASE_HPP_

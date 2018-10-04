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
#ifndef BITPRIM_DATABASE_UTXO_DATABASE_HPP_
#define BITPRIM_DATABASE_UTXO_DATABASE_HPP_

// #ifdef BITPRIM_DB_NEW

#include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <lmdb.h>

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

#include <bitprim/database/databases/utxo_entry.hpp>


#ifdef BITPRIM_UTXO_4BYTES_INDEX
#define BITPRIM_UXTO_WIRE true
#else
#define BITPRIM_UXTO_WIRE false
#endif


namespace libbitcoin {
namespace database {


//Note: same logic as is_stale()

//TODO(fernando): move to an utility file

template <typename Clock>
// std::chrono::time_point<Clock> to_time_point(uint32_t secs) {
std::chrono::time_point<Clock> to_time_point(std::chrono::seconds secs) {
    return std::chrono::time_point<Clock>(typename Clock::duration(secs));
}

template <typename Clock>
bool is_old_block_(uint32_t header_ts, std::chrono::seconds limit) {
    // using clock_t = std::chrono::system_clock;
    

    auto pepe = to_time_point<Clock>(std::chrono::seconds(header_ts));

    std::cout << "seconds since epoch 1: " << std::chrono::duration_cast<std::chrono::seconds>(Clock::now().time_since_epoch()).count() << '\n';
    std::cout << "seconds since epoch 2: " << std::chrono::duration_cast<std::chrono::seconds>(pepe.time_since_epoch()).count() << '\n';
    std::cout << std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - pepe).count() << "us.\n";    

    auto xxx = (Clock::now() - to_time_point<Clock>(std::chrono::seconds(header_ts))) >= limit;
    std::cout << xxx << std::endl;




    return (Clock::now() - to_time_point<Clock>(std::chrono::seconds(header_ts))) >= limit;
}

template <typename Clock>
bool is_old_block_(chain::block const& block, std::chrono::seconds limit) {
    return is_old_block_<Clock>(block.header().timestamp(), limit);
}

//TODO(fernando): move to an util/tools file
constexpr 
std::chrono::seconds blocks_to_seconds(uint32_t blocks) {
    // uint32_t const target_spacing_seconds = 10 * 60;
    return std::chrono::seconds(blocks * target_spacing_seconds);
}

enum class utxo_code {
    success = 0,
    success_duplicate_coinbase = 1,
    duplicated_key = 2,
    key_not_found = 3,
    db_empty = 4,
    other = 5
};

bool succeed(utxo_code code) {
    return code == utxo_code::success || code == utxo_code::success_duplicate_coinbase;
}

template <typename Clock = std::chrono::system_clock>
class BCD_API utxo_database_basis {
public:
    using path = boost::filesystem::path;

    constexpr static char block_header_db_name[] = "block_header";
    constexpr static char block_header_by_hash_db_name[] = "block_header_by_hash";
    constexpr static char utxo_db_name[] = "utxo_db";
    constexpr static char reorg_pool_name[] = "reorg_pool";
    constexpr static char reorg_index_name[] = "reorg_index";
    constexpr static char reorg_block_name[] = "reorg_block";

  
    utxo_database_basis(path const& db_dir, uint32_t reorg_pool_limit)
        : db_dir_(db_dir)
        , reorg_pool_limit_(reorg_pool_limit)
        , limit_(blocks_to_seconds(reorg_pool_limit))
    {}
    
    ~utxo_database_basis() {
        close();
    }

    // Non-copyable, non-movable
    utxo_database_basis(utxo_database_basis const&) = delete;
    utxo_database_basis& operator=(utxo_database_basis const&) = delete;


    // Initialize files and start.
    bool create() {
        boost::system::error_code ec;

        if ( ! create_directories(db_dir_, ec)) {
            // if (ec.value() == directory_exists) {
            //     LOG_ERROR(LOG_NODE) << format(BN_INITCHAIN_EXISTS) % directory;
            //     return false;
            // }

            // LOG_ERROR(LOG_NODE) << format(BN_INITCHAIN_NEW) % directory % ec.message();
            return false;
        }
        // LOG_INFO(LOG_NODE) << format(BN_INITIALIZING_CHAIN) % directory;

        return open();
    }    

    // Start files and primitives.
    bool open() {
        if ( ! create_and_open_environment()) {
            return false;
        }
        return open_databases();
    }

    // Close files.
    bool close() {
        if (db_created_) {
            mdb_dbi_close(env_, dbi_block_header_);
            mdb_dbi_close(env_, dbi_block_header_by_hash_);

            mdb_dbi_close(env_, dbi_utxo_);
            mdb_dbi_close(env_, dbi_reorg_pool_);
            mdb_dbi_close(env_, dbi_reorg_index_);

            mdb_dbi_close(env_, dbi_reorg_block_);
            db_created_ = false;
        }

        if (env_created_) {
            mdb_env_close(env_);
            env_created_ = false;
        }

        return true;
    }

    utxo_code push_genesis(chain::block const& block) {
        MDB_txn* db_txn;
        auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
        if (res0 != MDB_SUCCESS) {
            return utxo_code::other;
        }

        // auto hash = block.hash();
        // std::cout << encode_hash(hash) << std::endl;

        auto res = push_genesis(block, db_txn);
        if (! succeed(res)) {
            mdb_txn_abort(db_txn);
            return res;
        }

        auto res2 = mdb_txn_commit(db_txn);
        if (res2 != MDB_SUCCESS) {
            return utxo_code::other;
        }
        return res;
    }

    utxo_code push_block(chain::block const& block, uint32_t height, uint32_t median_time_past) {

        MDB_txn* db_txn;
        auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
        if (res0 != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::push_block - mdb_txn_begin " << res0;
            return utxo_code::other;
        }

        auto res = push_block(block, height, median_time_past, ! is_old_block(block), db_txn);
        if (! succeed(res)) {
            mdb_txn_abort(db_txn);
            return res;
        }

        auto res2 = mdb_txn_commit(db_txn);
        if (res2 != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::push_block - mdb_txn_commit " << res2;
            return utxo_code::other;
        }
        return res;
    }
    
    utxo_entry get_utxo(chain::output_point const& point) const {
        MDB_txn* db_txn;
        if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
            return utxo_entry{};
        }

        auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
        MDB_val key {keyarr.size(), keyarr.data()};
        MDB_val value;

        if (mdb_get(db_txn, dbi_utxo_, &key, &value) != MDB_SUCCESS) {
            mdb_txn_commit(db_txn);
            // mdb_txn_abort(db_txn);
            return utxo_entry{};
        }

        data_chunk data {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};
        auto res = utxo_entry::factory_from_data(data);

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return utxo_entry{};
        }

        return res;
    }
    
    utxo_code get_last_height(uint32_t& out_height) const {
        MDB_txn* db_txn;
        if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        MDB_cursor* cursor;
        if (mdb_cursor_open(db_txn, dbi_block_header_, &cursor) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        MDB_val key;
        // MDB_val value;
        int rc;

        if ((rc = mdb_cursor_get(cursor, &key, nullptr, MDB_LAST)) != MDB_SUCCESS) {
            return utxo_code::db_empty;  
        }

        // assert key.mv_size == 4;
        out_height = *static_cast<uint32_t*>(key.mv_data);
        
        mdb_cursor_close(cursor);

        // mdb_txn_abort(db_txn);
        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        return utxo_code::success;
    }
    
    std::pair<chain::header, uint32_t> get_header(hash_digest const& hash) const {
        MDB_txn* db_txn;
        if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
            return {};
        }

        MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};
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
        if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
            return chain::header{};
        }

        auto res = get_header(height, db_txn);

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return chain::header{};
        }

        return res;
    }
    
    utxo_code pop_block(chain::block& out_block) {
        uint32_t height;

        //TODO: (Mario) add overload with tx
        // The blockchain is empty (nothing to pop, not even genesis).
        auto res = get_last_height(height);
        if (res != utxo_code::success ) {
            return res;
        }

        //TODO: (Mario) add overload with tx
        // This should never become invalid if this call is protected.
        out_block = get_block_reorg(height);
        if ( ! out_block.is_valid()) {
            return utxo_code::key_not_found;
        }

        res = remove_block(out_block, height);
        if (res != utxo_code::success) {
            return res;
        }

        return utxo_code::success;
    }
    

    utxo_code get_first_reorg_block_height(uint32_t& out_height) const {
        MDB_txn* db_txn;
        if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        MDB_cursor* cursor;
        if (mdb_cursor_open(db_txn, dbi_reorg_block_, &cursor) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        MDB_val key;
        int rc;

        if ((rc = mdb_cursor_get(cursor, &key, nullptr, MDB_FIRST)) != MDB_SUCCESS) {
            return utxo_code::db_empty;  
        }

        // assert key.mv_size == 4;
        out_height = *static_cast<uint32_t*>(key.mv_data);
        
        mdb_cursor_close(cursor);

        // mdb_txn_abort(db_txn);
        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        return utxo_code::success;
    }

    utxo_code prune() {
        //TODO: (Mario) add overload with tx
        uint32_t last_height;
        auto res = get_last_height(last_height);
        if (res != utxo_code::success ) {
            return res;
        }
        
        if (last_height < reorg_pool_limit_) {
            return utxo_code::success;            
        }

        uint32_t first_height;
        res = get_first_reorg_block_height(first_height);
        if (res != utxo_code::success ) {
            return res;
        }

        if (first_height > last_height) {
            return utxo_code::success;  //TODO(fernando): base corrupta??            
        }

        auto reorg_count = last_height - first_height + 1;
        if (reorg_count <= reorg_pool_limit_) {
            return utxo_code::success;            
        }

        auto amount_to_delete = reorg_count - reorg_pool_limit_;

        MDB_txn* db_txn;
        if (mdb_txn_begin(env_, NULL, 0, &db_txn) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        res = prune_reorg_block(amount_to_delete, db_txn);
        if (res != utxo_code::success) {
            mdb_txn_abort(db_txn);
            return res;
        }

        res = prune_reorg_index(amount_to_delete, db_txn);
        if (res != utxo_code::success) {
            mdb_txn_abort(db_txn);
            return res;
        }

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        return utxo_code::success;
    }




private:
    bool is_old_block(chain::block const& block) const {
        return is_old_block_<Clock>(block, limit_);
    }

    bool create_and_open_environment() {

        if (mdb_env_create(&env_) != MDB_SUCCESS) {
            return false;
        }
        env_created_ = true;


        // E(mdb_env_set_maxreaders(env_, 1));
        // E(mdb_env_set_mapsize(env_, 10485760));

        auto res = mdb_env_set_mapsize(env_, size_t(10485760) * 1024 * 10);      //TODO(fernando): hardcoded
        // auto res = mdb_env_set_mapsize(env_, size_t(10485760) * 1024);      //TODO(fernando): hardcoded
        // auto res = mdb_env_set_mapsize(env_, size_t(10485760) * 10);      //TODO(fernando): hardcoded
        if (res != MDB_SUCCESS) {
            return false;
        }

        res = mdb_env_set_maxdbs(env_, 6);      //TODO(fernando): hardcoded
        if (res != MDB_SUCCESS) {
            return false;
        }

        //TODO(fernando): use MDB_RDONLY for Read-only node
        //                  MDB_WRITEMAP ????
        //                  MDB_NOMETASYNC ????
        //                  MDB_NOSYNC ????

        //TODO(fernando): put the 0664 in the CFG file
        //TODO(fernando): MDB_NOSYNC para IBD ??

        // res = mdb_env_open(env_, db_dir_.c_str(), MDB_FIXEDMAP | MDB_NORDAHEAD | MDB_NOMEMINIT, 0664);
        // res = mdb_env_open(env_, db_dir_.c_str(), MDB_FIXEDMAP, 0664);
        
        // res = mdb_env_open(env_, db_dir_.c_str(), 0, 0664);
        res = mdb_env_open(env_, db_dir_.c_str(), MDB_NORDAHEAD | MDB_NOSYNC, 0664);


        // std::cout << "utxo_database_basis::create_and_open_environment() - mdb_env_open: " << res << std::endl;
        return res == MDB_SUCCESS;
    }

    bool open_databases() {
        MDB_txn* db_txn;
        if (mdb_txn_begin(env_, NULL, 0, &db_txn) != MDB_SUCCESS) {
            return false;
        }

        //TODO(fernando): check if we can use an optimization for fixed size keys and values
        auto res = mdb_dbi_open(db_txn, block_header_db_name, MDB_CREATE | MDB_INTEGERKEY, &dbi_block_header_);
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

        db_created_ = true;

        return mdb_txn_commit(db_txn) == MDB_SUCCESS;
    }

    utxo_code insert_reorg_pool(uint32_t height, MDB_val& key, MDB_txn* db_txn) {
        MDB_val value;
        auto res = mdb_get(db_txn, dbi_utxo_, &key, &value);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_reorg_pool - mdb_get: " << res;        
            return utxo_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_reorg_pool - mdb_get: " << res;        
            return utxo_code::other;
        }

        res = mdb_put(db_txn, dbi_reorg_pool_, &key, &value, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_reorg_pool - mdb_put(0): " << res;        
            return utxo_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_reorg_pool - mdb_put(0): " << res;        
            return utxo_code::other;
        }

        MDB_val key_index {sizeof(height), &height};
        MDB_val value_index {key.mv_size, key.mv_data};
        // res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, MDB_APPENDDUP);
        // res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, MDB_APPEND);
        // res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, MDB_NODUPDATA);
        res = mdb_put(db_txn, dbi_reorg_index_, &key_index, &value_index, 0);
        
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_reorg_pool - mdb_put(1): " << res;
            return utxo_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_reorg_pool - mdb_put(1): " << res;
            return utxo_code::other;
        }

        return utxo_code::success;
    }

    utxo_code remove(uint32_t height, chain::output_point const& point, bool insert_reorg, MDB_txn* db_txn) {
        auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
        MDB_val key {keyarr.size(), keyarr.data()};

        if (insert_reorg) {
            auto res0 = insert_reorg_pool(height, key, db_txn);
            if (res0 != utxo_code::success) return res0;
        }

        auto res = mdb_del(db_txn, dbi_utxo_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove - mdb_del: " << res;
            return utxo_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove - mdb_del: " << res;
            return utxo_code::other;
        }
        return utxo_code::success;
    }

    //TODO(fernando): rename
    utxo_code insert(chain::output_point const& point, chain::output const& output, data_chunk const& fixed_data, MDB_txn* db_txn) {
        auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
        // auto valuearr = entry.to_data();
        auto valuearr = utxo_entry::to_data_pepe(output, fixed_data);

        MDB_val key   {keyarr.size(), keyarr.data()};
        MDB_val value {valuearr.size(), valuearr.data()};
        auto res = mdb_put(db_txn, dbi_utxo_, &key, &value, MDB_NOOVERWRITE);

        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert - mdb_put: " << res;        
            return utxo_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert - mdb_put: " << res;        
            return utxo_code::other;
        }
        return utxo_code::success;
    }

    utxo_code remove_inputs(uint32_t height, chain::input::list const& inputs, bool insert_reorg, MDB_txn* db_txn) {
        for (auto const& input: inputs) {
            auto res = remove(height, input.previous_output(), insert_reorg, db_txn);
            if (res != utxo_code::success) {
                return res;
            }
        }
        return utxo_code::success;
    }

    utxo_code insert_outputs(hash_digest const& txid, chain::output::list const& outputs, data_chunk const& fixed_data, MDB_txn* db_txn) {
        uint32_t pos = 0;
        for (auto const& output: outputs) {
            auto res = insert(chain::point{txid, pos}, output, fixed_data, db_txn);
            if (res != utxo_code::success) {
                return res;
            }
            ++pos;
        }
        return utxo_code::success;
    }

    utxo_code insert_outputs_error_treatment(uint32_t height, data_chunk const& fixed_data, hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
        auto res = insert_outputs(txid, outputs, fixed_data, db_txn);
        
        if (res == utxo_code::duplicated_key) {
            //TODO(fernando): log and continue
            return utxo_code::success_duplicate_coinbase;
        }
        return res;
    }

    utxo_code push_transaction_non_coinbase(uint32_t height, data_chunk const& fixed_data, chain::transaction const& tx, bool insert_reorg, MDB_txn* db_txn) {
        auto res = remove_inputs(height, tx.inputs(), insert_reorg, db_txn);
        if (res != utxo_code::success) {
            return res;
        }
        return insert_outputs_error_treatment(height, fixed_data, tx.hash(), tx.outputs(), db_txn);
    }

    template <typename I>
    utxo_code push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, MDB_txn* db_txn) {
        // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

        while (f != l) {
            auto const& tx = *f;
            auto res = push_transaction_non_coinbase(height, fixed_data, tx, insert_reorg, db_txn);
            if (res != utxo_code::success) {
                return res;
            }
            ++f;
        }
        return utxo_code::success;
    }

    utxo_code push_block_header(chain::block const& block, uint32_t height, MDB_txn* db_txn) {

        auto valuearr = block.header().to_data(true);
        MDB_val key {sizeof(height), &height};
        MDB_val value {valuearr.size(), valuearr.data()};
        auto res = mdb_put(db_txn, dbi_block_header_, &key, &value, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::push_block_header - mdb_put(0) " << res;
            return utxo_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::push_block_header - mdb_put(0) " << res;        
            return utxo_code::other;
        }

        auto key_by_hash_arr = block.hash();
        MDB_val key_by_hash {key_by_hash_arr.size(), key_by_hash_arr.data()};
        res = mdb_put(db_txn, dbi_block_header_by_hash_, &key_by_hash, &key, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::push_block_header - mdb_put(1) " << res;        
            return utxo_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::push_block_header - mdb_put(1) " << res;        
            return utxo_code::other;
        }

        return utxo_code::success;
    }

    utxo_code push_block_reorg(chain::block const& block, uint32_t height, MDB_txn* db_txn) {

        auto valuearr = block.to_data(false);
        MDB_val key {sizeof(height), &height};
        MDB_val value {valuearr.size(), valuearr.data()};
        auto res = mdb_put(db_txn, dbi_reorg_block_, &key, &value, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::push_block_header - mdb_put(0) " << res;
            return utxo_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::push_block_header - mdb_put(0) " << res;        
            return utxo_code::other;
        }

        return utxo_code::success;
    }

    utxo_code push_block(chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg, MDB_txn* db_txn) {
        //precondition: block.transactions().size() >= 1

        auto res = push_block_header(block, height, db_txn);
        if (res != utxo_code::success) {
            return res;
        }

        if ( insert_reorg ) {
            //prune 

            res = push_block_reorg(block, height, db_txn);
            if (res != utxo_code::success) {
                return res;
            }
        }
        
        auto const& txs = block.transactions();
        auto const& coinbase = txs.front();

        auto fixed = utxo_entry::to_data_fixed(height, median_time_past, true);
        auto res0 = insert_outputs_error_treatment(height, fixed, coinbase.hash(), coinbase.outputs(), db_txn);
        if ( ! succeed(res0)) {
            return res0;
        }

        fixed.back() = 0;   //TODO(fernando): comment this
        res = push_transactions_non_coinbase(height, fixed, txs.begin() + 1, txs.end(), insert_reorg, db_txn);
        if (res != utxo_code::success) {
            return res;
        }
        return res0;
    }

    utxo_code push_genesis(chain::block const& block, MDB_txn* db_txn) {
        auto res = push_block_header(block, 0, db_txn);
        return res;
    }

    utxo_code remove_outputs(hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
        uint32_t pos = outputs.size() - 1;
        for (auto const& output: boost::adaptors::reverse(outputs)) {
            chain::output_point const point {txid, pos};
            auto res = remove(0, point, false, db_txn);
            if (res != utxo_code::success) {
                return res;
            }
            --pos;
        }
        return utxo_code::success;
    }

    utxo_code insert_output_from_reorg_and_remove(chain::output_point const& point, MDB_txn* db_txn) {
        auto keyarr = point.to_data(BITPRIM_UXTO_WIRE);
        MDB_val key {keyarr.size(), keyarr.data()};

        MDB_val value;
        auto res = mdb_get(db_txn, dbi_reorg_pool_, &key, &value);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_output_from_reorg_and_remove - mdb_get: " << res;        
            return utxo_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_output_from_reorg_and_remove - mdb_get: " << res;        
            return utxo_code::other;
        }

        res = mdb_put(db_txn, dbi_utxo_, &key, &value, MDB_NOOVERWRITE);
        if (res == MDB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_output_from_reorg_and_remove - mdb_put(0): " << res;        
            return utxo_code::duplicated_key;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_output_from_reorg_and_remove - mdb_put(0): " << res;        
            return utxo_code::other;
        }

        res = mdb_del(db_txn, dbi_reorg_pool_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_output_from_reorg_and_remove - mdb_del: " << res;
            return utxo_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::insert_output_from_reorg_and_remove - mdb_del: " << res;
            return utxo_code::other;
        }
        return utxo_code::success;
    }

    utxo_code insert_inputs(chain::input::list const& inputs, MDB_txn* db_txn) {
        for (auto const& input: boost::adaptors::reverse(inputs)) {
            auto const& point = input.previous_output();

            auto res = insert_output_from_reorg_and_remove(point, db_txn);
            if (res != utxo_code::success) {
                return res;
            }
        }
        return utxo_code::success;
    }

    utxo_code remove_transaction_non_coinbase(chain::transaction const& tx, MDB_txn* db_txn) {
        auto res = remove_outputs(tx.hash(), tx.outputs(), db_txn);
        if (res != utxo_code::success) {    
            return res;
        }

        return insert_inputs(tx.inputs(), db_txn);
    }

    template <typename I>
    utxo_code remove_transactions_non_coinbase(I f, I l, MDB_txn* db_txn) {
        // precondition: [f, l) is a valid range and there are no coinbase transactions in it.
        
        // reverse order
        while (f != l) {
            --l;
            auto const& tx = *l;    
            auto res = remove_transaction_non_coinbase(tx, db_txn);
            if (res != utxo_code::success) {
                return res;
            }
        } 

        return utxo_code::success;
    }

    utxo_code remove_block_header(hash_digest const& hash, uint32_t height, MDB_txn* db_txn) {

        MDB_val key {sizeof(height), &height};
        auto res = mdb_del(db_txn, dbi_block_header_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove_block_header - mdb_del: " << res;
            return utxo_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove_block_header - mdb_del: " << res;
            return utxo_code::other;
        }

        MDB_val key_hash {hash.size(), const_cast<hash_digest&>(hash).data()};
        res = mdb_del(db_txn, dbi_block_header_by_hash_, &key_hash, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove_block_header - mdb_del: " << res;
            return utxo_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove_block_header - mdb_del: " << res;
            return utxo_code::other;
        }

        return utxo_code::success;
    }

    utxo_code remove_block_reorg(uint32_t height, MDB_txn* db_txn) {

        MDB_val key {sizeof(height), &height};
        auto res = mdb_del(db_txn, dbi_reorg_block_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove_block_reorg - mdb_del: " << res;
            return utxo_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove_block_reorg - mdb_del: " << res;
            return utxo_code::other;
        }
        return utxo_code::success;
    }

    utxo_code remove_reorg_index(uint32_t height, MDB_txn* db_txn) {

        MDB_val key {sizeof(height), &height};
        auto res = mdb_del(db_txn, dbi_reorg_index_, &key, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove_reorg_index - mdb_del: " << res;
            return utxo_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "utxo_database_basis::remove_reorg_index - mdb_del: " << res;
            return utxo_code::other;
        }
        return utxo_code::success;
    }

    utxo_code remove_block(chain::block const& block, uint32_t height, MDB_txn* db_txn) {
        //precondition: block.transactions().size() >= 1

        //borrar outputs insertados utxo
        //insertar los inputs que borre usando reorg pool
        //borrar reorg_pool_block, reorg_pool_index, reorg_pool
        //borrar headers y hash, altura

        auto const& txs = block.transactions();
        auto const& coinbase = txs.front();

        auto res = remove_transactions_non_coinbase(txs.begin() + 1, txs.end(), db_txn);
        if (res != utxo_code::success) {
            return res;
        }

        res = remove_outputs(coinbase.hash(), coinbase.outputs(), db_txn);
        if (res != utxo_code::success) {
            return res;
        }

        res = remove_block_header(block.hash(), height, db_txn);
        if (res != utxo_code::success) {
            return res;
        }

        res = remove_block_reorg(height, db_txn);
        if (res != utxo_code::success) {
            return res;
        }

        res = remove_reorg_index(height, db_txn);
        if (res != utxo_code::success) {
            return res;
        }

        return utxo_code::success;
    }

    chain::header get_header(uint32_t height, MDB_txn* db_txn) const {
        MDB_val key {sizeof(height), &height};
        MDB_val value;

        if (mdb_get(db_txn, dbi_block_header_, &key, &value) != MDB_SUCCESS) {
            return chain::header{};
        }

        data_chunk data {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};
        auto res = chain::header::factory_from_data(data);
        return res;
    }

    chain::block get_block_reorg(uint32_t height, MDB_txn* db_txn) const {
        MDB_val key {sizeof(height), &height};
        MDB_val value;

        if (mdb_get(db_txn, dbi_reorg_block_, &key, &value) != MDB_SUCCESS) {
            return chain::block{};
        }

        data_chunk data {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};
        auto res = chain::block::factory_from_data(data);
        return res;
    }

    chain::block get_block_reorg(uint32_t height) const {
        MDB_txn* db_txn;
        if (mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) != MDB_SUCCESS) {
            return chain::block{};
        }

        auto res = get_block_reorg(height, db_txn);

        if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
            return chain::block{};
        }

        return res;
    }
    
    utxo_code remove_block(chain::block const& block, uint32_t height) {
        MDB_txn* db_txn;
        auto res0 = mdb_txn_begin(env_, NULL, 0, &db_txn);
        if (res0 != MDB_SUCCESS) {
            return utxo_code::other;
        }

        auto res = remove_block(block, height, db_txn);
        if (res != utxo_code::success) {
            mdb_txn_abort(db_txn);
            return res;
        }

        auto res2 = mdb_txn_commit(db_txn);
        if (res2 != MDB_SUCCESS) {
            return utxo_code::other;
        }
        return utxo_code::success;
    }

    utxo_code prune_reorg_index(uint32_t amount_to_delete, MDB_txn* db_txn) {
        MDB_cursor* cursor;
        if (mdb_cursor_open(db_txn, dbi_reorg_index_, &cursor) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        // MDB_val key;
        MDB_val value;
        int rc;
        // while ((rc = mdb_cursor_get(cursor, &key, &value, MDB_NEXT)) == MDB_SUCCESS) {
        while ((rc = mdb_cursor_get(cursor, nullptr, &value, MDB_NEXT)) == MDB_SUCCESS) {
            // auto current_height = *static_cast<uint32_t*>(key.mv_data);
            auto res = mdb_del(db_txn, dbi_reorg_pool_, &value, NULL);
            if (res == MDB_NOTFOUND) {
                LOG_INFO(LOG_DATABASE) << "utxo_database_basis::prune_reorg_index - mdb_del: " << res;
                return utxo_code::key_not_found;
            }
            if (res != MDB_SUCCESS) {
                LOG_INFO(LOG_DATABASE) << "utxo_database_basis::prune_reorg_index - mdb_del: " << res;
                return utxo_code::other;
            }

            if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
                mdb_cursor_close(cursor);
                return utxo_code::other;
            }

            if (--amount_to_delete == 0) break;
        }
        
        mdb_cursor_close(cursor);
        return utxo_code::success;
    }

    utxo_code prune_reorg_block(uint32_t amount_to_delete, MDB_txn* db_txn) {
        //precondition: amount_to_delete >= 1

        MDB_cursor* cursor;
        if (mdb_cursor_open(db_txn, dbi_reorg_block_, &cursor) != MDB_SUCCESS) {
            return utxo_code::other;
        }

        MDB_val key;
        int rc;
        // while ((rc = mdb_cursor_get(cursor, &key, nullptr, MDB_NEXT)) == MDB_SUCCESS) {
        while ((rc = mdb_cursor_get(cursor, nullptr, nullptr, MDB_NEXT)) == MDB_SUCCESS) {
            // auto current_height = *static_cast<uint32_t*>(key.mv_data);

            if (mdb_cursor_del(cursor, 0) != MDB_SUCCESS) {
                mdb_cursor_close(cursor);
                return utxo_code::other;
            }

            if (--amount_to_delete == 0) break;
        }
        
        mdb_cursor_close(cursor);
        return utxo_code::success;
    }



//     /// Construct the database.
//     utxo_database_basis(path const& db_dir, uint32_t reorg_pool_limit);

//     /// Close the database.
//     ~utxo_database_basis();

//     /// 
//     static
//     bool succeed(utxo_code code);

//     /// Initialize a new transaction database.
//     bool create();

//     /// Call before using the database.
//     bool open();

//     /// Call to unload the memory map.
//     bool close();

//     /// TODO comment
//     utxo_code push_genesis(chain::block const& block);

//     /// Remove all the previous outputs and insert all the new outputs atomically.
//     utxo_code push_block(chain::block const& block, uint32_t height, uint32_t median_time_past);
    
//     /// TODO comment
//     utxo_entry get_utxo(chain::output_point const& point) const;

//     /// TODO comment
//     utxo_code get_last_height(uint32_t& out_height) const;

//     /// TODO comment
//     chain::header get_header(uint32_t height) const;

//     /// TODO comment
//     std::pair<chain::header, uint32_t> get_header(hash_digest const& hash) const;

//     utxo_code pop_block(chain::block& out_block);

//     utxo_code prune();

// private:
//     bool create_and_open_environment();
//     bool open_databases();

//     utxo_code insert_reorg_pool(uint32_t height, MDB_val& key, MDB_txn* db_txn);
//     utxo_code remove(uint32_t height, chain::output_point const& point, bool insert_reorg, MDB_txn* db_txn);
//     utxo_code insert(chain::output_point const& point, chain::output const& output, data_chunk const& fixed_data, MDB_txn* db_txn);

//     utxo_code remove_inputs(uint32_t height, chain::input::list const& inputs, bool insert_reorg, MDB_txn* db_txn);
//     utxo_code insert_outputs(hash_digest const& txid, chain::output::list const& outputs, data_chunk const& fixed_data, MDB_txn* db_txn);

//     utxo_code insert_outputs_error_treatment(uint32_t height, data_chunk const& fixed_data, hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn);
//     utxo_code push_transaction_non_coinbase(uint32_t height, data_chunk const& fixed_data, chain::transaction const& tx, bool insert_reorg, MDB_txn* db_txn);

//     template <typename I>
//     utxo_code push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, MDB_txn* db_txn);

//     utxo_code push_block_header(chain::block const& block, uint32_t height, MDB_txn* db_txn);

//     utxo_code push_block(chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg, MDB_txn* db_txn);

//     utxo_code push_genesis(chain::block const& block, MDB_txn* db_txn);

//     chain::header get_header(uint32_t height, MDB_txn* db_txn) const;


//     // Remove functions --------------------------------
//     utxo_code insert_inputs(chain::input::list const& inputs, MDB_txn* db_txn);

//     utxo_code insert_output_from_reorg_and_remove(chain::output_point const& point, MDB_txn* db_txn);

//     utxo_code remove_outputs(hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn);

//     utxo_code remove_transaction_non_coinbase(chain::transaction const& tx, MDB_txn* db_txn);

//     template <typename I>
//     utxo_code remove_transactions_non_coinbase(I f, I l, MDB_txn* db_txn);

//     utxo_code remove_block_header(hash_digest const& hash, uint32_t height, MDB_txn* db_txn);

//     utxo_code remove_block_reorg(uint32_t height, MDB_txn* db_txn);

//     utxo_code remove_reorg_index(uint32_t height, MDB_txn* db_txn);

//     utxo_code remove_block(chain::block const& block, uint32_t height, MDB_txn* db_txn);
//     utxo_code remove_block(chain::block const& block, uint32_t height);

//     // -------------------------------------------------

//     utxo_code push_block_reorg(chain::block const& block, uint32_t height, MDB_txn* db_txn);
//     chain::block get_block_reorg(uint32_t height, MDB_txn* db_txn) const;
//     chain::block get_block_reorg(uint32_t height) const;

//     bool is_old_block(chain::block const& block) const;

//     // -------------------------------------------------

//     utxo_code prune_reorg_block(uint32_t remove_until, MDB_txn* db_txn);
//     utxo_code prune_reorg_index(uint32_t remove_until, MDB_txn* db_txn);

    // -------------------------------------------------


    path const db_dir_;
    uint32_t reorg_pool_limit_;     //TODO(fernando): check if uint32_max is needed for NO-LIMIT???
    std::chrono::seconds const limit_;
    bool env_created_ = false;
    bool db_created_ = false;

    MDB_env* env_;
    MDB_dbi dbi_block_header_;
    MDB_dbi dbi_block_header_by_hash_;
    MDB_dbi dbi_utxo_;
    MDB_dbi dbi_reorg_pool_;
    MDB_dbi dbi_reorg_index_;
    MDB_dbi dbi_reorg_block_;
};

template <typename Clock>
constexpr char utxo_database_basis<Clock>::block_header_db_name[];           //key: block height, value: block header

template <typename Clock>
constexpr char utxo_database_basis<Clock>::block_header_by_hash_db_name[];   //key: block hash, value: block height

template <typename Clock>
constexpr char utxo_database_basis<Clock>::utxo_db_name[];                   //key: point, value: output

template <typename Clock>
constexpr char utxo_database_basis<Clock>::reorg_pool_name[];                //key: key: point, value: output

template <typename Clock>
constexpr char utxo_database_basis<Clock>::reorg_index_name[];               //key: block height, value: point list

template <typename Clock>
constexpr char utxo_database_basis<Clock>::reorg_block_name[];               //key: block height, value: block


using utxo_database = utxo_database_basis<std::chrono::system_clock>;

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

#endif // BITPRIM_DATABASE_UTXO_DATABASE_HPP_

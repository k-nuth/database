/**
 * Copyright (c) 2016-2018 Bitprim Inc.
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
#ifndef BITPRIM_DATABASE_INTERNAL_DATABASE_IPP_
#define BITPRIM_DATABASE_INTERNAL_DATABASE_IPP_

namespace libbitcoin {
namespace database {

using utxo_pool_t = std::unordered_map<chain::point, utxo_entry>;

template <typename Clock>
internal_database_basis<Clock>::internal_database_basis(path const& db_dir, uint32_t reorg_pool_limit, uint64_t db_max_size)
    : db_dir_(db_dir)
    , reorg_pool_limit_(reorg_pool_limit)
    , limit_(blocks_to_seconds(reorg_pool_limit))
    , db_max_size_(db_max_size)
{}

template <typename Clock>
internal_database_basis<Clock>::~internal_database_basis() {
    close();
}

template <typename Clock>
bool internal_database_basis<Clock>::create() {
    boost::system::error_code ec;

    if ( ! create_directories(db_dir_, ec)) {
        if (ec.value() == directory_exists) {
            LOG_ERROR(LOG_DATABASE) << "Failed because the directory " << db_dir_ << " already exists.";
            return false;
        }

        LOG_ERROR(LOG_DATABASE) << "Failed to create directory " << db_dir_ << " with error, '" << ec.message() << "'.";
        return false;
    }

    auto ret = open_internal();
    if ( ! ret ) {
        return false;
    }

    ret = create_db_mode_property();
    if ( ! ret ) {
        return false;
    }

    return true;
}    

template <typename Clock>
bool internal_database_basis<Clock>::create_db_mode_property() {

    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, 0, &db_txn);
    if (res != MDB_SUCCESS) {
        return false;
    }

    db_mode_code db_mode_;

#if defined(BITPRIM_DB_NEW_FULL) 
    db_mode_ = db_mode_code::db_new_full;
#elif defined(BITPRIM_DB_NEW_BLOCKS)
    db_mode_ = db_mode_code::db_new_with_blocks;
#elif defined(BITPRIM_DB_NEW_FULL_ASYNC)
    db_mode_ = db_mode_code::db_new_full_async;
#else
    db_mode_ = db_mode_code::db_new;
#endif

    property_code property_code_ = property_code::db_mode;

    MDB_val key {sizeof(property_code_), &property_code_};
    MDB_val value {sizeof(db_mode_), &db_mode_};

    res = mdb_put(db_txn, dbi_properties_, &key, &value, MDB_NOOVERWRITE);
    if (res != MDB_SUCCESS) {  
        mdb_txn_abort(db_txn);
        return false;
    }

    res = mdb_txn_commit(db_txn);
    if (res != MDB_SUCCESS) {
        return false;
    }

    return true;
}

template <typename Clock>
bool internal_database_basis<Clock>::open() {
    
    auto ret = open_internal();
    if ( ! ret ) {
        return false;
    }

    ret = verify_db_mode_property();
    if ( ! ret ) {
        return false;
    }

    return true;
}


template <typename Clock>
bool internal_database_basis<Clock>::open_internal() {
    
    if ( ! create_and_open_environment()) {
        LOG_ERROR(LOG_DATABASE) << "Error configuring LMDB environment.";
        return false;
    }

    return open_databases();
}

template <typename Clock>
bool internal_database_basis<Clock>::verify_db_mode_property() {

    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res != MDB_SUCCESS) {
        return false;
    }

    property_code property_code_ = property_code::db_mode;
    
    MDB_val key {sizeof(property_code_), &property_code_};
    MDB_val value;

    res = mdb_get(db_txn, dbi_properties_, &key, &value);
    if (res != MDB_SUCCESS) {  
        mdb_txn_abort(db_txn);
        return false;
    }

    auto db_mode_ = *static_cast<db_mode_code*>(value.mv_data);

    res = mdb_txn_commit(db_txn);
    if (res != MDB_SUCCESS) {
        return false;
    }

#if defined(BITPRIM_DB_NEW_FULL)
    return  db_mode_ == db_mode_code::db_new_full;     
#elif defined(BITPRIM_DB_NEW_BLOCKS)
    return  db_mode_ == db_mode_code::db_new_with_blocks;
#elif defined(BITPRIM_DB_NEW_FULL_ASYNC)
    return  db_mode_ == db_mode_code::db_new_full_async;
#else
    return  db_mode_ == db_mode_code::db_new;
#endif

}

template <typename Clock>
bool internal_database_basis<Clock>::close() {
    if (db_opened_) {
        mdb_dbi_close(env_, dbi_block_header_);
        mdb_dbi_close(env_, dbi_block_header_by_hash_);
        mdb_dbi_close(env_, dbi_utxo_);
        mdb_dbi_close(env_, dbi_reorg_pool_);
        mdb_dbi_close(env_, dbi_reorg_index_);
        mdb_dbi_close(env_, dbi_reorg_block_);
        mdb_dbi_close(env_, dbi_properties_);

#if defined(BITPRIM_DB_NEW_BLOCKS) || defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
        mdb_dbi_close(env_, dbi_block_db_);
#endif
        
#if defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
        mdb_dbi_close(env_, dbi_transaction_db_);
        mdb_dbi_close(env_, dbi_transaction_hash_db_);
        mdb_dbi_close(env_, dbi_history_db_);
        mdb_dbi_close(env_, dbi_spend_db_);
        mdb_dbi_close(env_, dbi_transaction_unconfirmed_db_);
#endif

#if defined(BITPRIM_DB_NEW_FULL_ASYNC)
        mdb_dbi_close(env_, dbi_blocks_index_db_);
#endif

        db_opened_ = false;
    }

    if (env_created_) {
        mdb_env_close(env_);
        
        env_created_ = false;
    }

    return true;
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_genesis(chain::block const& block) {
    
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

    auto res2 = mdb_txn_commit(db_txn);
    if (res2 != MDB_SUCCESS) {
        return result_code::other;
    }
    return res;
}

//TODO(fernando): optimization: consider passing a list of outputs to insert and a list of inputs to delete instead of an entire Block.
//                  avoiding inserting and erasing internal spenders

template <typename Clock>
result_code internal_database_basis<Clock>::push_block(chain::block const& block, uint32_t height, uint32_t median_time_past) {

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

    auto res2 = mdb_txn_commit(db_txn);
    if (res2 != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error commiting LMDB Transaction [push_block] " << res2;
        return result_code::other;
    }

    return res;
}

template <typename Clock>
utxo_entry internal_database_basis<Clock>::get_utxo(chain::output_point const& point) const {

    MDB_txn* db_txn;
    auto res0 = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res0 != MDB_SUCCESS) {        
        //std::cout << "bbbbbbbbbbbb" << static_cast<uint32_t>(res0) << std::endl;
        LOG_INFO(LOG_DATABASE) << "Error begining LMDB Transaction [get_utxo] " << res0;
        return {};
    }

    auto ret = get_utxo(point, db_txn);

    res0 = mdb_txn_commit(db_txn);
    if (res0 != MDB_SUCCESS) {
        //std::cout << "cccc" << static_cast<uint32_t>(res0) << std::endl;
        LOG_DEBUG(LOG_DATABASE) << "Error commiting LMDB Transaction [get_utxo] " << res0;        
        return {};
    }

    return ret;
}

template <typename Clock>
result_code internal_database_basis<Clock>::get_last_height(uint32_t& out_height) const {
    
    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res != MDB_SUCCESS) {
        return result_code::other;
    }

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_block_header_, &cursor) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
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

template <typename Clock>
std::pair<chain::header, uint32_t> internal_database_basis<Clock>::get_header(hash_digest const& hash) const {
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

template <typename Clock>
chain::header internal_database_basis<Clock>::get_header(uint32_t height) const {
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

template <typename Clock>
result_code internal_database_basis<Clock>::pop_block(chain::block& out_block) {
    uint32_t height;

    //TODO: (Mario) use only one transaction ?

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

template <typename Clock>
result_code internal_database_basis<Clock>::prune() {
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


template <typename Clock>
std::pair<result_code, utxo_pool_t> internal_database_basis<Clock>::get_utxo_pool_from(uint32_t from, uint32_t to) const {
    // precondition: from <= to
    utxo_pool_t pool;

    MDB_txn* db_txn;
    auto zzz = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (zzz != MDB_SUCCESS) {
        return {result_code::other, pool};
    }

    MDB_cursor* cursor;
    if (mdb_cursor_open(db_txn, dbi_reorg_index_, &cursor) != MDB_SUCCESS) {
        mdb_txn_commit(db_txn);
        return {result_code::other, pool};
    }

    MDB_val key {sizeof(from), &from};
    MDB_val value;

    // int rc = mdb_cursor_get(cursor, &key, &value, MDB_SET);
    int rc = mdb_cursor_get(cursor, &key, &value, MDB_SET_RANGE);
    if (rc != MDB_SUCCESS) {
        mdb_cursor_close(cursor);
        mdb_txn_commit(db_txn);
        return {result_code::key_not_found, pool};
    }

    auto current_height = *static_cast<uint32_t*>(key.mv_data);
    if (current_height < from) {
        mdb_cursor_close(cursor);
        mdb_txn_commit(db_txn);
        return {result_code::other, pool};
    }
    // if (current_height > from) {
    //     mdb_cursor_close(cursor);
    //     mdb_txn_commit(db_txn);
    //     return {result_code::other, pool};
    // }
    if (current_height > to) {
        mdb_cursor_close(cursor);
        mdb_txn_commit(db_txn);
        return {result_code::other, pool};
    }

    auto res = insert_reorg_into_pool(pool, value, db_txn);
    if (res != result_code::success) {
        mdb_cursor_close(cursor);
        mdb_txn_commit(db_txn);
        return {res, pool};
    }

    while ((rc = mdb_cursor_get(cursor, &key, &value, MDB_NEXT)) == MDB_SUCCESS) {
        current_height = *static_cast<uint32_t*>(key.mv_data);
        if (current_height > to) {
            mdb_cursor_close(cursor);
            mdb_txn_commit(db_txn);
            return {result_code::other, pool};
        }

        res = insert_reorg_into_pool(pool, value, db_txn);
        if (res != result_code::success) {
            mdb_cursor_close(cursor);
            mdb_txn_commit(db_txn);
            return {res, pool};
        }
    }
    
    mdb_cursor_close(cursor);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return {result_code::other, pool};
    }

    return {result_code::success, pool};
}


#if defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)

template <typename Clock>
result_code internal_database_basis<Clock>::push_transaction_unconfirmed(chain::transaction const& tx, uint32_t height) {

    MDB_txn* db_txn;
    if (mdb_txn_begin(env_, NULL, 0, &db_txn) != MDB_SUCCESS) {
        return result_code::other;
    }

    auto res = insert_transaction_unconfirmed(tx, height, db_txn);
    if (res != result_code::success) {
        mdb_txn_abort(db_txn);
        return res;
    }

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return result_code::other;
    }
}

#endif

// Private functions
// ------------------------------------------------------------------------------------------------------

template <typename Clock>
bool internal_database_basis<Clock>::is_old_block(chain::block const& block) const {
    return is_old_block_<Clock>(block, limit_);
}

template <typename Clock>
size_t internal_database_basis<Clock>::get_db_page_size() const {
    return boost::interprocess::mapped_region::get_page_size();
}

template <typename Clock>
size_t internal_database_basis<Clock>::adjust_db_size(size_t size) const {
    // precondition: env_ have to be created (mdb_env_create)
    // The mdb_env_set_mapsize should be a multiple of the OS page size.
    size_t const page_size = get_db_page_size();
    auto res = size_t(std::ceil(double(size) / page_size)) * page_size;
    return res;

    // size_t const mod = size % page_size;
    // return size + (mod != 0) ? (page_size - mod) : 0;
}    

template <typename Clock>
bool internal_database_basis<Clock>::create_and_open_environment() {

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

    //Bitprim: fastest flags
    //for more secure flags use: MDB_NORDAHEAD | MDB_NOSYNC  | MDB_NOTLS
    res = mdb_env_open(env_, db_dir_.string().c_str() , MDB_NORDAHEAD | MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC | MDB_NOTLS, env_open_mode_);
    return res == MDB_SUCCESS;
}

inline 
int compare_uint64(const MDB_val *a, const MDB_val *b) {
    const uint64_t va = *(const uint64_t *)a->mv_data;
    const uint64_t vb = *(const uint64_t *)b->mv_data;
    
    //std::cout << "va: " << va << std::endl;
    //std::cout << "vb: " << va << std::endl;

    return (va < vb) ? -1 : va > vb;
}

template <typename Clock>
bool internal_database_basis<Clock>::open_databases() {
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

    res = mdb_dbi_open(db_txn, db_properties_name, MDB_CREATE | MDB_INTEGERKEY, &dbi_properties_);
    if (res != MDB_SUCCESS) {
        return false;
    }

#if defined(BITPRIM_DB_NEW_BLOCKS) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
    res = mdb_dbi_open(db_txn, block_db_name, MDB_CREATE | MDB_INTEGERKEY, &dbi_block_db_);
    if (res != MDB_SUCCESS) {
        return false;
    }
#endif 

#if defined(BITPRIM_DB_NEW_FULL)

    res = mdb_dbi_open(db_txn, block_db_name, MDB_CREATE | MDB_DUPSORT | MDB_INTEGERKEY | MDB_DUPFIXED  | MDB_INTEGERDUP, &dbi_block_db_);
    if (res != MDB_SUCCESS) {
        return false;
    }

#endif

#if defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
    
    res = mdb_dbi_open(db_txn, transaction_db_name, MDB_CREATE | MDB_INTEGERKEY, &dbi_transaction_db_);
    if (res != MDB_SUCCESS) {
        return false;
    }

    res = mdb_dbi_open(db_txn, transaction_hash_db_name, MDB_CREATE, &dbi_transaction_hash_db_);
    if (res != MDB_SUCCESS) {
        return false;
    }

    res = mdb_dbi_open(db_txn, history_db_name, MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, &dbi_history_db_);
    if (res != MDB_SUCCESS) {
        return false;
    }

    res = mdb_dbi_open(db_txn, spend_db_name, MDB_CREATE, &dbi_spend_db_);
    if (res != MDB_SUCCESS) {
        return false;
    }

    res = mdb_dbi_open(db_txn, transaction_unconfirmed_db_name, MDB_CREATE, &dbi_transaction_unconfirmed_db_);
    if (res != MDB_SUCCESS) {
        return false;
    }

    mdb_set_dupsort(db_txn, dbi_history_db_, compare_uint64);

#endif //BITPRIM_DB_NEW_FULL


#if defined(BITPRIM_DB_NEW_FULL_ASYNC)

    res = mdb_dbi_open(db_txn, block_index_db_name, MDB_CREATE | MDB_DUPSORT | MDB_INTEGERKEY | MDB_DUPFIXED  | MDB_INTEGERDUP, &dbi_blocks_index_db_);
    if (res != MDB_SUCCESS) {
        return false;
    }

#endif

    db_opened_ = mdb_txn_commit(db_txn) == MDB_SUCCESS;
    return db_opened_;
}

#if defined(BITPRIM_DB_NEW_FULL_ASYNC)

template <typename Clock>
result_code internal_database_basis<Clock>::remove_inputs_full(hash_digest const& tx_id, uint32_t height, chain::input::list const& inputs, bool insert_reorg, MDB_txn* db_txn) {
    uint32_t pos = 0;
    for (auto const& input: inputs) {
        
        chain::input_point const inpoint {tx_id, pos};
        auto const& prevout = input.previous_output();
        
        auto res = insert_input_history(inpoint, height, input, db_txn);            
        if (res != result_code::success) {
            return res;
        }

        //set spender height in tx database
        //Bitprim: Commented because we don't validate transaction duplicates (BIP-30)
        /*res = set_spend(prevout, height, db_txn);
        if (res != result_code::success) {
            return res;
        }*/

        res = remove_utxo(height, prevout, insert_reorg, db_txn);
        if (res != result_code::success) {
            return res;
        }

        //insert in spend database
        res = insert_spend(prevout, inpoint, db_txn);
        if (res != result_code::success) {
            return res;
        }

        ++pos;
    }
    return result_code::success;
}


#endif



template <typename Clock>
result_code internal_database_basis<Clock>::remove_inputs(hash_digest const& tx_id, uint32_t height, chain::input::list const& inputs, bool insert_reorg, MDB_txn* db_txn) {
    uint32_t pos = 0;
    for (auto const& input: inputs) {
        
        chain::input_point const inpoint {tx_id, pos};
        auto const& prevout = input.previous_output();
        
#if defined(BITPRIM_DB_NEW_FULL)
        auto res = insert_input_history(inpoint, height, input, db_txn);            
        if (res != result_code::success) {
            return res;
        }

        //set spender height in tx database
        //Bitprim: Commented because we don't validate transaction duplicates (BIP-30)
        /*res = set_spend(prevout, height, db_txn);
        if (res != result_code::success) {
            return res;
        }*/

#else 
    result_code res;
#endif

        res = remove_utxo(height, prevout, insert_reorg, db_txn);
        if (res != result_code::success) {
            return res;
        }

#if defined(BITPRIM_DB_NEW_FULL)

        //insert in spend database
        res = insert_spend(prevout, inpoint, db_txn);
        if (res != result_code::success) {
            return res;
        }

#endif

        ++pos;
    }
    return result_code::success;
}

#if defined(BITPRIM_DB_NEW_FULL_ASYNC)
template <typename Clock>
result_code internal_database_basis<Clock>::insert_outputs_full(hash_digest const& tx_id, uint32_t height, chain::output::list const& outputs, data_chunk const& fixed_data, MDB_txn* db_txn) {
    uint32_t pos = 0;
    for (auto const& output: outputs) {
        
        auto res = insert_utxo(chain::point{tx_id, pos}, output, fixed_data, db_txn);
        if (res != result_code::success) {
            return res;
        }
        
        res = insert_output_history(tx_id, height, pos, output, db_txn);
        if (res != result_code::success) {
            return res;
        }

        ++pos;
    }
    return result_code::success;
}
#endif


template <typename Clock>
result_code internal_database_basis<Clock>::insert_outputs(hash_digest const& tx_id, uint32_t height, chain::output::list const& outputs, data_chunk const& fixed_data, MDB_txn* db_txn) {
    uint32_t pos = 0;
    for (auto const& output: outputs) {
        
        auto res = insert_utxo(chain::point{tx_id, pos}, output, fixed_data, db_txn);
        if (res != result_code::success) {
            return res;
        }

#if defined(BITPRIM_DB_NEW_FULL)
        
        res = insert_output_history(tx_id, height, pos, output, db_txn);
        if (res != result_code::success) {
            return res;
        }

#endif

        ++pos;
    }
    return result_code::success;
}

#if defined(BITPRIM_DB_NEW_FULL_ASYNC)
template <typename Clock>
result_code internal_database_basis<Clock>::insert_outputs_error_treatment_full(uint32_t height, data_chunk const& fixed_data, hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
    auto res = insert_outputs_full(txid,height, outputs, fixed_data, db_txn);
    
    if (res == result_code::duplicated_key) {
        // std::cout << "bbbhhhhhhhhhhh" << static_cast<uint32_t>(res) << "\n";
        //TODO(fernando): log and continue
        return result_code::success_duplicate_coinbase;
    }
    return res;
}
#endif

template <typename Clock>
result_code internal_database_basis<Clock>::insert_outputs_error_treatment(uint32_t height, data_chunk const& fixed_data, hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
    auto res = insert_outputs(txid,height, outputs, fixed_data, db_txn);
    
    if (res == result_code::duplicated_key) {
        // std::cout << "bbbhhhhhhhhhhh" << static_cast<uint32_t>(res) << "\n";
        //TODO(fernando): log and continue
        return result_code::success_duplicate_coinbase;
    }
    return res;
}


#if defined(BITPRIM_DB_NEW_FULL_ASYNC)
template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::push_transactions_outputs_non_coinbase_full(uint32_t height, data_chunk const& fixed_data, I f, I l, MDB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    while (f != l) {
        auto const& tx = *f;
        auto res = insert_outputs_error_treatment_full(height, fixed_data, tx.hash(), tx.outputs(), db_txn);
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }
    return result_code::success;
}
#endif

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::push_transactions_outputs_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, MDB_txn* db_txn) {
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

#if defined(BITPRIM_DB_NEW_FULL_ASYNC)

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::remove_transactions_inputs_non_coinbase_full(uint32_t height, I f, I l, bool insert_reorg, MDB_txn* db_txn) {
    while (f != l) {
        auto const& tx = *f;
        auto res = remove_inputs_full(tx.hash(), height, tx.inputs(), insert_reorg, db_txn);
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }
    return result_code::success;
}

#endif


template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::remove_transactions_inputs_non_coinbase(uint32_t height, I f, I l, bool insert_reorg, MDB_txn* db_txn) {
    while (f != l) {
        auto const& tx = *f;
        auto res = remove_inputs(tx.hash(), height, tx.inputs(), insert_reorg, db_txn);
        if (res != result_code::success) {
            return res;
        }
        ++f;
    }
    return result_code::success;
}

#if defined(BITPRIM_DB_NEW_FULL_ASYNC)

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::push_transactions_non_coinbase_full(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, MDB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    auto res = push_transactions_outputs_non_coinbase_full(height, fixed_data, f, l, db_txn);
    if (res != result_code::success) {
        return res;
    }

    return remove_transactions_inputs_non_coinbase_full(height, f, l, insert_reorg, db_txn);
}


#endif

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::push_transactions_non_coinbase(uint32_t height, data_chunk const& fixed_data, I f, I l, bool insert_reorg, MDB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    auto res = push_transactions_outputs_non_coinbase(height, fixed_data, f, l, db_txn);
    if (res != result_code::success) {
        return res;
    }

    return remove_transactions_inputs_non_coinbase(height, f, l, insert_reorg, db_txn);
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_block(chain::block const& block, uint32_t height, uint32_t median_time_past, bool insert_reorg, MDB_txn* db_txn) {
    //precondition: block.transactions().size() >= 1

    auto res = push_block_header(block, height, db_txn);
    if (res != result_code::success) {
        return res;
    }

    auto const& txs = block.transactions();     

#if defined(BITPRIM_DB_NEW_BLOCKS)
    res = insert_block_serialized(block, height, db_txn);        
    if (res != result_code::success) {
        // std::cout << "22222222222222" << static_cast<uint32_t>(res) << "\n";
        return res;
    }

#elif defined(BITPRIM_DB_NEW_FULL)  

    auto tx_count = get_tx_count(db_txn);
    res = insert_block_with_tx_id(block, height, tx_count, db_txn);        
    if (res != result_code::success) {
        // std::cout << "22222222222222" << static_cast<uint32_t>(res) << "\n";
        return res;
    }

    res = insert_transactions(txs.begin(), txs.end(), height, median_time_past, tx_count, db_txn);
    if (res == result_code::duplicated_key) {
        res = result_code::success_duplicate_coinbase;
    } else if (res != result_code::success) {
        return res;
    }

#elif defined(BITPRIM_DB_NEW_FULL_ASYNC) 

    bool indexing_completed = is_indexing_completed(db_txn);

    if (indexing_completed) {

        auto tx_count = get_tx_count(db_txn);
        res = insert_block_with_tx_id(block, height, tx_count, db_txn);        
        if (res != result_code::success) {
            // std::cout << "22222222222222" << static_cast<uint32_t>(res) << "\n";
            return res;
        }

        res = insert_transactions(txs.begin(), txs.end(), height, median_time_past, tx_count, db_txn);
        if (res == result_code::duplicated_key) {
            res = result_code::success_duplicate_coinbase;
        } else if (res != result_code::success) {
            return res;
        }    
    }
    else {

        res = insert_block_serialized(block, height, db_txn);        
        if (res != result_code::success) {
            // std::cout << "22222222222222" << static_cast<uint32_t>(res) << "\n";
            return res;
        }
    }

#endif

    if ( insert_reorg ) {
        res = push_block_reorg(block, height, db_txn);
        if (res != result_code::success) {
            return res;
        }
    }

    
    auto const& coinbase = txs.front();

    auto fixed = utxo_entry::to_data_fixed(height, median_time_past, true);                                     //TODO(fernando): podría estar afuera de la DBTx
    

#if defined(BITPRIM_DB_NEW_FULL_ASYNC)
    result_code res0;

    if (indexing_completed) {
        res0 = insert_outputs_error_treatment_full(height, fixed, coinbase.hash(), coinbase.outputs(), db_txn);     //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx

        if ( ! succeed(res0)) {
            // std::cout << "aaaaaaaaaaaaaaa" << static_cast<uint32_t>(res0) << "\n";
            return res0;
        }

        fixed.back() = 0;   //The last byte equal to 0 means NonCoinbaseTx    
        res = push_transactions_non_coinbase_full(height, fixed, txs.begin() + 1, txs.end(), insert_reorg, db_txn);
        if (res != result_code::success) {
            // std::cout << "bbb" << static_cast<uint32_t>(res) << "\n";
            return res;
        }
    }
    else {

        res0 = insert_outputs_error_treatment(height, fixed, coinbase.hash(), coinbase.outputs(), db_txn);     //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx

        if ( ! succeed(res0)) {
            // std::cout << "aaaaaaaaaaaaaaa" << static_cast<uint32_t>(res0) << "\n";
            return res0;
        }

        fixed.back() = 0;   //The last byte equal to 0 means NonCoinbaseTx    
        res = push_transactions_non_coinbase(height, fixed, txs.begin() + 1, txs.end(), insert_reorg, db_txn);
        if (res != result_code::success) {
            // std::cout << "bbb" << static_cast<uint32_t>(res) << "\n";
            return res;
        }

        // std::cout << "ddd" << static_cast<uint32_t>(res0) << "\n";
    }
    
#else
    auto res0 = insert_outputs_error_treatment(height, fixed, coinbase.hash(), coinbase.outputs(), db_txn);     //TODO(fernando): tx.hash() debe ser llamado fuera de la DBTx

    if ( ! succeed(res0)) {
        // std::cout << "aaaaaaaaaaaaaaa" << static_cast<uint32_t>(res0) << "\n";
        return res0;
    }

    fixed.back() = 0;   //The last byte equal to 0 means NonCoinbaseTx    
    res = push_transactions_non_coinbase(height, fixed, txs.begin() + 1, txs.end(), insert_reorg, db_txn);
    if (res != result_code::success) {
        // std::cout << "bbb" << static_cast<uint32_t>(res) << "\n";
        return res;
    }

    // std::cout << "ddd" << static_cast<uint32_t>(res0) << "\n";
    
#endif
    
    if (res == result_code::success_duplicate_coinbase)
        return res;
    
    return res0;
}

template <typename Clock>
result_code internal_database_basis<Clock>::push_genesis(chain::block const& block, MDB_txn* db_txn) {
    auto res = push_block_header(block, 0, db_txn);
    if (res != result_code::success) {
        return res;
    }

#if defined(BITPRIM_DB_NEW_BLOCKS) 
    res = insert_block_serialized(block, 0, db_txn);
    if (res != result_code::success) {
        return res;
    }
#elif defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
    auto tx_count = get_tx_count(db_txn);
    res = insert_block_with_tx_id(block, 0, tx_count, db_txn);
    if (res != result_code::success) {
        return res;
    }
    
    auto const& txs = block.transactions();
    auto const& coinbase = txs.front();
    auto const& hash = coinbase.hash();
    auto const median_time_past = block.header().validation.median_time_past;
    
    res = insert_transaction(tx_count, coinbase, 0, median_time_past, 0, db_txn);
    if (res != result_code::success && res != result_code::duplicated_key) {
        return res;
    }
        
    res = insert_output_history(hash, 0, 0, coinbase.outputs()[0], db_txn);
    if (res != result_code::success) {
        return res;
    }

#if defined(BITPRIM_DB_NEW_FULL_ASYNC)

    res = update_last_height_indexed(0, db_txn);
    if (res != result_code::success) {
        return res;
    }

#endif

#endif
    return res;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_outputs(hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn) {
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

template <typename Clock>
result_code internal_database_basis<Clock>::insert_inputs(chain::input::list const& inputs, MDB_txn* db_txn) {
    for (auto const& input: inputs) {
        auto const& point = input.previous_output();

        auto res = insert_output_from_reorg_and_remove(point, db_txn);
        if (res != result_code::success) {
            return res;
        }
    }
    return result_code::success;
}

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::insert_transactions_inputs_non_coinbase(I f, I l, MDB_txn* db_txn) {
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

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::remove_transactions_outputs_non_coinbase(I f, I l, MDB_txn* db_txn) {
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

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::remove_transactions_non_coinbase(I f, I l, MDB_txn* db_txn) {
    // precondition: [f, l) is a valid range and there are no coinbase transactions in it.

    auto res = insert_transactions_inputs_non_coinbase(f, l, db_txn);
    if (res != result_code::success) {
        return res;
    }
    return remove_transactions_outputs_non_coinbase(f, l, db_txn);
}


template <typename Clock>
result_code internal_database_basis<Clock>::remove_block(chain::block const& block, uint32_t height, MDB_txn* db_txn) {
    //precondition: block.transactions().size() >= 1

    auto const& txs = block.transactions();
    auto const& coinbase = txs.front();

    //UTXO
    auto res = remove_transactions_non_coinbase(txs.begin() + 1, txs.end(), db_txn);
    if (res != result_code::success) {
        return res;
    }

    //UTXO Coinbase
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
    if (res != result_code::success && res != result_code::key_not_found) {
        return res;
    }

#if defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
    //Transaction Database
    res = remove_transactions(block, height, db_txn);
    if (res != result_code::success) {
        return res;
    }
#endif //defined(BITPRIM_DB_NEW_FULL)

#if defined(BITPRIM_DB_NEW_BLOCKS) || defined(BITPRIM_DB_NEW_FULL) || defined(BITPRIM_DB_NEW_FULL_ASYNC)
    res = remove_blocks_db(height, db_txn);
    if (res != result_code::success) {
        return res;
    }
#endif //defined(BITPRIM_DB_NEW_BLOCKS) || defined(BITPRIM_DB_NEW_FULL)

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_block(chain::block const& block, uint32_t height) {
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

} // namespace database
} // namespace libbitcoin


#endif // BITPRIM_DATABASE_INTERNAL_DATABASE_IPP_

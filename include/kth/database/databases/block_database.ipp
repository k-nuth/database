// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_BLOCK_DATABASE_IPP_
#define KTH_DATABASE_BLOCK_DATABASE_IPP_

namespace kth::database {

/*
#if defined(KTH_DB_NEW_FULL)
    
template <typename Clock>
data_chunk internal_database_basis<Clock>::serialize_txs(domain::chain::block const& block) {
    data_chunk ret;
    auto const& txs = block.transactions();
    ret.reserve(txs.size() * kth::hash_size);

    for (auto const& tx : txs) {
        auto hash = tx.hash();
        ret.insert(ret.end(), hash.begin(), hash.end());
    }
    return ret;
}

#endif // defined(KTH_DB_NEW_FULL)
*/


//public
template <typename Clock>
std::pair<domain::chain::block, uint32_t> internal_database_basis<Clock>::get_block(hash_digest const& hash) const {
    auto key = kth_db_make_value(hash.size(), const_cast<hash_digest&>(hash).data());

    KTH_DB_txn* db_txn;
    auto res = kth_db_txn_begin(env_, NULL, KTH_DB_RDONLY, &db_txn);
    if (res != KTH_DB_SUCCESS) {
        return {};
    }

    KTH_DB_val value;
    if (kth_db_get(db_txn, dbi_block_header_by_hash_, &key, &value) != KTH_DB_SUCCESS) {
        kth_db_txn_commit(db_txn);
        // kth_db_txn_abort(db_txn);
        return {};
    }

    // assert kth_db_get_size(value) == 4;
    auto height = *static_cast<uint32_t*>(kth_db_get_data(value));

    auto block = get_block(height, db_txn);

    if (kth_db_txn_commit(db_txn) != KTH_DB_SUCCESS) {
        return {};
    }

    return {block, height};
}

//public
template <typename Clock>
domain::chain::block internal_database_basis<Clock>::get_block(uint32_t height) const { 
    KTH_DB_txn* db_txn;
    auto res = kth_db_txn_begin(env_, NULL, KTH_DB_RDONLY, &db_txn);
    if (res != KTH_DB_SUCCESS) {
        return domain::chain::block{};
    }

    auto block = get_block(height, db_txn);

    if (kth_db_txn_commit(db_txn) != KTH_DB_SUCCESS) {
        return domain::chain::block{};
    }

    return block;
}

template <typename Clock>
domain::chain::block internal_database_basis<Clock>::get_block(uint32_t height, KTH_DB_txn* db_txn) const {

    auto key = kth_db_make_value(sizeof(height), &height);
    
#if defined(KTH_DB_NEW_BLOCKS)
    
    KTH_DB_val value;

    if (kth_db_get(db_txn, dbi_block_db_, &key, &value) != KTH_DB_SUCCESS) {
        return domain::chain::block{};
    }

    auto data = db_value_to_data_chunk(value);
    auto res = domain::create<domain::chain::block>(data);
    return res;

#elif defined(KTH_DB_NEW_FULL)

    auto header = get_header(height, db_txn);
    if ( ! header.is_valid()) {
        return {};
    }

    domain::chain::transaction::list tx_list;

    KTH_DB_cursor* cursor;
    if (kth_db_cursor_open(db_txn, dbi_block_db_, &cursor) != KTH_DB_SUCCESS) {
        return {};
    }

    KTH_DB_val value;
    int rc;
    if ((rc = kth_db_cursor_get(cursor, &key, &value, MDB_SET)) == 0) {
       
        auto tx_id = *static_cast<uint32_t*>(kth_db_get_data(value));;
        auto const entry = get_transaction(tx_id, db_txn);
        
        if ( ! entry.is_valid()) {
            return {};
        }
        
        tx_list.push_back(std::move(entry.transaction()));
    
        while ((rc = kth_db_cursor_get(cursor, &key, &value, MDB_NEXT_DUP)) == 0) {
            auto tx_id = *static_cast<uint32_t*>(kth_db_get_data(value));;
            auto const entry = get_transaction(tx_id, db_txn);
            tx_list.push_back(std::move(entry.transaction()));    
        }
    } 
    
    kth_db_cursor_close(cursor);

    /*auto n = kth_db_get_size(value);
    auto f = static_cast<uint8_t*>(kth_db_get_data(value)); 
    //precondition: mv_size es multiplo de 32
    
    chain::transaction::list tx_list;
    tx_list.reserve(n / kth::hash_size);
    
    while (n != 0) {
        hash_digest h;
        std::copy(f, f + kth::hash_size, h.data());
        
        auto tx_entry = get_transaction(h,max_uint32, db_txn);
        
        if ( ! tx_entry.is_valid() ) {
            return chain::block{};
        }

        auto const& tx = tx_entry.transaction();

        tx_list.push_back(std::move(tx));

        n -= kth::hash_size;
        f += kth::hash_size;
    }*/
    
    return chain::block{header, std::move(tx_list)};

#else
    auto block = get_block_reorg(height, db_txn);
    return block;
#endif //defined(KTH_DB_NEW_FULL)
}


#if defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)

#if ! defined(KTH_DB_READONLY)

#if defined(KTH_DB_NEW_BLOCKS)
template <typename Clock>
result_code internal_database_basis<Clock>::insert_block(chain::block const& block, uint32_t height, KTH_DB_txn* db_txn) {
#elif defined(KTH_DB_NEW_FULL)
template <typename Clock>
result_code internal_database_basis<Clock>::insert_block(chain::block const& block, uint32_t height, uint64_t tx_count, KTH_DB_txn* db_txn) {
#endif

/*#if defined(KTH_DB_NEW_BLOCKS)
    auto data = block.to_data(false);
#elif defined(KTH_DB_NEW_FULL)
    auto data = serialize_txs(block);
#endif
*/

auto key = kth_db_make_value(sizeof(height), &height);

#if defined(KTH_DB_NEW_BLOCKS)

    //TODO: store tx hash
    auto data = block.to_data(false);
    auto value = kth_db_make_value(data.size(), data.data());

    auto res = kth_db_put(db_txn, dbi_block_db_, &key, &value, KTH_DB_APPEND);
    if (res == KTH_DB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE, "Duplicate key in Block DB [insert_block] ", res);
        return result_code::duplicated_key;
    }

    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error saving in Block DB [insert_block] ", res);
        return result_code::other;
    }

#elif defined(KTH_DB_NEW_FULL)

    auto const& txs = block.transactions();
    
    for (uint64_t i = tx_count; i<tx_count + txs.size();i++ ) {
        auto value = kth_db_make_value(sizeof(i), &i);

        auto res = kth_db_put(db_txn, dbi_block_db_, &key, &value, MDB_APPENDDUP);
        if (res == KTH_DB_KEYEXIST) {
            LOG_INFO(LOG_DATABASE, "Duplicate key in Block DB [insert_block] ", res);
            return result_code::duplicated_key;
        }

        if (res != KTH_DB_SUCCESS) {
            LOG_INFO(LOG_DATABASE, "Error saving in Block DB [insert_block] ", res);
            return result_code::other;
        }
    }

#endif

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::remove_blocks_db(uint32_t height, KTH_DB_txn* db_txn) {
    auto key = kth_db_make_value(sizeof(height), &height);
    
 #if defined(KTH_DB_NEW_BLOCKS)   
    
    auto res = kth_db_del(db_txn, dbi_block_db_, &key, NULL);
    if (res == KTH_DB_NOTFOUND) {
        LOG_INFO(LOG_DATABASE, "Key not found deleting blocks DB in LMDB [remove_blocks_db] - kth_db_del: ", res);
        return result_code::key_not_found;
    }
    if (res != KTH_DB_SUCCESS) {
        LOG_INFO(LOG_DATABASE, "Error deleting blocks DB in LMDB [remove_blocks_db] - kth_db_del: ", res);
        return result_code::other;
    }

#elif defined(KTH_DB_NEW_FULL)

    KTH_DB_cursor* cursor;
    if (kth_db_cursor_open(db_txn, dbi_block_db_, &cursor) != KTH_DB_SUCCESS) {
        return {};
    }

    KTH_DB_val value;
    int rc;
    if ((rc = kth_db_cursor_get(cursor, &key, &value, MDB_SET)) == 0) {
           
        if (kth_db_cursor_del(cursor, 0) != KTH_DB_SUCCESS) {
            kth_db_cursor_close(cursor);
            return result_code::other;
        }
        
        while ((rc = kth_db_cursor_get(cursor, &key, &value, MDB_NEXT_DUP)) == 0) {
            if (kth_db_cursor_del(cursor, 0) != KTH_DB_SUCCESS) {
                kth_db_cursor_close(cursor);
                return result_code::other;
            }    
        }
    } 
    
    kth_db_cursor_close(cursor);

#endif

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)
#endif //defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)


} // namespace kth::database

#endif // KTH_DATABASE_BLOCK_DATABASE_IPP_

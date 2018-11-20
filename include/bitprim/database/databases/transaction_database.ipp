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
#ifndef BITPRIM_DATABASE_TRANSACTION_DATABASE_HPP_
#define BITPRIM_DATABASE_TRANSACTION_DATABASE_HPP_

namespace libbitcoin {
namespace database {


#if defined(BITPRIM_DB_NEW_FULL)

//public
template <typename Clock>
transaction_entry internal_database_basis<Clock>::get_transaction(hash_digest const& hash, size_t fork_height, bool require_confirmed) const {
    
    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn);
    if (res != MDB_SUCCESS) {
        return transaction_entry{};
    }

    auto entry = get_transaction(hash, fork_height, require_confirmed, db_txn);

    if (mdb_txn_commit(db_txn) != MDB_SUCCESS) {
        return transaction_entry{};
    }

    return entry;

}

template <typename Clock>
template <typename I>
result_code internal_database_basis<Clock>::insert_transactions(I f, I l, uint32_t height, uint32_t median_time_past, MDB_txn* db_txn) {
    uint32_t pos = 0;
    while (f != l) {
        auto const& tx = *f;
        auto res = insert_transaction(tx, height, median_time_past, pos, db_txn);
        if (res != result_code::success && res != result_code::duplicated_key) {
            return res;
        }

        //remove unconfirmed transaction if exists
        //TODO (Mario): don't recalculate tx.hash
        res = remove_transaction_unconfirmed(tx.hash(), db_txn);
        if (res != result_code::success && res != result_code::key_not_found) {
            return res;
        } 

        ++f;
        ++pos;
    }

    return result_code::success;
}

template <typename Clock>
transaction_entry internal_database_basis<Clock>::get_transaction(hash_digest const& hash, size_t fork_height, bool require_confirmed, MDB_txn* db_txn) const {
    MDB_val key {hash.size(), const_cast<hash_digest&>(hash).data()};
    MDB_val value;

    auto res = mdb_get(db_txn, dbi_transaction_db_, &key, &value);
    
    if (res != MDB_SUCCESS) {
        return {};
    }

    auto data = db_value_to_data_chunk(value);
    auto entry = transaction_entry::factory_from_data(data);

    if ( !entry.is_valid() ) {
        return entry;
    } 

    if (! require_confirmed) {
        return entry;
    }

    auto const confirmed = entry.confirmed();
    
    return (confirmed && entry.height() > fork_height) || (require_confirmed && ! confirmed) ? transaction_entry{} : entry;
}

template <typename Clock>
result_code internal_database_basis<Clock>::insert_transaction(chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position, MDB_txn* db_txn) {
    auto key_arr = tx.hash();                                    //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key {key_arr.size(), key_arr.data()};

    auto valuearr = transaction_entry::factory_to_data(tx, height, median_time_past, position);
    MDB_val value {valuearr.size(), valuearr.data()}; 

    auto res = mdb_put(db_txn, dbi_transaction_db_, &key, &value, MDB_NOOVERWRITE);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key in Transaction DB [insert_transaction] " << res;
        return result_code::duplicated_key;
    }        

    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error saving in Transaction DB [insert_transaction] " << res;
        return result_code::other;
    }

    return result_code::success;
}


template <typename Clock>
result_code internal_database_basis<Clock>::remove_transactions(uint32_t height, MDB_txn* db_txn) {
    MDB_val key {sizeof(height), &height};
    MDB_val value;


    //To get the tx hashes to remove, we need to read the block db

    if (mdb_get(db_txn, dbi_block_db_, &key, &value) != MDB_SUCCESS) {
        return result_code::other;
    }

    auto n = value.mv_size;
    auto f = static_cast<uint8_t*>(value.mv_data); 
    //precondition: mv_size es multiplo de 32
    
    uint32_t pos = 0;
    while (n != 0) {
        hash_digest h;
        std::copy(f, f + libbitcoin::hash_size, h.data());
        
        MDB_val key_tx {h.size(), h.data()};
        
        auto const& entry = get_transaction(h, height, true, db_txn);
        if ( ! entry.is_valid() ) {
            return result_code::other;
        }

        auto const& tx = entry.transaction();
        
        auto res0 = remove_transaction_history_db(tx, height, db_txn);
        if (res0 != result_code::success) {
            return res0;
        }
        
        if (pos > 0) {
            res0 = remove_transaction_spend_db(tx, db_txn);
            if (res0 != result_code::success && res0 != result_code::key_not_found) {
                return res0;
            }
        }
        
        auto res = mdb_del(db_txn, dbi_transaction_db_, &key_tx, NULL);
        if (res == MDB_NOTFOUND) {
            LOG_INFO(LOG_DATABASE) << "Key not found deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::key_not_found;
        }
        if (res != MDB_SUCCESS) {
            LOG_INFO(LOG_DATABASE) << "Error deleting transaction DB in LMDB [remove_transactions] - mdb_del: " << res;
            return result_code::other;
        }
    
        n -= libbitcoin::hash_size;
        f += libbitcoin::hash_size;
        ++pos;
    }

    return result_code::success;
}


template <typename Clock>
result_code internal_database_basis<Clock>::update_transaction(chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position, MDB_txn* db_txn) {
    auto key_arr = tx.hash();                                    //TODO(fernando): podría estar afuera de la DBTx
    MDB_val key {key_arr.size(), key_arr.data()};

    auto valuearr = transaction_entry::factory_to_data(tx, height, median_time_past, position);
    MDB_val value {valuearr.size(), valuearr.data()}; 

    auto res = mdb_put(db_txn, dbi_transaction_db_, &key, &value, 0);
    if (res == MDB_KEYEXIST) {
        LOG_INFO(LOG_DATABASE) << "Duplicate key in Transaction DB [insert_transaction] " << res;
        return result_code::duplicated_key;
    }        

    if (res != MDB_SUCCESS) {
        LOG_INFO(LOG_DATABASE) << "Error saving in Transaction DB [insert_transaction] " << res;
        return result_code::other;
    }

    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::set_spend(chain::output_point const& point, uint32_t spender_height, MDB_txn* db_txn) {

    // Limit search to confirmed transactions at or below the spender height,
    // since a spender cannot spend above its own height.
    // Transactions are not marked as spent unless the spender is confirmed.
    // This is consistent with support for unconfirmed double spends.

    auto entry = get_transaction(point.hash(), spender_height, true, db_txn);
    
    // The transaction is not exist as confirmed at or below the height.
    if ( ! entry.is_valid() ) {
        return result_code::other;
    }

    auto const& tx = entry.transaction();
    
    if (point.index() >= tx.outputs().size()) {
        return result_code::other;
    }

    //update spender_height
    auto& output = tx.outputs()[point.index()];
    output.validation.spender_height = spender_height;

    //overwrite transaction
    auto ret = update_transaction(tx, entry.height(), entry.median_time_past(), entry.position(), db_txn);
    if (ret != result_code::success & ret != result_code::duplicated_key) {
        return ret;
    }
    
    return result_code::success;
}

template <typename Clock>
result_code internal_database_basis<Clock>::set_unspend(chain::output_point const& point, MDB_txn* db_txn) {
    return set_spend(point, chain::output::validation::not_spent, db_txn);
}


#endif //BITPRIM_NEW_DB_FULL


} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_TRANSACTION_DATABASE_HPP_

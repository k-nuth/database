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
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

#include <bitprim/database/databases/utxo_entry.hpp>

#include <lmdb.h>

namespace libbitcoin {
namespace database {
           
enum class utxo_code {
    success = 0,
    success_duplicate_coinbase = 1,
    duplicated_key = 2,
    key_not_found = 3,
    db_empty = 4,
    other = 5
};

class BCD_API utxo_database {
public:
    using path = boost::filesystem::path;

    constexpr static char block_header_db_name[] = "block_header";
    constexpr static char block_header_by_hash_db_name[] = "block_header_by_hash";

    constexpr static char utxo_db_name[] = "utxo_db";
    constexpr static char reorg_pool_name[] = "reorg_pool";
    constexpr static char reorg_index_name[] = "reorg_index";

    /// Construct the database.
    utxo_database(path const& db_dir);

    /// Close the database.
    ~utxo_database();

    /// 
    static
    bool succeed(utxo_code code);

    /// Initialize a new transaction database.
    bool create();

    /// Call before using the database.
    bool open();

    /// Call to unload the memory map.
    bool close();

    /// TODO comment
    utxo_code push_genesis(chain::block const& block);

    /// Remove all the previous outputs and insert all the new outputs atomically.
    utxo_code push_block(chain::block const& block, size_t height, uint32_t median_time_past);

    // /// TODO comment
    // utxo_code remove_block(chain::block const& block);

    /// TODO comment
    utxo_entry get(chain::output_point const& key) const;

    /// TODO comment
    utxo_code get_last_height(size_t& out_height) const;


private:
    bool create_and_open_environment();
    bool open_databases();


    utxo_code insert_reorg_pool(uint32_t height, MDB_val& key, MDB_txn* db_txn);
    utxo_code remove(uint32_t height, chain::output_point const& point, MDB_txn* db_txn);
    utxo_code insert(chain::output_point const& point, chain::output const& output, data_chunk const& fixed_data, MDB_txn* db_txn);


    utxo_code remove_inputs(uint32_t height, chain::input::list const& inputs, MDB_txn* db_txn);
    utxo_code insert_outputs(hash_digest const& txid, chain::output::list const& outputs, data_chunk const& fixed_data, MDB_txn* db_txn);


    utxo_code insert_outputs_error_treatment(size_t height, data_chunk const& fixed_data, hash_digest const& txid, chain::output::list const& outputs, MDB_txn* db_txn);
    utxo_code push_transaction_non_coinbase(size_t height, data_chunk const& fixed_data, chain::transaction const& tx, MDB_txn* db_txn);

    template <typename I>
    utxo_code push_transactions_non_coinbase(size_t height, data_chunk const& fixed_data, I f, I l, MDB_txn* db_txn);


    utxo_code push_block_header(chain::block const& block, size_t height, MDB_txn* db_txn);


    utxo_code push_block(chain::block const& block, size_t height, uint32_t median_time_past, MDB_txn* db_txn);
    
    utxo_code push_genesis(chain::block const& block, MDB_txn* db_txn);


    // utxo_code remove_transaction(chain::transaction const& tx, MDB_txn* db_txn);
    // utxo_code remove_transaction_non_coinbase(chain::transaction const& tx, MDB_txn* db_txn);
    // template <typename I>
    // utxo_code remove_transactions_non_coinbase(I f, I l, MDB_txn* db_txn);
    // utxo_code remove_block(chain::block const& block, MDB_txn* db_txn);


    path db_dir_;
    bool env_created_ = false;
    bool db_created_ = false;

    MDB_env* env_;
    
    MDB_dbi dbi_block_header_;
    MDB_dbi dbi_block_header_by_hash_;

    MDB_dbi dbi_utxo_;
    MDB_dbi dbi_reorg_pool_;
    MDB_dbi dbi_reorg_index_;
};

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

#endif // BITPRIM_DATABASE_UTXO_DATABASE_HPP_

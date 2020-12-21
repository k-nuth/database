// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_REORG_DATABASE_HPP_
#define KTH_DATABASE_REORG_DATABASE_HPP_

#include <kth/database/databases/generic_db.hpp>
// #include <kth/database/databases/lmdb_helper.hpp>
#include <kth/database/databases/result_code.hpp>

#include <kth/domain/chain/block.hpp>
#include <kth/domain/chain/output_point.hpp>


namespace kth::database {

#if ! defined(KTH_DB_READONLY)

result_code insert_reorg_pool(uint32_t height, KTH_DB_val& key, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_utxo, KTH_DB_dbi dbi_reorg_pool, KTH_DB_dbi dbi_reorg_index);

//TODO : remove this database in db_new_with_blocks and db_new_full
result_code push_block_reorg(domain::chain::block const& block, uint32_t height, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_block);

result_code insert_output_from_reorg_and_remove(domain::chain::output_point const& point, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_utxo, KTH_DB_dbi dbi_reorg_pool);

result_code remove_block_reorg(uint32_t height, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_block);

result_code remove_reorg_index(uint32_t height, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_index);

result_code prune_reorg_index(uint32_t remove_until, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_pool, KTH_DB_dbi dbi_reorg_index);

result_code prune_reorg_block(uint32_t amount_to_delete, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_block);

#endif // ! defined(KTH_DB_READONLY)

domain::chain::block get_block_reorg(uint32_t height, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_reorg_block) /*const*/;

domain::chain::block get_block_reorg(uint32_t height, KTH_DB_dbi dbi_reorg_block, KTH_DB_env* env) /*const*/;

result_code get_first_reorg_block_height(uint32_t& out_height, KTH_DB_dbi dbi_reorg_block, KTH_DB_env* env); /*const*/;

} // namespace kth::database

#endif // KTH_DATABASE_REORG_DATABASE_HPP_

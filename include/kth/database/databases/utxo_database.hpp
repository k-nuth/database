// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_UTXO_DATABASE_HPP_
#define KTH_DATABASE_UTXO_DATABASE_HPP_

#include <kth/database/databases/generic_db.hpp>
// #include <kth/database/databases/lmdb_helper.hpp>
#include <kth/database/databases/result_code.hpp>

#include <kth/domain/chain/output_point.hpp>


namespace kth::database {

#if ! defined(KTH_DB_READONLY)

result_code remove_utxo(uint32_t height, domain::chain::output_point const& point, bool insert_reorg, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_utxo, KTH_DB_dbi dbi_reorg_pool, KTH_DB_dbi dbi_reorg_index);

result_code insert_utxo(domain::chain::output_point const& point, domain::chain::output const& output, data_chunk const& fixed_data, KTH_DB_txn* db_txn, KTH_DB_dbi dbi_utxo);

#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_UTXO_DATABASE_HPP_

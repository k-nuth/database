// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_LMDB_HELPER_HPP_
#define KTH_DATABASE_LMDB_HELPER_HPP_

#include <utility>

#include <lmdb.h>

namespace kth::database {

std::pair<int, MDB_txn*> transaction_begin(MDB_env* env, MDB_txn* parent, unsigned int flags) {
    MDB_txn* db_txn;
    auto res = mdb_txn_begin(env, NULL, 0, &db_txn);
    return {res, db_txn};
}

std::pair<int, MDB_txn*> transaction_readonly_begin(MDB_env* env, MDB_txn* parent, unsigned int flags) {
    return transaction_begin(env, parent, flags | MDB_RDONLY);
}


#if ! defined(KTH_DB_READONLY)
#endif


} // namespace kth::database

#endif // KTH_DATABASE_LMDB_HELPER_HPP_

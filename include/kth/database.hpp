// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_HPP
#define KTH_DATABASE_HPP

/**
 * API Users: Include only this header. Direct use of other headers is fragile
 * and unsupported as header organization is subject to change.
 *
 * Maintainers: Do not include this header internal to this library.
 */

#include <kth/domain.hpp>
#include <kth/database/data_base.hpp>
#include <kth/database/define.hpp>
#include <kth/database/settings.hpp>
#include <kth/database/store.hpp>

#ifdef KTH_DB_UNSPENT_LEGACY
#include <kth/database/unspent_outputs.hpp>
#include <kth/database/unspent_transaction.hpp>
#endif // KTH_DB_UNSPENT_LEGACY

#include <kth/database/version.hpp>

#ifdef KTH_DB_LEGACY
#include <kth/database/legacy/databases/block_database.hpp>
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_NEW
#include <kth/database/databases/internal_database.hpp>
#endif // KTH_DB_NEW

#ifdef KTH_DB_HISTORY
#include <kth/database/legacy/databases/history_database.hpp>
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_SPENDS
#include <kth/database/legacy/databases/spend_database.hpp>
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_STEALTH
#include <kth/database/legacy/databases/stealth_database.hpp>
#endif // KTH_DB_STEALTH

#ifdef KTH_DB_LEGACY
#include <kth/database/legacy/databases/transaction_database.hpp>
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
#include <kth/database/legacy/databases/transaction_unconfirmed_database.hpp>
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

#include <kth/database/legacy/memory/accessor.hpp>
#include <kth/database/legacy/memory/allocator.hpp>
#include <kth/database/legacy/memory/memory.hpp>
#include <kth/database/legacy/memory/memory_map.hpp>
#include <kth/database/legacy/primitives/hash_table_header.hpp>
#include <kth/database/legacy/primitives/record_hash_table.hpp>
#include <kth/database/legacy/primitives/record_list.hpp>
#include <kth/database/legacy/primitives/record_manager.hpp>
#include <kth/database/legacy/primitives/record_multimap.hpp>
#include <kth/database/legacy/primitives/record_multimap_iterable.hpp>
#include <kth/database/legacy/primitives/record_multimap_iterator.hpp>
#include <kth/database/legacy/primitives/slab_hash_table.hpp>
#include <kth/database/legacy/primitives/slab_manager.hpp>

#ifdef KTH_DB_LEGACY
#include <kth/database/legacy/result/block_result.hpp>
#include <kth/database/legacy/result/transaction_result.hpp>
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
#include <kth/database/legacy/result/transaction_unconfirmed_result.hpp>
#endif // KTH_DB_TRANSACTION_UNCONFIRMED


#endif

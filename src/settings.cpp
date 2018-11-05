/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
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
#include <bitcoin/database/settings.hpp>

#include <boost/filesystem.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;

settings::settings()
    : directory("blockchain")
    , flush_writes(false)
    , file_growth_rate(50)
    , index_start_height(0)


#ifdef BITPRIM_DB_NEW
    , reorg_pool_limit(100)                                  //TODO(fernando): look for a good default
#if defined(BITPRIM_DB_NEW_BLOCKS) || defined(BITPRIM_DB_NEW_FULL)
    , db_max_size(200 * (uint64_t(1) << 30))  //200 GiB     //TODO(fernando): look for a good default
#else                                                         
    , db_max_size(100 * (uint64_t(1) << 30))  //100 GiB     //TODO(fernando): look for a good default
#endif // BITPRIM_DB_NEW_BLOCKS
    
    
#endif // BITPRIM_DB_NEW


    // Hash table sizes (must be configured).
#ifdef BITPRIM_DB_LEGACY
    , block_table_buckets(0)
    , transaction_table_buckets(0)
#endif // BITPRIM_DB_LEGACY
#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
    , transaction_unconfirmed_table_buckets(0)
#endif // BITPRIM_DB_TRANSACTION_UNCONFIRMED    

#ifdef BITPRIM_DB_SPENDS
    , spend_table_buckets(0)
#endif // BITPRIM_DB_SPENDS

#ifdef BITPRIM_DB_HISTORY
    , history_table_buckets(0)
#endif // BITPRIM_DB_HISTORY    

// #ifdef BITPRIM_DB_UNSPENT_LIBBITCOIN
    , cache_capacity(0)
// #endif // BITPRIM_DB_UNSPENT_LIBBITCOIN
{}

settings::settings(config::settings context)
  : settings()
{
    switch (context) {
        case config::settings::mainnet: {
#ifdef BITPRIM_DB_LEGACY
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
#endif // BITPRIM_DB_LEGACY

#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
            transaction_unconfirmed_table_buckets = 10000;
#endif // BITPRIM_DB_TRANSACTION_UNCONFIRMED    

#ifdef BITPRIM_DB_SPENDS
            spend_table_buckets = 250000000;
#endif // BITPRIM_DB_SPENDS

#ifdef BITPRIM_DB_HISTORY
            history_table_buckets = 107000000;
#endif // BITPRIM_DB_HISTORY    
            break;
        }
        case config::settings::testnet: {
            // TODO: optimize for testnet.
#ifdef BITPRIM_DB_LEGACY
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
#endif // BITPRIM_DB_LEGACY

#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
            transaction_unconfirmed_table_buckets = 10000;
#endif // BITPRIM_DB_TRANSACTION_UNCONFIRMED    

#ifdef BITPRIM_DB_SPENDS
            spend_table_buckets = 250000000;
#endif // BITPRIM_DB_SPENDS

#ifdef BITPRIM_DB_HISTORY
            history_table_buckets = 107000000;
#endif // BITPRIM_DB_HISTORY    

            break;
        }
        default:
        case config::settings::none: {}
    }
}

} // namespace database
} // namespace libbitcoin

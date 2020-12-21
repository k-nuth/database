// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_DB_PARAMETERS_HPP_
#define KTH_DATABASE_DB_PARAMETERS_HPP_

#ifdef KTH_INTERNAL_DB_4BYTES_INDEX
#define KTH_INTERNAL_DB_WIRE true
#else
#define KTH_INTERNAL_DB_WIRE false
#endif

#if defined(KTH_DB_READONLY)
#define KTH_DB_CONDITIONAL_CREATE 0
#else
#define KTH_DB_CONDITIONAL_CREATE KTH_DB_CREATE
#endif

#if defined(KTH_DB_READONLY)
#define KTH_DB_CONDITIONAL_READONLY KTH_DB_RDONLY
#else
#define KTH_DB_CONDITIONAL_READONLY 0
#endif

namespace kth::database {

#if defined(KTH_DB_NEW_FULL)
constexpr size_t max_dbs_ = 13;
#elif defined(KTH_DB_NEW_BLOCKS)
constexpr size_t max_dbs_ = 8;
#else
constexpr size_t max_dbs_ = 7;
#endif

// constexpr size_t env_open_mode_ = 0664;
constexpr size_t env_open_mode_ = 0644;
constexpr int directory_exists = 0;

} // namespace kth::database

#endif // KTH_DATABASE_DB_PARAMETERS_HPP_

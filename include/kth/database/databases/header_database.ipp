// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_HEADER_DATABASE_IPP_
#define KTH_DATABASE_HEADER_DATABASE_IPP_

#include <kth/infrastructure/log/source.hpp>
#include <kth/database/databases/header_abla_entry.hpp>

namespace kth::database {

#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::push_block_header(const domain::chain::block& block, uint32_t height, leveldb::WriteBatch& batch) {
    // Serialización del bloque
    const auto valuearr = to_data_with_abla_state(block);

    // Clave por altura
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));
    leveldb::Slice value(reinterpret_cast<const char*>(valuearr.data()), valuearr.size());

    // Inserta bloque por altura
    batch.Put(cf_block_header_, key, value);

    // Clave por hash
    const auto& hash = block.hash();
    leveldb::Slice hash_key(reinterpret_cast<const char*>(hash.data()), hash.size());

    // Guarda mapping hash -> altura
    batch.Put(cf_block_header_by_hash_, hash_key, key);

    return result_code::success;
}

#endif // ! defined(KTH_DB_READONLY)

template <typename Clock>
domain::chain::header internal_database_basis<Clock>::get_header(uint32_t height) const {
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));
    std::string value;

    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_block_header_, key, &value);
    if (!status.ok()) {
        return {};
    }

    byte_reader reader(reinterpret_cast<const uint8_t*>(value.data()), value.size());
    auto opt = get_header_and_abla_state_from_data(reader);
    if (!opt) {
        return {};
    }

    return std::get<0>(*opt); // solo el header, ignoramos abla_state
}

template <typename Clock>
std::optional<header_with_abla_state_t> internal_database_basis<Clock>::get_header_and_abla_state(uint32_t height) const {
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));
    std::string value;

    leveldb::Status status = db_->Get(leveldb::ReadOptions(), cf_block_header_, key, &value);
    if (!status.ok()) {
        return std::nullopt;
    }

    byte_reader reader(reinterpret_cast<const uint8_t*>(value.data()), value.size());
    auto opt = get_header_and_abla_state_from_data(reader);
    if (!opt) {
        return std::nullopt;
    }

    return opt;
}


#if ! defined(KTH_DB_READONLY)

template <typename Clock>
result_code internal_database_basis<Clock>::remove_block_header(const hash_digest& hash, uint32_t height, leveldb::WriteBatch& batch) {
    leveldb::Slice key(reinterpret_cast<const char*>(&height), sizeof(height));
    leveldb::Slice key_hash(reinterpret_cast<const char*>(hash.data()), hash.size());

    batch.Delete(cf_block_header_, key);
    batch.Delete(cf_block_header_by_hash_, key_hash);

    return result_code::success;
}


#endif // ! defined(KTH_DB_READONLY)

} // namespace kth::database

#endif // KTH_DATABASE_HEADER_DATABASE_IPP_

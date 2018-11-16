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
// #ifdef BITPRIM_DB_NEW

#include <bitprim/database/databases/transaction_unconfirmed_entry.hpp>

#include <cstddef>
#include <cstdint>

namespace libbitcoin { 
namespace database {

transaction_unconfirmed_entry::transaction_unconfirmed_entry(chain::transaction const& tx, uint32_t arrival_time)
    : transaction_(tx), arrival_time_(arrival_time)
{}

chain::transaction const& transaction_unconfirmed_entry::transaction() const {
    return transaction_;
}

uint32_t transaction_unconfirmed_entry::arrival_time() const {
    return arrival_time_;
}

// private
void transaction_unconfirmed_entry::reset() {
    transaction_ = chain::transaction{};
    arrival_time_ = max_uint32;
}

// Empty scripts are valid, validation relies on not_found only.
bool transaction_unconfirmed_entry::is_valid() const {
    return transaction_.is_valid() && arrival_time_ != bc::max_uint32;
}

// Size.
//-----------------------------------------------------------------------------
// constexpr
//TODO(fernando): make this constexpr 
size_t transaction_unconfirmed_entry::serialized_size(chain::transaction const& tx) {
#if ! defined(BITPRIM_USE_DOMAIN) || defined(BITPRIM_CACHED_RPC_DATA)
    return tx.serialized_size(false, true, true) 
#else
    return tx.serialized_size(false, true) 
#endif
         + sizeof(uint32_t);
}


// Serialization.
//-----------------------------------------------------------------------------

// static
data_chunk transaction_unconfirmed_entry::factory_to_data(chain::transaction const& tx, uint32_t arrival_time) {
    data_chunk data;
    auto const size = serialized_size(tx);
    data.reserve(size);
    data_sink ostream(data);
    factory_to_data(ostream, tx, arrival_time);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

// static
void transaction_unconfirmed_entry::factory_to_data(std::ostream& stream, chain::transaction const& tx, uint32_t arrival_time) {
    ostream_writer sink(stream);
    factory_to_data(sink, tx, arrival_time);
}


#if ! defined(BITPRIM_USE_DOMAIN)
// static
void transaction_unconfirmed_entry::factory_to_data(writer& sink, chain::transaction const& tx, uint32_t arrival_time) {
    tx.to_data(sink, false,true,true);
    sink.write_4_bytes_little_endian(arrival_time);
}
#endif


// Serialization.
//-----------------------------------------------------------------------------

data_chunk transaction_unconfirmed_entry::to_data() const {
    data_chunk data;
    auto const size = serialized_size(transaction_);
    data.reserve(size);
    data_sink ostream(data);
    to_data(ostream);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

void transaction_unconfirmed_entry::to_data(std::ostream& stream) const {
    ostream_writer sink(stream);
    to_data(sink);
}

#ifndef BITPRIM_USE_DOMAIN
void transaction_unconfirmed_entry::to_data(writer& sink) const {
    factory_to_data(sink, transaction_, arrival_time_);
}
#endif

// Deserialization.
//-----------------------------------------------------------------------------

transaction_unconfirmed_entry transaction_unconfirmed_entry::factory_from_data(data_chunk const& data) {
    transaction_unconfirmed_entry instance;
    instance.from_data(data);
    return instance;
}

transaction_unconfirmed_entry transaction_unconfirmed_entry::factory_from_data(std::istream& stream) {
    transaction_unconfirmed_entry instance;
    instance.from_data(stream);
    return instance;
}

#ifndef BITPRIM_USE_DOMAIN
transaction_unconfirmed_entry transaction_unconfirmed_entry::factory_from_data(reader& source) {
    transaction_unconfirmed_entry instance;
    instance.from_data(source);
    return instance;
}
#endif

bool transaction_unconfirmed_entry::from_data(const data_chunk& data) {
    data_source istream(data);
    return from_data(istream);
}

bool transaction_unconfirmed_entry::from_data(std::istream& stream) {
    istream_reader source(stream);
    return from_data(source);
}

#ifndef BITPRIM_USE_DOMAIN
bool transaction_unconfirmed_entry::from_data(reader& source) {
    reset();
    

    transaction_.from_data(source, false, true, false);
    arrival_time_ = source.read_4_bytes_little_endian();
    
    if ( ! source) {
        reset();
    }

    return source;
}
#endif


} // namespace database
} // namespace libbitcoin


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

#include <bitprim/database/databases/utxo_entry.hpp>

#include <cstddef>
#include <cstdint>
// #include <bitcoin/bitcoin.hpp>

namespace libbitcoin { 
namespace database {

utxo_entry::utxo_entry(chain::output output, uint32_t height, uint32_t median_time_past, bool coinbase)
    : output_(std::move(output)), height_(height), median_time_past_(median_time_past), coinbase_(coinbase)
{}

chain::output const& utxo_entry::output() const {
    return output_;
}

uint32_t utxo_entry::height() const {
    return height_;
}

uint32_t utxo_entry::median_time_past() const {
    return median_time_past_;
}

bool utxo_entry::coinbase() const {
    return coinbase_;
}

// private
void utxo_entry::reset() {
    output_ = chain::output{};
    height_ = max_uint32;
    median_time_past_ = max_uint32;
    coinbase_ = false;
}

// Empty scripts are valid, validation relies on not_found only.
bool utxo_entry::is_valid() const {
    return output_.is_valid() && height_ != bc::max_uint32 && median_time_past_ != max_uint32;
}


// Size.
//-----------------------------------------------------------------------------

// private static
constexpr
size_t utxo_entry::serialized_size_fixed() {
    return sizeof(uint32_t) + sizeof(uint32_t) + sizeof(bool);
}

size_t utxo_entry::serialized_size() const {
    return output_.serialized_size(false) + serialized_size_fixed();
}

// Serialization.
//-----------------------------------------------------------------------------

// static
data_chunk utxo_entry::to_data_fixed(uint32_t height, uint32_t median_time_past, bool coinbase) {
    data_chunk data;
    auto const size = serialized_size_fixed();
    data.reserve(size);
    data_sink ostream(data);
    to_data_fixed(ostream, height, median_time_past, coinbase);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

// static
void utxo_entry::to_data_fixed(std::ostream& stream, uint32_t height, uint32_t median_time_past, bool coinbase) {
    ostream_writer sink(stream);
    to_data_fixed(sink, height, median_time_past, coinbase);
}

#ifndef BITPRIM_USE_DOMAIN
// static
void utxo_entry::to_data_fixed(writer& sink, uint32_t height, uint32_t median_time_past, bool coinbase) {
    sink.write_4_bytes_little_endian(height);
    sink.write_4_bytes_little_endian(median_time_past);
    sink.write_byte(coinbase);
}
#endif

// static
data_chunk utxo_entry::to_data_with_fixed(chain::output const& output, data_chunk const& fixed) {
    //TODO(fernando):  reuse fixed vector (do not create a new one)
    data_chunk data;
    auto const size = output.serialized_size(false) + fixed.size();
    data.reserve(size);
    data_sink ostream(data);
    to_data_with_fixed(ostream, output, fixed);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

// static
void utxo_entry::to_data_with_fixed(std::ostream& stream, chain::output const& output, data_chunk const& fixed) {
    ostream_writer sink(stream);
    to_data_with_fixed(sink, output, fixed);
}

#ifndef BITPRIM_USE_DOMAIN
// static
void utxo_entry::to_data_with_fixed(writer& sink, chain::output const& output, data_chunk const& fixed) {
    output.to_data(sink, false);
    sink.write_bytes(fixed);
}
#endif


// Serialization.
//-----------------------------------------------------------------------------

data_chunk utxo_entry::to_data() const {
    data_chunk data;
    auto const size = serialized_size();
    data.reserve(size);
    data_sink ostream(data);
    to_data(ostream);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

void utxo_entry::to_data(std::ostream& stream) const {
    ostream_writer sink(stream);
    to_data(sink);
}

#ifndef BITPRIM_USE_DOMAIN
void utxo_entry::to_data(writer& sink) const {
    output_.to_data(sink, false);
    to_data_fixed(sink, height_, median_time_past_, coinbase_);
}
#endif

// Deserialization.
//-----------------------------------------------------------------------------

utxo_entry utxo_entry::factory_from_data(data_chunk const& data) {
    utxo_entry instance;
    instance.from_data(data);
    return instance;
}

utxo_entry utxo_entry::factory_from_data(std::istream& stream) {
    utxo_entry instance;
    instance.from_data(stream);
    return instance;
}

#ifndef BITPRIM_USE_DOMAIN
utxo_entry utxo_entry::factory_from_data(reader& source) {
    utxo_entry instance;
    instance.from_data(source);
    return instance;
}
#endif

bool utxo_entry::from_data(const data_chunk& data) {
    data_source istream(data);
    return from_data(istream);
}

bool utxo_entry::from_data(std::istream& stream) {
    istream_reader source(stream);
    return from_data(source);
}

#ifndef BITPRIM_USE_DOMAIN
bool utxo_entry::from_data(reader& source) {
    reset();
    
//     output_.from_data(source, false);
//     height_ = source.read_4_bytes_little_endian();
//     median_time_past_ = source.read_4_bytes_little_endian();
//     coinbase_ = source.read_byte();
    

//     if ( ! source) {
//         reset();
//     }

    return source;
}
#endif

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

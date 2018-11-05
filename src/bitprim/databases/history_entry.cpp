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

#include <bitprim/database/databases/history_entry.hpp>

#include <cstddef>
#include <cstdint>
// #include <bitcoin/bitcoin.hpp>

namespace libbitcoin { 
namespace database {

history_entry::history_entry(libbitcoin::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum)
    : point_kind_(kind), height_(height), index_(index), value_or_checksum_(value_or_checksum)
{}

chain::point_kind history_entry::point_kind() const {
    return point_kind_;
}

uint32_t history_entry::height() const {
    return height_;
}

uint32_t history_entry::index() const {
    return index_;
}

uint64_t history_entry::value_or_checksum() const {
    return value_or_checksum_;
}


// private
void history_entry::reset() {
    point_kind_ = libbitcoin::chain::point_kind::output;
    height_ = max_uint32;
    index_ = max_uint32;
    value_or_checksum_ = max_uint64;
}

// Empty scripts are valid, validation relies on not_found only.
bool history_entry::is_valid() const {
    return height_ != bc::max_uint32 && index_ != max_uint32 && value_or_checksum_ != max_uint64;
}


// Size.
//-----------------------------------------------------------------------------
constexpr
size_t history_entry::serialized_size() {
    return sizeof(chain::point_kind) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t);
}

// Serialization.
//-----------------------------------------------------------------------------

// static
data_chunk history_entry::factory_to_data(chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
    data_chunk data;
    auto const size = serialized_size();
    data.reserve(size);
    data_sink ostream(data);
    factory_to_data(ostream, kind, height, index, value_or_checksum);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

// static
void history_entry::factory_to_data(std::ostream& stream, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
    ostream_writer sink(stream);
    factory_to_data(sink, kind, height, index, value_or_checksum);
}

// static
void history_entry::factory_to_data(writer& sink, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
    
    sink.write_byte(static_cast<uint8_t>(kind));
    sink.write_4_bytes_little_endian(height);
    sink.write_4_bytes_little_endian(index);
    sink.write_8_bytes_little_endian(value_or_checksum);
}


// Serialization.
//-----------------------------------------------------------------------------

data_chunk history_entry::to_data() const {
    data_chunk data;
    auto const size = serialized_size();
    data.reserve(size);
    data_sink ostream(data);
    to_data(ostream);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

void history_entry::to_data(std::ostream& stream) const {
    ostream_writer sink(stream);
    to_data(sink);
}

void history_entry::to_data(writer& sink) const {
    //output_.to_data(sink, false);
    factory_to_data(sink, point_kind_, height_, index_, value_or_checksum_ );
}

// Deserialization.
//-----------------------------------------------------------------------------

history_entry history_entry::factory_from_data(data_chunk const& data) {
    history_entry instance;
    instance.from_data(data);
    return instance;
}

history_entry history_entry::factory_from_data(std::istream& stream) {
    history_entry instance;
    instance.from_data(stream);
    return instance;
}

history_entry history_entry::factory_from_data(reader& source) {
    history_entry instance;
    instance.from_data(source);
    return instance;
}

bool history_entry::from_data(const data_chunk& data) {
    data_source istream(data);
    return from_data(istream);
}

bool history_entry::from_data(std::istream& stream) {
    istream_reader source(stream);
    return from_data(source);
}

bool history_entry::from_data(reader& source) {
    reset();
    
    //output_.from_data(source, false);
    
    point_kind_ = static_cast<chain::point_kind>(source.read_byte()),
    height_ = source.read_4_bytes_little_endian();
    index_ = source.read_4_bytes_little_endian();
    value_or_checksum_ = source.read_8_bytes_little_endian();
    
    if ( ! source) {
        reset();
    }

    return source;
}

} // namespace database
} // namespace libbitcoin


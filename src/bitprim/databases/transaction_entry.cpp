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

#include <bitprim/database/databases/transaction_entry.hpp>

#include <cstddef>
#include <cstdint>

namespace libbitcoin { 
namespace database {


#ifdef BITPRIM_CURRENCY_BCH
#define BITPRIM_WITNESS_DEFAULT false
#define BITPRIM_POSITION_WRITER write_4_bytes_little_endian
#define BITPRIM_POSITION_READER read_4_bytes_little_endian
static constexpr auto position_size = sizeof(uint32_t);
const size_t position_max = max_uint32;
#else
#define BITPRIM_WITNESS_DEFAULT true
#define BITPRIM_POSITION_WRITER write_2_bytes_little_endian
#define BITPRIM_POSITION_READER read_2_bytes_little_endian
static constexpr auto position_size = sizeof(uint16_t);
const size_t position_max = max_uint16;
#endif

void write_position(writer& serial, uint32_t position) {
    serial.BITPRIM_POSITION_WRITER(position);
}

template <typename Deserializer>
uint32_t read_position(Deserializer& deserial) {
    return deserial.BITPRIM_POSITION_READER();
}

transaction_entry::transaction_entry(chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position)
    : transaction_(tx), height_(height), median_time_past_(median_time_past), position_(position)
{}

chain::transaction const& transaction_entry::transaction() const {
    return transaction_;
}

uint32_t transaction_entry::height() const {
    return height_;
}

uint32_t transaction_entry::median_time_past() const {
    return median_time_past_;
}

uint32_t transaction_entry::position() const {
    return position_;
}

// private
void transaction_entry::reset() {
    transaction_ = chain::transaction{};
    height_ = max_uint32;
    median_time_past_ = max_uint32;
    position_ = position_max;
}

// Empty scripts are valid, validation relies on not_found only.
bool transaction_entry::is_valid() const {
    return transaction_.is_valid() && height_ != bc::max_uint32 && median_time_past_ != max_uint32 && position_ != position_max;
}

// Size.
//-----------------------------------------------------------------------------
// constexpr
//TODO(fernando): make this constexpr 
size_t transaction_entry::serialized_size(chain::transaction const& tx) {
    return tx.serialized_size(false,true,false) + sizeof(uint32_t) + sizeof(uint32_t) + position_size;
}

// Serialization.
//-----------------------------------------------------------------------------

// static
data_chunk transaction_entry::factory_to_data(chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position) {
    data_chunk data;
    auto const size = serialized_size(tx);
    data.reserve(size);
    data_sink ostream(data);
    factory_to_data(ostream, tx, height, median_time_past, position);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

// static
void transaction_entry::factory_to_data(std::ostream& stream, chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position) {
    ostream_writer sink(stream);
    factory_to_data(sink, tx, height, median_time_past, position);
}



// static
void transaction_entry::factory_to_data(writer& sink, chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position) {
    tx.to_data(sink, false,true,false);
    sink.write_4_bytes_little_endian(height);
    sink.write_4_bytes_little_endian(median_time_past);
    write_position(sink, position);
    //sink.write_4_bytes_little_endian(position);
}


// Serialization.
//-----------------------------------------------------------------------------

data_chunk transaction_entry::to_data() const {
    data_chunk data;
    auto const size = serialized_size(transaction_);
    data.reserve(size);
    data_sink ostream(data);
    to_data(ostream);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

void transaction_entry::to_data(std::ostream& stream) const {
    ostream_writer sink(stream);
    to_data(sink);
}

#ifndef BITPRIM_USE_DOMAIN
void transaction_entry::to_data(writer& sink) const {
    factory_to_data(sink, transaction_, height_, median_time_past_, position_ );
}
#endif

// Deserialization.
//-----------------------------------------------------------------------------

transaction_entry transaction_entry::factory_from_data(data_chunk const& data) {
    transaction_entry instance;
    instance.from_data(data);
    return instance;
}

transaction_entry transaction_entry::factory_from_data(std::istream& stream) {
    transaction_entry instance;
    instance.from_data(stream);
    return instance;
}

#ifndef BITPRIM_USE_DOMAIN
transaction_entry transaction_entry::factory_from_data(reader& source) {
    transaction_entry instance;
    instance.from_data(source);
    return instance;
}
#endif

bool transaction_entry::from_data(const data_chunk& data) {
    data_source istream(data);
    return from_data(istream);
}

bool transaction_entry::from_data(std::istream& stream) {
    istream_reader source(stream);
    return from_data(source);
}

#ifndef BITPRIM_USE_DOMAIN
bool transaction_entry::from_data(reader& source) {
    reset();
    
    transaction_.from_data(source,false,true,false);
    height_ = source.read_4_bytes_little_endian();
    median_time_past_ = source.read_4_bytes_little_endian();
    //position_ = source.read_4_bytes_little_endian();
    position_ = read_position(source);

    if ( ! source) {
        reset();
    }

    return source;
}
#endif

bool transaction_entry::confirmed() {
    return position_ != position_max;
}

 bool transaction_entry::is_spent(size_t fork_height) const {

     // Cannot be spent if unconfirmed.
    if (position_ == position_max)
        return false;

    for (auto const& output : transaction_.outputs()) {
        const auto spender_height =  output.validation.spender_height;

        // A spend from above the fork height is not an actual spend.
        if (spender_height == chain::output::validation::not_spent || spender_height > fork_height)
            return false;
    }
    return true;
 }


} // namespace database
} // namespace libbitcoin


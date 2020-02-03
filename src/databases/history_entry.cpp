// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// #ifdef KTH_DB_NEW

#include <kth/database/databases/history_entry.hpp>

#include <cstddef>
#include <cstdint>
// #include <kth/domain.hpp>

namespace kth { 
namespace database {

history_entry::history_entry(uint64_t id, chain::point const& point, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum)
    : id_(id), point_(point), point_kind_(kind), height_(height), index_(index), value_or_checksum_(value_or_checksum)
{}


uint64_t history_entry::id() const {
    return id_;
}

chain::point const& history_entry::point() const {
    return point_;
}

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
    id_ = max_uint64;
    point_ = chain::point{};
    point_kind_ = kth::chain::point_kind::output;
    height_ = max_uint32;
    index_ = max_uint32;
    value_or_checksum_ = max_uint64;
}

// Empty scripts are valid, validation relies on not_found only.
bool history_entry::is_valid() const {
    return id_ != max_uint64 && point_.is_valid() && height_ != bc::max_uint32 && index_ != max_uint32 && value_or_checksum_ != max_uint64;
}

// Size.
//-----------------------------------------------------------------------------
// constexpr
//TODO(fernando): make chain::point::serialized_size() static and constexpr to make this constexpr too
size_t history_entry::serialized_size(chain::point const& point) {
    return sizeof(uint64_t) + point.serialized_size(false) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t);
}

// Serialization.
//-----------------------------------------------------------------------------

// static
data_chunk history_entry::factory_to_data(uint64_t id, chain::point const& point, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
    data_chunk data;
    auto const size = serialized_size(point);
    data.reserve(size);
    data_sink ostream(data);
    factory_to_data(ostream, id, point, kind, height, index, value_or_checksum);
    ostream.flush();
    BITCOIN_ASSERT(data.size() == size);
    return data;
}

// static
void history_entry::factory_to_data(std::ostream& stream, uint64_t id, chain::point const& point, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
    ostream_writer sink(stream);
    factory_to_data(sink, id, point, kind, height, index, value_or_checksum);
}


#ifndef KTH_USE_DOMAIN
// static
void history_entry::factory_to_data(writer& sink, uint64_t id, chain::point const& point, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
    sink.write_8_bytes_little_endian(id);
    point.to_data(sink, false);
    sink.write_byte(static_cast<uint8_t>(kind));
    sink.write_4_bytes_little_endian(height);
    sink.write_4_bytes_little_endian(index);
    sink.write_8_bytes_little_endian(value_or_checksum);
}
#endif

// Serialization.
//-----------------------------------------------------------------------------

data_chunk history_entry::to_data() const {
    data_chunk data;
    auto const size = serialized_size(point_);
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

#ifndef KTH_USE_DOMAIN
void history_entry::to_data(writer& sink) const {
    factory_to_data(sink, id_, point_, point_kind_, height_, index_, value_or_checksum_ );
}
#endif

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

#ifndef KTH_USE_DOMAIN
history_entry history_entry::factory_from_data(reader& source) {
    history_entry instance;
    instance.from_data(source);
    return instance;
}
#endif

bool history_entry::from_data(const data_chunk& data) {
    data_source istream(data);
    return from_data(istream);
}

bool history_entry::from_data(std::istream& stream) {
    istream_reader source(stream);
    return from_data(source);
}

#ifndef KTH_USE_DOMAIN
bool history_entry::from_data(reader& source) {
    reset();
    
    id_ = source.read_8_bytes_little_endian();
    point_.from_data(source, false);
    point_kind_ = static_cast<chain::point_kind>(source.read_byte()),
    height_ = source.read_4_bytes_little_endian();
    index_ = source.read_4_bytes_little_endian();
    value_or_checksum_ = source.read_8_bytes_little_endian();
    
    if ( ! source) {
        reset();
    }

    return source;
}
#endif

} // namespace database
} // namespace kth


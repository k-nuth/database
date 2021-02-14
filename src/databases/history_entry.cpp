// Copyright (c) 2016-2021 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// #ifdef KTH_DB_NEW

#include <kth/database/databases/history_entry.hpp>

#include <cstddef>
#include <cstdint>

#include <kth/infrastructure.hpp>

namespace kth::database { 

history_entry::history_entry(uint64_t id, domain::chain::point const& point, domain::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum)
    : id_(id), point_(point), point_kind_(kind), height_(height), index_(index), value_or_checksum_(value_or_checksum)
{}

uint64_t history_entry::id() const {
    return id_;
}

domain::chain::point const& history_entry::point() const {
    return point_;
}

domain::chain::point_kind history_entry::point_kind() const {
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
    point_ = domain::chain::point{};
    point_kind_ = kth::domain::chain::point_kind::output;
    height_ = max_uint32;
    index_ = max_uint32;
    value_or_checksum_ = max_uint64;
}

// Empty scripts are valid, validation relies on not_found only.
bool history_entry::is_valid() const {
    return id_ != max_uint64 && point_.is_valid() && height_ != kth::max_uint32 && index_ != max_uint32 && value_or_checksum_ != max_uint64;
}

// Size.
//-----------------------------------------------------------------------------
// constexpr
//TODO(fernando): make domain::chain::point::serialized_size() static and constexpr to make this constexpr too
size_t history_entry::serialized_size(domain::chain::point const& point) {
    return sizeof(uint64_t) + point.serialized_size(false) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t);
}

// Serialization.
//-----------------------------------------------------------------------------

// static
data_chunk history_entry::factory_to_data(uint64_t id, domain::chain::point const& point, domain::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
    data_chunk data;
    auto const size = serialized_size(point);
    data.reserve(size);
    data_sink ostream(data);
    factory_to_data(ostream, id, point, kind, height, index, value_or_checksum);
    ostream.flush();
    KTH_ASSERT(data.size() == size);
    return data;
}

// static
void history_entry::factory_to_data(std::ostream& stream, uint64_t id, domain::chain::point const& point, domain::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
    ostream_writer sink(stream);
    factory_to_data(sink, id, point, kind, height, index, value_or_checksum);
}

// Serialization.
//-----------------------------------------------------------------------------

data_chunk history_entry::to_data() const {
    data_chunk data;
    auto const size = serialized_size(point_);
    data.reserve(size);
    data_sink ostream(data);
    to_data(ostream);
    ostream.flush();
    KTH_ASSERT(data.size() == size);
    return data;
}

void history_entry::to_data(std::ostream& stream) const {
    ostream_writer sink(stream);
    to_data(sink);
}

} // namespace kth::database


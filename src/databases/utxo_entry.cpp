// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// #ifdef KTH_DB_NEW

#include <kth/database/databases/utxo_entry.hpp>

#include <cstddef>
#include <cstdint>
// #include <kth/domain.hpp>

#include <kth/infrastructure.hpp>

namespace kth::database { 

utxo_entry::utxo_entry(domain::chain::output output, uint32_t height, uint32_t median_time_past, bool coinbase)
    : output_(std::move(output)), height_(height), median_time_past_(median_time_past), coinbase_(coinbase)
{}

domain::chain::output const& utxo_entry::output() const {
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
    KTH_ASSERT(data.size() == size);
    return data;
}

// static
void utxo_entry::to_data_fixed(std::ostream& stream, uint32_t height, uint32_t median_time_past, bool coinbase) {
    ostream_writer sink(stream);
    to_data_fixed(sink, height, median_time_past, coinbase);
}

// static
data_chunk utxo_entry::to_data_with_fixed(chain::output const& output, data_chunk const& fixed) {
    //TODO(fernando):  reuse fixed vector (do not create a new one)
    data_chunk data;
    auto const size = output.serialized_size(false) + fixed.size();
    data.reserve(size);
    data_sink ostream(data);
    to_data_with_fixed(ostream, output, fixed);
    ostream.flush();
    KTH_ASSERT(data.size() == size);
    return data;
}

// static
void utxo_entry::to_data_with_fixed(std::ostream& stream, chain::output const& output, data_chunk const& fixed) {
    ostream_writer sink(stream);
    to_data_with_fixed(sink, output, fixed);
}


// Serialization.
//-----------------------------------------------------------------------------

data_chunk utxo_entry::to_data() const {
    data_chunk data;
    auto const size = serialized_size();
    data.reserve(size);
    data_sink ostream(data);
    to_data(ostream);
    ostream.flush();
    KTH_ASSERT(data.size() == size);
    return data;
}

void utxo_entry::to_data(std::ostream& stream) const {
    ostream_writer sink(stream);
    to_data(sink);
}

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

bool utxo_entry::from_data(const data_chunk& data) {
    data_source istream(data);
    return from_data(istream);
}

bool utxo_entry::from_data(std::istream& stream) {
    istream_reader source(stream);
    return from_data(source);
}

} // namespace kth::database

// #endif // KTH_DB_NEW

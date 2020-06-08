// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// #ifdef KTH_DB_NEW

#include <kth/database/databases/transaction_entry.hpp>

#include <cstddef>
#include <cstdint>

namespace kth { 
namespace database {

// void write_position(writer& serial, uint32_t position) {
//     serial.KTH_POSITION_WRITER(position);
// }

// template <typename Deserializer>
// uint32_t read_position(Deserializer& deserial) {
//     return deserial.KTH_POSITION_READER();
// }

transaction_entry::transaction_entry(domain::chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position)
    : transaction_(tx), height_(height), median_time_past_(median_time_past), position_(position)
{}

domain::chain::transaction const& transaction_entry::transaction() const {
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
#if defined(KTH_CACHED_RPC_DATA)
    return tx.serialized_size(false, true, false) 
#else
    return tx.serialized_size(false, true) 
#endif
         + sizeof(uint32_t) + sizeof(uint32_t) + position_size;
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
    KTH_ASSERT(data.size() == size);
    return data;
}

// static
void transaction_entry::factory_to_data(std::ostream& stream, chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position) {
    ostream_writer sink(stream);
    factory_to_data(sink, tx, height, median_time_past, position);
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
    KTH_ASSERT(data.size() == size);
    return data;
}

void transaction_entry::to_data(std::ostream& stream) const {
    ostream_writer sink(stream);
    to_data(sink);
}

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

bool transaction_entry::from_data(const data_chunk& data) {
    data_source istream(data);
    return from_data(istream);
}

bool transaction_entry::from_data(std::istream& stream) {
    istream_reader source(stream);
    return from_data(source);
}

bool transaction_entry::confirmed() {
    return position_ != position_max;
}

//Note(Knuth): We don't have spent information
/*
bool transaction_entry::is_spent(size_t fork_height) const {

     // Cannot be spent if unconfirmed.
    if (position_ == position_max)
        return false;

    for (auto const& output : transaction_.outputs()) {
        auto const spender_height =  output.validation.spender_height;

        // A spend from above the fork height is not an actual spend.
        if (spender_height == chain::output::validation::not_spent || spender_height > fork_height)
            return false;
    }
    return true;
}*/


} // namespace database
} // namespace kth


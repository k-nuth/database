// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_UTXO_ENTRY_HPP_
#define KTH_DATABASE_UTXO_ENTRY_HPP_

#include <kth/domain.hpp>
#include <kth/database/define.hpp>

namespace kth::database {

inline
persistent_data_chunk to_persistent_data(kth::domain::chain::output const& x, allocator_type allocator) {
    data_chunk tmp = x.to_data(false);
    return persistent_data_chunk(tmp.begin(), tmp.end(), allocator);
}

inline
kth::domain::chain::output from_persistent_data(persistent_data_chunk const& pdc) {
    auto dc = kth::data_chunk(pdc.begin(), pdc.end());
    return kth::domain::create<kth::domain::chain::output>(dc, false);
}

class KD_API utxo_entry {
public:

    utxo_entry() = default;

    utxo_entry(domain::chain::output output, uint32_t height, uint32_t median_time_past, bool coinbase);

    // Getters
    domain::chain::output const& output() const;
    uint32_t height() const;
    uint32_t median_time_past() const;
    bool coinbase() const;

    bool is_valid() const;

    size_t serialized_size() const;

    data_chunk to_data() const;
    void to_data(std::ostream& stream) const;

    template <typename W, KTH_IS_WRITER(W)>
    void to_data(W& sink) const {
        output_.to_data(sink, false);
        to_data_fixed(sink, height_, median_time_past_, coinbase_);
    }

    bool from_data(const data_chunk& data);
    bool from_data(std::istream& stream);

    template <typename R, KTH_IS_READER(R)>
    bool from_data(R& source) {
        reset();

        output_.from_data(source, false);
        height_ = source.read_4_bytes_little_endian();
        median_time_past_ = source.read_4_bytes_little_endian();
        coinbase_ = source.read_byte();

        if ( ! source) {
            reset();
        }

        return source;
    }

    static
    data_chunk to_data_fixed(uint32_t height, uint32_t median_time_past, bool coinbase);

    static
    void to_data_fixed(std::ostream& stream, uint32_t height, uint32_t median_time_past, bool coinbase);

    template <typename W, KTH_IS_WRITER(W)>
    static
    void to_data_fixed(W& sink, uint32_t height, uint32_t median_time_past, bool coinbase) {
        sink.write_4_bytes_little_endian(height);
        sink.write_4_bytes_little_endian(median_time_past);
        sink.write_byte(coinbase);
    }

    static
    data_chunk to_data_with_fixed(domain::chain::output const& output, data_chunk const& fixed);

    static
    void to_data_with_fixed(std::ostream& stream, domain::chain::output const& output, data_chunk const& fixed);

    template <typename W, KTH_IS_WRITER(W)>
    static
    void to_data_with_fixed(W& sink, domain::chain::output const& output, data_chunk const& fixed) {
        output.to_data(sink, false);
        sink.write_bytes(fixed);
    }

private:
    void reset();

    constexpr static
    size_t serialized_size_fixed();

    domain::chain::output output_;
    uint32_t height_ = max_uint32;
    uint32_t median_time_past_ = max_uint32;
    bool coinbase_;
};


class KD_API utxo_entry_persistent {
public:

    utxo_entry_persistent() = default;

    utxo_entry_persistent(domain::chain::output output, uint32_t height, uint32_t median_time_past, bool coinbase);

    // Getters
    persistent_data_chunk const& output_bytes() const;
    uint32_t height() const;
    uint32_t median_time_past() const;
    bool coinbase() const;

    bool is_valid() const;

    // size_t serialized_size() const;

    // data_chunk to_data() const;
    // void to_data(std::ostream& stream) const;

    // template <typename W, KTH_IS_WRITER(W)>
    // void to_data(W& sink) const {
    //     output_.to_data(sink, false);
    //     to_data_fixed(sink, height_, median_time_past_, coinbase_);
    // }

    // bool from_data(const data_chunk& data);
    // bool from_data(std::istream& stream);

    // template <typename R, KTH_IS_READER(R)>
    // bool from_data(R& source) {
    //     reset();

    //     output_.from_data(source, false);
    //     height_ = source.read_4_bytes_little_endian();
    //     median_time_past_ = source.read_4_bytes_little_endian();
    //     coinbase_ = source.read_byte();

    //     if ( ! source) {
    //         reset();
    //     }

    //     return source;
    // }

    // static
    // data_chunk to_data_fixed(uint32_t height, uint32_t median_time_past, bool coinbase);

    // static
    // void to_data_fixed(std::ostream& stream, uint32_t height, uint32_t median_time_past, bool coinbase);

    // template <typename W, KTH_IS_WRITER(W)>
    // static
    // void to_data_fixed(W& sink, uint32_t height, uint32_t median_time_past, bool coinbase) {
    //     sink.write_4_bytes_little_endian(height);
    //     sink.write_4_bytes_little_endian(median_time_past);
    //     sink.write_byte(coinbase);
    // }

    // static
    // data_chunk to_data_with_fixed(domain::chain::output const& output, data_chunk const& fixed);

    // static
    // void to_data_with_fixed(std::ostream& stream, domain::chain::output const& output, data_chunk const& fixed);

    // template <typename W, KTH_IS_WRITER(W)>
    // static
    // void to_data_with_fixed(W& sink, domain::chain::output const& output, data_chunk const& fixed) {
    //     output.to_data(sink, false);
    //     sink.write_bytes(fixed);
    // }

private:
    void reset();

    // constexpr static
    // size_t serialized_size_fixed();

    persistent_data_chunk output_;
    uint32_t height_ = max_uint32;
    uint32_t median_time_past_ = max_uint32;
    bool coinbase_;
};

} // namespace kth::database

#endif // KTH_DATABASE_UTXO_ENTRY_HPP_



// bip::bufferstream to_bufferstream(output const& x, bool wire) {
//     bip::bufferstream bufferstream;
//     x.to_data(bufferstream, wire);
//     return bufferstream;
// }

// persistent_data_chunk to_persistent_data(output const& x, allocator_type allocator, bool wire) {
//     bip::bufferstream bufferstream = to_bufferstream(x, wire);

//     persistent_data_chunk persistent_data((uint8_t*)bufferstream.data(),
//                                           (uint8_t*)bufferstream.data() + bufferstream.size(),
//                                           allocator);

//     return persistent_data;
// }


// persistent_data_chunk to_persistent_data(output const& x, bool wire, allocator_type allocator) {
//     persistent_data_chunk data(allocator);
//     auto const size = x.serialized_size(wire);
//     data.reserve(size);
//     data_sink ostream(data);
//     x.to_data(ostream, wire);
//     ostream.flush();
//     KTH_ASSERT(data.size() == size);
//     return data;
// }
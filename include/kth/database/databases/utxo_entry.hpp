// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_UTXO_ENTRY_HPP_
#define KTH_DATABASE_UTXO_ENTRY_HPP_

// #ifdef KTH_DB_NEW

#include <kth/domain.hpp>
#include <kth/database/define.hpp>

namespace kth::database {

class BCD_API utxo_entry {
public:

    utxo_entry() = default;

    utxo_entry(chain::output output, uint32_t height, uint32_t median_time_past, bool coinbase);

    // Getters
    chain::output const& output() const;
    uint32_t height() const;
    uint32_t median_time_past() const;
    bool coinbase() const;

    bool is_valid() const;

    size_t serialized_size() const;

    data_chunk to_data() const;
    void to_data(std::ostream& stream) const;

    template <Writer W, KTH_IS_WRITER(W)>
    void to_data(W& sink) const {
        output_.to_data(sink, false);
        to_data_fixed(sink, height_, median_time_past_, coinbase_);
    }

    bool from_data(const data_chunk& data);
    bool from_data(std::istream& stream);

    template <Reader R, KTH_IS_READER(R)>
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
    utxo_entry factory_from_data(data_chunk const& data);
    static
    utxo_entry factory_from_data(std::istream& stream);

    template <Reader R, KTH_IS_READER(R)>
    static
    utxo_entry factory_from_data(R& source) {
        utxo_entry instance;
        instance.from_data(source);
        return instance;
    }

    static
    data_chunk to_data_fixed(uint32_t height, uint32_t median_time_past, bool coinbase);

    static
    void to_data_fixed(std::ostream& stream, uint32_t height, uint32_t median_time_past, bool coinbase);

    template <Writer W, KTH_IS_WRITER(W)>
    static
    void to_data_fixed(W& sink, uint32_t height, uint32_t median_time_past, bool coinbase) {
        sink.write_4_bytes_little_endian(height);
        sink.write_4_bytes_little_endian(median_time_past);
        sink.write_byte(coinbase);
    }

    static
    data_chunk to_data_with_fixed(chain::output const& output, data_chunk const& fixed);

    static
    void to_data_with_fixed(std::ostream& stream, chain::output const& output, data_chunk const& fixed);

    template <Writer W, KTH_IS_WRITER(W)>
    static
    void to_data_with_fixed(W& sink, chain::output const& output, data_chunk const& fixed) {
        output.to_data(sink, false);
        sink.write_bytes(fixed);
    }

private:
    void reset();

    constexpr static
    size_t serialized_size_fixed();

    chain::output output_;
    uint32_t height_ = max_uint32;
    uint32_t median_time_past_ = max_uint32;
    bool coinbase_;
};

} // namespace kth::database

// #endif // KTH_DB_NEW

#endif // KTH_DATABASE_UTXO_ENTRY_HPP_

/**
 * Copyright (c) 2016-2017 Bitprim Inc.
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
#ifndef BITPRIM_DATABASE_UTXO_ENTRY_HPP_
#define BITPRIM_DATABASE_UTXO_ENTRY_HPP_

// #ifdef BITPRIM_DB_NEW

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

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

#ifdef BITPRIM_USE_DOMAIN
    template <Writer W, BITPRIM_IS_WRITER(W)>
    void to_data(W& sink) const {
        output_.to_data(sink, false);
        to_data_fixed(sink, height_, median_time_past_, coinbase_);
    }
#else
    void to_data(writer& sink) const;
#endif

    bool from_data(const data_chunk& data);
    bool from_data(std::istream& stream);

#ifdef BITPRIM_USE_DOMAIN
    template <Reader R, BITPRIM_IS_READER(R)>
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
#else
    bool from_data(reader& source);
#endif

    static
    utxo_entry factory_from_data(data_chunk const& data);
    static
    utxo_entry factory_from_data(std::istream& stream);

#ifdef BITPRIM_USE_DOMAIN
    template <Reader R, BITPRIM_IS_READER(R)>
    static
    utxo_entry factory_from_data(R& source) {
        utxo_entry instance;
        instance.from_data(source);
        return instance;
    }
#else
    static
    utxo_entry factory_from_data(reader& source);
#endif

    static
    data_chunk to_data_fixed(uint32_t height, uint32_t median_time_past, bool coinbase);

    static
    void to_data_fixed(std::ostream& stream, uint32_t height, uint32_t median_time_past, bool coinbase);

#ifdef BITPRIM_USE_DOMAIN
    template <Writer W, BITPRIM_IS_WRITER(W)>
    static
    void to_data_fixed(W& sink, uint32_t height, uint32_t median_time_past, bool coinbase) {
        sink.write_4_bytes_little_endian(height);
        sink.write_4_bytes_little_endian(median_time_past);
        sink.write_byte(coinbase);
    }
#else
    static
    void to_data_fixed(writer& sink, uint32_t height, uint32_t median_time_past, bool coinbase);
#endif

    static
    data_chunk to_data_with_fixed(chain::output const& output, data_chunk const& fixed);

    static
    void to_data_with_fixed(std::ostream& stream, chain::output const& output, data_chunk const& fixed);

#ifdef BITPRIM_USE_DOMAIN
    template <Writer W, BITPRIM_IS_WRITER(W)>
    static
    void to_data_with_fixed(W& sink, chain::output const& output, data_chunk const& fixed) {
        output.to_data(sink, false);
        sink.write_bytes(fixed);
    }
#else
    static
    void to_data_with_fixed(writer& sink, chain::output const& output, data_chunk const& fixed);
#endif

private:
    void reset();

    constexpr static
    size_t serialized_size_fixed();

    chain::output output_;
    uint32_t height_ = max_uint32;
    uint32_t median_time_past_ = max_uint32;
    bool coinbase_;
};

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

#endif // BITPRIM_DATABASE_UTXO_ENTRY_HPP_

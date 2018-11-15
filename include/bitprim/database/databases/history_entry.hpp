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
#ifndef BITPRIM_DATABASE_HISTORY_ENTRY_HPP_
#define BITPRIM_DATABASE_HISTORY_ENTRY_HPP_

// #ifdef BITPRIM_DB_NEW

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

class BCD_API history_entry {
public:

    history_entry() = default;

    history_entry(chain::point const& point, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);

    // Getters
    chain::point const& point() const;
    chain::point_kind point_kind() const;
    uint64_t value_or_checksum() const;
    uint32_t height() const;
    uint32_t index() const;

    bool is_valid() const;

    //TODO(fernando): make chain::point::serialized_size() static and constexpr to make this constexpr too
    // constexpr 
    static
    size_t serialized_size(chain::point const& point);

    data_chunk to_data() const;
    void to_data(std::ostream& stream) const;

#ifdef BITPRIM_USE_DOMAIN
    template <Writer W, BITPRIM_IS_WRITER(W)>
    void to_data(W& sink) const {
        factory_to_data(sink, point_, point_kind_, height_, index_, value_or_checksum_ );
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
#else
    bool from_data(reader& source);
#endif

    static
    history_entry factory_from_data(data_chunk const& data);
    static
    history_entry factory_from_data(std::istream& stream);


#ifdef BITPRIM_USE_DOMAIN
    template <Reader R, BITPRIM_IS_READER(R)>
    static
    history_entry factory_from_data(R& source) {
        history_entry instance;
        instance.from_data(source);
        return instance;
    }
#else
    history_entry factory_from_data(reader& source);
#endif

    static
    data_chunk factory_to_data(chain::point const& point, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);
    static
    void factory_to_data(std::ostream& stream,chain::point const& point, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);


#ifdef BITPRIM_USE_DOMAIN
    template <Writer W, BITPRIM_IS_WRITER(W)>
    static
    void factory_to_data(W& sink, chain::point const& point, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
        point.to_data(sink, false);
        sink.write_byte(static_cast<uint8_t>(kind));
        sink.write_4_bytes_little_endian(height);
        sink.write_4_bytes_little_endian(index);
        sink.write_8_bytes_little_endian(value_or_checksum);
    }
#else
    static
    void factory_to_data(writer& sink, chain::point const& point, chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);
#endif

private:
    void reset();


    chain::point point_;
    chain::point_kind point_kind_;
    uint32_t height_ = max_uint32;
    uint32_t index_ = max_uint32;
    uint64_t value_or_checksum_ = max_uint64;
};

} // namespace database
} // namespace libbitcoin


#endif // BITPRIM_DATABASE_HISTORY_ENTRY_HPP_

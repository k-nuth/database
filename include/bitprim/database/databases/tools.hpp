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
#ifndef BITPRIM_DATABASE_TOOLS_HPP_
#define BITPRIM_DATABASE_TOOLS_HPP_

#include <chrono>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

//Note: same logic as is_stale()

template <typename Clock>
inline
std::chrono::time_point<Clock> to_time_point(std::chrono::seconds secs) {
    return std::chrono::time_point<Clock>(typename Clock::duration(secs));
}

template <typename Clock>
inline
bool is_old_block_(uint32_t header_ts, std::chrono::seconds limit) {
    return (Clock::now() - to_time_point<Clock>(std::chrono::seconds(header_ts))) >= limit;
}

template <typename Clock>
inline
bool is_old_block_(chain::block const& block, std::chrono::seconds limit) {
    return is_old_block_<Clock>(block.header().timestamp(), limit);
}

constexpr
inline
std::chrono::seconds blocks_to_seconds(uint32_t blocks) {
    return std::chrono::seconds(blocks * target_spacing_seconds);   //10 * 60
}

inline
data_chunk db_value_to_data_chunk(MDB_val const& value) {
    return data_chunk{static_cast<uint8_t*>(value.mv_data), 
                      static_cast<uint8_t*>(value.mv_data) + value.mv_size};
}

uint32_t get_point_index(MDB_val const& value, bool wire) {

    // https://www.boost.org/doc/libs/1_68_0/libs/iostreams/doc/index.html

    // const char*           h = "Hello World!";
    // stream<array_source>  in(h, std::strlen(h));
    // std::string           hello;
    // std::getline(in, hello);
    // std::cout << hello << "\n";  // Prints "Hello World!"

    using boost::iostreams::stream;
    // using byte_source = boost::iostreams::basic_array_source<uint8_t>;

    using boost::iostreams::array_source;

    // stream<byte_source> str(static_cast<uint8_t*>(value.mv_data), value.mv_size);
    stream<array_source> str(static_cast<char*>(value.mv_data), value.mv_size);
    istream_reader source(str);

    if (wire) {
        return source.read_4_bytes_little_endian();
    }

    auto index = source.read_2_bytes_little_endian();
    if (index == max_uint16) {
        index = chain::point::null_index;
    }
    return index;
}

} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_TOOLS_HPP_

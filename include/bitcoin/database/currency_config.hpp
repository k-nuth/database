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
#ifndef LIBBITCOIN_DATABASE_CURRENCY_CONFIG_HPP_
#define LIBBITCOIN_DATABASE_CURRENCY_CONFIG_HPP_

#include <cstdint>
#include <boost/filesystem.hpp>
#include <bitcoin/database/define.hpp>

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
#endif // BITPRIM_CURRENCY_BCH


} // namespace database
} // namespace libbitcoin

#endif // LIBBITCOIN_DATABASE_CURRENCY_CONFIG_HPP_

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
#ifndef BITPRIM_DATABASE_RESULT_CODE_HPP_
#define BITPRIM_DATABASE_RESULT_CODE_HPP_

namespace libbitcoin {
namespace database {

enum class result_code {
    success = 0,
    success_duplicate_coinbase = 1,
    duplicated_key = 2,
    key_not_found = 3,
    db_empty = 4,
    no_data_to_prune = 5,
    db_corrupt = 6,
    other = 7
};

inline
bool succeed(result_code code) {
    return code == result_code::success || code == result_code::success_duplicate_coinbase;
}

} // namespace database
} // namespace libbitcoin

#endif // BITPRIM_DATABASE_RESULT_CODE_HPP_

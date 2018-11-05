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

    history_entry(libbitcoin::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);

    // Getters
    libbitcoin::chain::point_kind point_kind() const;
    uint64_t value_or_checksum() const;
    uint32_t height() const;
    uint32_t index() const;

    bool is_valid() const;

    size_t serialized_size() const;

    data_chunk to_data() const;
    void to_data(std::ostream& stream) const;
    void to_data(writer& sink) const;

    bool from_data(const data_chunk& data);
    bool from_data(std::istream& stream);
    bool from_data(reader& source);


    static
    utxo_entry factory_from_data(data_chunk const& data);
    static
    utxo_entry factory_from_data(std::istream& stream);
    static
    utxo_entry factory_from_data(reader& source);

    static
    data_chunk factory_to_data(libbitcoin::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);
    static
    void factory_to_data(std::ostream& stream, libbitcoin::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);
    static
    void factory_to_data(writer& sink, libbitcoin::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);

private:
    void reset();

    libbitcoin::chain::point_kind point_kind_;
    uint32_t height_ = max_uint32;
    uint32_t index_ = max_uint32;
    uint64_t value_or_checksum_ = max_uint64;
};

} // namespace database
} // namespace libbitcoin

// #endif // BITPRIM_DB_NEW

#endif // BITPRIM_DATABASE_HISTORY_ENTRY_HPP_

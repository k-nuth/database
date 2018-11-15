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
#ifndef BITPRIM_DATABASE_TRANSACTION_ENTRY_HPP_
#define BITPRIM_DATABASE_TRANSACTION_ENTRY_HPP_

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

class BCD_API transaction_entry {
public:

    transaction_entry() = default;

    transaction_entry(chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position);

    // Getters
    chain::transaction const& transaction() const;
    uint32_t height() const;
    uint32_t median_time_past() const;
    uint32_t position() const;

    bool is_valid() const;

    //TODO(fernando): make this constexpr 
    // constexpr 
    static
    size_t serialized_size(chain::transaction const& tx);

    data_chunk to_data() const;
    void to_data(std::ostream& stream) const;


#ifdef BITPRIM_USE_DOMAIN
    template <Writer W, BITPRIM_IS_WRITER(W)>
    void to_data(W& sink) const {
        factory_to_data(sink, transaction_, height_, median_time_past_, position_ );
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
    
        transaction_.from_data(source,false,true,false);
        height_ = source.read_4_bytes_little_endian();
        median_time_past_ = source.read_4_bytes_little_endian();
        position_ = read_position(source);

        if ( ! source) {
            reset();
        }

        return source;
    }    
#else
    bool from_data(reader& source);
#endif

    bool confirmed();

    bool is_spent(size_t fork_height) const;

    static
    transaction_entry factory_from_data(data_chunk const& data);
    static
    transaction_entry factory_from_data(std::istream& stream);


#ifdef BITPRIM_USE_DOMAIN
    template <Reader R, BITPRIM_IS_READER(R)>
    static
    transaction_entry factory_from_data(R& source) {
        transaction_entry instance;
        instance.from_data(source);
        return instance;
    }
#else
    transaction_entry factory_from_data(reader& source);
#endif

    static
    data_chunk factory_to_data(chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position);
    static
    void factory_to_data(std::ostream& stream, chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position);


#ifdef BITPRIM_USE_DOMAIN
    template <Writer W, BITPRIM_IS_WRITER(W)>
    static
    void factory_to_data(W& sink, chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position) {
        tx.to_data(sink, false, true, false);
        sink.write_4_bytes_little_endian(height);
        sink.write_4_bytes_little_endian(median_time_past);
        write_position(sink, position);
    }
#else
    static
    void factory_to_data(writer& sink, chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position);
#endif


private:
    void reset();

    chain::transaction transaction_;
    uint32_t height_;
    uint32_t median_time_past_;
    uint32_t position_;
};

} // namespace database
} // namespace libbitcoin


#endif // BITPRIM_DATABASE_TRANSACTION_ENTRY_HPP_

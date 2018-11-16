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
#ifndef BITPRIM_DATABASE_TRANSACTION_UNCONFIRMED_ENTRY_HPP_
#define BITPRIM_DATABASE_TRANSACTION_UNCONFIRMED_ENTRY_HPP_

#include <bitcoin/bitcoin.hpp>

#include <bitcoin/database/currency_config.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {


class BCD_API transaction_unconfirmed_entry {
public:

    transaction_unconfirmed_entry() = default;

    transaction_unconfirmed_entry(chain::transaction const& tx, uint32_t arrival_time);

    // Getters
    chain::transaction const& transaction() const;
    uint32_t arrival_time() const;

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
        factory_to_data(sink, transaction_, arrival_time_);
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
    
#if defined(BITPRIM_CACHED_RPC_DATA)    
        transaction_.from_data(source, false, true, true);
#else
        transaction_.from_data(source, false, true);
#endif
        arrival_time_ = source.read_4_bytes_little_endian();
        
        if ( ! source) {
            reset();
        }

        return source;
    }    
#else
    bool from_data(reader& source);
#endif

    static
    transaction_unconfirmed_entry factory_from_data(data_chunk const& data);
    static
    transaction_unconfirmed_entry factory_from_data(std::istream& stream);


#ifdef BITPRIM_USE_DOMAIN
    template <Reader R, BITPRIM_IS_READER(R)>
    static
    transaction_unconfirmed_entry factory_from_data(R& source) {
        transaction_unconfirmed_entry instance;
        instance.from_data(source);
        return instance;
    }
#else
    transaction_unconfirmed_entry factory_from_data(reader& source);
#endif

    static
    data_chunk factory_to_data(chain::transaction const& tx, uint32_t arrival_time);
    static
    void factory_to_data(std::ostream& stream, chain::transaction const& tx, uint32_t arrival_time);


#ifdef BITPRIM_USE_DOMAIN
    template <Writer W, BITPRIM_IS_WRITER(W)>
    static
    void factory_to_data(W& sink, chain::transaction const& tx, uint32_t arrival_time) {

#if defined(BITPRIM_CACHED_RPC_DATA)    
        tx.to_data(sink, false, true, true);
#else
        tx.to_data(sink, false, true);
#endif

        sink.write_4_bytes_little_endian(arrival_time);
    }
#else
    static
    void factory_to_data(writer& sink, chain::transaction const& tx, uint32_t arrival_time);
#endif


private:
    void reset();

    chain::transaction transaction_;
    uint32_t arrival_time_;
};

} // namespace database
} // namespace libbitcoin


#endif // BITPRIM_DATABASE_TRANSACTION_UNCONFIRMED_ENTRY_HPP_

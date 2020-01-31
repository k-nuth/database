// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TRANSACTION_ENTRY_HPP_
#define KTH_DATABASE_TRANSACTION_ENTRY_HPP_

#include <bitcoin/bitcoin.hpp>

#include <bitcoin/database/currency_config.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {


#ifdef KTH_USE_DOMAIN
template <Writer W, KTH_IS_WRITER(W)>
void write_position(W& serial, uint32_t position) {
    serial.KTH_POSITION_WRITER(position);
}
#else
inline
void write_position(writer& serial, uint32_t position) {
    serial.KTH_POSITION_WRITER(position);
}
#endif

template <typename Deserializer>
uint32_t read_position(Deserializer& deserial) {
    return deserial.KTH_POSITION_READER();
}

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


#ifdef KTH_USE_DOMAIN
    template <Writer W, KTH_IS_WRITER(W)>
    void to_data(W& sink) const {
        factory_to_data(sink, transaction_, height_, median_time_past_, position_ );
    }

#else
    void to_data(writer& sink) const;
#endif

    bool from_data(const data_chunk& data);
    bool from_data(std::istream& stream);


#ifdef KTH_USE_DOMAIN
    template <Reader R, KTH_IS_READER(R)>
    bool from_data(R& source) {
        reset();
    
#if defined(KTH_CACHED_RPC_DATA)    
        transaction_.from_data(source, false, true, false);
#else
        transaction_.from_data(source, false, true);
#endif
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

    //Bitprim: we don't have spent information
    //bool is_spent(size_t fork_height) const;

    static
    transaction_entry factory_from_data(data_chunk const& data);
    static
    transaction_entry factory_from_data(std::istream& stream);


#ifdef KTH_USE_DOMAIN
    template <Reader R, KTH_IS_READER(R)>
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


#ifdef KTH_USE_DOMAIN
    template <Writer W, KTH_IS_WRITER(W)>
    static
    void factory_to_data(W& sink, chain::transaction const& tx, uint32_t height, uint32_t median_time_past, uint32_t position) {

#if defined(KTH_CACHED_RPC_DATA)    
        tx.to_data(sink, false, true, false);
#else
        tx.to_data(sink, false, true);
#endif

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
} // namespace kth


#endif // KTH_DATABASE_TRANSACTION_ENTRY_HPP_

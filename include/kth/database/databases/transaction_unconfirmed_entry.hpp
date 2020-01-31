// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TRANSACTION_UNCONFIRMED_ENTRY_HPP_
#define KTH_DATABASE_TRANSACTION_UNCONFIRMED_ENTRY_HPP_

#include <bitcoin/bitcoin.hpp>

#include <bitcoin/database/currency_config.hpp>
#include <bitcoin/database/define.hpp>

namespace kth {
namespace database {


class BCD_API transaction_unconfirmed_entry {
public:

    transaction_unconfirmed_entry() = default;

    transaction_unconfirmed_entry(chain::transaction const& tx, uint32_t arrival_time, uint32_t height);

    // Getters
    chain::transaction const& transaction() const;
    uint32_t arrival_time() const;
    uint32_t height() const;

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
        factory_to_data(sink, transaction_, arrival_time_, height_);
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
        transaction_.from_data(source, false, true, true);
#else
        transaction_.from_data(source, false, true);
#endif
        arrival_time_ = source.read_4_bytes_little_endian();
        
        height_ = source.read_4_bytes_little_endian();

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


#ifdef KTH_USE_DOMAIN
    template <Reader R, KTH_IS_READER(R)>
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
    data_chunk factory_to_data(chain::transaction const& tx, uint32_t arrival_time, uint32_t height);
    static
    void factory_to_data(std::ostream& stream, chain::transaction const& tx, uint32_t arrival_time, uint32_t height);


#ifdef KTH_USE_DOMAIN
    template <Writer W, KTH_IS_WRITER(W)>
    static
    void factory_to_data(W& sink, chain::transaction const& tx, uint32_t arrival_time, uint32_t height) {

#if defined(KTH_CACHED_RPC_DATA)    
        tx.to_data(sink, false, true, true);
#else
        tx.to_data(sink, false, true);
#endif

        sink.write_4_bytes_little_endian(arrival_time);
        sink.write_4_bytes_little_endian(height);

    }
#else
    static
    void factory_to_data(writer& sink, chain::transaction const& tx, uint32_t arrival_time, uint32_t height);
#endif


private:
    void reset();

    chain::transaction transaction_;
    uint32_t arrival_time_;
    uint32_t height_;
};

} // namespace database
} // namespace kth


#endif // KTH_DATABASE_TRANSACTION_UNCONFIRMED_ENTRY_HPP_

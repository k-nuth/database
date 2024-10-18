// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_HISTORY_ENTRY_HPP_
#define KTH_DATABASE_HISTORY_ENTRY_HPP_

#include <kth/domain.hpp>
#include <kth/database/define.hpp>

namespace kth::database {

class KD_API history_entry {
public:

    history_entry() = default;

    history_entry(uint64_t id, domain::chain::point const& point, domain::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);

    // Getters
    uint64_t id () const;
    domain::chain::point const& point() const;
    domain::chain::point_kind point_kind() const;
    uint64_t value_or_checksum() const;
    uint32_t height() const;
    uint32_t index() const;

    bool is_valid() const;

    //TODO(fernando): make domain::chain::point::serialized_size() static and constexpr to make this constexpr too
    // constexpr
    static
    size_t serialized_size(domain::chain::point const& point);

    data_chunk to_data() const;
    void to_data(std::ostream& stream) const;

    template <typename W, KTH_IS_WRITER(W)>
    void to_data(W& sink) const {
        factory_to_data(sink,id_, point_, point_kind_, height_, index_, value_or_checksum_ );
    }

    // bool from_data(const data_chunk& data);
    // bool from_data(std::istream& stream);

    // template <typename R, KTH_IS_READER(R)>
    // bool from_data(R& source) {
    //     reset();

    //     id_ = source.read_8_bytes_little_endian();
    //     point_.from_data(source, false);
    //     point_kind_ = static_cast<domain::chain::point_kind>(source.read_byte()),
    //     height_ = source.read_4_bytes_little_endian();
    //     index_ = source.read_4_bytes_little_endian();
    //     value_or_checksum_ = source.read_8_bytes_little_endian();

    //     if ( ! source) {
    //         reset();
    //     }

    //     return source;
    // }

    //TODO: move to .cpp file
    static
    expect<history_entry> from_data(byte_reader& reader) {
        auto const id = reader.read_little_endian<uint64_t>();
        if ( ! id) {
            return make_unexpected(id.error());
        }

        auto const point = domain::chain::point::from_data(reader, false);
        if ( ! point) {
            return make_unexpected(point.error());
        }

        auto const point_kind = reader.read_byte();
        if ( ! point_kind) {
            return make_unexpected(point_kind.error());
        }

        auto const height = reader.read_little_endian<uint32_t>();
        if ( ! height) {
            return make_unexpected(height.error());
        }

        auto const index = reader.read_little_endian<uint32_t>();
        if ( ! index) {
            return make_unexpected(index.error());
        }

        auto const value_or_checksum = reader.read_little_endian<uint64_t>();
        if ( ! value_or_checksum) {
            return make_unexpected(value_or_checksum.error());
        }

        return history_entry(
            *id,
            *point,
            domain::chain::point_kind(*point_kind),
            *height,
            *index,
            *value_or_checksum
        );
    }

    static
    data_chunk factory_to_data(uint64_t id, domain::chain::point const& point, domain::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);

    static
    void factory_to_data(std::ostream& stream,uint64_t id, domain::chain::point const& point, domain::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum);

    template <typename W, KTH_IS_WRITER(W)>
    static
    void factory_to_data(W& sink, uint64_t id, domain::chain::point const& point, domain::chain::point_kind kind, uint32_t height, uint32_t index, uint64_t value_or_checksum) {
        sink.write_8_bytes_little_endian(id);
        point.to_data(sink, false);
        sink.write_byte(static_cast<uint8_t>(kind));
        sink.write_4_bytes_little_endian(height);
        sink.write_4_bytes_little_endian(index);
        sink.write_8_bytes_little_endian(value_or_checksum);
    }

private:
    void reset();

    uint64_t id_ = max_uint64;
    domain::chain::point point_;
    domain::chain::point_kind point_kind_;
    uint32_t height_ = max_uint32;
    uint32_t index_ = max_uint32;
    uint64_t value_or_checksum_ = max_uint64;
};

} // namespace kth::database


#endif // KTH_DATABASE_HISTORY_ENTRY_HPP_

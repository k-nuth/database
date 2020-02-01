// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_TOOLS_HPP_
#define KTH_DATABASE_TOOLS_HPP_

#include <chrono>

#include <kth/domain.hpp>

namespace kth {
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

} // namespace database
} // namespace kth

#endif // KTH_DATABASE_TOOLS_HPP_

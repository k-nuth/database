// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_PROPERTY_CODE_HPP_
#define KTH_DATABASE_PROPERTY_CODE_HPP_

namespace kth::database {

// TODO(fernando): rename

enum class property_code {
    db_mode = 0,
};

enum class db_mode_code {
    db_new = 0,
    db_new_with_blocks = 1,
    db_new_full = 2
};

} // namespace kth::database

#endif // KTH_DATABASE_PROPERTY_CODE_HPP_

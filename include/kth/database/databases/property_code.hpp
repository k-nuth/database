// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_PROPERTY_CODE_HPP_
#define KTH_DATABASE_PROPERTY_CODE_HPP_

#include <algorithm>
#include <cctype>
#include <istream>

namespace kth::database {

// TODO(fernando): rename

enum class property_code {
    db_mode = 0,
};

enum class db_mode_type {
    pruned,
    blocks,
    full,
};

istream& operator>> (istream &in, db_mode_type& db_mode) {
    std::string mode_str;
    in >> mode_str;

    std::transform(mode_str.begin(), mode_str.end(), mode_str.begin(),
        [](unsigned char c){ return std::toupper(c); });

    if (token == "PRUNED") {
        db_mode = db_mode_type::pruned;
    }
    else if (token == "BLOCKS") {
        db_mode = db_mode_type::blocks;
    }
    else if (token == "FULL") {
        db_mode = db_mode_type::full;
    }
    else {
        throw boost::program_options::validation_error("Invalid DB Mode");
    }

    return in;
}

} // namespace kth::database

#endif // KTH_DATABASE_PROPERTY_CODE_HPP_

// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <bitcoin/database/version.hpp>

namespace libbitcoin { namespace database {

char const* version() {
    return KTH_DATABASE_VERSION;
}

}} /*namespace libbitcoin::database*/


/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
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
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::chain;
using namespace bc::database;

#define DIRECTORY "utxo_database"

struct utxo_database_directory_setup_fixture {
    utxo_database_directory_setup_fixture() {
        // std::cout << "utxo_database_directory_setup_fixture" << std::endl;
        error_code ec;
        remove_all(DIRECTORY, ec);
        BOOST_REQUIRE(create_directories(DIRECTORY, ec));

        utxo_database db(DIRECTORY "/utxo_db");
        BOOST_REQUIRE(db.create());
        // BOOST_REQUIRE(db.close());
        // BOOST_REQUIRE(db.open());
    }

    ////~utxo_database_directory_setup_fixture()
    ////{
    ////    error_code ec;
    ////    remove_all(DIRECTORY, ec);
    ////}
};

BOOST_FIXTURE_TEST_SUITE(utxo_tests, utxo_database_directory_setup_fixture)

#ifdef BITPRIM_DB_NEW

// BOOST_AUTO_TEST_CASE(utxo_database__create) {
//     utxo_database db(DIRECTORY "/utxo_db");
//     BOOST_REQUIRE(db.create());
// }

BOOST_AUTO_TEST_CASE(utxo_database__open) {
    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
}

BOOST_AUTO_TEST_CASE(utxo_database__insert_genesis) {
    std::string genesis_enc =
        "01000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"
        "29ab5f49"
        "ffff001d"
        "1dac2b7c"
        "01"
        "01000000"
        "01"
        "0000000000000000000000000000000000000000000000000000000000000000ffffffff"
        "4d"
        "04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73"
        "ffffffff"
        "01"
        "00f2052a01000000"
        "43"
        "4104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
        "00000000";

    data_chunk data;
    decode_base16(data, genesis_enc);
    const auto genesis = chain::block::factory_from_data(data);

    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    BOOST_REQUIRE(db.push_block(genesis));

    hash_digest txid;
    std::string txid_enc = "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b";
    BOOST_REQUIRE(decode_hash(txid, txid_enc));

    auto script = db.get(output_point{txid, 0});

    std::string script_enc = "434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac";
    // std::cout << script_enc << std::endl;
    // std::cout << encode_base16(script.to_data(true)) << std::endl;
    BOOST_REQUIRE(encode_base16(script.to_data(true)) == script_enc);
}


BOOST_AUTO_TEST_CASE(utxo_database__insert_duplicate) {
    std::string genesis_enc =
        "01000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"
        "29ab5f49"
        "ffff001d"
        "1dac2b7c"
        "01"
        "01000000"
        "01"
        "0000000000000000000000000000000000000000000000000000000000000000ffffffff"
        "4d"
        "04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73"
        "ffffffff"
        "01"
        "00f2052a01000000"
        "43"
        "4104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
        "00000000";

    data_chunk data;
    decode_base16(data, genesis_enc);
    const auto genesis = chain::block::factory_from_data(data);

    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    BOOST_REQUIRE(db.push_block(genesis));
    BOOST_REQUIRE(! db.push_block(genesis));
}


#endif // BITPRIM_DB_NEW

BOOST_AUTO_TEST_SUITE_END()

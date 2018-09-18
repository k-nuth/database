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

chain::block get_block(std::string const& enc) {
    data_chunk data;
    decode_base16(data, enc);
    return chain::block::factory_from_data(data);
}

chain::block get_genesis() {
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
    return get_block(genesis_enc);
}

BOOST_FIXTURE_TEST_SUITE(utxo_tests, utxo_database_directory_setup_fixture)

// #ifdef BITPRIM_DB_NEW

// BOOST_AUTO_TEST_CASE(utxo_database__create) {
//     utxo_database db(DIRECTORY "/utxo_db");
//     BOOST_REQUIRE(db.create());
// }

BOOST_AUTO_TEST_CASE(utxo_database__open) {
    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
}

BOOST_AUTO_TEST_CASE(utxo_database__insert_genesis) {
    auto const genesis = get_genesis();

    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    BOOST_REQUIRE(db.push_block(genesis));

    hash_digest txid;
    std::string txid_enc = "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b";
    BOOST_REQUIRE(decode_hash(txid, txid_enc));

    auto script = db.get(output_point{txid, 0});
    BOOST_REQUIRE(script.is_valid());

    std::string script_enc = "434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac";
    // std::cout << script_enc << std::endl;
    // std::cout << encode_base16(script.to_data(true)) << std::endl;
    BOOST_REQUIRE(encode_base16(script.to_data(true)) == script_enc);
}

BOOST_AUTO_TEST_CASE(utxo_database__insert_duplicate) {
    auto const genesis = get_genesis();

    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    BOOST_REQUIRE(db.push_block(genesis));
    BOOST_REQUIRE(! db.push_block(genesis));
}

BOOST_AUTO_TEST_CASE(utxo_database__spend) {
    auto const orig = get_block("01000000a594fda9d85f69e762e498650d6fdb54d838657cea7841915203170000000000a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f505da904ce6ed5b1b017fe8070101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b015cffffffff0100f2052a01000000434104283338ffd784c198147f99aed2cc16709c90b1522e3b3637b312a6f9130e0eda7081e373a96d36be319710cd5c134aaffba81ff08650d7de8af332fe4d8cde20ac00000000");
    auto const spender = get_block("01000000ba8b9cda965dd8e536670f9ddec10e53aab14b20bacad27b9137190000000000190760b278fe7b8565fda3b968b918d5fd997f993b23674c0af3b6fde300b38f33a5914ce6ed5b1b01e32f570201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b014effffffff0100f2052a01000000434104b68a50eaa0287eff855189f949c1c6e5f58b37c88231373d8a59809cbae83059cc6469d65c665ccfd1cfeb75c6e8e19413bba7fbff9bc762419a76d87b16086eac000000000100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");

    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    BOOST_REQUIRE(db.push_block(orig));

    hash_digest txid;
    std::string txid_enc = "f5d8ee39a430901c91a5917b9f2dc19d6d1a0e9cea205b009ca73dd04470b9a6";
    BOOST_REQUIRE(decode_hash(txid, txid_enc));
    auto script = db.get(output_point{txid, 0});
    BOOST_REQUIRE(script.is_valid());
    std::string script_enc = "434104283338ffd784c198147f99aed2cc16709c90b1522e3b3637b312a6f9130e0eda7081e373a96d36be319710cd5c134aaffba81ff08650d7de8af332fe4d8cde20ac";
    BOOST_REQUIRE(encode_base16(script.to_data(true)) == script_enc);


    // --------------------------------------------------------------------------------------------------------------------------
    BOOST_REQUIRE(db.push_block(spender));

    script = db.get(output_point{txid, 0});
    BOOST_REQUIRE(! script.is_valid());

    txid_enc = "c06fbab289f723c6261d3030ddb6be121f7d2508d77862bb1e484f5cd7f92b25";
    BOOST_REQUIRE(decode_hash(txid, txid_enc));
    script = db.get(output_point{txid, 0});
    BOOST_REQUIRE(script.is_valid());
    script_enc = "434104b68a50eaa0287eff855189f949c1c6e5f58b37c88231373d8a59809cbae83059cc6469d65c665ccfd1cfeb75c6e8e19413bba7fbff9bc762419a76d87b16086eac";
    BOOST_REQUIRE(encode_base16(script.to_data(true)) == script_enc);

    txid_enc = "5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2";
    BOOST_REQUIRE(decode_hash(txid, txid_enc));
    script = db.get(output_point{txid, 0});
    BOOST_REQUIRE(script.is_valid());
    script_enc = "1976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac";
    BOOST_REQUIRE(encode_base16(script.to_data(true)) == script_enc);
}


/*
orig
79880
00000000002e872c6fbbcf39c93ef0d89e33484ebf457f6829cbf4b561f3af5a
01000000a594fda9d85f69e762e498650d6fdb54d838657cea7841915203170000000000a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f505da904ce6ed5b1b017fe8070101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b015cffffffff0100f2052a01000000434104283338ffd784c198147f99aed2cc16709c90b1522e3b3637b312a6f9130e0eda7081e373a96d36be319710cd5c134aaffba81ff08650d7de8af332fe4d8cde20ac00000000

80000
gasta
000000000043a8c0fd1d6f726790caa2a406010d19efd2780db27bdbbd93baf6
01000000ba8b9cda965dd8e536670f9ddec10e53aab14b20bacad27b9137190000000000190760b278fe7b8565fda3b968b918d5fd997f993b23674c0af3b6fde300b38f33a5914ce6ed5b1b01e32f570201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b014effffffff0100f2052a01000000434104b68a50eaa0287eff855189f949c1c6e5f58b37c88231373d8a59809cbae83059cc6469d65c665ccfd1cfeb75c6e8e19413bba7fbff9bc762419a76d87b16086eac000000000100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000
*/

// #endif // BITPRIM_DB_NEW

BOOST_AUTO_TEST_SUITE_END()

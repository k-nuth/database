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
#include <tuple>

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



std::tuple<MDB_env*, MDB_dbi, MDB_dbi, MDB_dbi> open_dbs() {
    MDB_env* env_;
    MDB_dbi dbi_utxo_;
    MDB_dbi dbi_reorg_pool_;
    MDB_dbi dbi_reorg_index_;
    MDB_txn* db_txn;

    char utxo_db_name[] = "utxo_db";
    char reorg_pool_name[] = "reorg_pool";
    char reorg_index_name[] = "reorg_index";

    BOOST_REQUIRE(mdb_env_create(&env_) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_env_set_mapsize(env_, size_t(10485760) * 1024) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_env_set_maxdbs(env_, 3) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_env_open(env_, "utxo_database/utxo_db", MDB_FIXEDMAP | MDB_NORDAHEAD | MDB_NOMEMINIT, 0664) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_txn_begin(env_, NULL, 0, &db_txn) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_dbi_open(db_txn, utxo_db_name, 0, &dbi_utxo_) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_dbi_open(db_txn, reorg_pool_name, 0, &dbi_reorg_pool_) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_dbi_open(db_txn, reorg_index_name, MDB_DUPSORT | MDB_INTEGERKEY | MDB_DUPFIXED, &dbi_reorg_index_) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_txn_commit(db_txn) == MDB_SUCCESS);

    return {env_, dbi_utxo_, dbi_reorg_pool_, dbi_reorg_index_};
}

void check_reorg_output(MDB_env* env_, MDB_dbi& dbi_reorg_pool_, std::string txid_enc, uint32_t pos, std::string output_enc) {
    MDB_txn* db_txn;

    hash_digest txid;
    BOOST_REQUIRE(decode_hash(txid, txid_enc));
    output_point point{txid, pos};
    auto keyarr = point.to_data(false);
    MDB_val key {keyarr.size(), keyarr.data()};
    MDB_val value;

    BOOST_REQUIRE(mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_get(db_txn, dbi_reorg_pool_, &key, &value) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_txn_commit(db_txn) == MDB_SUCCESS);

    data_chunk data {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};
    auto output = chain::output::factory_from_data(data, false);

    BOOST_REQUIRE(encode_base16(output.to_data(true)) == output_enc);
}

void check_reorg_output_just_existence(MDB_env* env_, MDB_dbi& dbi_reorg_pool_, std::string txid_enc, uint32_t pos) {
    MDB_txn* db_txn;

    hash_digest txid;
    BOOST_REQUIRE(decode_hash(txid, txid_enc));
    output_point point{txid, pos};
    auto keyarr = point.to_data(false);
    MDB_val key {keyarr.size(), keyarr.data()};
    MDB_val value;

    BOOST_REQUIRE(mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_get(db_txn, dbi_reorg_pool_, &key, &value) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_txn_commit(db_txn) == MDB_SUCCESS);
}



void check_reorg_index(MDB_env* env_, MDB_dbi& dbi_reorg_index_, std::string txid_enc, uint32_t pos, uint32_t height) {
    MDB_txn* db_txn;
    MDB_val value;

    MDB_val height_key {sizeof(uint32_t), &height};
    BOOST_REQUIRE(mdb_txn_begin(env_, NULL, MDB_RDONLY, &db_txn) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_get(db_txn, dbi_reorg_index_, &height_key, &value) == MDB_SUCCESS);
    BOOST_REQUIRE(mdb_txn_commit(db_txn) == MDB_SUCCESS);
    data_chunk data2 {static_cast<uint8_t*>(value.mv_data), static_cast<uint8_t*>(value.mv_data) + value.mv_size};
    auto point_indexed = chain::point::factory_from_data(data2, false);

    hash_digest txid;
    BOOST_REQUIRE(decode_hash(txid, txid_enc));
    output_point point{txid, pos};

    BOOST_REQUIRE(point == point_indexed);
}

size_t db_count_items(MDB_env *env, MDB_dbi dbi) {
    MDB_val key, data;
	MDB_txn *txn;    
    MDB_cursor *cursor;
    int rc;

    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    mdb_cursor_open(txn, dbi, &cursor);

    size_t count = 0;    
    while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {

        data_chunk key_bytes {static_cast<uint8_t*>(key.mv_data), static_cast<uint8_t*>(key.mv_data) + key.mv_size};
        std::reverse(begin(key_bytes), end(key_bytes));
        std::cout << encode_base16(key_bytes) << std::endl;

        data_chunk value_bytes {static_cast<uint8_t*>(data.mv_data), static_cast<uint8_t*>(data.mv_data) + data.mv_size};
        std::reverse(begin(value_bytes), end(value_bytes));
        std::cout << encode_base16(value_bytes) << std::endl;

        ++count;
    }
    std::cout << "---------------------------------\n" << std::endl;
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);

    return count;
}

size_t db_count_index_by_height(MDB_env *env, MDB_dbi dbi, size_t height) {
    MDB_val data;
	MDB_txn *txn;    
    MDB_cursor *cursor;
    int rc;

    MDB_val height_key {sizeof(uint32_t), &height};

    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    mdb_cursor_open(txn, dbi, &cursor);

    size_t count = 0;    


    if ((rc = mdb_cursor_get(cursor, &height_key, &data, MDB_SET)) == 0) {
        // data_chunk key_bytes {static_cast<uint8_t*>(height_key.mv_data), static_cast<uint8_t*>(height_key.mv_data) + height_key.mv_size};
        // std::reverse(begin(key_bytes), end(key_bytes));
        // std::cout << encode_base16(key_bytes) << std::endl;

        // data_chunk value_bytes {static_cast<uint8_t*>(data.mv_data), static_cast<uint8_t*>(data.mv_data) + data.mv_size};
        // std::reverse(begin(value_bytes), end(value_bytes));
        // std::cout << encode_base16(value_bytes) << std::endl;

        ++count;

        while ((rc = mdb_cursor_get(cursor, &height_key, &data, MDB_NEXT_DUP)) == 0) {
            // data_chunk key_bytes {static_cast<uint8_t*>(height_key.mv_data), static_cast<uint8_t*>(height_key.mv_data) + height_key.mv_size};
            // std::reverse(begin(key_bytes), end(key_bytes));
            // std::cout << encode_base16(key_bytes) << std::endl;

            // data_chunk value_bytes {static_cast<uint8_t*>(data.mv_data), static_cast<uint8_t*>(data.mv_data) + data.mv_size};
            // std::reverse(begin(value_bytes), end(value_bytes));
            // std::cout << encode_base16(value_bytes) << std::endl;

            ++count;
        }
    } else {
        // std::cout << "no encontre el primero\n" << std::endl;    
    }
    std::cout << "---------------------------------\n" << std::endl;
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);

    return count;
}

void check_index_and_pool(MDB_env *env, MDB_dbi& dbi_index, MDB_dbi& dbi_pool) {
    MDB_val key, data;
    MDB_val value;
	MDB_txn *txn;    
    MDB_cursor *cursor;
    int rc;

    mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    mdb_cursor_open(txn, dbi_index, &cursor);

    while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
        BOOST_REQUIRE(mdb_get(txn, dbi_pool, &data, &value) == MDB_SUCCESS);
    }
    
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
}

// ---------------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(utxo_tests, utxo_database_directory_setup_fixture)

// #ifdef BITPRIM_DB_NEW

BOOST_AUTO_TEST_CASE(utxo_database__open) {
    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
}

BOOST_AUTO_TEST_CASE(utxo_database__insert_genesis) {
    auto const genesis = get_genesis();

    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    BOOST_REQUIRE(db.push_block(genesis, 0, 1) == utxo_code::success);  //TODO(fernando): median_time_past

    hash_digest txid;
    std::string txid_enc = "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b";
    BOOST_REQUIRE(decode_hash(txid, txid_enc));

    auto entry = db.get(output_point{txid, 0});
    BOOST_REQUIRE(entry.is_valid());

    BOOST_REQUIRE(entry.height() == 0);
    BOOST_REQUIRE(entry.median_time_past() == 1);
    BOOST_REQUIRE(entry.coinbase());

    auto output = entry.output();
    BOOST_REQUIRE(output.is_valid());


    std::string output_enc = "00f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac";
    BOOST_REQUIRE(encode_base16(output.to_data(true)) == output_enc);
}

BOOST_AUTO_TEST_CASE(utxo_database__insert_success_duplicate_coinbase) {
    auto const genesis = get_genesis();

    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    auto res = db.push_block(genesis, 0, 1);       //TODO(fernando): median_time_past
    
    BOOST_REQUIRE(res == utxo_code::success);
    BOOST_REQUIRE(utxo_database::succeed(res));
    
    res = db.push_block(genesis, 0, 1);            //TODO(fernando): median_time_past
    BOOST_REQUIRE(res == utxo_code::success_duplicate_coinbase);
    BOOST_REQUIRE(utxo_database::succeed(res));
}

BOOST_AUTO_TEST_CASE(utxo_database__key_not_found) {
    auto const spender = get_block("01000000ba8b9cda965dd8e536670f9ddec10e53aab14b20bacad27b9137190000000000190760b278fe7b8565fda3b968b918d5fd997f993b23674c0af3b6fde300b38f33a5914ce6ed5b1b01e32f570201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b014effffffff0100f2052a01000000434104b68a50eaa0287eff855189f949c1c6e5f58b37c88231373d8a59809cbae83059cc6469d65c665ccfd1cfeb75c6e8e19413bba7fbff9bc762419a76d87b16086eac000000000100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");
    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    BOOST_REQUIRE(db.push_block(spender, 1, 1) == utxo_code::key_not_found);        //TODO(fernando): median_time_past
}

BOOST_AUTO_TEST_CASE(utxo_database__insert_duplicate) {
    auto const orig = get_block("01000000a594fda9d85f69e762e498650d6fdb54d838657cea7841915203170000000000a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f505da904ce6ed5b1b017fe8070101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b015cffffffff0100f2052a01000000434104283338ffd784c198147f99aed2cc16709c90b1522e3b3637b312a6f9130e0eda7081e373a96d36be319710cd5c134aaffba81ff08650d7de8af332fe4d8cde20ac00000000");
    auto const spender = get_block("01000000ba8b9cda965dd8e536670f9ddec10e53aab14b20bacad27b9137190000000000190760b278fe7b8565fda3b968b918d5fd997f993b23674c0af3b6fde300b38f33a5914ce6ed5b1b01e32f570201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b014effffffff0100f2052a01000000434104b68a50eaa0287eff855189f949c1c6e5f58b37c88231373d8a59809cbae83059cc6469d65c665ccfd1cfeb75c6e8e19413bba7fbff9bc762419a76d87b16086eac000000000100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");


    // std::cout << encode_hash(orig.hash()) << std::endl;
    // std::cout << encode_hash(spender.hash()) << std::endl;


    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    BOOST_REQUIRE(db.push_block(orig, 0, 1) == utxo_code::success);                 //TODO(fernando): median_time_past
    BOOST_REQUIRE(db.push_block(spender, 1, 1) == utxo_code::success);              //TODO(fernando): median_time_past
    BOOST_REQUIRE(db.push_block(spender, 1, 1) == utxo_code::key_not_found);        //TODO(fernando): median_time_past
}

BOOST_AUTO_TEST_CASE(utxo_database__spend) {
    //79880
    auto const orig = get_block("01000000a594fda9d85f69e762e498650d6fdb54d838657cea7841915203170000000000a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f505da904ce6ed5b1b017fe8070101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b015cffffffff0100f2052a01000000434104283338ffd784c198147f99aed2cc16709c90b1522e3b3637b312a6f9130e0eda7081e373a96d36be319710cd5c134aaffba81ff08650d7de8af332fe4d8cde20ac00000000");
    //80000
    auto const spender = get_block("01000000ba8b9cda965dd8e536670f9ddec10e53aab14b20bacad27b9137190000000000190760b278fe7b8565fda3b968b918d5fd997f993b23674c0af3b6fde300b38f33a5914ce6ed5b1b01e32f570201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b014effffffff0100f2052a01000000434104b68a50eaa0287eff855189f949c1c6e5f58b37c88231373d8a59809cbae83059cc6469d65c665ccfd1cfeb75c6e8e19413bba7fbff9bc762419a76d87b16086eac000000000100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");

    utxo_database db(DIRECTORY "/utxo_db");
    BOOST_REQUIRE(db.open());
    BOOST_REQUIRE(db.push_block(orig, 0, 1) == utxo_code::success);         //TODO(fernando): median_time_past

    hash_digest txid;
    std::string txid_enc = "f5d8ee39a430901c91a5917b9f2dc19d6d1a0e9cea205b009ca73dd04470b9a6";
    BOOST_REQUIRE(decode_hash(txid, txid_enc));
    auto entry = db.get(output_point{txid, 0});
    BOOST_REQUIRE(entry.is_valid());

    BOOST_REQUIRE(entry.height() == 0);
    BOOST_REQUIRE(entry.median_time_past() == 1);
    BOOST_REQUIRE(entry.coinbase());


    auto output = entry.output();
    BOOST_REQUIRE(output.is_valid());

    std::string output_enc = "00f2052a01000000434104283338ffd784c198147f99aed2cc16709c90b1522e3b3637b312a6f9130e0eda7081e373a96d36be319710cd5c134aaffba81ff08650d7de8af332fe4d8cde20ac";
    BOOST_REQUIRE(encode_base16(output.to_data(true)) == output_enc);


    // --------------------------------------------------------------------------------------------------------------------------
    BOOST_REQUIRE(db.push_block(spender, 1, 1) == utxo_code::success);          //TODO(fernando): median_time_past

    entry = db.get(output_point{txid, 0});
    output = entry.output();
    BOOST_REQUIRE(! output.is_valid());

    txid_enc = "c06fbab289f723c6261d3030ddb6be121f7d2508d77862bb1e484f5cd7f92b25";
    BOOST_REQUIRE(decode_hash(txid, txid_enc));
    entry = db.get(output_point{txid, 0});
    BOOST_REQUIRE(entry.is_valid());
    BOOST_REQUIRE(entry.height() == 1);
    BOOST_REQUIRE(entry.median_time_past() == 1);
    BOOST_REQUIRE(entry.coinbase());

    output = entry.output();
    BOOST_REQUIRE(output.is_valid());
    output_enc = "00f2052a01000000434104b68a50eaa0287eff855189f949c1c6e5f58b37c88231373d8a59809cbae83059cc6469d65c665ccfd1cfeb75c6e8e19413bba7fbff9bc762419a76d87b16086eac";
    BOOST_REQUIRE(encode_base16(output.to_data(true)) == output_enc);

    txid_enc = "5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2";
    BOOST_REQUIRE(decode_hash(txid, txid_enc));
    entry = db.get(output_point{txid, 0});
    BOOST_REQUIRE(entry.is_valid());
    BOOST_REQUIRE(entry.height() == 1);
    BOOST_REQUIRE(entry.median_time_past() == 1);
    BOOST_REQUIRE( ! entry.coinbase());


    output = entry.output();
    BOOST_REQUIRE(output.is_valid());
    output_enc = "00f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac";
    BOOST_REQUIRE(encode_base16(output.to_data(true)) == output_enc);
}


BOOST_AUTO_TEST_CASE(utxo_database__reorg) {
    //79880 - 00000000002e872c6fbbcf39c93ef0d89e33484ebf457f6829cbf4b561f3af5a
    auto const orig = get_block("01000000a594fda9d85f69e762e498650d6fdb54d838657cea7841915203170000000000a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f505da904ce6ed5b1b017fe8070101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b015cffffffff0100f2052a01000000434104283338ffd784c198147f99aed2cc16709c90b1522e3b3637b312a6f9130e0eda7081e373a96d36be319710cd5c134aaffba81ff08650d7de8af332fe4d8cde20ac00000000");
    //80000 - 000000000043a8c0fd1d6f726790caa2a406010d19efd2780db27bdbbd93baf6
    auto const spender = get_block("01000000ba8b9cda965dd8e536670f9ddec10e53aab14b20bacad27b9137190000000000190760b278fe7b8565fda3b968b918d5fd997f993b23674c0af3b6fde300b38f33a5914ce6ed5b1b01e32f570201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704e6ed5b1b014effffffff0100f2052a01000000434104b68a50eaa0287eff855189f949c1c6e5f58b37c88231373d8a59809cbae83059cc6469d65c665ccfd1cfeb75c6e8e19413bba7fbff9bc762419a76d87b16086eac000000000100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");

    {
        utxo_database db(DIRECTORY "/utxo_db");
        BOOST_REQUIRE(db.open());
        BOOST_REQUIRE(db.push_block(orig, 0, 1) == utxo_code::success);     //TODO(fernando): median_time_past
        BOOST_REQUIRE(db.push_block(spender, 1, 1) == utxo_code::success);     //TODO(fernando): median_time_past
    }   //close() implicit

    MDB_env* env_;
    MDB_dbi dbi_utxo_;
    MDB_dbi dbi_reorg_pool_;
    MDB_dbi dbi_reorg_index_;
    MDB_txn* db_txn;
    std::tie(env_, dbi_utxo_, dbi_reorg_pool_, dbi_reorg_index_) = open_dbs();

    check_reorg_output(env_, dbi_reorg_pool_, "f5d8ee39a430901c91a5917b9f2dc19d6d1a0e9cea205b009ca73dd04470b9a6", 0, "00f2052a01000000434104283338ffd784c198147f99aed2cc16709c90b1522e3b3637b312a6f9130e0eda7081e373a96d36be319710cd5c134aaffba81ff08650d7de8af332fe4d8cde20ac");
    check_reorg_index(env_, dbi_reorg_index_, "f5d8ee39a430901c91a5917b9f2dc19d6d1a0e9cea205b009ca73dd04470b9a6", 0, 1);
}


BOOST_AUTO_TEST_CASE(utxo_database__reorg_index) {

    // Block #4334
    // BlockHash 000000009cdad3c55df9c9bc88265329254a6c8ca810fa7f0e953c947df86dc7
    auto const b0 = get_block("010000005a0aeca67f7e43582c2b0138b2daa0c8dc1bedbb2477cfba2d3f96bf0000000065dabdbdb83e9820e4f666f3634d88308909789f7ae29e730812784a96485e3cd5899749ffff001dc2544c010101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d028e00ffffffff0100f2052a01000000434104b48f20398caaf3ff5d40710e0af87a4b86fd19d125ddacf15a2a023831d1731350e5fd40d0e28bb6481ad1843847213764feb98a2dd041069a8c39c842e1da93ac00000000");
    // Block #4966
    // BlockHash 000000004f6a440a95a5d2d6c89f3e6b46587cd43f76efbaa96ef5d37ea90961
    auto const b1 = get_block("010000002cba11cca7170e1da742fcfb83d15c091bac17a79c089e944dda904a00000000b7404d6a9c451a9f527a7fbeb54839c2bca2eac7b138cdd700be19d733efa0fc82609e49ffff001df66339030101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0121ffffffff0100f2052a01000000434104c20502b45fe276d418a32b55435cb4361dea4e173c36a8e0ad52518b17f2d48cde4336c8ac8d2270e6040469f11c56036db1feef803fb529e36d0f599261cb19ac00000000");
    // Block #4561
    // BlockHash 0000000089abf237d732a1515f7066e7ba29e1833664da8b704c8575c0465223
    auto const b2 = get_block("0100000052df1ff74876f2de37db341e12f932768f2a18dcc09dd024a1e676aa00000000cc07817ab589551d698ba7eb2a6efd6670d6951792ad52e2bd5832bf2f4930ecb5f19949ffff001d4045c6010101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d029f00ffffffff0100f2052a010000004341044bdc62d08cc074664cef09b24b01125a5e92fc2d61f821d6fddac367eb80b06a0a6148feabb0d25717f9eb84950ef0d3e7fe49ce7fb5a6e14da881a5b2bc64c0ac00000000");
    // Block #4991
    // BlockHash 00000000fa413253e1d30ff687f239b528330c810e3de86e42a538175682599d
    auto const b3 = get_block("0100000040eb019191a99f1f3ef5e04606314d80a635e214ca3347388259ad4000000000f61fefef8ee758b273ee64e1bf5c07485dd74cd065a5ce0d59827e0700cad0d98c9f9e49ffff001d20a92f010101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d013affffffff0100f2052a01000000434104debec4c8f781d5223a90aa90416ee51abf15d37086cc2d9be67873534fb06ae68c8a4e0ed0a7eedff9c97fe23f03e652d7f44501286dc1f75148bfaa9e386300ac00000000");
    // Block #4556
    // BlockHash 0000000054d4f171b0eab3cd4e31da4ce5a1a06f27b39bf36c5902c9bb8ef5c4
    auto const b4 = get_block("0100000021a06106f13b4f0112a63e77fae3a48ffe10716ff3cdfb35371032990000000015327dc99375fc1fdc02e15394369daa6e23ad4dc27e7c1c4af21606add5b068dadf9949ffff001dda8444000101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d029600ffffffff0100f2052a0100000043410475804bd8be2560ab2ebd170228429814d60e58d7415a93dc51f23e4cb2f8b5188dd56fa8d519e9ef9c16d6ab22c1c8304e10e74a28444eb26821948b2d1482a4ac00000000");

    // Block #5217
    // BlockHash 0000000025075f093c42a0393c844bc59f90024b18a9f588f6fa3fc37487c3c2
    auto const spender0 = get_block("01000000944bb801c604dda3d51758f292afdca8d973288434c8fe4bf0b5982d000000008a7d204ffe05282b05f280459401b59be41b089cefc911f4fb5641f90309d942b929a149ffff001d1b8d847f0301000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d02c500ffffffff0100f2052a01000000434104f4af426e464d972012256f4cbe5df528aa99b1ceb489968a56cf6b295e6fad1473be89f66fbd3d16adf3dfba7c5253517d11d1d188fe858720497c4fc0a1ef9dac00000000010000000465dabdbdb83e9820e4f666f3634d88308909789f7ae29e730812784a96485e3c000000004948304502204c52c2301dcc3f6af7a3ef2ad118185ca2d52a7ae90013332ad53732a085b8d4022100f074ab99e77d5d4c54eb6bfc82c42a094d7d7eaf632d52897ef058c617a2bb2301ffffffffb7404d6a9c451a9f527a7fbeb54839c2bca2eac7b138cdd700be19d733efa0fc000000004847304402206c55518aa596824d1e760afcfeb7b0103a1a82ea8dcd4c3474becc8246ba823702205009cbc40affa3414f9a139f38a86f81a401193f759fb514b9b1d4e2e49f82a401ffffffffcc07817ab589551d698ba7eb2a6efd6670d6951792ad52e2bd5832bf2f4930ec0000000049483045022100b485b4daa4af75b7b34b4f2338e7b96809c75fab5577905ade0789c7f821a69e022010d73d2a3c7fcfc6db911dead795b0aa7d5448447ad5efc7e516699955a18ac801fffffffff61fefef8ee758b273ee64e1bf5c07485dd74cd065a5ce0d59827e0700cad0d9000000004a493046022100bc6e89ee580d1c721b15c36d0a1218c9e78f6f7537616553341bbd1199fe615a02210093062f2c1a1c87f55b710011976a03dff57428e38dd640f6fbdef0fa52ad462d01ffffffff0100c817a80400000043410408998c08bbe6bba756e9b864722fe76ca403929382db2b120f9f621966b00af48f4b014b458bccd4f2acf63b1487ecb9547bc87bdecb08e9c4d08c138c76439aac00000000010000000115327dc99375fc1fdc02e15394369daa6e23ad4dc27e7c1c4af21606add5b068000000004a49304602210086b55b7f2fa5395d1e90a85115ada930afa01b86116d6bbeeecd8e2b97eefbac022100d653846d378845df2ced4b4923dcae4b6ddd5e8434b25e1602928235054c8d5301ffffffff0100f2052a01000000434104b68b035858a00051ca70dd4ba297168d9a3720b642c2e0cd08846bfbb144233b11b24c4b8565353b579bd7109800e42a1fc1e20dbdfbba6a12d0089aab313181ac00000000");



    {
        utxo_database db(DIRECTORY "/utxo_db");
        BOOST_REQUIRE(db.open());
        BOOST_REQUIRE(db.push_block(b0, 0, 1) == utxo_code::success);           //TODO(fernando): median_time_past
        BOOST_REQUIRE(db.push_block(b1, 1, 1) == utxo_code::success);              
        BOOST_REQUIRE(db.push_block(b2, 2, 1) == utxo_code::success);
        BOOST_REQUIRE(db.push_block(b3, 3, 1) == utxo_code::success);
        BOOST_REQUIRE(db.push_block(b4, 4, 1)      == utxo_code::success);
        BOOST_REQUIRE(db.push_block(spender0, 5, 1) == utxo_code::success);
    }   //close() implicit


    MDB_env* env_; MDB_dbi dbi_utxo_; MDB_dbi dbi_reorg_pool_; MDB_dbi dbi_reorg_index_; MDB_txn* db_txn;
    std::tie(env_, dbi_utxo_, dbi_reorg_pool_, dbi_reorg_index_) = open_dbs();

    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "3c5e48964a781208739ee27a9f78098930884d63f366f6e420983eb8bdbdda65", 0);
    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "fca0ef33d719be00d7cd38b1c7eaa2bcc23948b5be7f7a529f1a459c6a4d40b7", 0);
    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "ec30492fbf3258bde252ad921795d67066fd6e2aeba78b691d5589b57a8107cc", 0);
    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "d9d0ca00077e82590dcea565d04cd75d48075cbfe164ee73b258e78eefef1ff6", 0);
    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "68b0d5ad0616f24a1c7c7ec24dad236eaa9d369453e102dc1ffc7593c97d3215", 0);

    BOOST_REQUIRE(db_count_items(env_, dbi_reorg_pool_) == 5);
    BOOST_REQUIRE(db_count_items(env_, dbi_reorg_index_) == 5);

    BOOST_REQUIRE(db_count_index_by_height(env_, dbi_reorg_index_, 5) == 5);
    BOOST_REQUIRE(db_count_index_by_height(env_, dbi_reorg_index_, 8) == 0);

    check_index_and_pool(env_, dbi_reorg_index_, dbi_reorg_pool_);
}
 

BOOST_AUTO_TEST_CASE(utxo_database__reorg_index2) {

    // Block #4334
    // BlockHash 000000009cdad3c55df9c9bc88265329254a6c8ca810fa7f0e953c947df86dc7
    auto const b0 = get_block("010000005a0aeca67f7e43582c2b0138b2daa0c8dc1bedbb2477cfba2d3f96bf0000000065dabdbdb83e9820e4f666f3634d88308909789f7ae29e730812784a96485e3cd5899749ffff001dc2544c010101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d028e00ffffffff0100f2052a01000000434104b48f20398caaf3ff5d40710e0af87a4b86fd19d125ddacf15a2a023831d1731350e5fd40d0e28bb6481ad1843847213764feb98a2dd041069a8c39c842e1da93ac00000000");
    // Block #4966
    // BlockHash 000000004f6a440a95a5d2d6c89f3e6b46587cd43f76efbaa96ef5d37ea90961
    auto const b1 = get_block("010000002cba11cca7170e1da742fcfb83d15c091bac17a79c089e944dda904a00000000b7404d6a9c451a9f527a7fbeb54839c2bca2eac7b138cdd700be19d733efa0fc82609e49ffff001df66339030101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0121ffffffff0100f2052a01000000434104c20502b45fe276d418a32b55435cb4361dea4e173c36a8e0ad52518b17f2d48cde4336c8ac8d2270e6040469f11c56036db1feef803fb529e36d0f599261cb19ac00000000");
    // Block #4561
    // BlockHash 0000000089abf237d732a1515f7066e7ba29e1833664da8b704c8575c0465223
    auto const b2 = get_block("0100000052df1ff74876f2de37db341e12f932768f2a18dcc09dd024a1e676aa00000000cc07817ab589551d698ba7eb2a6efd6670d6951792ad52e2bd5832bf2f4930ecb5f19949ffff001d4045c6010101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d029f00ffffffff0100f2052a010000004341044bdc62d08cc074664cef09b24b01125a5e92fc2d61f821d6fddac367eb80b06a0a6148feabb0d25717f9eb84950ef0d3e7fe49ce7fb5a6e14da881a5b2bc64c0ac00000000");
    // Block #4991
    // BlockHash 00000000fa413253e1d30ff687f239b528330c810e3de86e42a538175682599d
    auto const b3 = get_block("0100000040eb019191a99f1f3ef5e04606314d80a635e214ca3347388259ad4000000000f61fefef8ee758b273ee64e1bf5c07485dd74cd065a5ce0d59827e0700cad0d98c9f9e49ffff001d20a92f010101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d013affffffff0100f2052a01000000434104debec4c8f781d5223a90aa90416ee51abf15d37086cc2d9be67873534fb06ae68c8a4e0ed0a7eedff9c97fe23f03e652d7f44501286dc1f75148bfaa9e386300ac00000000");
    // Block #4556
    // BlockHash 0000000054d4f171b0eab3cd4e31da4ce5a1a06f27b39bf36c5902c9bb8ef5c4
    auto const b4 = get_block("0100000021a06106f13b4f0112a63e77fae3a48ffe10716ff3cdfb35371032990000000015327dc99375fc1fdc02e15394369daa6e23ad4dc27e7c1c4af21606add5b068dadf9949ffff001dda8444000101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d029600ffffffff0100f2052a0100000043410475804bd8be2560ab2ebd170228429814d60e58d7415a93dc51f23e4cb2f8b5188dd56fa8d519e9ef9c16d6ab22c1c8304e10e74a28444eb26821948b2d1482a4ac00000000");
    // Block #9
    // BlockHash 000000008d9dc510f23c2657fc4f67bea30078cc05a90eb89e84cc475c080805
    auto const b5 = get_block("01000000c60ddef1b7618ca2348a46e868afc26e3efc68226c78aa47f8488c4000000000c997a5e56e104102fa209c6a852dd90660a20b2d9c352423edce25857fcd37047fca6649ffff001d28404f530101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0134ffffffff0100f2052a0100000043410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a6909a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b412a3ac00000000");


    // Block #5217
    // BlockHash 0000000025075f093c42a0393c844bc59f90024b18a9f588f6fa3fc37487c3c2
    auto const spender0 = get_block("01000000944bb801c604dda3d51758f292afdca8d973288434c8fe4bf0b5982d000000008a7d204ffe05282b05f280459401b59be41b089cefc911f4fb5641f90309d942b929a149ffff001d1b8d847f0301000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d02c500ffffffff0100f2052a01000000434104f4af426e464d972012256f4cbe5df528aa99b1ceb489968a56cf6b295e6fad1473be89f66fbd3d16adf3dfba7c5253517d11d1d188fe858720497c4fc0a1ef9dac00000000010000000465dabdbdb83e9820e4f666f3634d88308909789f7ae29e730812784a96485e3c000000004948304502204c52c2301dcc3f6af7a3ef2ad118185ca2d52a7ae90013332ad53732a085b8d4022100f074ab99e77d5d4c54eb6bfc82c42a094d7d7eaf632d52897ef058c617a2bb2301ffffffffb7404d6a9c451a9f527a7fbeb54839c2bca2eac7b138cdd700be19d733efa0fc000000004847304402206c55518aa596824d1e760afcfeb7b0103a1a82ea8dcd4c3474becc8246ba823702205009cbc40affa3414f9a139f38a86f81a401193f759fb514b9b1d4e2e49f82a401ffffffffcc07817ab589551d698ba7eb2a6efd6670d6951792ad52e2bd5832bf2f4930ec0000000049483045022100b485b4daa4af75b7b34b4f2338e7b96809c75fab5577905ade0789c7f821a69e022010d73d2a3c7fcfc6db911dead795b0aa7d5448447ad5efc7e516699955a18ac801fffffffff61fefef8ee758b273ee64e1bf5c07485dd74cd065a5ce0d59827e0700cad0d9000000004a493046022100bc6e89ee580d1c721b15c36d0a1218c9e78f6f7537616553341bbd1199fe615a02210093062f2c1a1c87f55b710011976a03dff57428e38dd640f6fbdef0fa52ad462d01ffffffff0100c817a80400000043410408998c08bbe6bba756e9b864722fe76ca403929382db2b120f9f621966b00af48f4b014b458bccd4f2acf63b1487ecb9547bc87bdecb08e9c4d08c138c76439aac00000000010000000115327dc99375fc1fdc02e15394369daa6e23ad4dc27e7c1c4af21606add5b068000000004a49304602210086b55b7f2fa5395d1e90a85115ada930afa01b86116d6bbeeecd8e2b97eefbac022100d653846d378845df2ced4b4923dcae4b6ddd5e8434b25e1602928235054c8d5301ffffffff0100f2052a01000000434104b68b035858a00051ca70dd4ba297168d9a3720b642c2e0cd08846bfbb144233b11b24c4b8565353b579bd7109800e42a1fc1e20dbdfbba6a12d0089aab313181ac00000000");

    // Block #170
    // BlockHash 00000000d1145790a8694403d4063f323d499e655c83426834d4ce2f8dd4a2ee
    auto const spender1 = get_block("0100000055bd840a78798ad0da853f68974f3d183e2bd1db6a842c1feecf222a00000000ff104ccb05421ab93e63f8c3ce5c2c2e9dbb37de2764b3a3175c8166562cac7d51b96a49ffff001d283e9e700201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0102ffffffff0100f2052a01000000434104d46c4968bde02899d2aa0963367c7a6ce34eec332b32e42e5f3407e052d64ac625da6f0718e7b302140434bd725706957c092db53805b821a85b23a7ac61725bac000000000100000001c997a5e56e104102fa209c6a852dd90660a20b2d9c352423edce25857fcd3704000000004847304402204e45e16932b8af514961a1d3a1a25fdf3f4f7732e9d624c6c61548ab5fb8cd410220181522ec8eca07de4860a4acdd12909d831cc56cbbac4622082221a8768d1d0901ffffffff0200ca9a3b00000000434104ae1a62fe09c5f51b13905f07f06b99a2f7159b2225f374cd378d71302fa28414e7aab37397f554a7df5f142c21c1b7303b8a0626f1baded5c72a704f7e6cd84cac00286bee0000000043410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a6909a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b412a3ac00000000");


    {
        utxo_database db(DIRECTORY "/utxo_db");
        BOOST_REQUIRE(db.open());
        BOOST_REQUIRE(db.push_block(b0, 0, 1) == utxo_code::success);              //TODO(fernando): median_time_past
        BOOST_REQUIRE(db.push_block(b1, 1, 1) == utxo_code::success);
        BOOST_REQUIRE(db.push_block(b2, 2, 1) == utxo_code::success);
        BOOST_REQUIRE(db.push_block(b3, 3, 1) == utxo_code::success);
        BOOST_REQUIRE(db.push_block(b4, 4, 1)      == utxo_code::success);
        BOOST_REQUIRE(db.push_block(b5, 5, 1)      == utxo_code::success);
        BOOST_REQUIRE(db.push_block(spender0, 6, 1) == utxo_code::success);
        BOOST_REQUIRE(db.push_block(spender1, 7, 1) == utxo_code::success);
    }   //close() implicit


    MDB_env* env_; MDB_dbi dbi_utxo_; MDB_dbi dbi_reorg_pool_; MDB_dbi dbi_reorg_index_; MDB_txn* db_txn;
    std::tie(env_, dbi_utxo_, dbi_reorg_pool_, dbi_reorg_index_) = open_dbs();

    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "3c5e48964a781208739ee27a9f78098930884d63f366f6e420983eb8bdbdda65", 0);
    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "fca0ef33d719be00d7cd38b1c7eaa2bcc23948b5be7f7a529f1a459c6a4d40b7", 0);
    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "ec30492fbf3258bde252ad921795d67066fd6e2aeba78b691d5589b57a8107cc", 0);
    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "d9d0ca00077e82590dcea565d04cd75d48075cbfe164ee73b258e78eefef1ff6", 0);
    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "68b0d5ad0616f24a1c7c7ec24dad236eaa9d369453e102dc1ffc7593c97d3215", 0);

    check_reorg_output_just_existence(env_, dbi_reorg_pool_, "0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9", 0);

    // BOOST_REQUIRE(db_count_items(env_, dbi_reorg_pool_) == 6);
    // BOOST_REQUIRE(db_count_items(env_, dbi_reorg_index_) == 6);

    BOOST_REQUIRE(db_count_index_by_height(env_, dbi_reorg_index_, 6) == 5);
    BOOST_REQUIRE(db_count_index_by_height(env_, dbi_reorg_index_, 7) == 1);
    BOOST_REQUIRE(db_count_index_by_height(env_, dbi_reorg_index_, 8) == 0);

    check_index_and_pool(env_, dbi_reorg_index_, dbi_reorg_pool_);
}


/*
------------------------------------------------------------------------------------------
Block #170
BlockHash 00000000d1145790a8694403d4063f323d499e655c83426834d4ce2f8dd4a2ee
0100000055bd840a78798ad0da853f68974f3d183e2bd1db6a842c1feecf222a00000000ff104ccb05421ab93e63f8c3ce5c2c2e9dbb37de2764b3a3175c8166562cac7d51b96a49ffff001d283e9e700201000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0102ffffffff0100f2052a01000000434104d46c4968bde02899d2aa0963367c7a6ce34eec332b32e42e5f3407e052d64ac625da6f0718e7b302140434bd725706957c092db53805b821a85b23a7ac61725bac000000000100000001c997a5e56e104102fa209c6a852dd90660a20b2d9c352423edce25857fcd3704000000004847304402204e45e16932b8af514961a1d3a1a25fdf3f4f7732e9d624c6c61548ab5fb8cd410220181522ec8eca07de4860a4acdd12909d831cc56cbbac4622082221a8768d1d0901ffffffff0200ca9a3b00000000434104ae1a62fe09c5f51b13905f07f06b99a2f7159b2225f374cd378d71302fa28414e7aab37397f554a7df5f142c21c1b7303b8a0626f1baded5c72a704f7e6cd84cac00286bee0000000043410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a6909a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b412a3ac00000000


Tx0: Coinbase

Tx1
    hash:           f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16
    bytes hexa:     0100000001c997a5e56e104102fa209c6a852dd90660a20b2d9c352423edce25857fcd3704000000004847304402204e45e16932b8af514961a1d3a1a25fdf3f4f7732e9d624c6c61548ab5fb8cd410220181522ec8eca07de4860a4acdd12909d831cc56cbbac4622082221a8768d1d0901ffffffff0200ca9a3b00000000434104ae1a62fe09c5f51b13905f07f06b99a2f7159b2225f374cd378d71302fa28414e7aab37397f554a7df5f142c21c1b7303b8a0626f1baded5c72a704f7e6cd84cac00286bee0000000043410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a6909a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b412a3ac00000000
    prev out:       0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9        Block #9


------------------------------------------------------------------------------------------
Block #9
BlockHash 000000008d9dc510f23c2657fc4f67bea30078cc05a90eb89e84cc475c080805
01000000c60ddef1b7618ca2348a46e868afc26e3efc68226c78aa47f8488c4000000000c997a5e56e104102fa209c6a852dd90660a20b2d9c352423edce25857fcd37047fca6649ffff001d28404f530101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0134ffffffff0100f2052a0100000043410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a6909a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b412a3ac00000000

------------------------------------------------------------------------------------------
Block #5217
BlockHash 0000000025075f093c42a0393c844bc59f90024b18a9f588f6fa3fc37487c3c2
01000000944bb801c604dda3d51758f292afdca8d973288434c8fe4bf0b5982d000000008a7d204ffe05282b05f280459401b59be41b089cefc911f4fb5641f90309d942b929a149ffff001d1b8d847f0301000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d02c500ffffffff0100f2052a01000000434104f4af426e464d972012256f4cbe5df528aa99b1ceb489968a56cf6b295e6fad1473be89f66fbd3d16adf3dfba7c5253517d11d1d188fe858720497c4fc0a1ef9dac00000000010000000465dabdbdb83e9820e4f666f3634d88308909789f7ae29e730812784a96485e3c000000004948304502204c52c2301dcc3f6af7a3ef2ad118185ca2d52a7ae90013332ad53732a085b8d4022100f074ab99e77d5d4c54eb6bfc82c42a094d7d7eaf632d52897ef058c617a2bb2301ffffffffb7404d6a9c451a9f527a7fbeb54839c2bca2eac7b138cdd700be19d733efa0fc000000004847304402206c55518aa596824d1e760afcfeb7b0103a1a82ea8dcd4c3474becc8246ba823702205009cbc40affa3414f9a139f38a86f81a401193f759fb514b9b1d4e2e49f82a401ffffffffcc07817ab589551d698ba7eb2a6efd6670d6951792ad52e2bd5832bf2f4930ec0000000049483045022100b485b4daa4af75b7b34b4f2338e7b96809c75fab5577905ade0789c7f821a69e022010d73d2a3c7fcfc6db911dead795b0aa7d5448447ad5efc7e516699955a18ac801fffffffff61fefef8ee758b273ee64e1bf5c07485dd74cd065a5ce0d59827e0700cad0d9000000004a493046022100bc6e89ee580d1c721b15c36d0a1218c9e78f6f7537616553341bbd1199fe615a02210093062f2c1a1c87f55b710011976a03dff57428e38dd640f6fbdef0fa52ad462d01ffffffff0100c817a80400000043410408998c08bbe6bba756e9b864722fe76ca403929382db2b120f9f621966b00af48f4b014b458bccd4f2acf63b1487ecb9547bc87bdecb08e9c4d08c138c76439aac00000000010000000115327dc99375fc1fdc02e15394369daa6e23ad4dc27e7c1c4af21606add5b068000000004a49304602210086b55b7f2fa5395d1e90a85115ada930afa01b86116d6bbeeecd8e2b97eefbac022100d653846d378845df2ced4b4923dcae4b6ddd5e8434b25e1602928235054c8d5301ffffffff0100f2052a01000000434104b68b035858a00051ca70dd4ba297168d9a3720b642c2e0cd08846bfbb144233b11b24c4b8565353b579bd7109800e42a1fc1e20dbdfbba6a12d0089aab313181ac00000000


Tx0: Coinbase

Tx1
    hash:           9fa5efd12e4bdba914bf1acd03981c6e31eabaa8a8bd85fc2be36afe5a787c06
    bytes hexa:     010000000465dabdbdb83e9820e4f666f3634d88308909789f7ae29e730812784a96485e3c000000004948304502204c52c2301dcc3f6af7a3ef2ad118185ca2d52a7ae90013332ad53732a085b8d4022100f074ab99e77d5d4c54eb6bfc82c42a094d7d7eaf632d52897ef058c617a2bb2301ffffffffb7404d6a9c451a9f527a7fbeb54839c2bca2eac7b138cdd700be19d733efa0fc000000004847304402206c55518aa596824d1e760afcfeb7b0103a1a82ea8dcd4c3474becc8246ba823702205009cbc40affa3414f9a139f38a86f81a401193f759fb514b9b1d4e2e49f82a401ffffffffcc07817ab589551d698ba7eb2a6efd6670d6951792ad52e2bd5832bf2f4930ec0000000049483045022100b485b4daa4af75b7b34b4f2338e7b96809c75fab5577905ade0789c7f821a69e022010d73d2a3c7fcfc6db911dead795b0aa7d5448447ad5efc7e516699955a18ac801fffffffff61fefef8ee758b273ee64e1bf5c07485dd74cd065a5ce0d59827e0700cad0d9000000004a493046022100bc6e89ee580d1c721b15c36d0a1218c9e78f6f7537616553341bbd1199fe615a02210093062f2c1a1c87f55b710011976a03dff57428e38dd640f6fbdef0fa52ad462d01ffffffff0100c817a80400000043410408998c08bbe6bba756e9b864722fe76ca403929382db2b120f9f621966b00af48f4b014b458bccd4f2acf63b1487ecb9547bc87bdecb08e9c4d08c138c76439aac00000000
    prev out:       3c5e48964a781208739ee27a9f78098930884d63f366f6e420983eb8bdbdda65        Block #4334
                    fca0ef33d719be00d7cd38b1c7eaa2bcc23948b5be7f7a529f1a459c6a4d40b7        Block #4966
                    ec30492fbf3258bde252ad921795d67066fd6e2aeba78b691d5589b57a8107cc        Block #4561
                    d9d0ca00077e82590dcea565d04cd75d48075cbfe164ee73b258e78eefef1ff6        Block #4991

Tx2
    hash:           bea2f18440827950003bfcc130d0a82ebff013cec3848cdc8cb1c06e15672333
    bytes hexa:     010000000115327dc99375fc1fdc02e15394369daa6e23ad4dc27e7c1c4af21606add5b068000000004a49304602210086b55b7f2fa5395d1e90a85115ada930afa01b86116d6bbeeecd8e2b97eefbac022100d653846d378845df2ced4b4923dcae4b6ddd5e8434b25e1602928235054c8d5301ffffffff0100f2052a01000000434104b68b035858a00051ca70dd4ba297168d9a3720b642c2e0cd08846bfbb144233b11b24c4b8565353b579bd7109800e42a1fc1e20dbdfbba6a12d0089aab313181ac00000000
    prev out:       68b0d5ad0616f24a1c7c7ec24dad236eaa9d369453e102dc1ffc7593c97d3215        Block #4556



------------------------------------------------------------------------------------------
Block #4334
BlockHash 000000009cdad3c55df9c9bc88265329254a6c8ca810fa7f0e953c947df86dc7
010000005a0aeca67f7e43582c2b0138b2daa0c8dc1bedbb2477cfba2d3f96bf0000000065dabdbdb83e9820e4f666f3634d88308909789f7ae29e730812784a96485e3cd5899749ffff001dc2544c010101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d028e00ffffffff0100f2052a01000000434104b48f20398caaf3ff5d40710e0af87a4b86fd19d125ddacf15a2a023831d1731350e5fd40d0e28bb6481ad1843847213764feb98a2dd041069a8c39c842e1da93ac00000000

Block #4966
BlockHash 000000004f6a440a95a5d2d6c89f3e6b46587cd43f76efbaa96ef5d37ea90961
010000002cba11cca7170e1da742fcfb83d15c091bac17a79c089e944dda904a00000000b7404d6a9c451a9f527a7fbeb54839c2bca2eac7b138cdd700be19d733efa0fc82609e49ffff001df66339030101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0121ffffffff0100f2052a01000000434104c20502b45fe276d418a32b55435cb4361dea4e173c36a8e0ad52518b17f2d48cde4336c8ac8d2270e6040469f11c56036db1feef803fb529e36d0f599261cb19ac00000000

Block #4561
BlockHash 0000000089abf237d732a1515f7066e7ba29e1833664da8b704c8575c0465223
0100000052df1ff74876f2de37db341e12f932768f2a18dcc09dd024a1e676aa00000000cc07817ab589551d698ba7eb2a6efd6670d6951792ad52e2bd5832bf2f4930ecb5f19949ffff001d4045c6010101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d029f00ffffffff0100f2052a010000004341044bdc62d08cc074664cef09b24b01125a5e92fc2d61f821d6fddac367eb80b06a0a6148feabb0d25717f9eb84950ef0d3e7fe49ce7fb5a6e14da881a5b2bc64c0ac00000000

Block #4991
BlockHash 00000000fa413253e1d30ff687f239b528330c810e3de86e42a538175682599d
0100000040eb019191a99f1f3ef5e04606314d80a635e214ca3347388259ad4000000000f61fefef8ee758b273ee64e1bf5c07485dd74cd065a5ce0d59827e0700cad0d98c9f9e49ffff001d20a92f010101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d013affffffff0100f2052a01000000434104debec4c8f781d5223a90aa90416ee51abf15d37086cc2d9be67873534fb06ae68c8a4e0ed0a7eedff9c97fe23f03e652d7f44501286dc1f75148bfaa9e386300ac00000000

Block #4556
BlockHash 0000000054d4f171b0eab3cd4e31da4ce5a1a06f27b39bf36c5902c9bb8ef5c4
0100000021a06106f13b4f0112a63e77fae3a48ffe10716ff3cdfb35371032990000000015327dc99375fc1fdc02e15394369daa6e23ad4dc27e7c1c4af21606add5b068dadf9949ffff001dda8444000101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0804ffff001d029600ffffffff0100f2052a0100000043410475804bd8be2560ab2ebd170228429814d60e58d7415a93dc51f23e4cb2f8b5188dd56fa8d519e9ef9c16d6ab22c1c8304e10e74a28444eb26821948b2d1482a4ac00000000

*/

// #endif // BITPRIM_DB_NEW

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <filesystem>

#include <test_helpers.hpp>
#include <kth/database.hpp>

using namespace boost::system;
using namespace std::filesystem;
using namespace kth::domain::chain;
using namespace kth::database;

transaction random_tx(size_t fudge) {
    static auto const genesis = block::genesis_mainnet();
    auto result = genesis.transactions()[0];
    result.inputs()[0].previous_output().set_index(fudge);
    return result;
}

#define DIRECTORY "block_database"

class block_database_directory_setup_fixture {
public:
    block_database_directory_setup_fixture() {
        std::error_code ec;
        remove_all(DIRECTORY, ec);
        REQUIRE(create_directories(DIRECTORY, ec));
    }

    ////~block_database_directory_setup_fixture()
    ////{
    ////    error_code ec;
    ////    remove_all(DIRECTORY, ec);
    ////}
};

BOOST_FIXTURE_TEST_SUITE(database_tests, block_database_directory_setup_fixture)

#ifdef KTH_DB_LEGACY
TEST_CASE("block database  test", "[None]") {
    auto block0 = block::genesis_mainnet();
    block0.transactions().push_back(random_tx(0));
    block0.transactions().push_back(random_tx(1));
    //auto const h0 = block0.header.hash();

    block block1;
    block1.set_header(header(block0.header()));
    block1.header().set_nonce(4);
    block1.transactions().push_back(random_tx(2));
    block1.transactions().push_back(random_tx(3));
    block1.transactions().push_back(random_tx(4));
    block1.transactions().push_back(random_tx(5));
    //auto const h1 = block1.header.hash();

    block block2;
    block2.set_header(header(block0.header()));
    block2.header().set_nonce(110);
    block2.transactions().push_back(random_tx(6));
    block2.transactions().push_back(random_tx(7));
    block2.transactions().push_back(random_tx(8));
    block2.transactions().push_back(random_tx(9));
    block2.transactions().push_back(random_tx(10));
    auto const h2 = block2.header().hash();

    block block3;
    block3.set_header(header(block0.header()));
    block3.header().set_nonce(88);
    block3.transactions().push_back(random_tx(11));
    block3.transactions().push_back(random_tx(12));
    block3.transactions().push_back(random_tx(13));
    //auto const h3 = block3.header().hash();

    block block4a;
    block4a.set_header(header(block0.header()));
    block4a.header().set_nonce(63);
    block4a.transactions().push_back(random_tx(14));
    block4a.transactions().push_back(random_tx(15));
    block4a.transactions().push_back(random_tx(16));
    auto const h4a = block4a.header().hash();

    block block5a;
    block5a.set_header(header(block0.header()));
    block5a.header().set_nonce(99);
    block5a.transactions().push_back(random_tx(17));
    block5a.transactions().push_back(random_tx(18));
    block5a.transactions().push_back(random_tx(19));
    block5a.transactions().push_back(random_tx(20));
    block5a.transactions().push_back(random_tx(21));
    auto const h5a = block5a.header().hash();

    block block4b;
    block4b.set_header(header(block0.header()));
    block4b.header().set_nonce(633);
    block4b.transactions().push_back(random_tx(22));
    block4b.transactions().push_back(random_tx(23));
    block4b.transactions().push_back(random_tx(24));
    auto const h4b = block4b.header().hash();

    block block5b;
    block5b.set_header(header(block0.header()));
    block5b.header().set_nonce(222);
    block5b.transactions().push_back(random_tx(25));
    block5b.transactions().push_back(random_tx(26));
    block5b.transactions().push_back(random_tx(27));
    block5b.transactions().push_back(random_tx(28));
    block5b.transactions().push_back(random_tx(29));
    auto const h5b = block5b.header().hash();

    store::create(DIRECTORY "/block_lookup");
    store::create(DIRECTORY "/block_rows");
    block_database db(DIRECTORY "/block_lookup", DIRECTORY "/block_rows", 1000, 50);
    REQUIRE(db.create());

    size_t height;
    REQUIRE( ! db.top(height));

    db.store(block0, 0);
    db.store(block1, 1);
    db.store(block2, 2);
    db.store(block3, 3);
    REQUIRE(db.top(height));
    REQUIRE(height == 3u);

    // Fetch block 2 by hash.
    {
        auto res_h2 = db.get(h2);
        REQUIRE(res_h2);
        REQUIRE(res_h2.header().hash() == h2);
        REQUIRE(res_h2.transaction_hash(0) == block2.transactions()[0].hash());
        REQUIRE(res_h2.transaction_hash(1) == block2.transactions()[1].hash());
        REQUIRE(res_h2.transaction_hash(2) == block2.transactions()[2].hash());
        REQUIRE(res_h2.transaction_hash(3) == block2.transactions()[3].hash());
        REQUIRE(res_h2.transaction_hash(4) == block2.transactions()[4].hash());
    }

    // Try a fork event.
    db.store(block4a, 4);
    db.store(block5a, 5);

    // Fetch blocks.
    {
        auto result4a = db.get(4);
        REQUIRE(result4a);
        REQUIRE(result4a.header().hash() == h4a);
        auto res5a = db.get(5);
        REQUIRE(res5a);
        REQUIRE(res5a.header().hash() == h5a);
    }

    // Unlink old chain.
    REQUIRE(db.top(height));
    REQUIRE(height == 5u);
    db.unlink(4);
    REQUIRE(db.top(height));
    REQUIRE(height == 3u);

    // Block 3 exists.
    {
        auto result3 = db.get(3);
        REQUIRE(result3);
    }

    // No blocks exist now.
    {
        auto result4_none = db.get(4);
        REQUIRE( ! result4_none);
        auto res5_none = db.get(5);
        REQUIRE( ! res5_none);
    }

    // Add new blocks.
    db.store(block4b, 4);
    db.store(block5b, 5);
    REQUIRE(db.top(height));
    REQUIRE(height == 5u);

    // Fetch blocks.
    {
        auto result4b = db.get(4);
        REQUIRE(result4b);
        REQUIRE(result4b.header().hash() == h4b);
        auto res5b = db.get(5);
        REQUIRE(res5b);
        REQUIRE(res5b.header().hash() == h5b);
        REQUIRE(res5b.transaction_hash(0) == block5b.transactions()[0].hash());
        REQUIRE(res5b.transaction_hash(1) == block5b.transactions()[1].hash());
        REQUIRE(res5b.transaction_hash(2) == block5b.transactions()[2].hash());
        REQUIRE(res5b.transaction_hash(3) == block5b.transactions()[3].hash());
        REQUIRE(res5b.transaction_hash(4) == block5b.transactions()[4].hash());
    }

    // Test also fetch by hash.
    {
        auto res_h5b = db.get(h5b);
        REQUIRE(res_h5b);
        REQUIRE(res_h5b.header().hash() == h5b);
        db.synchronize();
    }
}
#endif // KTH_DB_LEGACY

// End Test Suite

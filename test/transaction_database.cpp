// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::chain;
using namespace bc::database;

#define DIRECTORY "transaction_database"

class transaction_database_directory_setup_fixture
{
public:
    transaction_database_directory_setup_fixture()
    {
        error_code ec;
        remove_all(DIRECTORY, ec);
        BOOST_REQUIRE(create_directories(DIRECTORY, ec));
    }

    ////~transaction_database_directory_setup_fixture()
    ////{
    ////    error_code ec;
    ////    remove_all(DIRECTORY, ec);
    ////}
};

BOOST_FIXTURE_TEST_SUITE(database_tests, transaction_database_directory_setup_fixture)

#ifdef KTH_DB_LEGACY
BOOST_AUTO_TEST_CASE(transaction_database__test)
{
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, "0100000001537c9d05b5f7d67b09e5108e3bd5e466909cc9403ddd98bc42973f366fe729410600000000ffffffff0163000000000000001976a914fe06e7b4c88a719e92373de489c08244aee4520b88ac00000000"));

    transaction tx1;
    BOOST_REQUIRE(tx1.from_data(wire_tx1, true));

    auto const h1 = tx1.hash();

    data_chunk wire_tx2;
    BOOST_REQUIRE(decode_base16(wire_tx2, "010000000147811c3fc0c0e750af5d0ea7343b16ea2d0c291c002e3db778669216eb689de80000000000ffffffff0118ddf505000000001976a914575c2f0ea88fcbad2389a372d942dea95addc25b88ac00000000"));

    transaction tx2;
    BOOST_REQUIRE(tx2.from_data(wire_tx2, true));

    auto const h2 = tx2.hash();

    store::create(DIRECTORY "/transaction");
    transaction_database db(DIRECTORY "/transaction", 1000, 50, 0);
    BOOST_REQUIRE(db.create());

    db.store(tx1, 110, 0, 88);
    db.store(tx2, 4, 0, 6);

    auto const result1 = db.get(h1, max_size_t, false);
    BOOST_REQUIRE(result1.transaction().hash() == h1);

    auto const result2 = db.get(h2, max_size_t, false);
    BOOST_REQUIRE(result2.transaction().hash() == h2);

    db.synchronize();
}
#endif // KTH_DB_LEGACY

BOOST_AUTO_TEST_SUITE_END()

// Copyright (c) 2016-2022 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test_helpers.hpp>

#include <filesystem>
#include <future>
#include <memory>

#include <kth/database.hpp>

using namespace kth::domain::chain;
using namespace kth::database;
using namespace kth::domain::wallet;
using namespace boost::system;
using namespace std::filesystem;

#ifdef KTH_DB_LEGACY
void test_block_exists(data_base const& interface, size_t height, block const& block0, bool indexed) {
#ifdef KTH_DB_HISTORY
    auto const& history_store = interface.history();
#endif // KTH_DB_HISTORY

    auto const block_hash = block0.hash();
    auto r0 = interface.blocks().get(height);
    auto r0_byhash = interface.blocks().get(block_hash);

    REQUIRE(r0);
    REQUIRE(r0_byhash);
    REQUIRE(r0.hash() == block_hash);
    REQUIRE(r0_byhash.hash() == block_hash);
    REQUIRE(r0.height() == height);
    REQUIRE(r0_byhash.height() == height);
    REQUIRE(r0.transaction_count() == block0.transactions().size());
    REQUIRE(r0_byhash.transaction_count() == block0.transactions().size());

    // TODO: test tx hashes.

    for (size_t i = 0; i < block0.transactions().size(); ++i) {
        auto const& tx = block0.transactions()[i];
        auto const tx_hash = tx.hash();
        REQUIRE(r0.transaction_hash(i) == tx_hash);
        REQUIRE(r0_byhash.transaction_hash(i) == tx_hash);

        auto r0_tx = interface.transactions().get(tx_hash, max_size_t, false);
        REQUIRE(r0_tx);
        REQUIRE(r0_byhash);
        REQUIRE(r0_tx.transaction().hash() == tx_hash);
        REQUIRE(r0_tx.height() == height);
        REQUIRE(r0_tx.position() == i);

        if ( ! tx.is_coinbase()) {
            for (uint32_t j = 0; j < tx.inputs().size(); ++j) {
                auto const& input = tx.inputs()[j];
                input_point spend{ tx_hash, j };
                REQUIRE(spend.index() == j);

#ifdef KTH_DB_SPENDS
                auto r0_spend = interface.spends().get(input.previous_output());
                REQUIRE(r0_spend.is_valid());
                REQUIRE(r0_spend.hash() == spend.hash());
                REQUIRE(r0_spend.index() == spend.index());
#endif // KTH_DB_SPENDS

                if ( ! indexed) {
                    continue;
                }

                auto const addresses = input.addresses();
                ////auto const& prevout = input.previous_output();
                ////auto const address = prevout.validation.cache.addresses();

#ifdef KTH_DB_HISTORY
                for (auto const& address : addresses) {
                    auto history = history_store.get(address.hash(), 0, 0);
                    auto found = false;

                    for (auto const& row: history) {
                        if (row.point == spend) {
                            REQUIRE(row.height == height);
                            found = true;
                            break;
                        }
                    }

                    REQUIRE(found);
                }
#endif // KTH_DB_HISTORY                
            }
        }

        if ( ! indexed) {
            return;
        }

        for (size_t j = 0; j < tx.outputs().size(); ++j) {
            auto const& output = tx.outputs()[j];
            output_point outpoint{ tx_hash, static_cast<uint32_t>(j) };
            auto const addresses = output.addresses();

#ifdef KTH_DB_HISTORY
            for (auto const& address : addresses) {
                auto history = history_store.get(address.hash(), 0, 0);
                auto found = false;

                for (auto const& row: history) {
                    REQUIRE(row.is_valid());

                    if (row.point == outpoint) {
                        REQUIRE(row.height == height);
                        REQUIRE(row.value == output.value());
                        found = true;
                        break;
                    }
                }

                REQUIRE(found);
            }
#endif // KTH_DB_HISTORY
        }
    }
}


void test_block_not_exists(data_base const& interface, block const& block0, bool indexed) {
#ifdef KTH_DB_HISTORY
    auto const& history_store = interface.history();
#endif // KTH_DB_HISTORY

    ////auto const r0_byhash = interface.blocks().get(block0.hash());
REQUIRE( ! r0_byhash);
    for (size_t i = 0; i < block0.transactions().size(); ++i) {
        auto const& tx = block0.transactions()[i];
        auto const tx_hash = tx.hash();

        if ( ! tx.is_coinbase()) {
            for (size_t j = 0; j < tx.inputs().size(); ++j) {
                auto const& input = tx.inputs()[j];
                input_point spend{ tx_hash, static_cast<uint32_t>(j) };

#ifdef KTH_DB_SPENDS                
                auto r0_spend = interface.spends().get(input.previous_output());
                REQUIRE( ! r0_spend.is_valid());
#endif // KTH_DB_SPENDS

                if ( ! indexed) {
                    continue;
                }

                auto const addresses = input.addresses();
                ////auto const& prevout = input.previous_output();
                ////auto const address = prevout.validation.cache.addresses();

#ifdef KTH_DB_HISTORY
                for (auto const& address : addresses) {
                    auto history = history_store.get(address.hash(), 0, 0);
                    auto found = false;

                    for (auto const& row: history) {
                        if (row.point == spend) {
                            found = true;
                            break;
                        }
                    }

                    REQUIRE( ! found);
                }
#endif // KTH_DB_HISTORY                
            }
        }

        if ( ! indexed) {
            return;
        }

        for (size_t j = 0; j < tx.outputs().size(); ++j) {
            auto const& output = tx.outputs()[j];
            output_point outpoint{ tx_hash, static_cast<uint32_t>(j) };
            auto const addresses = output.addresses();

#ifdef KTH_DB_HISTORY
            for (auto const& address : addresses) {
                auto history = history_store.get(address.hash(), 0, 0);
                auto found = false;

                for (auto const& row: history) {
                    if (row.point == outpoint) {
                        found = true;
                        break;
                    }
                }

                REQUIRE( ! found);
            }
#endif // KTH_DB_HISTORY            
        }
    }
}
#endif // KTH_DB_LEGACY

block read_block(const std::string hex) {
    data_chunk data;
    REQUIRE(decode_base16(data, hex));
    block result;
    REQUIRE(result.from_data(data));
    return result;
}

#define DIRECTORY "data_base"

class data_base_setup_fixture {
public:
    data_base_setup_fixture() {
        error_code ec;
        remove_all(DIRECTORY, ec);
        REQUIRE(create_directories(DIRECTORY, ec));
    }
};

BOOST_FIXTURE_TEST_SUITE(data_base_tests, data_base_setup_fixture)

#define MAINNET_BLOCK1 \
    "010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000982" \
    "051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e857233e0e61bc6649ffff00" \
    "1d01e3629901010000000100000000000000000000000000000000000000000000000000000" \
    "00000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e8" \
    "53519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a60" \
    "4f8141781e62294721166bf621e73a82cbf2342c858eeac00000000"

#define MAINNET_BLOCK2 \
    "010000004860eb18bf1b1620e37e9490fc8a427514416fd75159ab86688e9a8300000000d5f" \
    "dcc541e25de1c7a5addedf24858b8bb665c9f36ef744ee42c316022c90f9bb0bc6649ffff00" \
    "1d08d2bd6101010000000100000000000000000000000000000000000000000000000000000" \
    "00000000000ffffffff0704ffff001d010bffffffff0100f2052a010000004341047211a824" \
    "f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073dee6c89064984f03385" \
    "237d92167c13e236446b417ab79a0fcae412ae3316b77ac00000000"

#define MAINNET_BLOCK3 \
    "01000000bddd99ccfda39da1b108ce1a5d70038d0a967bacb68b6b63065f626a0000000044f" \
    "672226090d85db9a9f2fbfe5f0f9609b387af7be5b7fbb7a1767c831c9e995dbe6649ffff00" \
    "1d05e0ed6d01010000000100000000000000000000000000000000000000000000000000000" \
    "00000000000ffffffff0704ffff001d010effffffff0100f2052a0100000043410494b9d3e7" \
    "6c5b1629ecf97fff95d7a4bbdac87cc26099ada28066c6ff1eb9191223cd897194a08d0c272" \
    "6c5747f1db49e8cf90e75dc3e3550ae9b30086f3cd5aaac00000000"

class data_base_accessor : public data_base {
public:
    data_base_accessor(settings const& settings)
        : data_base(settings) {}

    void push_all(block_const_ptr_list_const_ptr in_blocks, size_t first_height, dispatcher& dispatch, result_handler handler) {
        data_base::push_all(in_blocks, first_height, dispatch, handler);
    }

    void pop_above(block_const_ptr_list_ptr out_blocks, hash_digest const& fork_hash, dispatcher& dispatch, result_handler handler) {
        data_base::pop_above(out_blocks, fork_hash, dispatch, handler);
    }
};

static 
code push_all_result(data_base_accessor& instance, block_const_ptr_list_const_ptr in_blocks, size_t first_height, dispatcher& dispatch) {
    std::promise<code> promise;
    auto const handler = [&promise](code ec) {
        promise.set_value(ec);
    };
    instance.push_all(in_blocks, first_height, dispatch, handler);
    return promise.get_future().get();
}

static 
code pop_above_result(data_base_accessor& instance, block_const_ptr_list_ptr out_blocks, hash_digest const& fork_hash, dispatcher& dispatch) {
    std::promise<code> promise;
    auto const handler = [&promise](code ec) {
        promise.set_value(ec);
    };
    instance.pop_above(out_blocks, fork_hash, dispatch, handler);
    return promise.get_future().get();
}

#ifdef KTH_DB_LEGACY
TEST_CASE("data base  pushpop  test", "[None]") {
    std::cout << "begin data_base push/pop test" << std::endl;

    create_directory(DIRECTORY);
    database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.index_start_height = 0;

#ifdef KTH_DB_LEGACY
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
#endif // KTH_DB_LEGACY

#ifdef KTH_DB_SPENDS
    settings.spend_table_buckets = 42;
#endif // KTH_DB_SPENDS

#ifdef KTH_DB_HISTORY
    settings.history_table_buckets = 42;
#endif // KTH_DB_HISTORY

#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    settings.transaction_unconfirmed_table_buckets = 42;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED    
     

    // If index_height is set to anything other than 0 or max it can cause
    // false negatives since it excludes entries below the specified height.
    auto const indexed = settings.index_start_height < store::without_indexes;

    size_t height;
    threadpool pool(1);
    dispatcher dispatch(pool, "test");
    data_base_accessor instance(settings);
    auto const block0 = block::genesis_mainnet();
    REQUIRE(instance.create(block0));
    test_block_exists(instance, 0, block0, indexed);
    REQUIRE(instance.blocks().top(height));
    REQUIRE(height == 0);

    // This tests a missing parent, not a database failure.
    // A database failure would prevent subsequent read/write operations.
    std::cout << "push block #1 (store_block_missing_parent)" << std::endl;
    auto invalid_block1 = read_block(MAINNET_BLOCK1);
    invalid_block1.set_header(domain::chain::header{});
    REQUIRE(instance.push(invalid_block1 == 1), error::store_block_missing_parent);

    std::cout << "push block #1" << std::endl;
    auto const block1 = read_block(MAINNET_BLOCK1);
    test_block_not_exists(instance, block1, indexed);
    REQUIRE(instance.push(block1 == 1), error::success);
    test_block_exists(instance, 0, block0, indexed);
    REQUIRE(instance.blocks().top(height));
    REQUIRE(height == 1u);
    test_block_exists(instance, 1, block1, indexed);

    std::cout << "push_all blocks #2 & #3" << std::endl;
    auto const block2_ptr = std::make_shared<domain::message::block const>(read_block(MAINNET_BLOCK2));
    auto const block3_ptr = std::make_shared<domain::message::block const>(read_block(MAINNET_BLOCK3));
    auto const blocks_push_ptr = std::make_shared<const block_const_ptr_list>(block_const_ptr_list{ block2_ptr, block3_ptr });
    test_block_not_exists(instance, *block2_ptr, indexed);
    test_block_not_exists(instance, *block3_ptr, indexed);
    REQUIRE(push_all_result(instance == blocks_push_ptr, 2, dispatch), error::success);
    test_block_exists(instance, 1, block1, indexed);
    REQUIRE(instance.blocks().top(height));
    REQUIRE(height == 3u);
    test_block_exists(instance, 3, *block3_ptr, indexed);
    test_block_exists(instance, 2, *block2_ptr, indexed);

    std::cout << "insert block #2 (store_block_duplicate)" << std::endl;
    REQUIRE(instance.insert(*block2_ptr == 2), error::store_block_duplicate);

    std::cout << "pop_above block 1 (blocks #2 & #3)" << std::endl;
    auto const blocks_popped_ptr = std::make_shared<block_const_ptr_list>();
    REQUIRE(pop_above_result(instance == blocks_popped_ptr, block1.hash(), dispatch), error::success);
    REQUIRE(instance.blocks().top(height));
    REQUIRE(height == 1u);
    REQUIRE(blocks_popped_ptr->size() == 2u);
    REQUIRE(*(*blocks_popped_ptr)[0] == *block2_ptr);
    REQUIRE(*(*blocks_popped_ptr)[1] == *block3_ptr);
    test_block_not_exists(instance, *block3_ptr, indexed);
    test_block_not_exists(instance, *block2_ptr, indexed);
    test_block_exists(instance, 1, block1, indexed);
    test_block_exists(instance, 0, block0, indexed);

    std::cout << "push block #3 (store_block_invalid_height)" << std::endl;
    REQUIRE(instance.push(*block3_ptr == 3), error::store_block_invalid_height);

    std::cout << "insert block #2" << std::endl;
    REQUIRE(instance.insert(*block2_ptr == 2), error::success);
    REQUIRE(instance.blocks().top(height));
    REQUIRE(height == 2u);

    std::cout << "pop_above block 0 (block #1 & #2)" << std::endl;
    blocks_popped_ptr->clear();
    REQUIRE(pop_above_result(instance == blocks_popped_ptr, block0.hash(), dispatch), error::success);
    REQUIRE(instance.blocks().top(height));
    REQUIRE(height == 0u);
    REQUIRE(*(*blocks_popped_ptr)[0] == block1);
    REQUIRE(*(*blocks_popped_ptr)[1] == *block2_ptr);
    test_block_not_exists(instance, block1, indexed);
    test_block_not_exists(instance, *block2_ptr, indexed);
    test_block_exists(instance, 0, block0, indexed);

    std::cout << "end push/pop test" << std::endl;
}
#endif // KTH_DB_LEGACY

// End Boost Suite

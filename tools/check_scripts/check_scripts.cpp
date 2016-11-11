//Execution:
//  cd /opt/libbitcoin3
//  /opt/devel/bitprim-cmake/bitprim-build/build/bitprim-database/tools.check_scripts mainnet-blockchain/block_table mainnet-blockchain/block_index mainnet-blockchain/transaction_table

#include <cstddef> //std::size_t



    

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <bitcoin/database.hpp>

using namespace boost;
using namespace bc;
using namespace bc::database;

void print_bytes(uint8_t const* data, size_t n) {
    while (n != 0) {
        printf("%02x", *data);
        ++data;
        --n;
    }
    printf("\n");
}

opcodetype get_opcode(uint8_t* f, uint8_t* l) {
    if (f == l)
        return OP_INVALIDOPCODE;

    uint32_t opcode = *f++;

    // Immediate operand
    if (opcode <= OP_PUSHDATA4) {
        
        size_t nSize = 0;

        if (opcode < OP_PUSHDATA1) {
            nSize = opcode;
        } else if (opcode == OP_PUSHDATA1) {
            if (f == l)
                return OP_INVALIDOPCODE;
            nSize = *f++;
        } else if (opcode == OP_PUSHDATA2) {
            if (std::distance(f, l) < 2)
                return OP_INVALIDOPCODE;
            nSize = ReadLE16(f);
            f += 2;
        } else if (opcode == OP_PUSHDATA4) {
            if (std::distance(f, l) < 4)
                return OP_INVALIDOPCODE;
            nSize = ReadLE32(f);
            f += 4;
        }

        //TODO: Fer: redundant check
        if (std::distance(f, l) < 0 || std::distance(f, l) < nSize)
            return OP_INVALIDOPCODE;

        f += nSize;
    }

    return (opcodetype)opcode;
}


bool evaluate_script(std::vector<td::vector<uint8_t>>& stack, uint8_t* data, size_t size) {

    size_t i = 0;
    while (i < size) {
        auto opcode = get_opcode(data, data + size);
        if (opcode == OP_INVALIDOPCODE) {
            //Verify if there is an error in getting opcode
        }

        if (opcode == OP_CAT ||
            opcode == OP_SUBSTR ||
            opcode == OP_LEFT ||
            opcode == OP_RIGHT ||
            opcode == OP_INVERT ||
            opcode == OP_AND ||
            opcode == OP_OR ||
            opcode == OP_XOR ||
            opcode == OP_2MUL ||
            opcode == OP_2DIV ||
            opcode == OP_MUL ||
            opcode == OP_DIV ||
            opcode == OP_MOD ||
            opcode == OP_LSHIFT ||
            opcode == OP_RSHIFT) {
            // ERROR Disabled opcodes.
        }

        switch (opcode) {
            case OP_1NEGATE:
            case OP_1:
            case OP_2:
            case OP_3:
            case OP_4:
            case OP_5:
            case OP_6:
            case OP_7:
            case OP_8:
            case OP_9:
            case OP_10:
            case OP_11:
            case OP_12:
            case OP_13:
            case OP_14:
            case OP_15:
            case OP_16: {
                //Push values on the Stack
            }
            break;

            case OP_NOP:
                break;

            case OP_CHECKLOCKTIMEVERIFY: {
                break;
            }

            case OP_NOP1: case OP_NOP3: case OP_NOP4: case OP_NOP5:
            case OP_NOP6: case OP_NOP7: case OP_NOP8: case OP_NOP9: case OP_NOP10: {
                //??
            }
            break;

            case OP_IF:
            case OP_NOTIF: {
                // <expression> if [statements] [else [statements]] endif
            }
            break;

            case OP_ELSE: {
            }
            break;

            case OP_ENDIF: {
            }
            break;

            case OP_VERIFY: {
            }
            break;

            case OP_RETURN: {
            }
            break;





            // Stack ops ----------------------------------------------------------------------
            case OP_TOALTSTACK: {
            }
            break;

            case OP_FROMALTSTACK: {
            }
            break;

            case OP_2DROP: {
                // (x1 x2 -- )
            }
            break;

            case OP_2DUP: {
                // (x1 x2 -- x1 x2 x1 x2)
            }
            break;

            case OP_3DUP: {
                // (x1 x2 x3 -- x1 x2 x3 x1 x2 x3)
            }
            break;

            case OP_2OVER: {
                // (x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2)
            }
            break;

            case OP_2ROT: {
                // (x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2)
            }
            break;

            case OP_2SWAP: {
                // (x1 x2 x3 x4 -- x3 x4 x1 x2)
            }
            break;

            case OP_IFDUP: {
                // (x - 0 | x x)
            }
            break;

            case OP_DEPTH: {
                // -- stacksize
            }
            break;

            case OP_DROP: {
                // (x -- )
            }
            break;

            case OP_DUP: {
                // (x -- x x)
            }
            break;

            case OP_NIP: {
                // (x1 x2 -- x2)
            }
            break;

            case OP_OVER: {
                // (x1 x2 -- x1 x2 x1)
            }
            break;

            case OP_PICK:
            case OP_ROLL: {
                // (xn ... x2 x1 x0 n - xn ... x2 x1 x0 xn)
                // (xn ... x2 x1 x0 n - ... x2 x1 x0 xn)
            }
            break;

            case OP_ROT: {
                // (x1 x2 x3 -- x2 x3 x1)
                //  x2 x1 x3  after first swap
                //  x2 x3 x1  after second swap
            }
            break;

            case OP_SWAP: {
                // (x1 x2 -- x2 x1)
            }
            break;

            case OP_TUCK: {
                // (x1 x2 -- x2 x1 x2)
            }
            break;


            case OP_SIZE: {
                // (in -- in size)
            }
            break;


            // Bitwise logic ----------------------------------------------------------------------
            case OP_EQUAL:
            case OP_EQUALVERIFY:
            //case OP_NOTEQUAL: // use OP_NUMNOTEQUAL
            {
                // (x1 x2 - bool)
            }
            break;


            // Numeric ----------------------------------------------------------------------
            case OP_1ADD:
            case OP_1SUB:
            case OP_NEGATE:
            case OP_ABS:
            case OP_NOT:
            case OP_0NOTEQUAL: {
                // (in -- out)
            }
            break;

            case OP_ADD:
            case OP_SUB:
            case OP_BOOLAND:
            case OP_BOOLOR:
            case OP_NUMEQUAL:
            case OP_NUMEQUALVERIFY:
            case OP_NUMNOTEQUAL:
            case OP_LESSTHAN:
            case OP_GREATERTHAN:
            case OP_LESSTHANOREQUAL:
            case OP_GREATERTHANOREQUAL:
            case OP_MIN:
            case OP_MAX: {
                // (x1 x2 -- out)

            }
            break;

            case OP_WITHIN: {
                // (x min max -- out)
            }
            break;


            // Crypto ----------------------------------------------------------------------
            case OP_RIPEMD160:
            case OP_SHA1:
            case OP_SHA256:
            case OP_HASH160:
            case OP_HASH256: {
                // (in -- hash)
            }
            break;                                   

            case OP_CODESEPARATOR: {
                // Hash starts after the code separator
            }
            break;

            case OP_CHECKSIG:
            case OP_CHECKSIGVERIFY: {
                // (sig pubkey -- bool)
            }
            break;

            case OP_CHECKMULTISIG:
            case OP_CHECKMULTISIGVERIFY: {
                // ([sig ...] num_of_signatures [pubkey ...] num_of_pubkeys -- bool)
            }
            break;

            default:
                //ERROR!
        }

        //Check stack's sizes
        // Size limits

        ++i;
    }

    return true; //OK!
}

void process_scripts(chain::script const& input_script, chain::script const& output_script) {

    const auto input_script_data = input_script.to_data(false);
    const auto output_script_data = output_script.to_data(false);

    std::vector<td::vector<uint8_t>> stack;

    if ( ! evaluate_script(stack, input_script_data.data(), input_script_data.size())) {
        std::cout << "error in input script: ";
        print_bytes(input_script_data.data(), input_script_data.size());
    }

    if ( ! evaluate_script(stack, output_script_data.data(), output_script_data.size())) {
        std::cout << "error in output script: ";
        print_bytes(output_script_data.data(), output_script_data.size());
    }

}

void process_tx(transaction_database const& tx_db, chain::transaction const& tx) {
    
    for (const auto& input: tx.inputs) {
        if (input.previous_output.hash != null_hash) {

            auto utxo_tx_res = tx_db.get(input.previous_output.hash);
            
            if (utxo_tx_res) {
                auto const& utxo_tx = utxo_tx_res.transaction();

                if (input.previous_output.index < tx.outputs.size()) {
                    auto const& output = tx.outputs[input.previous_output.index];
                    process_scripts(input.script, output.script);
                } else {
                    std::cout << "output not found hash: " << encode_hash(input.previous_output.hash) << " - index: " << input.previous_output.index << '\n';    
                }
            } else {
                std::cout << "output not found hash: " << encode_hash(input.previous_output.hash) << '\n';
            }
        }
    }
}

void process_block(transaction_database const& tx_db, block_result const& block) {

    //TODO: Fer: ignorar las coinbase Tx

    auto tx_count = block.transaction_count();
    for (size_t i = 0; i < tx_count; ++i) {

        auto tx_hash = block.transaction_hash(i);
        auto tx_res = tx_db.get(tx_hash);

        if (tx_res) {
            auto const& tx = tx_res.transaction();
            if (! tx.is_coinbase()) {
                process_tx(tx_db, tx);    
            }
        }
    }
}


int main(int argc, char** argv) {

    const std::string block_lookup_filename = argv[1];   //block_table
    const std::string block_index_filename = argv[2];    //block_index
    const std::string tx_filename = argv[3];             //transaction_table

    block_database blocks_db(block_lookup_filename, block_index_filename);
    transaction_database tx_db(tx_filename);

    const auto block_db_start_result   = blocks_db.start();
    BITCOIN_ASSERT(block_db_start_result);
    const auto tx_db_start_result   = tx_db.start();
    BITCOIN_ASSERT(tx_db_start_result);

    constexpr size_t height_max = 436789;

    for (size_t i = 0; i < height_max; ++i) {
        auto const& block = blocks_db.get(i);

        if (block) {
            process_block(tx_db, block);
        } else {
            std::cout << "block not found #" << i << '\n';
        }

        if (i % 10000 == 0) {
            std::cout << "block height: " << i << '\n';
        }
    }


    // auto item_data = tx_db.get_first_item();
    // auto result = transaction_result(std::get<2>(item_data));

    // while (result) {
    //     // std::cout << "height: " << result.height() << std::endl;
    //     // std::cout << "index: " << result.index() << std::endl;
    //     // std::cout << "tx: " << encode_base16(data) << std::endl;

    //     const auto tx = result.transaction();
    //     const data_chunk data = tx.to_data();
    //     auto bucket = std::get<0>(item_data);

    //     if (bucket % 1000 == 0) {
    //         std::cout << "bucket: " << bucket << '\n';
    //     }

    //     for (const auto& input: tx.inputs) {
    //         if (input.previous_output.hash != null_hash) {

    //             if (true) {


    //             }                 
    //         }
    //     }

    //     item_data = tx_db.get_next_item(bucket, std::get<1>(item_data));        
    //     result = transaction_result(std::get<2>(item_data));
    // }

    // auto bucket = std::get<0>(item_data);
    // auto file_offset = std::get<1>(item_data);
    // auto tx_result = std::get<2>(item_data);
    // auto tx_result_valid = bool(tx_result);
    // std::cout << "tx_result_valid: " << tx_result_valid << std::endl;
    // std::cout << "bucket: " << bucket << std::endl;

    return 0;
}

/*
input script: 47304402204e45e16932b8af514961a1d3a1a25fdf3f4f7732e9d624c6c61548ab5fb8cd410220181522ec8eca07de4860a4acdd12909d831cc56cbbac4622082221a8768d1d0901
output script: 4104ae1a62fe09c5f51b13905f07f06b99a2f7159b2225f374cd378d71302fa28414e7aab37397f554a7df5f142c21c1b7303b8a0626f1baded5c72a704f7e6cd84cac

input script: 473044022027542a94d6646c51240f23a76d33088d3dd8815b25e9ea18cac67d1171a3212e02203baf203c6e7b80ebd3e588628466ea28be572fe1aaa3f30947da4763dd3b3d2b01
output script: 410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a6909a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b412a3ac

input script: 47304402201f27e51caeb9a0988a1e50799ff0af94a3902403c3ad4068b063e7b4d1b0a76702206713f69bd344058b0dee55a9798759092d0916dbbc3e592fee43060005ddc17401
output script: 410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a6909a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b412a3ac

input script: 483045022052ffc1929a2d8bd365c6a2a4e3421711b4b1e1b8781698ca9075807b4227abcb0221009984107ddb9e3813782b095d0d84361ed4c76e5edaf6561d252ae162c2341cfb01
output script: 410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a6909a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b412a3ac

input script: 473044022041d56d649e3ca8a06ffc10dbc6ba37cb958d1177cc8a155e83d0646cd5852634022047fd6a02e26b00de9f60fb61326856e66d7a0d5e2bc9d01fb95f689fc705c04b01
output script: 4104fe1b9ccf732e1f6b760c5ed3152388eeeadd4a073e621f741eb157e6a62e3547c8e939abbd6a513bf3a1fbe28f9ea85a4e64c526702435d726f7ff14da40bae4ac

input script: 493046022100f493d504a670e6280bc76ee285accf2796fd6a630659d4fa55dccb793fc9346402210080ecfbe069101993eb01320bb1d6029c138b27835e20ad54c232c36ffe10786301
output script: 4104ea0d6650c8305f1213a89c65fc8f4343a5dac8e985c869e51d3aa02879b57c60cff49fcb99314d02dfc612d654e4333150ef61fa569c1c66415602cae387baf7ac

input script: 483045022100c12a7d54972f26d14cb311339b5122f8c187417dde1e8efb6841f55c34220ae0022066632c5cd4161efa3a2837764eee9eb84975dd54c2de2865e9752585c53e7cce01
output script: 410411db93e1dcdb8a016b49840f8c53bc1eb68a382e97b1482ecad7b148a6909a5cb2e0eaddfb84ccf9744464f82e160bfa9b8b64f9d4c03f999b8643f656b412a3ac

input script: 483045022100cb2c6b346a978ab8c61b18b5e9397755cbd17d6eb2fe0083ef32e067fa6c785a02206ce44e613f31d9a6b0517e46f3db1576e9812cc98d159bfdaf759a5014081b5c01
output script: 4104ea1feff861b51fe3f5f8a3b12d0f4712db80e919548a80839fc47c6a21e66d957e9c5d8cd108c7a2d2324bad71f9904ac0ae7336507d785b17a2c115e427a32fac

input script: 483045022047957cdd957cfd0becd642f6b84d82f49b6cb4c51a91f49246908af7c3cfdf4a022100e96b46621f1bffcf5ea5982f88cef651e9354f5791602369bf5a82a6cd61a62501
output script: 4104ea1feff861b51fe3f5f8a3b12d0f4712db80e919548a80839fc47c6a21e66d957e9c5d8cd108c7a2d2324bad71f9904ac0ae7336507d785b17a2c115e427a32fac

input script: 47304402204165be9a4cbab8049e1af9723b96199bfd3e85f44c6b4c0177e3962686b26073022028f638da23fc003760861ad481ead4099312c60030d4cb57820ce4d33812a5ce01
output script: 4104ea1feff861b51fe3f5f8a3b12d0f4712db80e919548a80839fc47c6a21e66d957e9c5d8cd108c7a2d2324bad71f9904ac0ae7336507d785b17a2c115e427a32fac

input script: 473044022038ea59740da72eec2490a0b32fa6004139524fefba7e78c5d0aed40a5c07f39b02205b1adea529c3cc5cdbb900a8515d778b8e982d63b13386d955962a93f70cd27101
output script: 4104f36c67039006ec4ed2c885d7ab0763feb5deb9633cf63841474712e4cf0459356750185fc9d962d0f4a1e08e1a84f0c9a9f826ad067675403c19d752530492dcac

input script: 49304602210083ec8bd391269f00f3d714a54f4dbd6b8004b3e9c91f3078ff4fca42da456f4d0221008dfe1450870a717f59a494b77b36b7884381233555f8439dac4ea969977dd3f401
output script: 41044a656f065871a353f216ca26cef8dde2f03e8c16202d2e8ad769f02032cb86a5eb5e56842e92e19141d60a01928f8dd2c875a390f67c1f6c94cfc617c0ea45afac

input script: 483045022100aa46504baa86df8a33b1192b1b9367b4d729dc41e389f2c04f3e5c7f0559aae702205e82253a54bf5c4f65b7428551554b2045167d6d206dfe6a2e198127d3f7df1501
output script: 41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac

input script: 47304402202329484c35fa9d6bb32a55a70c0982f606ce0e3634b69006138683bcd12cbb6602200c28feb1e2555c3210f1dddb299738b4ff8bbe9667b68cb8764b5ac17b7adf0001
output script: 41044a656f065871a353f216ca26cef8dde2f03e8c16202d2e8ad769f02032cb86a5eb5e56842e92e19141d60a01928f8dd2c875a390f67c1f6c94cfc617c0ea45afac

input script: 47304402205d6058484157235b06028c30736c15613a28bdb768ee628094ca8b0030d4d6eb0220328789c9a2ec27ddaec0ad5ef58efded42e6ea17c2e1ce838f3d6913f5e95db601
output script: 41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac

input script: 493046022100c45af050d3cea806cedd0ab22520c53ebe63b987b8954146cdca42487b84bdd6022100b9b027716a6b59e640da50a864d6dd8a0ef24c76ce62391fa3eabaf4d2886d2d01
output script: 41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac

input script: 473044022016e7a727a061ea2254a6c358376aaa617ac537eb836c77d646ebda4c748aac8b0220192ce28bf9f2c06a6467e6531e27648d2b3e2e2bae85159c9242939840295ba501
output script: 41044a656f065871a353f216ca26cef8dde2f03e8c16202d2e8ad769f02032cb86a5eb5e56842e92e19141d60a01928f8dd2c875a390f67c1f6c94cfc617c0ea45afac

input script: 493046022100b7a1a755588d4190118936e15cd217d133b0e4a53c3c15924010d5648d8925c9022100aaef031874db2114f2d869ac2de4ae53908fbfea5b2b1862e181626bb9005c9f01
output script: 41046a0765b5865641ce08dd39690aade26dfbf5511430ca428a3089261361cef170e3929a68aee3d8d4848b0c5111b0a37b82b86ad559fd2a745b44d8e8d9dfdc0cac

input script: 47304402201092da40af6dea8abcbeefb8586335b26d39d36be9b6c38d6c9cc18f20dd5886022045964de79a9008f68d53fc9bc58f9e30b224a1b98dbfda5c7b7b860f32c6aef101
output script: 410497304efd3ab14d0dcbf1e901045a25f4b5dbaf576d074506fd8ded4122ba6f6bec0ed4698ce0e7928c0eaf9ddfb5387929b5d697e82e7aabebe04c10e5c87164ac

input script: 483045022100b0a1d0a00251c56809a5ab5d7ba6cbe68b82c9bf4f806ee39c568ae537572c840220781ce69017ec3b2d6f96ffff4d19c80c224f40c73b8c26cba4b30e7f4171579b01
output script: 410497304efd3ab14d0dcbf1e901045a25f4b5dbaf576d074506fd8ded4122ba6f6bec0ed4698ce0e7928c0eaf9ddfb5387929b5d697e82e7aabebe04c10e5c87164ac

input script: 483045022100c8e980f43c616232e2d59dce08a5edb84aaa0915ea49780a8af367330216084a02203cc2628f16f995c7aaf6104cba64971963a4e084e4fbd0b6bcf825b47a09f8e301
output script: 410497304efd3ab14d0dcbf1e901045a25f4b5dbaf576d074506fd8ded4122ba6f6bec0ed4698ce0e7928c0eaf9ddfb5387929b5d697e82e7aabebe04c10e5c87164ac

input script: 4830450220745a8d99c51f98f5c93b8d2f5f14a1f2d8cc42ff7329645681bcafe846cbf50d022100b24e31186129f3ae6cc8a226d1eda389373652a9cf2095631fcc4345067c1ff301
output script: 410497304efd3ab14d0dcbf1e901045a25f4b5dbaf576d074506fd8ded4122ba6f6bec0ed4698ce0e7928c0eaf9ddfb5387929b5d697e82e7aabebe04c10e5c87164ac

input script: 483045022100ca65b3f290724d6c56fc333570fa342f2477f34b2a6c93c2e2d7216d9fe9088e022077e259a29ed1f988fab2b9f2ce17a4a56a20c188cadc72bca94e06a73826966501
output script: 410497304efd3ab14d0dcbf1e901045a25f4b5dbaf576d074506fd8ded4122ba6f6bec0ed4698ce0e7928c0eaf9ddfb5387929b5d697e82e7aabebe04c10e5c87164ac

input script: 483045022024fd7345df2b2bd0e6f8416529046b7d52bda5ffdb70146bc6d72b1ba73cabcd022100ff99c03006cc8f28d92e686f0ae640d20395177f329d0a9dbd560fd2a55aeee701
output script: 4104888d890e1bd84c9e2ac363a9774414a081eb805cd2c0d52e49efc7170ebf342f1cdb284a2e2eb754fc8dd4525fe0caa3d3a525214d0b504dd75376b2f63804a8ac

input script: 48304502204db0060c0a9b054c6c01438f24fa069c4596ced90865f317f666b69fef1af36a0221008970b06f2f1e86547ad2c8e627780d546c7aa6fbc8352fed45b230e9a1e8bfcd01
output script: 41044bca633a91de10df85a63d0a24cb09783148fe0e16c92e937fc4491580c860757148effa0595a955f44078b48ba67fa198782e8bb68115da0daa8fde5301f7f9ac

input script: 4830450220456c80524bf6d7c542839791067ea98afe10e1b271422808d2117469fc33a7dd0221008beb72efed006d244ab4933bb150a7f4330f2ce850b4f77f7884e4350efc03c701

output script: 41042c0960594d5e48ccc8edb8a7ac622d5b542a1058257aecbf5f2e75ae8cfffeb98ac6cd8aa268a671857053c2d094725ffa2d22c5fc7872ddb6ffc8ee298a3ec9ac
input script: 483045022020f0110caa1819d3338267b466b162bee8a7b955393e7a1b3ee25bc4f33b3216022100c1117a48db19917536687ebe4feea58f864b625d4ec7b506d3ef02b3ac022ac201

output script: 41042c0960594d5e48ccc8edb8a7ac622d5b542a1058257aecbf5f2e75ae8cfffeb98ac6cd8aa268a671857053c2d094725ffa2d22c5fc7872ddb6ffc8ee298a3ec9ac
input script: 493046022100e26d9ff76a07d68369e5782be3f8532d25ecc8add58ee256da6c550b52e8006b022100b4431f5a9a4dcb51cbdcaae935218c0ae4cfc8aa903fe4e5bac4c208290b7d5d01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100a2ab7cdc5b67aca032899ea1b262f6e8181060f5a34ee667a82dac9c7b7db4c3022100911bc945c4b435df8227466433e56899fbb65833e4853683ecaa12ee840d16bf01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 47304402200c5094ea5d8cbc1e0041f4683404ddbe220be65919c66f93ce9f927962ee260902205ea9ac93b5c029be554131d7da4223755aef2651aad549011d4830752294ee7901

output script: 410445440b3c43352f268bc7630cd1d7c78b947910b8d5514f22d78599bdd9d13cd06f800525bec3a9d6e576e073b08c6e0b794b719adcfba2bada9638b78cbf87d9ac
input script: 48304502200374874001b8b6c7fdf779f0badf5c2401b67090e237d00d3710dbe2e2249c0a022100814ba5f7937be2314604d18076501f2d9589b6f304e547685ce8b85f481db82601

output script: 410445440b3c43352f268bc7630cd1d7c78b947910b8d5514f22d78599bdd9d13cd06f800525bec3a9d6e576e073b08c6e0b794b719adcfba2bada9638b78cbf87d9ac
input script: 47304402203b21912c0509b021fae41f578cd61a9540de559391214878c2028357453ee04d02202c4ba57b923326fab61688de2f979a67781477275517520bc622bc53368f11d501

output script: 410445440b3c43352f268bc7630cd1d7c78b947910b8d5514f22d78599bdd9d13cd06f800525bec3a9d6e576e073b08c6e0b794b719adcfba2bada9638b78cbf87d9ac
input script: 483045022100f7b1e59337171c41237947e0e3d72061715614bdc022665ad425152d54a67f8b0220781cebea276de74079e202e849ef08767369160cf5acd92a831078b6c303094901

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 47304402206cacd91f5085add22787c4ca84d4275416f9aff3839aa79e8537c9d9bbaee212022056c9d0731148692753f21e2723f58eb6a6eb8d4f922a99c71cf306bef54f88f501

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 4830450220746132159305d30a643c242dfa38501a13299b5df70bd88e8455b1c1001de65a022100d4920be4af1e12b78bd96e23e32ecdfc69f6fc5bbee418243ec58fa7d473002101

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 47304402206cac8d98c4aa192c9e7368dbed3bd055a191580ba3ee9ab0d52d128dcb1b410a0220066b5508416f52707132b7f38751ba038bad62cae0f567732045b9b54a9bda0a01

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 48304502210082cad1fb70c2ecab86602de899e93c0fa8d55724e87105db2bc708b69d35de3302206b8c8c4695ef082e9a9cffb7df3b49a41c1c35dfd1b42a58ddd551b6e2f403a701

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 47304402204140b7e88df248205d238372118f832d997ec42a9f61f0b0644efbf8a61fe6a402203675074f4ad95e8bc51ca249f7703de6a7d1928213e9a523925e98a2c929f29201

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 473044022031666ff7423f79c8e569d58a53270eadd488527ea866c9bd2b1120546aea9593022037b33f76f9e970e54127071fb04b51eaf58bb682f2784c782586a0f365a5cd1601

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 4830450221008aef18502118716e086473faa76b1769322c1bf81428f50235de4a0119af7efa02202a2b707fa084ed53026cdc87c1c2e9de4df9738e6f7bbec01dea409b2931fe5d01

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 47304402203073af03df4756faeb79d3e559480d2c4408b1a9d96cc99b4270be7f250d7883022040c735cc8f5d7eb6bbcc46e2f929f1deeb6ebe86dcdf0e013030174ba14a694401

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 47304402200f81a2710ab31beb3f8ccb3a2639bd003941a6847c592a3d9885f6750bf017010220056f6ce979a10e4cdba9d06a7017bf920c121df0df2fac615298c75d8f09eaa501

output script: 4104b6c51d273c142a7a015ab6bd68d738dfcbc0f9015140dc5f2c7b3b944429ab3474348cde91ec1aeda4dd24324dcc2fa6498b0dde87c855a839b090f6e8b016e6ac
input script: 48304502202466ca9b80920af2f518962695dd31e35d5d688729dc17e52cf6d05369bca69b022100edd5e092b81d4b64001f1c437fd6a9fe710bfdbcbb0eb00fa9d66099ed50c67101

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 47304402202991e2c0a6fedd64b6e2d0caa54b256aa689dcb75d3d92a53bf41b9305e152ab0220710bcc25136cdcfc233f33d5527227642e2bdaec64070847b1c2e44f1c6b17d201

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
output not found hash: e36f06a8dfe44c3d64be2d3fe56c77f91f6a39da4a5ffc086ecb5db9664e8583 - index: 1
input script: 483045022100de3dce233f69eae1d003a92213fd233112f80c331265bbe22994deeacf84bdbd02207c331411d3a62089b7c632a5e53c20d5d2e4de72235b60f1a3eb80f0f98c3f5a01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100add2e315ff749421aa5c204fc9d1ac6304b55337471e47b69be6433f9186aa4602202b97f5a29c1fe9b2a1acb5695369217c39d7a94fc3c8ed7cbb4ed4487694317b01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100bd60476ed58cf35ff5f8cd6157b18be18844dfbbb6cca265bcb0bd57a8f9818a0220296f5cb8757e0d1dafe8cd8e8ee0a330fe6b717227132fc13ed4508cdb3cd8bd01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 473044022013c666135f5da4a55cd38d7352c9e6664d2cb16c16738dee22f8bdcb126aadba02207f43d3e86621d8ed20d74bed867822b0cc410b4a2bdfa5904c6fe6545a01668301

output script: 4104275a742d3ba1587ec4a491e7278a87d58aa98f847b154fbdf3d8b014c2449e5896c5266ea3bf293b17aad0d4b68a6c7503e37bd096ac8be889d30620032134a3ac
input script: 493046022100c0afee1f91f286ae921ea3b102a5691449a5d911f7a9a86437af56b63d4611dc02210088df25568767fbf67ba35ececd530d9813f9b975aff1b726b6f02c4b0ec08be301

output script: 4104275a742d3ba1587ec4a491e7278a87d58aa98f847b154fbdf3d8b014c2449e5896c5266ea3bf293b17aad0d4b68a6c7503e37bd096ac8be889d30620032134a3ac
input script: 483045022100aeb28ed90f8f5f3349005a5512642cfead97d345251727a16ae5af45d9981a2902202027daa716740d12d5cd938dd6c4f2e9df70dec88f01b4b1ad2371258dfb267701

output script: 4104275a742d3ba1587ec4a491e7278a87d58aa98f847b154fbdf3d8b014c2449e5896c5266ea3bf293b17aad0d4b68a6c7503e37bd096ac8be889d30620032134a3ac
input script: 47304402205fcde524cf2d7bb0011236b3812cb3cd361d1889b22d6bad73f5b5ad26da5a3202200606e8ed41379f2713409b7210c6e9da88ec760c3509f481893a551bafac3d0c01

output script: 4104275a742d3ba1587ec4a491e7278a87d58aa98f847b154fbdf3d8b014c2449e5896c5266ea3bf293b17aad0d4b68a6c7503e37bd096ac8be889d30620032134a3ac
input script: 48304502205be59cd4abd3d97574966f451ffc807f9310889a086b0ebc45ef58fbda63a30d022100c416812158296f999579fe4aed6824bf46a20786b96a4327388211e9e536077301

output script: 4104275a742d3ba1587ec4a491e7278a87d58aa98f847b154fbdf3d8b014c2449e5896c5266ea3bf293b17aad0d4b68a6c7503e37bd096ac8be889d30620032134a3ac
input script: 47304402201bab9c5067802b35e5b9a4d6d1295c4083ec4cf4bbfbf8160e3349fe76aedf1602207ad037ba6a5a5e12d49dbbec015320a47ab5a220a9f28e0e7d8ca0ede8e4e8ed01

output script: 4104275a742d3ba1587ec4a491e7278a87d58aa98f847b154fbdf3d8b014c2449e5896c5266ea3bf293b17aad0d4b68a6c7503e37bd096ac8be889d30620032134a3ac
input script: 483045022068f1a311655950e291cfada7bb3c55bb16f673a5e354ed3a0fd96448f9141b28022100e8075060eefd673ce6fc824f9afc6faaa9169b5c2b6e54df3fa6802ae94d3e6701

output script: 4104275a742d3ba1587ec4a491e7278a87d58aa98f847b154fbdf3d8b014c2449e5896c5266ea3bf293b17aad0d4b68a6c7503e37bd096ac8be889d30620032134a3ac
input script: 493046022100fd887e37eee5b10b809281f3b6fdcda022ae4e8eceed10d9fde0b2c3ee27bb9b022100ca9fa47465329316369c9ce363e5dadfb3eae49e03c106cfd261ba861095147501

output script: 4104275a742d3ba1587ec4a491e7278a87d58aa98f847b154fbdf3d8b014c2449e5896c5266ea3bf293b17aad0d4b68a6c7503e37bd096ac8be889d30620032134a3ac
input script: 4730440220462d63fc6db62da9c243120808166c07b49db2fad7e6ca15f178a5bbc1b4eaec022061c6218333978d150e32e73b2860fc6ddc39b9085aaedf5e8d81a974eea354ea01

output script: 4104bc3ee049bebf27e6e29403aeb61a1c48acee5d4c3b687252a99add1b7dbe38272e42882747493f2d2e6c92e22dc46567491f3cd5a25259e269ac66649ef6d871ac
input script: 473044022049590e4994d16d070ba1d01faa0caa3eb1707507aedad238d2a46791b0acd677022040e53cebd1084c8c5e160e4babb5977dcff53b98a810c220cb8a9f298490ed4401

output script: 4104bc3ee049bebf27e6e29403aeb61a1c48acee5d4c3b687252a99add1b7dbe38272e42882747493f2d2e6c92e22dc46567491f3cd5a25259e269ac66649ef6d871ac
input script: 483045022100c9096e525039b05002873c6b50a9f31cc401925f8ece442e3b73da9627aec89702205428da514f7511ddfc115512fdf7ab2be3024c3a22c779ce0b8ef44339c7270801

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100e770a83ecf0f8171f99e7dc207ea7b738384feed35bf13cf89d99cc1657a7bf3022004c634d45a1094eb8f76630d6dc4a30b96d65103057517de343e446a9cfd0b9401

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 4830450220668b1d6e745d511f8f9b63fafb0384df6a79d58ed3b101b17d63129026f10203022100ed5ab3f34e19b905d1ac26c5a18274cee58a8d6ea6426f039c47576f849b475501

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100a6f242ed8523125ee94768140ece304a195264d2cdcb6bb9cbc31a94a5af496b02206638e5e543ee541b4fef24e69b8fba4c854556857d5f3f48f0e06ce1564cf56101

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 4930460221009b059769db4f8341df29899d5ac7dbc09d97f045b64358aaca7b11d3b929eabe022100ad371104c2ea35e6f8a835114e7508ff1f9938d050c70a5ce771ba8d409e1b0401

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100f66f06cf345e190bf06dda0b46f63c0b490c6d194c545e7f4e7f7256067a877702204498e6ba7c69806cf51a3bd86e3768f3ef33610c82c732f6aa8792939b39550701

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 48304502202eb60bb5a5780a694bf8c1eb87401f2ffc24785d3f7d8a8599dc50512c3da0bd022100809370c0947745f615b09577e9994db7d6fe9608834d85cf08d1a07920da816c01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100f9b4a0ab165419032c9a6ea4f46bee3d174c58349b8cadbce1687be40515b35602207982d0f5fa6f31d3fd407a57f927ad89f8d6aef3b3837f7759efd6c5726eeb7901

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 4830450220530e3e17883f1b713a7916aaf56ce3c826c4399ba538fde55b8ca2b4d04fee5f022100ec04aef75c377ed059f834fa1da2ea56be584873aa63148f67cdba48b4e8ee5e01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100b79ac300729beafff89642ca2887a3c12a28dc872b5a0c7bd5f2203f3df81a75022057d24b2ea3e8a10d31b0e738397ca765b93c78b11c6d57d3aa292bce58c7dd2e01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100ef834ae2a62722c631a7211a87ef7394fa66fc73b6640990743604576b0c12be0220161f372e298e6ad62982132e0c213f31eb19bfa0b8d161b9b94841749f42c5ca01

output script: 4104e667d2070767a3226b5435409ddcad58740a4b6e720f11a688fe6a6c217d6e2e3d2d8a3bff7ceb129e9736b7c975ca54c32ce25ce96c9a007467b1f9e4490a06ac
input script: 47304402207056c048eec242f7ca536c832570d94bf25460288f2b4920c6574e534a7400a102206fa19681a44449cb40aeabbfbe7dcf00d8bbd5838cc066cc096ef3da7c0b1d4201

output script: 4104e667d2070767a3226b5435409ddcad58740a4b6e720f11a688fe6a6c217d6e2e3d2d8a3bff7ceb129e9736b7c975ca54c32ce25ce96c9a007467b1f9e4490a06ac
input script: 4830450220218cd108ae2e3a69db9147e0af14b4d715ff9df36b31ef595848914e98a7404e022100c23ced214985633067a7baa0af464a09886790d25500b42ba6465a72ce7b46c601

output script: 4104e667d2070767a3226b5435409ddcad58740a4b6e720f11a688fe6a6c217d6e2e3d2d8a3bff7ceb129e9736b7c975ca54c32ce25ce96c9a007467b1f9e4490a06ac
input script: 48304502202cc881eae11a1cd38a16ddc7a56a023631bcf218932ac491edb29bce6438071a022100adde4b5ce37cf7c0671ef0f717e4992335800a0b7a246afee3c5eb00dbad135e01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 4730440220274f9a50e79bd3de984dd9bcc790d133c63a51372378909ffc2e4c59ffaccc6702202a01baad68a2c416fc181268ca0e86092526686c895c34e3fd6b2ad5c947dc6f01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100e62e5723f7d809a1de22501be31429c0f97fa545fec14041a46e47ead22ecd1f022100fde7d3091ad9c6904e2e3e7fd980616dc4caa59f2e521a975553a4aceda3832701

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 48304502203574b06ea162b61d6cc42ca391806f870895f52ec3ebdac659abe009969ff506022100befa03ca5935fe17737cb21d0d9168347f394a4b41f38f0d507f87316a00e8e001

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100a381bcc12ea362a29609c1302bfe29766cdfb32f4583dc7251fe6d4da2f4227202206db26c30d3e54b49fd68ed4fc9631062700914b0104b4417fcccdedd86ea8f8801

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 4830450220378bd78f0ff710d7f126ba62b0518fb7890faa1fc611c05a9e36bebbba5728e7022100adb012db9cac277293ddcb384f332e31f002ec70e8ee2fee4469ef3ce56d9b8a01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100bf0a84a40ae694b530c19ced181a1e932061e87acdc98a671adefff3abbdeab5022100e59e6816ea83fb5cc2a601edb8bf9e216ad80e4013b317151e90a41532b5d64801

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 493046022100a6aa43e5845d20ed02902984950d6cb26bc023a55348504c50e00a2a10d1b93f02210080b10b0144924f5ad9d7c26ee17ac15b15942896ac4d73bd90d869ffb80acbdc01

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 483045022100a86126835ca9c8a8e9d261af9c367981ad1a583d277ba0c335733c1e98f0956e0220440dffbaac20c19faa933876b1f7f2471c7ea76c344f8b6e034f3c5b75654cab01

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 48304502203a131171f724cc80bc5e1025f7121369b670fd14b6780d1e4e5159f3746023b4022100e4f5e4be3e38c76e6d773111dc027c6215f0e2d1d3781f430cd68850352a9dc401

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 48304502200524f00881c3a0655f2aa66a9d828606b5d80749156c15429075ac896eff4dba022100c5235e82e230257a9219a5a53b35dd554ad8205bc1ee8f70f96c05d0b095653401

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 4830450220529c7fe3795f9cda859092d1bdf2a9659c88d9e57805ac062cacad678fd552f5022100e6e8091c7ee0fdbdd6e576048b5122a44de7593e094667a2e25cc05f3589310701

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 4830450220770c3dbb35bdfc5e198f8ae0ebda3c6b08d6fd1d6a562fa0315118db075041e6022100a372b0abafbda9eabb9f0cf10a7a6e98a02307587dc4fa09e559373a2272584201

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 483045022054de840fcb22a4e1149d14b5d079339ac2da5913274f6026d73e0e9bc5579ff1022100c4b34fb1d4efa59ea4e130e1be4c3e5ec673e4726f08388157021d2149127eef01

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 4830450220604b369e4005b30286ae528a8a644d0ad5d5bf849bb009e73a00db0c61eae091022100aff413a1f8e65b30c186c8ddf259cdff700888d99cee3311d87b3b11d8850f4501

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 47304402203f1b85ff75902374480ddcb34be32809ae70b276311300996bdd28ee29b1caee0220435ef3ad3bbc0b1560aa3712de344ec8453a66c87f70f0cd499d1a8df82e061301

output script: 4104554a9abcde5e79acb2552e6159b287c72c9c31b1a2510a877e3c8dc3cb1780da707b2364a229a3e7df8cd09a8e2c477589035b3590a7f9cd624a227b5624c574ac
input script: 483045022100ae5eb1fd253a6024b3d75e87b8768a4e7c80e30415c24099f0c320e555efb92102203527fc5d9de1819da74e8a7679c86d17165204e105dc16ba761005599880851c01

output script: 4104364534e8db175dc05f84ea3ca97989abf57f8d8b6671cccce1c12ef935d18c1287c29a35c2f0a518d709044aae89a344284f330c2d11649aa4da08a813251712ac
input script: 493046022100aa7b6c7ebb12904d072731d83f15a3e8bc2097ded757d794cef7d912f8ae1d97022100d4edd90392b35ce3e6c623eff99a1090ee5c163810c0ec87d38da151c45815a101

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100902dfd9d40c6433db5ba207cd0e6bda0e21691ca4365e0642b3b0ffb637f742c02206bb8e0a6b5b6a757ce96f31b71536eeb1c317b8727e0e802c82dc7673707829801

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100d0b911befed591b2a870c063743161a95bd9cce6aec930ad535e28c31a27a83c022100a17e56b003e47be5ce6b1d6a78b94b612c4ba9f6653551aa3ab0be00aeb4d54901

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 47304402202d6c9ff44cb65154d8ca854ffcbbb255c380b5bbe6ab9639a2ac82c8dd52394f02207cf9b13943e04a73a63eefa2a1c80f287d89f798209e4b253e37948e1261e0d101

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100c45ec8d4cba18eda841ad8312196814183c43f395c172559903b7c1ebaf78029022100b5c80333de65d79c77a0307d57f7ca9b2a006cd84f5d3c5ddb349dbf48dc094d01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100e8a77f5e501430669b8fbb38b1f1f91706d1ea54c56482dcfbac40607f90b797022100fdac29e939b2265046f658ef60db266381586dc116bc5fbc4cdc89cabd43261e01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100ce9d0d6262c78fce8b444b6aef0c6e9dcdb53c6fc4ce124b7b6907d811d29deb0220056803812a68f5619ee1eccff5d8ca627597023fd95f6fdcd4b6f9e1eb2a462101

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100efd0a3e750e678ef1d6651c66be8f5c7ca8f78178537b2962bdb0426c1d5a26602201500df94033d4fe0496bf199f2ecb176599784fd762a1844f004b39fe2f4ccb501

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 47304402206474bed8fbe4e5a362078e71f9e43916070af3afb6c4287fd518559bf84e752b022052f59e58511809b129fb3a57efcc99462288eb7fbd7835fef669266ca94e48a101

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100e6932f09658140f57300ad3e8deaa26c6bf9e272af8fd6ede47ac49c0b8c1fba0220796bbd5b4219353ca863a797668c62dc3aea98669cb94a328adf706d846e005301

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 4730440220246b80498a201c80555ca8d027d3401e1e2a295b74dec131ba2fcb6403d934c90220762a5e41adaead26d5a7d31f6bd65416111cbe373da9e76238408c5145a1327901

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 47304402201683874f8a0b0a6e9b29f180af101f94861bbc5456ea24be09462b3f08c2c49f02203ed75363c7bac1ba9ff54d2d2815379a43cc02eaebe8a46f6f8c7de9492cdbb601

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 473044022040ab8ac3d3061b2f2076e796548e0409c33933f979ef46279fea89a9c00caf4d022068feac60d9bb5a0481ce8faa8d8386c0199eeef0a27324a73beb6abcdd8109e101

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100e876af910e5dd8bb98ebf831103dc94ef7aa227a9bd309fba46dfeb7071233af022100f734080203dca489449ae2e9efc9dc729d82745cf15bf85659255f01f8fd668901

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 4930460221008d8e28b4d7961e07d0a9ffe3befd391266563997b329534d3a01a678d305b760022100f6a27cdd08d17f15716203130801f30f8d4af655e867bcdb68100a2f5c104f6c01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 48304502204f5b403e4d68d1338835bcec099a7ac0a31234c53f56952c4e8aa5c0d9512b7f022100d2fd966abc6d263f375779912baac5301029993ade3a3a473bcc6841bbe0513601

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 47304402203acfef5c0772a403a62bfeb37e3dee9e7eeeaf3aea4bcc25601becf4b0dd79ce02200842695a5a1e588d90154ddd50dde0f7ee64c688d74d02c3707456353a963f4401

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 48304502200f9971d6b32fb6fc9e0b03bd7b49b87032a9ca13cab797d05b166aeb1fb3326b022100ba2050e962792aca8d07178fbf3eac95f874ceebec47f05193e2b16fb7e3d54601

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 48304502210093c6bf17aebaf9efbbf8816ccef362bb91fc506b98f1b6ac8a3629816e5643e9022036e84b31f2665e63d7110ab162656a360ac91641b911f71b30b4e77dd2f0d6cc01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 48304502206a1dba42aac7f10b58a46bb565339e4e394848ac6deaf8ae5f04a8da4a5565ad0221009f82fba7972819f132b6a6b8357fbde4c9aaee6e0c93ab8cc0fa0833f1062c2301

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 4730440220610056e43b2b16d9fffecb9c64c4ae0c4d1c501ed58b691a6484a7fcf5e156db0220395d2e75db9c70aab8facf46ec197f9e7d4aeee8ccd5f0d5f5f01e473a13d09801

output script: 410455e8f7cb1dabec2bcd34a899c2c11e03d2b68b6e4bd1f967897e2756c8a0b58633d112a7951415a01749104d7c41dbf682be98304ec7910ad8816d171e4c17acac
input script: 483045022100e460be8a4175e53fc1b4fd37e225a3efe0e7bfbf5d01b51f83f68adf01dc094902202a0e3a1424d63def00caa311b7ecdae63bafe77f99ee6cadce8f3ded52c98f3c01

output script: 410455e8f7cb1dabec2bcd34a899c2c11e03d2b68b6e4bd1f967897e2756c8a0b58633d112a7951415a01749104d7c41dbf682be98304ec7910ad8816d171e4c17acac
input script: 473044022027bf8021569b623046c5ed2a05fac1cd18809a5bfbdbb1cf51b9ea256c91eefd0220678a611e7a006be791189126d9ba8bd104e9a4697cec5d69f9b43a2eaa63e0e301

output script: 410455e8f7cb1dabec2bcd34a899c2c11e03d2b68b6e4bd1f967897e2756c8a0b58633d112a7951415a01749104d7c41dbf682be98304ec7910ad8816d171e4c17acac
input script: 493046022100af434a144a1b41b1297124141c1808a79c764b0a589530d4ad9a4f5e25bad990022100f654609affcc4ece5df7dd35509853f326ce1c099c0fdaf84509065fef2a535901

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100ea94200ca9f3f3b2484be3a82eb188563f8ff85bd825ab1fcab83ead8c02b677022065c14b9e8b6f1b452cedde21809a9cdc1b52f2cb698c984f2d416749eca50d5a01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100f2d6f5846c704b2f0aa0a15c672bf7874874d81cf5b1d73b28e28eea37c39891022100cc84c49ebc1ae0b5a6794e90fe56d121c6a9a7310898a65737c3edbbeaefcb3801

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 48304502201f3478e93f8ad56a1f64db2b9aa225023477b747850ae5185fe564e9b115e880022100cfb064585fdec49f3c5ef66e55b6e585d20647ba29d9c6d0ea5f05e61fd9cc1701

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100817a4fc3e8e4de9f056b3629b9f4dbff3debd661cb99ce108072bda2c51d989c0221009e97cb14436a9578cbc0ecd266888bb2f36ef738a1c33c1c521aa23a2676d7dc01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022040af37d6fe2f5e38dca4e98fccff3b057f730d6bee9f81fe7f825bdf388b26c7022100f53be622168f0fa58412772116090b0408cc2833b5772e39a869e186594b9bb301

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100b2d3c287543d9a8c02b8d35f273b9c0b20436f456d2f23e03b123bc96bc05adf022100ddbd8fd77d38b8abd1936eb56b575f69ed39e42274fa0700f0b1117e9f38c0ce01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100bf20a0cba79518c9d074a835e6c2fdc7655e19a8af9bca19396aa161d8579ad2022100ae052e1e4491cd627c495a352f66111c9b4e797a8784f4286506dbd27235349401

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 473044022072b268013c40b1fee0e5de85e316f32cd3e3856cf53ed0c3982814000d6d1f9b02201eee3563dd12ac222ec97cdf7f628e1eddfabc60253d503a985511893d31fbd001

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 493046022100fbbac336e35bdb9ce3a3156d1cc1823baf9adc5b409eeed850062177cebadc91022100b19e1b863c85f5204c77367b2fba031d66218163206ed29f879b4c5c01e7571501

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100c767ad483ba6b6ef4ce794027fa7dd4f643a70bae647b5c82790a53f53638c360220069f115affee11117257e33fccb04d63b948e45162d0fcee7d7ec519af6f81ca01

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 483045022100db423ea5f892e6f728f1f15f0897a2b0db909659b2944884c0f48801ea36549702206ee752d43f1d51bfb5d5be705face81c808538350786821682ba1bc6ec3dc4b101

output script: 76a91412ab8dc588ca9d5787dde7eb29569da63c3a238c88ac
input script: 48304502204464dc3788af495d691d7e89aca897370aa1f65031da6595df603dbe506d78c3022100c85950deefdc003cce2eaf6525cfa6f6016e120031ed0b21a09419cf9910d3fb01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 473044022025a37ed650db76723a98f94ce3537892985498e10d4b20856a350c434feefde00220164d1202a73f14419fbdc46c578deae5e4dde458ed907c77178b845e05f01f3c01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 47304402204874b63a4f744e5aa97dfc3a1a6125ed09b9ad57c46ec78cab90eb2906902796022072e172a835c4fa95025795f6313480d6e7d6f544cb4dec04a93f2a8cb9063d4b01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 4730440220518da6a6da8cdbc747aea8f78a8f773b562b6adfb6972d293fd9318540c519610220664652b9668cd1ffde5907a43e198f49b8baafb7ba28e4d62467bce77ad5250c01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 4830450220406b522dfabc547b3bc01c373627165b5680ed91fcb1bd403e5a5f0121fd863a022100979ff0c4a5a792dcbac2c00bfcf6c28d878b8b119119c5011b4a825a4394975801

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 493046022100dba98c672b3f10afa4a037df2cc013c75619881a70ec1b402f427efc2e01d908022100faca338d588a81a7e22d22b1b21a205145d421d448a1a56b08c889499f633c2a01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 48304502203a4047759f64e87866df43db7ad7c0c753577e8bdc161986ec38f6112ee503e7022100e654fc17aa69360c00cab21a4f2506f82f72a721129a160322157b9279bddd0a01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 4930460221008a0164e232e91868bc6aa60f7c9818111f4914473de946c224d22ad2a315082902210086b428fcd70492ecdf524fe08ef3cc571bf80597b2ccd35fb08e96055d80717001

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 493046022100d57b90ae62844bb1e4c0ee5645f78aa04236c7be0066a088beee697c5e18aa4a022100bd387bcbe2dbeda88a71fe60354f583ebb91f8adf83cc3854227d0d0cfa05aec01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 493046022100ad59b9d13cb85848d071cca54940dd949092b5a02df827ecfcd72236be0a0d87022100fa9991d76fb3f04286540e8087fb0a015d23695e242da4667965e835a1fdb65201

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 48304502200ebc08834c1dec996b4c3e56d6f15323d7873e08bd33286d732686b08dfbab83022100f49734964bcaace79517e206e4f6c0da2fa84ffdd09b0f7beb7cd90a552d947101

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 47304402202c2abbc41211af1daecaa8aa9cf1e6fc64110f905e20bf10572360647b021c5702206371ec96cab58640452f22a099685f7de214a58f470df0e0e0b3865b5314e90301

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 483045022100844deca73803f6a00a10952f833b40c8f75b9a446abccf72688fd163d7d0e9d802200238bb58d74c0405be87f0e832e8e634425ce80dcfa1ee3d5d373a211bca8d5801

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 493046022100b39a1a7734a7b943338ff505d2e7499167f18161800aae3a7be2e1e84b32a13e022100e6365d947101b6bd0694277bcd8b786d891c8f62877d03639ba0aa0e0cf895ee01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 493046022100c1386af429a3b7f1b7f3a09a514799850c7a9f417247260834faced9eddeb6450221009eb2bdd405536f519aac790a56e68d660250c0f0ead7359215e219731ba5fb0c01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 48304502200c05d4e9ba5e31ffe9a9a10abbe64c65360a25b9df86818dcd9cb520dce8464e022100fb5179c27bfafca6f7152a5cbb3b7d9f5bed39e8c26a242e962c353a28ce1d5b01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 483045022100fc77f82033bed120b5a86009d7c565a8ed97ac3bf93e84a840113cbe107987880220143d23f61761f7d78eee734c08c8333ecb3c9522e52e6767f50df7ea2e1be58c01

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 47304402207a8d847e124f5b51f02a488659d4f6c00737d4e402c193f2029dd783c6738ff90220144048c4d8eb83176b7536c022ca366bd81a478307d2fdc33d99beafdd5b098901

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 493046022100d8bf1b5633988c29834783e3e4e60ebb6bc8a3d2aafbb6ae74ac7708042ed0ac022100b47f4b1c159f6636d749972e9934d00860fbfa3511391671a9efe6f7acd4a21001

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 483045022100a6ebf7e2ce53cb97e1d3659d8fee6d684ce4f8232d011fb95caa73aa18b157fc02202c9527d3e174a888dec9864f701f19a9b0924140337d12d2e0199ea54a98224401

output script: 76a914482f0027662731277fdfa3b7f639c976a3bab11e88ac
input script: 4730440220558e2b55f5ed08b3af18133688507218050007554b8d1a8f5c4d35878944c9a6022076d5cbf036deb1fbd122b8bb829a8e6247d5bbdea543fb7ff64d833f8cb0f3d501

output script: 76a9147bbaf3e7e5175be45aa2c51d1dfbd244ac90e1c388ac
input script: 483045022100822d95033d96b49a3688206a6bb2539319668c5277559c63248579bfb1e3e1510220145b46ce2c9dbf39fb6670c1544ffe0568cafc2878cbd8a513969876bcaa8bd401

output script: 76a9147bbaf3e7e5175be45aa2c51d1dfbd244ac90e1c388ac
input script: 493046022100b97fd7620074a8cf5852395b68fec60c8fabd6efbaf908eb645950e08f7811fc022100b6dfd47a031daae21a985bb8fe53773d55b83e3e4499f2202103fc96deb6b9d701

output script: 76a91454c22ddfa26c9fd050414919dff5320c45a33e5888ac
input script: 48304502205e75cfc18f0965e5a69655b040cb86e41ada89ff5b9c41c7a7376b4ee09a44d0022100acc38bb1b7b227fe2852059e2a76484bae7c584044ece5f4e06e33ec4c60aa2f01

output script: 76a9146934efcef36903b5b45ebd1e5f862d1b63a99fa588ac
input script: 4930460221009f8aef83489d5c3524b68ddf77e8af8ceb5cba89790d31d2d2db0c80b9cbfd26022100bb2c13e15bb356a4accdd55288e8b2fd39e204a93d849ccf749eaef9d8162787014104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dc

output script: 76a9146934efcef36903b5b45ebd1e5f862d1b63a99fa588ac
input script: 493046022100b687c4436277190953466b3e4406484e89a4a4b9dbefea68cf5979f74a8ef5b1022100d32539ffb88736f3f9445fa6dd484b443ebb31af1471ee65071c7414e3ec007b014104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dc

output script: 410403c344438944b1ec413f7530aaa6130dd13562249d07d53ba96d8ac4f59832d05c837e36efd9533a6adf1920465fed2a4553fb357844f2e41329603c320753f4ac

input script: 483045022100ef78daeb60d6332fa6f91ee93d95486d8601b5f2c1d1dc77633801dc6c0eb419022015b19e34de00ae729e20b97de8ac58ea8bb9227ba91a33bfaa26b7480e8a000501
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 473044022100a154551bb4360cc21ea35cb5825739273136d442331c3d36fbc0229718c56c4d021f320abfcb786b8da7de5f9618996d412223b5e8cee13c250a4cf6afe9c0fe0601
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 493046022100ce0a48fa70d0d39e54278691846492b9dca4949a9cbe8029a8d0df12273843cb022100cdcf8f7d862dedf028c6db3754435e4c9fbeae349a0b10698da635f75089632d01
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 493046022100f8fe1baf5e0ea324944085b1fef9686cf7f92c6a8135fd2dcd0b02c779bda6fd022100dc35626c91eb153ab857efbd0868c965acaa73fbc61d03a88d660cabed4079f601
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 47304402205e96e3b00bcb93f49a31033e7ad413cd86a6919d2c7c2dd370970dddd527aad402205399f6bdc0d2cddcf5c3b079b252413e23287115a416778c8665a13994d1a0e401
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 483045022100a86b66f2049e28b27fc6ff13661e18320ab9b91d14170e4089b3152a3147238902202033814979be520ed19b7819d691a5f862f71493e86288d80b7c09891b26801801
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 483045022100e0b8042f08ad0ab956b35e325779c9aac245d0d8f3c6e0129c09433d0cfd2f2702206dedbffdc5ca31bcf27e07c00703a03ea969f0251a20cd5dd79097bcb841ac2201
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 49304602210098005b0a8c051c659d96fbbb65b7aa32fb4fcece418023c1e6daa09e310dda9e022100a80cab0f026fdbbb0ad697ffb4c8985da9e3e92588b82cf61ed9d1c33572e53801
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 473044022054d9a0b132ca4718c2796d3ab76e56664865f7b9f142c05ec2fea7e2af086f8f0220249e2bee1b61485b1613682f52df03b70a5bcdc3ef919d2f159d5b9d3bf2ec2a014104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dc
output script: 76a9146934efcef36903b5b45ebd1e5f862d1b63a99fa588ac

input script: 4930460221009eb96373476e21706455102848d09d5cf92ccb624c3366b1c616a45e5c6171f2022100e809eeb6659a1c1c2d34ad39f3434a93745a94cbccda59a251072ee6d2b6634901
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 4930460221008249d3ff20dd1fb30afbe751a61430beebbc52be5066200fd1ad0f97b08bd4ed022100d3cd88e140f97cd4c788f60f2f485e37539c2f25cb1cf54a9802f7f55e76bb86014104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dc
output script: 76a9146934efcef36903b5b45ebd1e5f862d1b63a99fa588ac

input script: 483045022020bf532bc54b3a9ba5c0a9e829901d3383647d2c0355230c4660782552d7d23602210081f2a8bcb3bc2ae89d4536cf90ddea262790508ed9c9257cc87ea9872a3ad0d501
output script: 4104f9804cfb86fb17441a6562b07c4ee8f012bdb2da5be022032e4b87100350ccc7c0f4d47078b06c9d22b0ec10bdce4c590e0d01aed618987a6caa8c94d74ee6dcac

input script: 47304402203cb5086f76c19f4120523550f063808e8e8b8e22455b65079d7fc373b01c85fe0220128389fd174bba0b035e916c65ff59ff6ed46b7720dded11b86e06f918330d5e01
output script: 76a914ae5348c31e7154d26d29da3ab03383534647919e88ac

input script: 483045022100d218788520228f202ee29ab37136e78c6f907e6a376acec375442623f91da9c202201a98db00687af1171626d98be8a78f9a733e50c3b8e56640f8f284c8c19ed5cc014104f9f7d18f6262010cd24bc8414bbd15a61396ef817bc7f3c64a200db3c9aec9f4bbf891b7e520386862f0c17aae7d0bdb65d005e19295bdf1aef6834ddd36edc8
output script: 76a9147bbaf3e7e5175be45aa2c51d1dfbd244ac90e1c388ac

input script: 4930460221009768fcd8c39f59ad4b22bb2dbbd5bd750c9dea1a42a1731d5dfbf7ea91ec2589022100f1b65bb2eb1e0783254ca446c39c0b305cb4e9d9fea0f01e91cac2ff65d8df8501
output script: 4104e6e2e3419cb358abdb6a5a1eaba1d911be99cbedfe30aee9f36331800ead690c7a200e6cf2662708c362a6a50fd1f45aa73dfb8e76cadfa095c34e8db87f8f3dac
*/
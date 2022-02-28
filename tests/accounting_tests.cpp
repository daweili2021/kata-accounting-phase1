#include <boost/test/unit_test.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>
#include "contracts.hpp"
#include <unistd.h>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

/* used to be this
using namespace eosio::testing;
using namespace eosio;
using namespace fc;
//using namespace eosio::chain;
*/

BOOST_AUTO_TEST_SUITE(accounting_tests)


BOOST_AUTO_TEST_CASE(post) try {

    tester t{setup_policy::none};

   // Create eosio.token account and load the contract
   	t.create_account("eosio.token"_n, config::system_account_name, false, true);
   	t.set_code("eosio.token"_n, eosio::testing::contracts::token_wasm());
   	t.set_abi("eosio.token"_n, eosio::testing::contracts::token_abi().data());
    // Create a test account and load the accounting contract
    t.create_account("alice"_n, config::system_account_name, false, true);
    t.set_code("alice"_n, eosio::testing::contracts::accounting_wasm());
    t.set_abi("alice"_n, eosio::testing::contracts::accounting_abi().data());
   
    t.produce_block();

    // create token SYS
    t.push_action(
        "eosio.token"_n, "create"_n, "eosio.token"_n,
        mutable_variant_object("issuer", "eosio.token")(
        "maximum_supply", "10000000.0000 SYS")
    );

    //issue SYS tokens
    t.push_action(
        "eosio.token"_n, "issue"_n, "eosio.token"_n,
        mutable_variant_object
        ("to", "eosio.token")
        ("quantity", "1000.0000 SYS")
        ("memo", "")
    );
    
    // Send symbols to alice
    t.push_action("eosio.token"_n, "transfer"_n, {"eosio.token"_n, "alice"_n},
        mutable_variant_object("from", "eosio.token")("to", "alice")(
        "quantity", "200.0000 SYS")("memo", "memo"));
    // check the balance of the default category
    t.push_action(
            "alice"_n, "checkbalance"_n, "alice"_n,
            mutable_variant_object("category", "default")("amount", "400.0000 SYS")
    );

    t.push_action(
            "alice"_n, "addcategory"_n, "alice"_n,
            mutable_variant_object("new_category", "checking")
    );
    
    t.push_action(
            "alice"_n, "addcategory"_n, "alice"_n,
            mutable_variant_object("new_category", "saving")
    );
    
    // Duplicate category name
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "create"_n, "alice"_n,
                        mutable_variant_object("new_category", "checking")
                    );
        }(),
        fc::exception);
    
    // internal transfers
    t.push_action(
            "alice"_n, "internaltran"_n, "alice"_n,
            mutable_variant_object("acct_from", "default")("acct_to", "checking")("amount", "40.0000 SYS")("memo", "default to checking")
        );
    // check the balance of the checking category
    t.push_action(
            "alice"_n, "checkbalance"_n, "alice"_n,
            mutable_variant_object("category", "checking")("amount", "40.0000 SYS")
    );
    t.push_action(
            "alice"_n, "internaltran"_n, "alice"_n,
            mutable_variant_object("acct_from", "default")("acct_to", "saving")("amount", "60.0000 SYS")("memo", "default to saving")
        );
    // check the balance of the saving category
    t.push_action(
            "alice"_n, "checkbalance"_n, "alice"_n,
            mutable_variant_object("category", "saving")("amount", "60.0000 SYS")
    );
    t.push_action(
            "alice"_n, "internaltran"_n, "alice"_n,
            mutable_variant_object("acct_from", "saving")("acct_to", "checking")("amount", "10.0000 SYS")("memo", "saving to checking")
        );
    t.push_action(
            "alice"_n, "internaltran"_n, "alice"_n,
            mutable_variant_object("acct_from", "saving")("acct_to", "checking")("amount", "0.0001 SYS")("memo", "saving to checking: minimum amount")
        );
    
    // external transfers
    t.push_action("eosio.token"_n, "transfer"_n, "alice"_n,
         mutable_variant_object("from", "alice")("to", "eosio.token")(
        "quantity", "10.0000 SYS")("memo", "default to eosio"));
    // check the balance of the default category
    t.push_action(
            "alice"_n, "checkbalance"_n, "alice"_n,
            mutable_variant_object("category", "default")("amount", "290.0000 SYS")
    );
    // negative amount
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "internaltran"_n, "alice"_n,
                        mutable_variant_object("acct_from", "checking")("acct_to", "saving")("amount", "-10.0000 SYS")("memo", "negative amount test")
                    );
        }(),
        fc::exception);
    // overdraft from cheking to saving
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "internaltran"_n, "alice"_n,
                        mutable_variant_object("acct_from", "checking")("acct_to", "saving")("amount", "51.0000 SYS")("memo", "overdraft test")
                    );
        }(),
        fc::exception);
    // overdraft from saving to default
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "internaltran"_n, "alice"_n,
                        mutable_variant_object("acct_from", "saving")("acct_to", "default")("amount", "300.0000 SYS")("memo", "overdraft test")
                    );
        }(),
        fc::exception);
    // nonexistent category
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "internaltran"_n, "alice"_n,
                        mutable_variant_object("acct_from", "saing")("acct_to", "default")("amount", "30.0000 SYS")("memo", "nonexistent category test")
                    );
        }(),
        fc::exception);
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "internaltran"_n, "alice"_n,
                        mutable_variant_object("acct_from", "saving")("acct_to", "dfault")("amount", "30.0000 SYS")("memo", "nonexistent category test")
                    );
        }(),
        fc::exception);
    // overdraft from default
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "eosio.token"_n, "transfer"_n, "alice"_n,
                        mutable_variant_object("from", "alice")("to", "eosio.token")("balance", "5000.0000 SYS")("memo", "overdraft from the default category")
                    );
        }(),
        fc::exception);

    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "eosio.token"_n, "transfer"_n, "alice"_n,
                        mutable_variant_object("from", "alice")("to", "eosio.token")("balance", "-50.0000 SYS")("memo", "negative allowed?")
                    );
        }(),
        fc::exception);
    
    // kata phase 2 test cases
    // singlepay action
    t.push_action(
            "alice"_n, "singlepay"_n, "alice"_n,
            mutable_variant_object("from", "saving")("to", "eosio.token")("balance", "10.0000 SYS")("memo", "saving to eosio")
    );
    // category saing doesn't exist
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "setfuturetra"_n, "alice"_n,
                        mutable_variant_object("category", "saing")("amount", "10.0000 SYS")("time", "1577836806")
                    );
        }(),
        fc::exception);
    // overdraft
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "setfuturetra"_n, "alice"_n,
                        mutable_variant_object("category", "saving")("amount", "100000.0000 SYS")("time", "1577836806")
                    );
        }(),
        fc::exception);
    
    // The current time of the boost test framework is around 1577836801
    // setfuturetra action
    t.push_action(
            "alice"_n, "setfuturetra"_n, "alice"_n,
            mutable_variant_object("category", "saving")("amount", "10.0000 SYS")("time", "1577836863")
    );
    // category cheking doesn't exist
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "setfuturetra"_n, "alice"_n,
                        mutable_variant_object("category", "cheking")("amount", "10.0000 SYS")("time", "1577836806")
                    );
        }(),
        fc::exception);
    // overdraft
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "setfuturetra"_n, "alice"_n,
                        mutable_variant_object("category", "saving")("amount", "100000.0000 SYS")("time", "1577836806")
                    );
        }(),
        fc::exception);
    // given time is not a future time
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "setfuturetra"_n, "alice"_n,
                        mutable_variant_object("category", "saving")("amount", "10.0000 SYS")("time", "1577836800")
                    );
        }(),
        fc::exception);
    // only one future transfer is allowed
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "setfuturetra"_n, "alice"_n,
                        mutable_variant_object("category", "saving")("amount", "10.0000 SYS")("time", "1577836806")
                    );
        }(),
        fc::exception);
    t.push_action(
            "alice"_n, "setfuturetra"_n, "alice"_n,
            mutable_variant_object("category", "checking")("amount", "10.0000 SYS")("time", "1577836803")
    );
    
    t.push_action(
        "alice"_n, "listfuture"_n, "alice"_n,
        mutable_variant_object()
    );

    // dofuturetran action
    // too early to execute the scheduled transfer
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "dofuturetran"_n, "alice"_n,
                        mutable_variant_object("category", "checking")
                    );
        }(),
        fc::exception);
    t.produce_blocks(10);
    sleep(10);
    t.produce_block();
    // unable to cancel if the current time is older than the scheduled time
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "cancelfuture"_n, "alice"_n,
                        mutable_variant_object("category", "checking")
                    );
        }(),
        fc::exception);
    t.push_action(
            "alice"_n, "dofuturetran"_n, "alice"_n,
            mutable_variant_object("category", "checking")
    );

    // cancelfuture action
    // cancel non-existent future transfer
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "canclefuture"_n, "alice"_n,
                        mutable_variant_object("category", "checking")
                    );
        }(),
        fc::exception);
    t.push_action(
            "alice"_n, "cancelfuture"_n, "alice"_n,
            mutable_variant_object("category", "saving")
    );
    
    // setrecurring action
    t.push_action(
            "alice"_n, "setrecurring"_n, "alice"_n,
            mutable_variant_object("category", "checking")("amount", "10.0000 SYS")("time", "1577836823")("interval", "10")
    );
    // category checing doesn't exist
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "setrecurring"_n, "alice"_n,
                        mutable_variant_object("category", "checing")("amount", "10.0000 SYS")("time", "1577836823")("interval", "10")
                    );
        }(),
        fc::exception);
    // overdraft
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "setrecurring"_n, "alice"_n,
                        mutable_variant_object("category", "checking")("amount", "10000.0000 SYS")("time", "1577836823")("interval", "10")
                    );
        }(),
        fc::exception);
    // not a future time
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "setrecurring"_n, "alice"_n,
                        mutable_variant_object("category", "checking")("amount", "10.0000 SYS")("time", "1577836800")("interval", "10")
                    );
        }(),
        fc::exception);

    // recurringtra action
    // category cheking doesn't exist
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "recurringtra"_n, "alice"_n,
                        mutable_variant_object("category", "cheking")
                    );
        }(),
        fc::exception);

    t.push_action(
        "alice"_n, "listfuture"_n, "alice"_n,
        mutable_variant_object()
    );

    // generictran action
    t.push_action(
        "alice"_n, "generictran"_n, "alice"_n,
        mutable_variant_object()
    );

    // cancelrecur action
    t.push_action(
                "alice"_n, "cancelrecur"_n, "alice"_n,
                mutable_variant_object("category", "checking")
            );
    // category cheking doesn't exist
    BOOST_CHECK_THROW(
        [&] {
            t.push_action(
                        "alice"_n, "cancelrecur"_n, "alice"_n,
                        mutable_variant_object("category", "cheking")
                    );
        }(),
        fc::exception);
}


FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()

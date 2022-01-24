#include <boost/test/unit_test.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>
#include "contracts.hpp"

using namespace eosio::testing;
using namespace eosio;
using namespace fc;
using namespace eosio::chain;


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
    vector<name> accounts = {"eosio.token"_n, "alice"_n};
    t.push_action("eosio.token"_n, "transfer"_n, accounts,
        mutable_variant_object("from", "eosio.token")("to", "alice")(
        "quantity", "200.0000 SYS")("memo", "memo"));
    
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
    t.push_action(
            "alice"_n, "internaltran"_n, "alice"_n,
            mutable_variant_object("acct_from", "default")("acct_to", "saving")("amount", "60.0000 SYS")("memo", "default to saving")
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
}


FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()

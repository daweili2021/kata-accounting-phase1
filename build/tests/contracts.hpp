#pragma once

#include <eosio/testing/tester.hpp>

namespace eosio { namespace testing {

struct contracts {
   static std::vector<uint8_t> token_wasm() { return read_wasm("/Users/dawei.li/Work/eosio.contracts/build/contracts/eosio.token/eosio.token.wasm"); }
   static std::vector<char>    token_abi() { return read_abi("/Users/dawei.li/Work/eosio.contracts/build/contracts/eosio.token/eosio.token.abi"); }

   static std::vector<uint8_t> accounting_wasm() { return read_wasm("/Users/dawei.li/Work/kata_accounting/build/tests/../accounting/accounting.wasm"); }
   static std::vector<char>    accounting_abi() { return read_abi("/Users/dawei.li/Work/kata_accounting/build/tests/../accounting/accounting.abi"); }
};

}}

cmake_minimum_required(VERSION 3.8)
project(accounting)



set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( accounting accounting ${CMAKE_SOURCE_DIR}/accounting.cpp )

target_include_directories(
        accounting PUBLIC
        ${EOSIO_CONTRACTS_ROOT}/contracts/eosio.token/include
        ${EOSIO_ROOT}/libraries/chain/include 
        )
    
set_target_properties(accounting
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

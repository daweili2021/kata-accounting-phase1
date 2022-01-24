cmake_minimum_required( VERSION 3.8 )

project(accounting_tests)
set(EOSIO_WASM_OLD_BEHAVIOR "Off")

find_package(eosio)

include(CTest)
enable_testing()

configure_file(${CMAKE_SOURCE_DIR}/contracts.hpp.in ${CMAKE_BINARY_DIR}/contracts.hpp)

include_directories(${CMAKE_BINARY_DIR}
	${EOSIO_CONTRACTS_ROOT}/tests
	#${EOSIO_CONTRACTS_ROOT}/build/tests
	${EOSIO_CONTRACTS_ROOT}/contracts/eosio.token/include
    #${EOSIO_ROOT}/libraries/chain/include
)

add_eosio_test(accounting_tests accounting_tests.cpp main.cpp)

target_compile_options(accounting_tests PUBLIC -ftemplate-backtrace-limit=0)

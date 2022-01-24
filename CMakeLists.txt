cmake_minimum_required(VERSION 3.8)
project(kata_accounting_project)

include(ExternalProject)

# if no cdt root is given use default path
#if(EOSIO_CDT_ROOT STREQUAL "" OR NOT EOSIO_CDT_ROOT)
#    message("EOSIO_CDT_ROOT is missing")
#    find_package(eosio.cdt)
#endif()

find_package(eosio)
set(EOSIO_ROOT $ENV{EOSIO_ROOT})
set(EOSIO_CDT_ROOT $ENV{EOSIO_CDT_ROOT})
set(EOSIO_CONTRACTS_ROOT $ENV{EOSIO_CONTRACTS_ROOT})
message("DEBUG MESSAGE")
message(${CMAKE_BINARY_DIR})
ExternalProject_Add(
        kata_project
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/src
        BINARY_DIR ${CMAKE_BINARY_DIR}/accounting
        CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${EOSIO_CDT_ROOT}/lib/cmake/eosio.cdt/EosioWasmToolchain.cmake  -DEOSIO_ROOT=${EOSIO_ROOT} -DEOSIO_CONTRACTS_ROOT=${EOSIO_CONTRACTS_ROOT}
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
        INSTALL_COMMAND ""
        BUILD_ALWAYS 1
)


ExternalProject_Add(
    accounting_tests
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/tests
    BINARY_DIR ${CMAKE_BINARY_DIR}/tests
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DEOSIO_ROOT=${EOSIO_ROOT} -DEOSIO_CONTRACTS_ROOT=${EOSIO_CONTRACTS_ROOT}
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    TEST_COMMAND ""
    INSTALL_COMMAND ""
    BUILD_ALWAYS 1
)

add_dependencies(accounting_tests kata_project)



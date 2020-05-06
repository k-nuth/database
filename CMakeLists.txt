# Copyright (c) 2016-2020 Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.8)

# kth-database
#==============================================================================
project(kth-database
        VERSION 0
        LANGUAGES CXX C)
#        VERSION 0.11.0

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Check for baseline language coverage in the compiler for the C++14 standard.
#------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Process options.
#==============================================================================

# Implement --use-conan
#------------------------------------------------------------------------------
option(USE_CONAN "Use Conan Build Tool." OFF)
option(NO_CONAN_AT_ALL "Conan totally disabled." OFF)

if (NO_CONAN_AT_ALL)
  set(USE_CONAN OFF)
endif()


# Inherit --enable-shared and define BOOST_TEST_DYN_LINK.
#------------------------------------------------------------------------------
option(ENABLE_SHARED "" OFF)

# Implement POSITION_INDEPENDENT_CODE
#------------------------------------------------------------------------------
option(ENABLE_POSITION_INDEPENDENT_CODE "Enable POSITION_INDEPENDENT_CODE property" ON)

# Implement --with-tests and declare WITH_TESTS.
#------------------------------------------------------------------------------
option(WITH_TESTS "Compile with unit tests." ON)

# Implement --with-tools and declare WITH_TOOLS.
#------------------------------------------------------------------------------
option(WITH_TOOLS "Compile with tools." OFF)


set(KTH_PROJECT_VERSION "-" CACHE STRING "Specify the Knuth Project Version.")
# message(${KTH_PROJECT_VERSION})

set(MICROARCHITECTURE "x86_64" CACHE STRING "Specify the Microarchitecture (x86_64|...).")
message( STATUS "Knuth: Compiling for ${MICROARCHITECTURE}")
# add_definitions(-DKTH_MICROARCHITECTURE=${MICROARCHITECTURE})
# add_definitions(-DKTH_MICROARCHITECTURE_STR="${MICROARCHITECTURE}")

set(MARCH_ID "" CACHE STRING "Specify the Microarchitecture ID (x86_64|...).")
message( STATUS "Knuth: Compiling for Microarchitecture ID ${MARCH_ID}")


# Implement --with-measurements and declare WITH_MEASUREMENTS.
#------------------------------------------------------------------------------
option(WITH_MEASUREMENTS "Measurements enabled." OFF)
if (WITH_MEASUREMENTS)
  message(STATUS "Knuth: MEASUREMENTS enabled")
  add_definitions(-DWITH_MEASUREMENTS)
endif()

# Implement --db_readonly_mode and declare KTH_DB_READONLY.
#------------------------------------------------------------------------------
option(DB_READONLY_MODE "Readonly DB mode enabled." OFF)
if (DB_READONLY_MODE)
  message(STATUS "Knuth: Readonly DB mode enabled")
  add_definitions(-DKTH_DB_READONLY)
endif()

# Implement --with-cached-rpc-data and declare WITH_CACHED_RPC_DATA.
#------------------------------------------------------------------------------
option(WITH_CACHED_RPC_DATA "Cached RPC data enabled." OFF)
if (WITH_CACHED_RPC_DATA)
  message(STATUS "Knuth: Cached RPC data enabled")
  add_definitions(-DKTH_CACHED_RPC_DATA)
endif()


# Implement and declare DB_TRANSACTION_UNCONFIRMED.
#------------------------------------------------------------------------------
option(DB_TRANSACTION_UNCONFIRMED "DB Transactions Unconfirmed enabled." OFF)
if (DB_TRANSACTION_UNCONFIRMED)
  message(STATUS "Knuth: DB Transactions Unconfirmed enabled")
  add_definitions(-DKTH_DB_TRANSACTION_UNCONFIRMED)
endif()

# Implement and declare DB_SPENDS.
#------------------------------------------------------------------------------
option(DB_SPENDS "DB Spends enabled." OFF)
if (DB_SPENDS)
  message(STATUS "Knuth: DB Spends enabled")
  add_definitions(-DKTH_DB_SPENDS)
endif()

# Implement and declare DB_HISTORY.
#------------------------------------------------------------------------------
option(DB_HISTORY "DB History enabled." OFF)
if (DB_HISTORY)
  message(STATUS "Knuth: DB History enabled")
  add_definitions(-DKTH_DB_HISTORY)
endif()

# Implement and declare DB_STEALTH.
#------------------------------------------------------------------------------
option(DB_STEALTH "DB Stealth enabled." OFF)
if (DB_STEALTH)
  message(STATUS "Knuth: DB Stealth enabled")
  add_definitions(-DKTH_DB_STEALTH)
endif()

# Implement and declare DB_UNSPENT_LEGACY.
#------------------------------------------------------------------------------
option(DB_UNSPENT_LEGACY "DB Unspent Legacy enabled." OFF)
if (DB_UNSPENT_LEGACY)
  message(STATUS "Knuth: DB Unspent Legacy enabled")
  add_definitions(-DKTH_DB_UNSPENT_LEGACY)
endif()

# Implement and declare DB_LEGACY.
#------------------------------------------------------------------------------
option(DB_LEGACY "DB Legacy enabled." OFF)
if (DB_LEGACY)
  message(STATUS "Knuth: DB Legacy enabled")
  add_definitions(-DKTH_DB_LEGACY)
endif()

# Implement and declare DB_NEW.
#------------------------------------------------------------------------------
option(DB_NEW "DB New enabled." OFF)
if (DB_NEW)
  message(STATUS "Knuth: DB New enabled")
  add_definitions(-DKTH_DB_NEW)
endif()

# Implement and declare DB_NEW_BLOCKS.
#------------------------------------------------------------------------------
option(DB_NEW_BLOCKS "DB Blocks New enabled." OFF)
if (DB_NEW_BLOCKS)
  message(STATUS "Knuth: DB Blocks New enabled")
  add_definitions(-DKTH_DB_NEW_BLOCKS)
endif()

# Implement and declare DB_NEW_FULL.
#------------------------------------------------------------------------------
option(DB_NEW_FULL "DB New Full enabled." OFF)
if (DB_NEW_FULL)
  message(STATUS "Knuth: DB New Full enabled")
  add_definitions(-DKTH_DB_NEW_FULL)
endif()

# Implement --JUST_KTH_SOURCES for linting.
#------------------------------------------------------------------------------
option(JUST_KTH_SOURCES "Just Knuth source code to be linted." OFF)
if (JUST_KTH_SOURCES)
  message(STATUS "Knuth: Just Knuth source code to be linted: enabled")
endif()

# # Implement --binlog.
# #------------------------------------------------------------------------------
# option(BINLOG "Uses BinLog logging library." OFF)
# if (BINLOG)
#   add_definitions(-DKTH_USE_BINLOG)
# endif()

set(LOG_LIBRARY "boost" CACHE STRING "Setting for the logging library (boost|spdlog|binlog).")

if (${LOG_LIBRARY} STREQUAL "boost")
  add_definitions(-DKTH_LOG_LIBRARY_BOOST)
elseif (${LOG_LIBRARY} STREQUAL "spdlog")
  add_definitions(-DKTH_LOG_LIBRARY_SPDLOG)
elseif (${LOG_LIBRARY} STREQUAL "binlog")
  add_definitions(-DKTH_LOG_LIBRARY_BINLOG)
else()
  message(FATAL_ERROR "Invalid Logging Library: ${LOG_LIBRARY}")
endif()

message(STATUS "Knuth: Logging Library: ${LOG_LIBRARY}")

# Implement --use_libmdbx.
#------------------------------------------------------------------------------
option(USE_LIBMDBX "Uses libmdbx DB library." OFF)
if (USE_LIBMDBX)
  add_definitions(-DKTH_USE_LIBMDBX)
endif()


#------------------------------------------------------------------------------
set(CURRENCY "BCH" CACHE STRING "Specify the Cryptocurrency (BCH|BTC|LTC).")

if (${CURRENCY} STREQUAL "BCH")
  add_definitions(-DKTH_CURRENCY_BCH)
elseif (${CURRENCY} STREQUAL "BTC")
  add_definitions(-DKTH_CURRENCY_BTC)
elseif (${CURRENCY} STREQUAL "LTC")
  add_definitions(-DKTH_CURRENCY_LTC)
else()
  message(FATAL_ERROR "Invalid Cryptocurrency: ${CURRENCY}")
endif()

message(STATUS "Knuth: Cryptocurrency: ${CURRENCY}")


# Conan infrastructure
#==============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/ci_utils/cmake/knuthbuildinfo.cmake)
    include(${CMAKE_SOURCE_DIR}/ci_utils/cmake/knuthbuildinfo.cmake)
else()
    message( STATUS "knuthbuildinfo doent exists")
endif()



# Check dependencies.
#==============================================================================
#------------------------------------------------------------------------------
if (NO_CONAN_AT_ALL)
  # Require Boost of at least version 1.56.0 and output ${boost_CPPFLAGS/LDFLAGS}.
  #------------------------------------------------------------------------------
  if (NOT ENABLE_SHARED)
    set(Boost_USE_STATIC_LIBS ON)
  endif()

  if (MSVC)
    find_package(
      Boost 1.56.0 REQUIRED
      COMPONENTS atomic chrono date_time filesystem iostreams locale log log_setup program_options regex system thread unit_test_framework
    )
  else()
    find_package(
      Boost 1.56.0 REQUIRED
      COMPONENTS chrono date_time filesystem iostreams locale log program_options regex system thread unit_test_framework
    )
  endif()
endif()

if (NOT USE_CONAN)
    # Require kth-domain of at least version 0 and output ${kth_domain_CPPFLAGS/LIBS/PKG}.
    #------------------------------------------------------------------------------
    if (NOT TARGET kth-domain)
            find_package(kth-domain 0 REQUIRED)
    endif()
endif()


# Set flags.
#==============================================================================
include(CheckCXXCompilerFlag)

# Including common functions
#------------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/ci_utils/cmake)

include(KnuthTools)


# Warn on all stuff.
#------------------------------------------------------------------------------
if (NOT MSVC)
  _add_c_compile_flag(-Wall _has_all_warning_flag)
else()
  _add_c_compile_flag(-W4 _has_all_warning_flag)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Warn on extra stuff.
#------------------------------------------------------------------------------
if (NOT MSVC)
  _add_c_compile_flag(-Wextra _has_extra_warning_flag)
endif()

# Be really annoying.
#------------------------------------------------------------------------------
_add_c_compile_flag(-Wpedantic _has_pedantic_warning_flag)
if (_has_pedantic_warning_flag)
  _add_c_compile_flag(-pedantic _has_pedantic_flag)
endif()

# Conform to style.
#------------------------------------------------------------------------------
_add_cxx_compile_flag(-Wno-missing-braces _has_no_missing_braces_warning_flag)

# Conflict in stdlib under clang. Enabled in clang only.
#------------------------------------------------------------------------------
_add_cxx_compile_flag(-Wno-mismatched-tags _has_no_mismatched_tags_warning_flag)

# Clean up boost 1.55 headers. Enabled in gcc only.
#------------------------------------------------------------------------------
_add_c_compile_flag(-Wno-deprecated-declarations _has_no_deprecated_declarations_warning_flag)

# Protect stack.
#------------------------------------------------------------------------------
_add_link_flag(-fstack-protector _has_stack_protector_flag)

# Protect stack comprehensively.
#------------------------------------------------------------------------------
_add_link_flag(-fstack-protector-all _has_stack_protector_all_flag)

# Hide internal functions from external libs. Enabled in gcc only.
#------------------------------------------------------------------------------
_add_cxx_compile_flag(-fvisibility-hidden _has_visibility_hidden_flag)

# Hide inlines from external libs. Enabled in gcc only.
#------------------------------------------------------------------------------
_add_cxx_compile_flag(-fvisibility-inlines-hidden _has_visibility_inlines_hidden_flag)

# Target Windows Vista. Enabled in msvc only.
#------------------------------------------------------------------------------
if (MSVC) # OR MINGW)
  add_definitions(-D_WIN32_WINNT=0x0600)
endif()

# Build
#==============================================================================

# src/kth-database.la => ${libdir}
#------------------------------------------------------------------------------
set(MODE STATIC)
if (ENABLE_SHARED)
    set(MODE SHARED)
endif()

set(kth_database_sources_just_legacy
    src/data_base.cpp
    src/settings.cpp
    src/store.cpp
    src/version.cpp
)



if (DB_LEGACY)
    set(kth_database_sources_just_legacy 
        ${kth_database_sources_just_legacy}
        src/legacy/memory/accessor.cpp
        src/legacy/memory/allocator.cpp
        src/legacy/memory/memory_map.cpp
        # src/mman-win32/mman.c
        src/legacy/primitives/record_list.cpp
        src/legacy/primitives/record_manager.cpp
        src/legacy/primitives/record_multimap_iterable.cpp
        src/legacy/primitives/record_multimap_iterator.cpp
        src/legacy/primitives/slab_manager.cpp
    
        src/legacy/databases/block_database.cpp
        src/legacy/databases/transaction_database.cpp
        src/legacy/result/block_result.cpp
        src/legacy/result/transaction_result.cpp
    )

    if (MSVC) # OR MINGW)
        set(kth_database_sources_just_legacy 
            ${kth_database_sources_just_legacy}
            src/legacy/mman-win32/mman.c
        )
    endif()
  
endif()


if (DB_TRANSACTION_UNCONFIRMED)
    set(kth_database_sources_just_legacy 
        ${kth_database_sources_just_legacy}
        src/legacy/databases/transaction_unconfirmed_database.cpp
        src/legacy/result/transaction_unconfirmed_result.cpp
        )
endif()

if (DB_SPENDS)
    set(kth_database_sources_just_legacy 
        ${kth_database_sources_just_legacy}
        src/legacy/databases/spend_database.cpp
    )
endif()

if (DB_HISTORY)
    set(kth_database_sources_just_legacy 
        ${kth_database_sources_just_legacy}
        src/legacy/databases/history_database.cpp
    )
endif()

if (DB_STEALTH)
    set(kth_database_sources_just_legacy 
        ${kth_database_sources_just_legacy}
        src/legacy/databases/stealth_database.cpp
    )
endif()

if (DB_UNSPENT_LEGACY)
    set(kth_database_sources_just_legacy 
        ${kth_database_sources_just_legacy}
        src/legacy/unspent_outputs.cpp
        src/legacy/unspent_transaction.cpp
    )
endif()

if (DB_LEGACY)
    set(kth_database_sources_just_legacy 
        ${kth_database_sources_just_legacy}
            src/legacy/databases/block_database.cpp
            src/legacy/databases/transaction_database.cpp
            src/legacy/result/block_result.cpp
            src/legacy/result/transaction_result.cpp
        )
endif()

set(kth_database_sources_just_kth
)

if (DB_NEW)
    set(kth_database_sources_just_kth 
        ${kth_database_sources_just_kth}
        src/databases/utxo_entry.cpp
    )
endif()

if (DB_NEW_FULL)
    set(kth_database_sources_just_kth 
        ${kth_database_sources_just_kth}
        src/databases/history_entry.cpp
        src/databases/transaction_entry.cpp
        src/databases/transaction_unconfirmed_entry.cpp
    )
endif()

if (NOT JUST_KTH_SOURCES)
  set(kth_database_sources 
    ${kth_database_sources_just_legacy}
  )
endif()

set(kth_database_sources 
  ${kth_database_sources}   
  ${kth_database_sources_just_kth}         
)

add_library(kth-database ${MODE} ${kth_database_sources})



# add_library(kth-database ${MODE}
#     src/data_base.cpp
#     src/settings.cpp
#     src/store.cpp
#     src/unspent_outputs.cpp
#     src/unspent_transaction.cpp
#     src/version.cpp

#     src/databases/block_database.cpp
#     src/databases/history_database.cpp
#     src/databases/spend_database.cpp
#     src/databases/stealth_database.cpp
#     src/databases/transaction_database.cpp
#     src/databases/transaction_unconfirmed_database.cpp

#     src/legacy/memory/accessor.cpp
#     src/legacy/memory/allocator.cpp
#     src/legacy/memory/memory_map.cpp
#     src/mman-win32/mman.c
#     src/legacy/primitives/record_list.cpp
#     src/legacy/primitives/record_manager.cpp
#     src/legacy/primitives/record_multimap_iterable.cpp
#     src/legacy/primitives/record_multimap_iterator.cpp
#     src/legacy/primitives/slab_manager.cpp
#     src/result/block_result.cpp
#     src/result/transaction_result.cpp
#     src/result/transaction_unconfirmed_result.cpp
# )

if (ENABLE_POSITION_INDEPENDENT_CODE)
    set_property(TARGET kth-database PROPERTY POSITION_INDEPENDENT_CODE ON)
endif(ENABLE_POSITION_INDEPENDENT_CODE)


target_include_directories(kth-database PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)


# target_compile_definitions(kth-database PUBLIC -DKTH_PROJECT_VERSION="\\"${KTH_PROJECT_VERSION}\\"") #TODO(fernando): manage with Conan????
target_compile_definitions(kth-database PUBLIC -DKTH_PROJECT_VERSION="${KTH_PROJECT_VERSION}") #TODO(fernando): manage with Conan????

if (NOT ENABLE_SHARED)
    target_compile_definitions(kth-database PUBLIC -DBCD_STATIC -DBC_STATIC)
endif()

# if (NOT NO_CONAN_AT_ALL)
#   target_link_libraries(kth-database PUBLIC ${CONAN_LIBS})
# else()
#   target_link_libraries(kth-database kth-domain)
# endif()


if (NOT USE_CONAN)
    target_link_libraries(kth-database PUBLIC kth-domain)
endif()

if (NOT NO_CONAN_AT_ALL)
    target_link_libraries(kth-database PUBLIC ${CONAN_LIBS})
endif()


# if (USE_CONAN)
    if (MINGW)
        target_link_libraries(kth-database PUBLIC ws2_32 wsock32) #TODO(fernando): manage with Conan
    endif()
# endif()

_group_sources(kth-database "${CMAKE_CURRENT_LIST_DIR}")

# Tests
#==============================================================================

if (WITH_TESTS)
    enable_testing()
endif()

# local: test/kth_database_test
#------------------------------------------------------------------------------
if (WITH_TESTS)
    if (DB_LEGACY)
        add_executable(kth_database_test
                test/block_database.cpp
                test/data_base.cpp
                test/hash_table.cpp
                test/history_database.cpp
                test/main.cpp
                test/spend_database.cpp
                test/structure.cpp
                test/transaction_database.cpp
                )
    endif()

    if (DB_NEW)
        add_executable(kth_database_test
                test/main.cpp
                test/internal_database.cpp
                )
    endif()

    target_link_libraries(kth_database_test PUBLIC kth-database)

    _group_sources(kth_database_test "${CMAKE_CURRENT_LIST_DIR}/test")

    # _add_tests(kth_database_test
    #         hash_table_tests
    #         structure_tests
    # )

    if (DB_LEGACY)
        _add_tests(kth_database_test
                database_tests
                data_base_tests
        )
    endif()

    
    if (DB_NEW)
        _add_tests(kth_database_test
            internal_db_tests
        )
    endif()


endif()

# Tools
#==============================================================================
# local: tools/block_db/block_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.block_db
            tools/block_db/block_db.cpp)
    target_link_libraries(tools.block_db kth-database)
    _group_sources(tools.block_db "${CMAKE_CURRENT_LIST_DIR}/tools/block_db")
endif()

# local: tools/count_records/count_records
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.count_records
            tools/count_records/count_records.cpp)
    target_link_libraries(tools.count_records kth-database)
    _group_sources(tools.count_records "${CMAKE_CURRENT_LIST_DIR}/tools/count_records")
endif()

# local: tools/history_db/history_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.history_db
            tools/history_db/history_db.cpp)
    target_link_libraries(tools.history_db kth-database)
    _group_sources(tools.history_db "${CMAKE_CURRENT_LIST_DIR}/tools/history_db")
endif()

# local: tools/initchain/initchain_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.initchain_db
            tools/initchain/initchain.cpp)
    target_link_libraries(tools.initchain_db kth-database)
    _group_sources(tools.initchain_db "${CMAKE_CURRENT_LIST_DIR}/tools/initchain")
endif()

# local: tools/mmr_add_row/mmr_add_row
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.mmr_add_row
            tools/mmr_add_row/mmr_add_row.cpp)
    target_link_libraries(tools.mmr_add_row kth-database)
    _group_sources(tools.mmr_add_row "${CMAKE_CURRENT_LIST_DIR}/tools/mmr_add_row")
endif()

# local: tools/mmr_create/mmr_create
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.mmr_create
            tools/mmr_create/mmr_create.cpp)
    target_link_libraries(tools.mmr_create kth-database)
    _group_sources(tools.mmr_create "${CMAKE_CURRENT_LIST_DIR}/tools/mmr_create")
endif()

# local: tools/mmr_delete_last_row/mmr_delete_last_row
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.mmr_delete_last_row
            tools/mmr_delete_last_row/mmr_delete_last_row.cpp)
    target_link_libraries(tools.mmr_delete_last_row kth-database)
    _group_sources(tools.mmr_delete_last_row "${CMAKE_CURRENT_LIST_DIR}/tools/mmr_delete_last_row")
endif()

# local: tools/mmr_lookup/mmr_lookup
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.mmr_lookup
            tools/mmr_lookup/mmr_lookup.cpp)
    target_link_libraries(tools.mmr_lookup kth-database)
    _group_sources(tools.mmr_lookup "${CMAKE_CURRENT_LIST_DIR}/tools/mmr_lookup")
endif()

# local: tools/read_htdb_record_value/read_htdb_record_value
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.read_htdb_record_value
            tools/read_htdb_record_value/read_htdb_record_value.cpp)
    target_link_libraries(tools.read_htdb_record_value kth-database)
    _group_sources(tools.read_htdb_record_value "${CMAKE_CURRENT_LIST_DIR}/tools/read_htdb_record_value")
endif()
# local: tools/read_htdb_slab_value/read_htdb_slab_value
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.read_htdb_slab_value
            tools/read_htdb_slab_value/read_htdb_slab_value.cpp)
    target_link_libraries(tools.read_htdb_slab_value kth-database)
    _group_sources(tools.read_htdb_slab_value "${CMAKE_CURRENT_LIST_DIR}/tools/read_htdb_slab_value")
endif()

# local: tools/show_array/show_array
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.show_array
            tools/show_array/show_array.cpp)
    target_link_libraries(tools.show_array kth-database)
    _group_sources(tools.show_array "${CMAKE_CURRENT_LIST_DIR}/tools/show_array")
endif()

# local: tools/show_linked_records/show_linked_records
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.show_linked_records
            tools/show_linked_records/show_linked_records.cpp)
    target_link_libraries(tools.show_linked_records kth-database)
    _group_sources(tools.show_linked_records "${CMAKE_CURRENT_LIST_DIR}/tools/initchain")
endif()

# local: tools/show_records/show_records
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.show_records
            tools/show_records/show_records.cpp)
    target_link_libraries(tools.show_records kth-database)
    _group_sources(tools.show_records "${CMAKE_CURRENT_LIST_DIR}/tools/show_records")
endif()

# local: tools/spend_db/spend_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.spend_db
            tools/spend_db/spend_db.cpp)
    target_link_libraries(tools.spend_db kth-database)
    _group_sources(tools.spend_db "${CMAKE_CURRENT_LIST_DIR}/tools/spend_db")
endif()

# local: tools/stealth_db/stealth_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.stealth_db
            tools/stealth_db/stealth_db.cpp)
    target_link_libraries(tools.stealth_db kth-database)
    _group_sources(tools.stealth_db "${CMAKE_CURRENT_LIST_DIR}/tools/stealth_db")
endif()

# local: tools/transaction_db/transaction_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.transaction_db
            tools/transaction_db/transaction_db.cpp)
    target_link_libraries(tools.transaction_db kth-database)
    _group_sources(tools.transaction_db "${CMAKE_CURRENT_LIST_DIR}/tools/transaction_db")
endif()

# local: tools/unspent_db/unspent_db
#------------------------------------------------------------------------------
# if (WITH_TOOLS)
#   add_executable(tools.unspent_db
#     tools/unspent_db/unspent_db.cpp)
#   target_link_libraries(tools.unspent_db kth-database)
# endif()

# local: tools/build_utxo/build_utxo
#------------------------------------------------------------------------------
# if (WITH_TOOLS)
#   add_executable(tools.build_utxo
#     tools/build_utxo/build_utxo.cpp)
#   target_link_libraries(tools.build_utxo kth-database)
#   _group_sources(tools.transaction_db "${CMAKE_CURRENT_LIST_DIR}/tools/build_utxo")
# endif()


# # local: tools/check_scripts/check_scripts
# #------------------------------------------------------------------------------
# if (WITH_TOOLS)
#   add_executable(tools.check_scripts
#     tools/check_scripts/check_scripts.cpp)
#   target_link_libraries(tools.check_scripts kth-database)
#   _group_sources(tools.transaction_db "${CMAKE_CURRENT_LIST_DIR}/tools/check_scripts")
# endif()


# Install
#==============================================================================
install(TARGETS kth-database
        EXPORT kth-database
        ARCHIVE DESTINATION lib)

set(_kth_headers
        kth/database/data_base.hpp
        kth/database/unspent_outputs.hpp
        kth/database/unspent_transaction.hpp
        kth/database/databases/block_database.hpp
        kth/database/databases/history_database.hpp
        kth/database/databases/spend_database.hpp
        kth/database/databases/stealth_database.hpp
        kth/database/databases/transaction_database.hpp
        kth/database/databases/transaction_unconfirmed_database.hpp
        kth/database/define.hpp
        kth/database/legacy/impl/hash_table_header.ipp
        kth/database/legacy/impl/record_hash_table.ipp
        kth/database/legacy/impl/record_multimap.ipp
        kth/database/legacy/impl/record_row.ipp
        kth/database/legacy/impl/remainder.ipp
        kth/database/legacy/impl/slab_hash_table.ipp
        kth/database/legacy/impl/slab_row.ipp
        kth/database/legacy/memory/accessor.hpp
        kth/database/legacy/memory/allocator.hpp
        kth/database/legacy/memory/memory.hpp
        kth/database/legacy/memory/memory_map.hpp
        kth/database/legacy/primitives/hash_table_header.hpp
        kth/database/legacy/primitives/record_hash_table.hpp
        kth/database/legacy/primitives/record_list.hpp
        kth/database/legacy/primitives/record_manager.hpp
        kth/database/legacy/primitives/record_multimap.hpp
        kth/database/legacy/primitives/record_multimap_iterable.hpp
        kth/database/legacy/primitives/record_multimap_iterator.hpp
        kth/database/legacy/primitives/slab_hash_table.hpp
        kth/database/legacy/primitives/slab_manager.hpp
        kth/database/result/block_result.hpp
        kth/database/result/transaction_result.hpp
        kth/database/result/transaction_unconfirmed_result.hpp
        kth/database/settings.hpp
        kth/database/store.hpp
        kth/database/version.hpp
        kth/database.hpp)

foreach (_header ${_kth_headers})
    get_filename_component(_directory "${_header}" DIRECTORY)
    install(FILES "include/${_header}" DESTINATION "include/${_directory}")
endforeach()

# Export
#==============================================================================
include(CMakePackageConfigHelpers)

export(EXPORT kth-database
        FILE "${CMAKE_CURRENT_BINARY_DIR}/kth-databaseTargets.cmake")
export(PACKAGE kth-database)

if (UNIX)
    set(_config_package_location "lib/cmake")
elseif (WIN32)
    set(_config_package_location "cmake")
endif()

install(EXPORT kth-database
        FILE kth-databaseTargets.cmake
        DESTINATION ${_config_package_location})

configure_file(kth-databaseConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/kth-databaseConfig.cmake"
        @ONLY)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/kth-databaseConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/kth-databaseConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/kth-databaseConfigVersion.cmake"
        DESTINATION ${_config_package_location})

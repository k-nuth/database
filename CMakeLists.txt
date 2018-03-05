#
# Copyright (c) 2017 Bitprim developers (see AUTHORS)
#
# This file is part of Bitprim.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.4)

# bitprim-database
#==============================================================================
project(bitprim-database
        VERSION 0.8.0
        LANGUAGES CXX C)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Check for baseline language coverage in the compiler for the C++11 standard.
#------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 11)
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


set(BITPRIM_BUILD_NUMBER "-" CACHE STRING "Specify the Bitprim Build Number.")
# message(${BITPRIM_BUILD_NUMBER})


# Conan infrastructure
#==============================================================================
if (EXISTS ${CMAKE_SOURCE_DIR}/bitprimbuildinfo.cmake)
    include(${CMAKE_SOURCE_DIR}/bitprimbuildinfo.cmake)
else()
    message( STATUS "bitprimbuildinfo doent exists")
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
    # Require bitprim-core of at least version 0.7.0 and output ${bitprim_core_CPPFLAGS/LIBS/PKG}.
    #------------------------------------------------------------------------------
    if (NOT TARGET bitprim-core)
            find_package(bitprim-core 0.7.0 REQUIRED)
    endif()
endif()

#if (UNIX)
#  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/lib/cmake")
#elseif (WIN32)
#  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "cmake")
#endif()


# Set flags.
#==============================================================================
include(CheckCXXCompilerFlag)

# Including common functions
#------------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(BitprimTools)


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

# src/bitprim-database.la => ${libdir}
#------------------------------------------------------------------------------
set(MODE STATIC)
if (ENABLE_SHARED)
    set(MODE SHARED)
endif()

add_library(bitprim-database ${MODE}
        src/data_base.cpp
        src/settings.cpp
        src/store.cpp
        src/unspent_outputs.cpp
        src/unspent_transaction.cpp
        src/version.cpp
        src/databases/block_database.cpp
        src/databases/history_database.cpp
        src/databases/spend_database.cpp
        src/databases/stealth_database.cpp
        src/databases/transaction_database.cpp
        src/databases/transaction_unconfirmed_database.cpp

        src/memory/accessor.cpp
        src/memory/allocator.cpp
        src/memory/memory_map.cpp
        src/mman-win32/mman.c
        src/primitives/record_list.cpp
        src/primitives/record_manager.cpp
        src/primitives/record_multimap_iterable.cpp
        src/primitives/record_multimap_iterator.cpp
        src/primitives/slab_manager.cpp
        src/result/block_result.cpp
        src/result/transaction_result.cpp
        src/result/transaction_unconfirmed_result.cpp
        )


if (ENABLE_POSITION_INDEPENDENT_CODE)
    set_property(TARGET bitprim-database PROPERTY POSITION_INDEPENDENT_CODE ON)
endif(ENABLE_POSITION_INDEPENDENT_CODE)


target_include_directories(bitprim-database PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)


target_compile_definitions(bitprim-database PUBLIC -DBITPRIM_BUILD_NUMBER="\\"${BITPRIM_BUILD_NUMBER}\\"")

target_compile_definitions(bitprim-database PUBLIC -DBCD_STATIC -DBC_STATIC)

# if (NOT NO_CONAN_AT_ALL)
#   target_link_libraries(bitprim-database PUBLIC ${CONAN_LIBS})
# else()
#   target_link_libraries(bitprim-database bitprim-core)
# endif()


if (NOT USE_CONAN)
    target_link_libraries(bitprim-database PUBLIC bitprim-core)
endif()

if (NOT NO_CONAN_AT_ALL)
    target_link_libraries(bitprim-database PUBLIC ${CONAN_LIBS})
endif()


# if (USE_CONAN)
    if (MINGW)
        target_link_libraries(bitprim-database PUBLIC ws2_32 wsock32) #TODO(fernando): manage with Conan
    endif()
# endif()

_group_sources(bitprim-database "${CMAKE_CURRENT_LIST_DIR}")

# Tests
#==============================================================================

if (WITH_TESTS)
    enable_testing()
endif()

# local: test/bitprim_database_test
#------------------------------------------------------------------------------
if (WITH_TESTS)
    add_executable(bitprim_database_test
            test/block_database.cpp
            test/data_base.cpp
            test/hash_table.cpp
            test/history_database.cpp
            test/main.cpp
            test/spend_database.cpp
            test/structure.cpp
            test/transaction_database.cpp
            #        test/unspent_database.cpp
            )
    target_link_libraries(bitprim_database_test PUBLIC bitprim-database)

    _group_sources(bitprim_database_test "${CMAKE_CURRENT_LIST_DIR}/test")

    _add_tests(bitprim_database_test
            database_tests
            hash_table_tests
            structure_tests
            data_base_tests)
endif()

# Tools
#==============================================================================
# local: tools/block_db/block_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.block_db
            tools/block_db/block_db.cpp)
    target_link_libraries(tools.block_db bitprim-database)
    _group_sources(tools.block_db "${CMAKE_CURRENT_LIST_DIR}/tools/block_db")
endif()

# local: tools/count_records/count_records
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.count_records
            tools/count_records/count_records.cpp)
    target_link_libraries(tools.count_records bitprim-database)
    _group_sources(tools.count_records "${CMAKE_CURRENT_LIST_DIR}/tools/count_records")
endif()

# local: tools/history_db/history_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.history_db
            tools/history_db/history_db.cpp)
    target_link_libraries(tools.history_db bitprim-database)
    _group_sources(tools.history_db "${CMAKE_CURRENT_LIST_DIR}/tools/history_db")
endif()

# local: tools/initchain/initchain_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.initchain_db
            tools/initchain/initchain.cpp)
    target_link_libraries(tools.initchain_db bitprim-database)
    _group_sources(tools.initchain_db "${CMAKE_CURRENT_LIST_DIR}/tools/initchain")
endif()

# local: tools/mmr_add_row/mmr_add_row
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.mmr_add_row
            tools/mmr_add_row/mmr_add_row.cpp)
    target_link_libraries(tools.mmr_add_row bitprim-database)
    _group_sources(tools.mmr_add_row "${CMAKE_CURRENT_LIST_DIR}/tools/mmr_add_row")
endif()

# local: tools/mmr_create/mmr_create
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.mmr_create
            tools/mmr_create/mmr_create.cpp)
    target_link_libraries(tools.mmr_create bitprim-database)
    _group_sources(tools.mmr_create "${CMAKE_CURRENT_LIST_DIR}/tools/mmr_create")
endif()

# local: tools/mmr_delete_last_row/mmr_delete_last_row
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.mmr_delete_last_row
            tools/mmr_delete_last_row/mmr_delete_last_row.cpp)
    target_link_libraries(tools.mmr_delete_last_row bitprim-database)
    _group_sources(tools.mmr_delete_last_row "${CMAKE_CURRENT_LIST_DIR}/tools/mmr_delete_last_row")
endif()

# local: tools/mmr_lookup/mmr_lookup
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.mmr_lookup
            tools/mmr_lookup/mmr_lookup.cpp)
    target_link_libraries(tools.mmr_lookup bitprim-database)
    _group_sources(tools.mmr_lookup "${CMAKE_CURRENT_LIST_DIR}/tools/mmr_lookup")
endif()

# local: tools/read_htdb_record_value/read_htdb_record_value
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.read_htdb_record_value
            tools/read_htdb_record_value/read_htdb_record_value.cpp)
    target_link_libraries(tools.read_htdb_record_value bitprim-database)
    _group_sources(tools.read_htdb_record_value "${CMAKE_CURRENT_LIST_DIR}/tools/read_htdb_record_value")
endif()
# local: tools/read_htdb_slab_value/read_htdb_slab_value
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.read_htdb_slab_value
            tools/read_htdb_slab_value/read_htdb_slab_value.cpp)
    target_link_libraries(tools.read_htdb_slab_value bitprim-database)
    _group_sources(tools.read_htdb_slab_value "${CMAKE_CURRENT_LIST_DIR}/tools/read_htdb_slab_value")
endif()

# local: tools/show_array/show_array
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.show_array
            tools/show_array/show_array.cpp)
    target_link_libraries(tools.show_array bitprim-database)
    _group_sources(tools.show_array "${CMAKE_CURRENT_LIST_DIR}/tools/show_array")
endif()

# local: tools/show_linked_records/show_linked_records
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.show_linked_records
            tools/show_linked_records/show_linked_records.cpp)
    target_link_libraries(tools.show_linked_records bitprim-database)
    _group_sources(tools.show_linked_records "${CMAKE_CURRENT_LIST_DIR}/tools/initchain")
endif()

# local: tools/show_records/show_records
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.show_records
            tools/show_records/show_records.cpp)
    target_link_libraries(tools.show_records bitprim-database)
    _group_sources(tools.show_records "${CMAKE_CURRENT_LIST_DIR}/tools/show_records")
endif()

# local: tools/spend_db/spend_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.spend_db
            tools/spend_db/spend_db.cpp)
    target_link_libraries(tools.spend_db bitprim-database)
    _group_sources(tools.spend_db "${CMAKE_CURRENT_LIST_DIR}/tools/spend_db")
endif()

# local: tools/stealth_db/stealth_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.stealth_db
            tools/stealth_db/stealth_db.cpp)
    target_link_libraries(tools.stealth_db bitprim-database)
    _group_sources(tools.stealth_db "${CMAKE_CURRENT_LIST_DIR}/tools/stealth_db")
endif()

# local: tools/transaction_db/transaction_db
#------------------------------------------------------------------------------
if (WITH_TOOLS)
    add_executable(tools.transaction_db
            tools/transaction_db/transaction_db.cpp)
    target_link_libraries(tools.transaction_db bitprim-database)
    _group_sources(tools.transaction_db "${CMAKE_CURRENT_LIST_DIR}/tools/transaction_db")
endif()

# local: tools/unspent_db/unspent_db
#------------------------------------------------------------------------------
# if (WITH_TOOLS)
#   add_executable(tools.unspent_db
#     tools/unspent_db/unspent_db.cpp)
#   target_link_libraries(tools.unspent_db bitprim-database)
# endif()

# local: tools/build_utxo/build_utxo
#------------------------------------------------------------------------------
# if (WITH_TOOLS)
#   add_executable(tools.build_utxo
#     tools/build_utxo/build_utxo.cpp)
#   target_link_libraries(tools.build_utxo bitprim-database)
#   _group_sources(tools.transaction_db "${CMAKE_CURRENT_LIST_DIR}/tools/build_utxo")
# endif()


# # local: tools/check_scripts/check_scripts
# #------------------------------------------------------------------------------
# if (WITH_TOOLS)
#   add_executable(tools.check_scripts
#     tools/check_scripts/check_scripts.cpp)
#   target_link_libraries(tools.check_scripts bitprim-database)
#   _group_sources(tools.transaction_db "${CMAKE_CURRENT_LIST_DIR}/tools/check_scripts")
# endif()


# Install
#==============================================================================
install(TARGETS bitprim-database
        EXPORT bitprim-database
        ARCHIVE DESTINATION lib)

set(_bitprim_headers
        bitcoin/database/data_base.hpp
        bitcoin/database/unspent_outputs.hpp
        bitcoin/database/unspent_transaction.hpp
        bitcoin/database/databases/block_database.hpp
        bitcoin/database/databases/history_database.hpp
        bitcoin/database/databases/spend_database.hpp
        bitcoin/database/databases/stealth_database.hpp
        bitcoin/database/databases/transaction_database.hpp
        bitcoin/database/databases/transaction_unconfirmed_database.hpp
        bitcoin/database/define.hpp
        bitcoin/database/impl/hash_table_header.ipp
        bitcoin/database/impl/record_hash_table.ipp
        bitcoin/database/impl/record_multimap.ipp
        bitcoin/database/impl/record_row.ipp
        bitcoin/database/impl/remainder.ipp
        bitcoin/database/impl/slab_hash_table.ipp
        bitcoin/database/impl/slab_row.ipp
        bitcoin/database/memory/accessor.hpp
        bitcoin/database/memory/allocator.hpp
        bitcoin/database/memory/memory.hpp
        bitcoin/database/memory/memory_map.hpp
        bitcoin/database/primitives/hash_table_header.hpp
        bitcoin/database/primitives/record_hash_table.hpp
        bitcoin/database/primitives/record_list.hpp
        bitcoin/database/primitives/record_manager.hpp
        bitcoin/database/primitives/record_multimap.hpp
        bitcoin/database/primitives/record_multimap_iterable.hpp
        bitcoin/database/primitives/record_multimap_iterator.hpp
        bitcoin/database/primitives/slab_hash_table.hpp
        bitcoin/database/primitives/slab_manager.hpp
        bitcoin/database/result/block_result.hpp
        bitcoin/database/result/transaction_result.hpp
        bitcoin/database/result/transaction_unconfirmed_result.hpp
        bitcoin/database/settings.hpp
        bitcoin/database/store.hpp
        bitcoin/database/version.hpp
        bitcoin/database.hpp)

foreach (_header ${_bitprim_headers})
    get_filename_component(_directory "${_header}" DIRECTORY)
    install(FILES "include/${_header}" DESTINATION "include/${_directory}")
endforeach()

# Export
#==============================================================================
include(CMakePackageConfigHelpers)

export(EXPORT bitprim-database
        FILE "${CMAKE_CURRENT_BINARY_DIR}/bitprim-databaseTargets.cmake")
export(PACKAGE bitprim-database)

if (UNIX)
    set(_config_package_location "lib/cmake")
elseif (WIN32)
    set(_config_package_location "cmake")
endif()

install(EXPORT bitprim-database
        FILE bitprim-databaseTargets.cmake
        DESTINATION ${_config_package_location})

configure_file(bitprim-databaseConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/bitprim-databaseConfig.cmake"
        @ONLY)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/bitprim-databaseConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/bitprim-databaseConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/bitprim-databaseConfigVersion.cmake"
        DESTINATION ${_config_package_location})

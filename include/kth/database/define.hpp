// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_DEFINE_HPP
#define KTH_DATABASE_DEFINE_HPP

#include <cstdint>
#include <vector>
#include <kth/domain.hpp>

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/containers/flat_map.hpp>

#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>

#include <boost/container_hash/hash.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/unordered_map.hpp>


// Now we use the generic helper definitions to
// define KD_API and KD_INTERNAL.
// KD_API is used for the public API symbols. It either DLL imports or
// DLL exports (or does nothing for static build)
// KD_INTERNAL is used for non-api symbols.

#if defined KD_STATIC
    #define KD_API
    #define KD_INTERNAL
#elif defined KD_DLL
    #define KD_API      BC_HELPER_DLL_EXPORT
    #define KD_INTERNAL BC_HELPER_DLL_LOCAL
#else
    #define KD_API      BC_HELPER_DLL_IMPORT
    #define KD_INTERNAL BC_HELPER_DLL_LOCAL
#endif

// Log name.
#define LOG_DATABASE "[db] "

// Remap safety is required if the mmap file is not fully preallocated.
#define REMAP_SAFETY

// Allocate safety is required for support of concurrent write operations.
#define ALLOCATE_SAFETY

namespace bip = boost::interprocess;


namespace kth::database {

using array_index = uint32_t;
using file_offset = uint64_t;

using allocator_type = bip::allocator<uint8_t, bip::managed_mapped_file::segment_manager>;
// using persistent_data_chunk = bip::vector<uint8_t, allocator_type>;
using persistent_data_chunk = std::vector<uint8_t, allocator_type>;



} // namespace kth::database

#endif

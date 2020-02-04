// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_DATABASE_DEFINE_HPP
#define KTH_DATABASE_DEFINE_HPP

#include <cstdint>
#include <vector>
#include <kth/domain.hpp>

// Now we use the generic helper definitions to
// define BCD_API and BCD_INTERNAL.
// BCD_API is used for the public API symbols. It either DLL imports or
// DLL exports (or does nothing for static build)
// BCD_INTERNAL is used for non-api symbols.

#if defined BCD_STATIC
    #define BCD_API
    #define BCD_INTERNAL
#elif defined BCD_DLL
    #define BCD_API      BC_HELPER_DLL_EXPORT
    #define BCD_INTERNAL BC_HELPER_DLL_LOCAL
#else
    #define BCD_API      BC_HELPER_DLL_IMPORT
    #define BCD_INTERNAL BC_HELPER_DLL_LOCAL
#endif

// Log name.
#define LOG_DATABASE "database"

// Remap safety is required if the mmap file is not fully preallocated.
#define REMAP_SAFETY

// Allocate safety is required for support of concurrent write operations.
#define ALLOCATE_SAFETY

namespace kth {
namespace database {

typedef uint32_t array_index;
typedef uint64_t file_offset;

} // namespace database
} // namespace kth

#endif

///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2015 libbitcoin-database developers (see COPYING).
//
//        GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
//
///////////////////////////////////////////////////////////////////////////////
#ifndef LIBBITCOIN_DATABASE_VERSION_HPP_
#define LIBBITCOIN_DATABASE_VERSION_HPP_

/**
 * The semantic version of this repository as: [major].[minor].[patch]
 * For interpretation of the versioning scheme see: http://semver.org
 */

#ifdef BITPRIM_PROJECT_VERSION
#define BITPRIM_DATABASE_VERSION BITPRIM_PROJECT_VERSION
#else
#define BITPRIM_DATABASE_VERSION "0.0.0"
#endif


namespace libbitcoin { namespace database {
char const* version();
}} /*namespace libbitcoin::database*/
 

#endif // LIBBITCOIN_DATABASE_VERSION_HPP_

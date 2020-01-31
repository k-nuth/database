///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2015 libbitcoin-database developers (see COPYING).
//
//        GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
//
///////////////////////////////////////////////////////////////////////////////
#ifndef KTH_DATABASE_VERSION_HPP_
#define KTH_DATABASE_VERSION_HPP_

/**
 * The semantic version of this repository as: [major].[minor].[patch]
 * For interpretation of the versioning scheme see: http://semver.org
 */

#ifdef KTH_PROJECT_VERSION
#define KTH_DATABASE_VERSION KTH_PROJECT_VERSION
#else
#define KTH_DATABASE_VERSION "0.0.0"
#endif


namespace libbitcoin { namespace database {
char const* version();
}} /*namespace libbitcoin::database*/
 

#endif // KTH_DATABASE_VERSION_HPP_

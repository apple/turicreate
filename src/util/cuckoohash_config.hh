/*! \file */

#ifndef _CUCKOOHASH_CONFIG_HH
#define _CUCKOOHASH_CONFIG_HH

#include <cstddef>

//! SLOT_PER_BUCKET is the maximum number of keys per bucket
const size_t DEFAULT_SLOT_PER_BUCKET = 4;

//! DEFAULT_SIZE is the default number of elements in an empty hash
//! table
const size_t DEFAULT_SIZE = (1U << 16) * DEFAULT_SLOT_PER_BUCKET;

//! set LIBCUCKOO_DEBUG to 1 to enable debug output
#define LIBCUCKOO_DEBUG 0

#endif // _CUCKOOHASH_CONFIG_HH

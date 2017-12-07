#ifndef _CUCKOOHASH_UTIL_HH
#define _CUCKOOHASH_UTIL_HH

#include <pthread.h>
#include "cuckoohash_config.hh" // for LIBCUCKOO_DEBUG

#if LIBCUCKOO_DEBUG
#  define LIBCUCKOO_DBG(fmt, args...)                                   \
     fprintf(stderr, "\x1b[32m""[libcuckoo:%s:%d:%lu] " fmt"" "\x1b[0m", \
             __FILE__,__LINE__, (unsigned long)pthread_self(), ##args)
#else
#  define LIBCUCKOO_DBG(fmt, args...)  do {} while (0)
#endif

// For enabling certain methods based on a condition. Here's an example.
// ENABLE_IF(some_cond, type, static, inline) method() {
//     ...
// }
#define ENABLE_IF(preamble, condition, return_type)                     \
     template <class Bogus=void*>                                       \
          preamble typename std::enable_if<sizeof(Bogus) &&             \
          condition, return_type>::type

#endif // _CUCKOOHASH_UTIL_HH

/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_get_date_h
#define cm_get_date_h

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Parse a date/time string.  Treat relative times with respect to 'now'. */
time_t cm_get_date(time_t now, const char* str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

/* Copyright libuv project contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "internal.h"

#if defined(__APPLE__)
/* Special case for CMake bootstrap: no clock_gettime on macOS < 10.12 */

#ifndef CMAKE_BOOTSTRAP
#error "This code path meant only for use during CMake bootstrap."
#endif

#include <mach/mach.h>
#include <mach/mach_time.h>

uint64_t uv__hrtime(uv_clocktype_t type) {
  static mach_timebase_info_data_t info;

  if ((ACCESS_ONCE(uint32_t, info.numer) == 0 ||
       ACCESS_ONCE(uint32_t, info.denom) == 0) &&
      mach_timebase_info(&info) != KERN_SUCCESS)
    abort();

  return mach_absolute_time() * info.numer / info.denom;
}

#else

#include <stdint.h>
#include <time.h>

#undef NANOSEC
#define NANOSEC ((uint64_t) 1e9)

uint64_t uv__hrtime(uv_clocktype_t type) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (((uint64_t) ts.tv_sec) * NANOSEC + ts.tv_nsec);
}

#endif

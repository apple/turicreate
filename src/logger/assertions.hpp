/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Copyright (c) 2005, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// This file contains #include information about logging-related stuff.
// Pretty much everybody needs to #include this file so that they can
// log various happenings.
//
#ifndef _ASSERTIONS_H_
#define _ASSERTIONS_H_

#ifndef TURI_LOGGER_THROW_ON_FAILURE
#define TURI_LOGGER_THROW_ON_FAILURE
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>    // for write()
#endif
#include <string.h>    // for strlen(), strcmp()
#include <assert.h>
#include <errno.h>     // for errno
#include <sstream>
#include <cassert>
#include <cmath>
#include <sstream>
#include <logger/logger.hpp>
#include <logger/fail_method.hpp>
#include <logger/backtrace.hpp>

#include <boost/typeof/typeof.hpp>

#include <util/code_optimization.hpp>
#include <util/sys_util.hpp>

extern void __print_back_trace();

/*
 * I am not too happy about this. But this seems to be the only practical way
 * to disable the null conversion error in the check_op below, thus allow
 * ASSERT_NE(someptr, NULL);
 */
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion-null"
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion-null"
#endif
// On some systems (like freebsd), we can't call write() at all in a
// global constructor, perhaps because errno hasn't been set up.
// Calling the write syscall is safer (it doesn't set errno), so we
// prefer that.  Note we don't care about errno for logging: we just
// do logging on a best-effort basis.
#define WRITE_TO_STDERR(buf, len) (logbuf(LOG_FATAL, buf, len))

/**
 * \ingroup turilogger
 * \addtogroup assertions Logging Assertions
 * \brief Assertions
 * \{
 */
/**
 * \def __CHECK(condition)
 * CHECK dies with a fatal error if condition is not true.  It is *not*
 * controlled by NDEBUG, so the check will be executed regardless of
 * compilation mode.  
 */
/**
 * \def ASSERT_TRUE(val)
 * Expects val to evaluate to true, dies with a fatal error otherwise. Synonyms are 
 * EXPECT_TRUE
 */
/**
 * \def ASSERT_FALSE(val)
 * Expects val to evaluate to false, dies with a fatal error otherwise. Synonyms are 
 * EXPECT_FALSE
 */
/**
 * \def __CHECK_EQ(val1, val2)
 * Expects val1 == val2, dies with a fatal error otherwise. Synonyms are
 * ASSERT_EQ and EXPECT_EQ.
 */
/**
 * \def __CHECK_NE(val1, val2)
 * Expects val1 != val2, dies with a fatal error otherwise. Synonyms are
 * ASSERT_NE and EXPECT_NE
 */
/**
 * \def __CHECK_LE(val1, val2)
 * Expects val1 <= val2, dies with a fatal error otherwise. Synonyms are
 * ASSERT_LE and EXPECT_LE
 */
/**
 * \def __CHECK_LT(val1, val2)
 * Expects val1 < val2, dies with a fatal error otherwise. Synonyms are
 * ASSERT_LT and EXPECT_LT
 */
/**
 * \def __CHECK_GE(val1, val2)
 * Expects val1 >= val2, dies with a fatal error otherwise. Synonyms are
 * ASSERT_GE and EXPECT_GE
 */
/**
 * \def __CHECK_GT(val1, val2)
 * Expects val1 > val2, dies with a fatal error otherwise. Synonyms are
 * ASSERT_GT and EXPECT_GT
 */
/**
 * \def __CHECK_DELTA(val1, val2, delta)
 * Expects |val1 - val2| <= delta difference,
 * dies with a fatal error otherwise. Mainly meant for floating point values.
 * ASSERT_DELTA and EXPECT_DELTA
 */
/**
 * \def DASSERT_TRUE(val)
 * Like \ref ASSERT_TRUE but is compiled away by NDEBUG.
 */
/**
 * \def DASSERT_FALSE(val)
 * Like \ref ASSERT_FALSE but is compiled away by NDEBUG.
 */
/**
 * \def __DCHECK_EQ(val1, val2)
 * Like \ref __CHECK_EQ but is compiled away by NDEBUG. Synonyms are
 * DASSERT_EQ.
 */
/**
 * \def __DCHECK_NE(val1, val2)
 * Like \ref __CHECK_NE but is compiled away by NDEBUG. Synonyms are
 * DASSERT_NE.
 */
/**
 * \def __DCHECK_LE(val1, val2)
 * Like \ref __CHECK_LE but is compiled away by NDEBUG. Synonyms are
 * DASSERT_LE.
 */
/**
 * \def __DCHECK_LT(val1, val2)
 * Like \ref __CHECK_LT but is compiled away by NDEBUG. Synonyms are
 * DASSERT_LT.
 */
/**
 * \def __DCHECK_GE(val1, val2)
 * Like \ref __CHECK_GE but is compiled away by NDEBUG. Synonyms are
 * DASSERT_GE.
 */
/**
 * \def __DCHECK_GT(val1, val2)
 * Like \ref __CHECK_GT but is compiled away by NDEBUG. Synonyms are
 * DASSERT_GT.
 */
/**
 * \def __DCHECK_DELTA(val1, val2, delta)
 * Like \ref __CHECK_DELTA but is compiled away by NDEBUG. Synonyms are
 * DASSERT_DELTA.
 */
/**
 * \}
 */
// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    __CHECK(fp->Write(x) == 4)
#define __CHECK(condition)                                           \
  do {                                                               \
    if (UNLIKELY(!(condition))) {                                    \
      auto throw_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE_ERROR) { \
        std::ostringstream ss;                                       \
        ss << "Check failed (" << __FILE__ << ":" << __LINE__        \
           << "): " << #condition << std::endl;                      \
        logstream(LOG_ERROR) << ss.str();                            \
        __print_back_trace();                                        \
        LOGGED_TURI_LOGGER_FAIL_METHOD(ss.str());                    \
      };                                                             \
      throw_error();                                                 \
      TURI_BUILTIN_UNREACHABLE();                                    \
    }                                                                \
  } while (0)

// This prints errno as well.  errno is the posix defined last error
// number. See errno.h
#define __PCHECK(condition)                                                 \
  do {                                                                      \
    if (UNLIKELY(!(condition))) {                                           \
      auto throw_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE_ERROR) {        \
        const int _PCHECK_err_no_ = errno;                                  \
        std::ostringstream ss;                                              \
        ss << "Assertion failed (" << __FILE__ << ":" << __LINE__           \
           << "): " << #condition << ": " << strerror(err_no) << std::endl; \
        logstream(LOG_ERROR) << ss.str();                                   \
        __print_back_trace();                                               \
        LOGGED_TURI_LOGGER_FAIL_METHOD(ss.str());                           \
      };                                                                    \
      throw_error();                                                        \
      TURI_BUILTIN_UNREACHABLE();                                           \
    }                                                                       \
  } while (0)

// Helper macro for binary operators; prints the two values on error
// Don't use this macro directly in your code, use __CHECK_EQ et al below

// WARNING: These don't compile correctly if one of the arguments is a pointer
// and the other is NULL. To work around this, simply static_cast NULL to the
// type of the desired pointer.
#define __CHECK_OP(op, val1, val2)                                            \
  do {                                                                        \
    const auto _CHECK_OP_v1_ = val1;                                          \
    const auto _CHECK_OP_v2_ = val2;                                          \
    if (__builtin_expect(!((_CHECK_OP_v1_)op(decltype(val1))(_CHECK_OP_v2_)), \
                         0)) {                                                \
      auto throw_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE_ERROR) {          \
        std::ostringstream ss;                                                \
        ss << "Assertion failed: (" << __FILE__ << ":" << __LINE__            \
           << "): " << #val1 << #op << #val2 << "  [" << _CHECK_OP_v1_ << ' ' \
           << #op << ' ' << _CHECK_OP_v2_ << "]" << std::endl;                \
        logstream(LOG_ERROR) << ss.str();                                     \
        __print_back_trace();                                                 \
        LOGGED_TURI_LOGGER_FAIL_METHOD(ss.str());                             \
      };                                                                      \
      throw_error();                                                          \
      TURI_BUILTIN_UNREACHABLE();                                             \
    }                                                                         \
  } while (0)

#define __CHECK_DELTA(val1, val2, delta)                                      \
  do {                                                                        \
    const double _CHECK_OP_v1_ = val1;                                        \
    const double _CHECK_OP_v2_ = val2;                                        \
    const double _CHECK_OP_delta_ = delta;                                    \
    if (__builtin_expect(                                                     \
            !(std::abs((_CHECK_OP_v1_) - (_CHECK_OP_v2_)) <= _CHECK_OP_delta_),\
            0)) {                                                             \
      auto throw_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE_ERROR) {          \
        std::ostringstream ss;                                                \
        ss << "Assertion failed: (" << __FILE__ << ":" << __LINE__ << "): "   \
           << "abs(" << #val1 << " - " << #val2 << ") <= " << #delta << ". [" \
           << "abs(" << _CHECK_OP_v2_ << " - " << _CHECK_OP_v1_ << ") > "     \
           << _CHECK_OP_delta_ << "]" << std::endl;                           \
        logstream(LOG_ERROR) << ss.str();                                     \
        __print_back_trace();                                                 \
        LOGGED_TURI_LOGGER_FAIL_METHOD(ss.str());                             \
      };                                                                      \
      throw_error();                                                          \
      TURI_BUILTIN_UNREACHABLE();                                             \
    }                                                                         \
  } while (0)

#define __CHECK_EQ(val1, val2) __CHECK_OP(==, val1, val2)
#define __CHECK_NE(val1, val2) __CHECK_OP(!=, val1, val2)
#define __CHECK_LE(val1, val2) __CHECK_OP(<=, val1, val2)
#define __CHECK_LT(val1, val2) __CHECK_OP(< , val1, val2)
#define __CHECK_GE(val1, val2) __CHECK_OP(>=, val1, val2)
#define __CHECK_GT(val1, val2) __CHECK_OP(> , val1, val2)

// Synonyms for __CHECK_* that are used in some unittests.
#define EXPECT_EQ(val1, val2) __CHECK_EQ(val1, val2)
#define EXPECT_DELTA(val1, val2, delta) __CHECK_DELTA(val1, val2, delta)
#define EXPECT_NE(val1, val2) __CHECK_NE(val1, val2)
#define EXPECT_LE(val1, val2) __CHECK_LE(val1, val2)
#define EXPECT_LT(val1, val2) __CHECK_LT(val1, val2)
#define EXPECT_GE(val1, val2) __CHECK_GE(val1, val2)
#define EXPECT_GT(val1, val2) __CHECK_GT(val1, val2)
#define ASSERT_EQ(val1, val2) EXPECT_EQ(val1, val2)
#define ASSERT_DELTA(val1, val2, delta) __CHECK_DELTA(val1, val2, delta)
#define ASSERT_NE(val1, val2) EXPECT_NE(val1, val2)
#define ASSERT_LE(val1, val2) EXPECT_LE(val1, val2)
#define ASSERT_LT(val1, val2) EXPECT_LT(val1, val2)
#define ASSERT_GE(val1, val2) EXPECT_GE(val1, val2)
#define ASSERT_GT(val1, val2) EXPECT_GT(val1, val2)
// As are these variants.
#define EXPECT_TRUE(cond)     __CHECK(cond)
#define EXPECT_FALSE(cond)    __CHECK(!(cond))
#define EXPECT_STREQ(a, b)    __CHECK(strcmp(a, b) == 0)
#define ASSERT_TRUE(cond)     EXPECT_TRUE(cond)
#define ASSERT_FALSE(cond)    EXPECT_FALSE(cond)
#define ASSERT_STREQ(a, b)    EXPECT_STREQ(a, b)

#define ASSERT_UNREACHABLE()  { EXPECT_TRUE(false); assert(false); TURI_BUILTIN_UNREACHABLE(); }

// Convenience wrapper since this is a very common case.
#define AU() { ASSERT_UNREACHABLE(); }

#define ASSERT_MSG(condition, fmt, ...)                                  \
  do {                                                                   \
    if (__builtin_expect(!(condition), 0)) {                             \
      auto throw_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE_ERROR) {     \
        logstream(LOG_ERROR) << "Check failed: " << #condition << ":\n"; \
        std::ostringstream ss;                                           \
        ss << "Assertion Failure: " << #condition << ": " << fmt;        \
        logger(LOG_ERROR, fmt, ##__VA_ARGS__);                           \
        __print_back_trace();                                            \
        TURI_LOGGER_FAIL_METHOD(ss.str().c_str());                       \
        ASSERT_UNREACHABLE();                                            \
      };                                                                 \
      throw_error();                                                     \
      TURI_BUILTIN_UNREACHABLE();                                        \
    }                                                                    \
  } while (0)

// Used for (libc) functions that return -1 and set errno
#define __CHECK_ERR(invocation)  __PCHECK((invocation) != -1)

// A few more checks that only happen in debug mode
#ifdef NDEBUG
#define __DCHECK_EQ(val1, val2)
#define __DCHECK_DELTA(val1, val2, delta)
#define __DCHECK_NE(val1, val2)
#define __DCHECK_LE(val1, val2)
#define __DCHECK_LT(val1, val2)
#define __DCHECK_GE(val1, val2)
#define __DCHECK_GT(val1, val2)
#define DASSERT_TRUE(cond)
#define DASSERT_FALSE(cond)
#define DASSERT_EQ(val1, val2)
#define DASSERT_DELTA(val1, val2, delta)
#define DASSERT_NE(val1, val2)
#define DASSERT_LE(val1, val2)
#define DASSERT_LT(val1, val2)
#define DASSERT_GE(val1, val2)
#define DASSERT_GT(val1, val2)

#define DASSERT_MSG(condition, fmt, ...)

#else
#define __DCHECK_EQ(val1, val2)  __CHECK_EQ(val1, val2)
#define __DCHECK_DELTA(val1, val2, delta) __CHECK_DELTA(val1, val2, delta)
#define __DCHECK_NE(val1, val2)  __CHECK_NE(val1, val2)
#define __DCHECK_LE(val1, val2)  __CHECK_LE(val1, val2)
#define __DCHECK_LT(val1, val2)  __CHECK_LT(val1, val2)
#define __DCHECK_GE(val1, val2)  __CHECK_GE(val1, val2)
#define __DCHECK_GT(val1, val2)  __CHECK_GT(val1, val2)
#define DASSERT_TRUE(cond)     ASSERT_TRUE(cond)
#define DASSERT_FALSE(cond)    ASSERT_FALSE(cond)
#define DASSERT_EQ(val1, val2) ASSERT_EQ(val1, val2)
#define DASSERT_DELTA(val1, val2, delta) ASSERT_DELTA(val1, val2, delta)
#define DASSERT_NE(val1, val2) ASSERT_NE(val1, val2)
#define DASSERT_LE(val1, val2) ASSERT_LE(val1, val2)
#define DASSERT_LT(val1, val2) ASSERT_LT(val1, val2)
#define DASSERT_GE(val1, val2) ASSERT_GE(val1, val2)
#define DASSERT_GT(val1, val2) ASSERT_GT(val1, val2)


#define DASSERT_MSG(condition, fmt, ...)                                \
  ASSERT_MSG(condition, fmt, ##__VA_ARGS__)

#endif


#ifdef ERROR
#undef ERROR      // may conflict with ERROR macro on windows
#endif

#define BOOST_ENABLE_ASSERT_HANDLER

namespace boost {
  inline void assertion_failed(
    char const * expr, char const * function, char const * file, long line) {

    std::cerr << "Boost assertion failed: " << expr << std::endl;
    ASSERT_UNREACHABLE();
  }

  inline void assertion_failed_msg(
    char const * expr, char const * msg, char const * function,
    char const * file, long line) {

    std::cerr << "Boost assertion failed: " << expr << std::endl;
    std::cerr << "Boost assertion message: " << msg << std::endl;
    ASSERT_UNREACHABLE();
  }
}

#endif // _LOGGING_H_


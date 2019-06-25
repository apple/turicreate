/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TEST_MACROS_HPP
#define TEST_MACROS_HPP
#include <mutex>
static std::recursive_mutex __b_lock__;
#define _TS_ADD_LOCK_GUARD std::lock_guard<std::recursive_mutex> __guard_b__(__b_lock__)

#define TS_ASSERT(x)                     do { _TS_ADD_LOCK_GUARD; BOOST_CHECK(x); } while(0)
#define TS_ASSERT_EQUALS(x,y)            do { _TS_ADD_LOCK_GUARD; BOOST_TEST((x) == (y)); } while(0)
#define TS_ASSERT_DIFFERS(x,y)           do { _TS_ADD_LOCK_GUARD; BOOST_TEST((x) != (y)); } while(0)
#define TS_ASSERT_LESS_THAN_EQUALS(x,y)  do { _TS_ADD_LOCK_GUARD; BOOST_TEST((x) <= (y)); } while(0)
#define TS_ASSERT_LESS_THAN(x,y)         do { _TS_ADD_LOCK_GUARD; BOOST_TEST((x) < (y)); } while(0)
#define TS_ASSERT_DELTA(x,y,e)           do { _TS_ADD_LOCK_GUARD; BOOST_CHECK_SMALL(double((x)-(y)),double(e)); } while(0)
#define TS_ASSERT_THROWS_NOTHING(expr)   do { _TS_ADD_LOCK_GUARD; BOOST_CHECK_NO_THROW(expr); } while(0)
#define TS_ASSERT_THROWS_ANYTHING(expr)  \
do { \
  _TS_ADD_LOCK_GUARD; \
  bool threw=false; \
  try { \
    expr; \
  } catch (...) {\
    threw=true;\
  } \
  BOOST_TEST(threw==true);\
} while(0)
#define TS_ASSERT_THROWS(expr,type)      do { _TS_ADD_LOCK_GUARD; BOOST_CHECK_THROW(expr,type); } while(0)
#define TS_FAIL(msg)                     do { _TS_ADD_LOCK_GUARD; BOOST_FAIL(msg); } while(0)
#define TS_WARN(msg)                     do { _TS_ADD_LOCK_GUARD; BOOST_TEST_MESSAGE(msg); } while(0)
#define TS_ASSERT_SAME_DATA(x,y,size)    do { _TS_ADD_LOCK_GUARD; BOOST_CHECK_EQUAL_COLLECTIONS(x, x+size, y, y+size); } while(0)

#endif

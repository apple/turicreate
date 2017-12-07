/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TEST_MACROS_HPP
#define TEST_MACROS_HPP

#define TS_ASSERT(x)                     BOOST_CHECK(x);
#define TS_ASSERT_EQUALS(x,y)            BOOST_TEST((x) == (y));
#define TS_ASSERT_DIFFERS(x,y)           BOOST_TEST((x) != (y));
#define TS_ASSERT_LESS_THAN_EQUALS(x,y)  BOOST_TEST((x) <= (y));
#define TS_ASSERT_LESS_THAN(x,y)         BOOST_TEST((x) < (y));
#define TS_ASSERT_DELTA(x,y,e)           BOOST_CHECK_SMALL(double((x)-(y)),double(e));
#define TS_ASSERT_THROWS_NOTHING(expr)   BOOST_CHECK_NO_THROW(expr);
#define TS_ASSERT_THROWS_ANYTHING(expr)  \
{ \
  bool threw=false; \
  try { \
    expr; \
  } catch (...) {\
    threw=true;\
  } \
  BOOST_TEST(threw==true);\
}
#define TS_ASSERT_THROWS(expr,type)      BOOST_CHECK_THROW(expr,type);
#define TS_FAIL(msg)                     BOOST_FAIL(msg);
#define TS_WARN(msg)                     BOOST_TEST_MESSAGE(msg);
#define TS_ASSERT_SAME_DATA(x,y,size)    BOOST_CHECK_EQUAL_COLLECTIONS(x, x+size, y, y+size)

#endif

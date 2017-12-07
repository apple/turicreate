/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define STR(s) #s
#define ML_ASSERT(x) { if (!(x)) { \
    std::cout << __FILE__ << ":" << __LINE__ << ": error: " << STR(x) << " was false, expected true." << std::endl; \
    return 1; \
} } \

#define ML_ASSERT_GOOD(x) ML_ASSERT((x).good())
#define ML_ASSERT_BAD(x) ML_ASSERT(!((x).good()))
#define ML_ASSERT_EQ(x, y) ML_ASSERT((x) == (y))
#define ML_ASSERT_LT(x, y) ML_ASSERT((x) < (y))
#define ML_ASSERT_GT(x, y) ML_ASSERT((x) > (y))

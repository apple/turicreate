/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <capi/TuriCreate.h>

class test_flexible_type : public CxxTest::TestSuite {
 public:

  void test_flex_double() {
    tc_error* error = NULL; 

    tc_flexible_type* ft = tc_ft_create_from_double(1, &error);

    CAPI_CHECK_ERROR(error);
    TS_ASSERT(ft != NULL); 

  }
};


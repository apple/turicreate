/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#include <toolkits/style_transfer/style_transfer_data_iterator.hpp>

#include <string>

#include "utils.hpp"

using namespace style_transfer_testing;
using namespace turi::style_transfer;

BOOST_AUTO_TEST_CASE(test_initialization) {
  const size_t test_batch_size = 6;
  const size_t expected_batch_array[] = { 6, 6, 6, 6, 6, 6, 6, 6, 2 };

  size_t expected_batch_size = sizeof(expected_batch_array)/sizeof(expected_batch_array[0]);

  turi::gl_sarray style_sarray = style_transfer_testing::random_image_sarray(8);
  turi::gl_sarray content_sarray = style_transfer_testing::random_image_sarray(50);
  
  data_iterator::parameters params;

  params.style = style_sarray;
  params.content = content_sarray;

  style_transfer_data_iterator iter(params);

  for (size_t x = 0; x < expected_batch_size; x++) {
    std::vector<st_example> test_batch = iter.next_batch(test_batch_size);
    TS_ASSERT(test_batch.size() == expected_batch_array[x]);
  }
}
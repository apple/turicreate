/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <vector>
#include <string>
#include <core/random/random.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>

#include <toolkits/sparse_similarity/sliced_itemitem_matrix.hpp>
#include <core/util/cityhash_tc.hpp>

using namespace turi;
using namespace turi::sparse_sim;

struct item_sim_utilities  {

 public:
  void test_specific_case() {

    // Test on a 16x16 grid with target_item_count = 16.  This will be
    // 8 passes of 1 row, then 1 passes of 2 rows, then a row of 3
    // rows as there are 6 + 5 + 4 elements in rows 10, 11, and 12;
    // this can fit in 16 bytes.

    std::vector<size_t> true_block_boundaries = {0,1,2,3,4,5,6,7, 8,10,13,16};

    auto block_boundaries = calculate_upper_triangular_slice_structure(16, 16, 11);

    DASSERT_EQ(block_boundaries.size(), 12);
    for(size_t i = 0; i < block_boundaries.size(); ++i) {
      DASSERT_EQ(block_boundaries[i], true_block_boundaries[i]);
    }
  }

  void test_block_boundary_sanity() {

    for(size_t num_items : {5, 10, 50, 100} ) {

      ////////////////////////////////////////////////////////////////////////////////
      // Accumulate the counts.

      std::vector<size_t> n_block_results;

      for(size_t target_item_count = num_items;
          target_item_count < num_items*num_items;
          target_item_count += num_items) {

        auto block_boundaries
            = calculate_upper_triangular_slice_structure(num_items, target_item_count, num_items);

        size_t num_blocks = block_boundaries.size() - 1;

        DASSERT_LE(num_blocks, num_items);
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t blocks_if_simple_slices =
          (target_item_count + (num_items - 1) / num_items);
        DASSERT_LE(num_blocks, blocks_if_simple_slices);
        DASSERT_EQ(block_boundaries.back(), num_items);

        for(size_t i = 0; i < num_blocks; ++i) {
          DASSERT_LE(block_boundaries[i], block_boundaries[i+1]);

          // Test that the block boundaries are actually increasing
          if(i != 0) {
            TURI_ATTRIBUTE_UNUSED_NDEBUG size_t block_size_1 =
              block_boundaries[i] -  block_boundaries[i-1];
            TURI_ATTRIBUTE_UNUSED_NDEBUG size_t block_size_2 =
              block_boundaries[i+1] -  block_boundaries[i];
            if(block_boundaries[i+1] != num_items) {
              DASSERT_LE(block_size_1, block_size_2);
            }
          }
        }

        n_block_results.push_back(num_blocks);
      }

      // Now test that the number of blocks goes down as we increase the
      // target memory usage.

      for(size_t i = 1; i < n_block_results.size(); ++i) {
        DASSERT_GE(n_block_results[i-1], n_block_results[i]);
      }
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_item_sim_utilities, item_sim_utilities)
BOOST_AUTO_TEST_CASE(test_specific_case) {
  item_sim_utilities::test_specific_case();
}
BOOST_AUTO_TEST_CASE(test_block_boundary_sanity) {
  item_sim_utilities::test_block_boundary_sanity();
}
BOOST_AUTO_TEST_SUITE_END()

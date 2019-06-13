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
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <toolkits/recsys/train_test_split.hpp>
#include <core/storage/sframe_data/sframe_iterators.hpp>



using namespace turi;
using namespace turi::recsys;

struct train_test_split  {
 public:
  void run_split_test(size_t num_users, size_t num_items,
                      size_t val_users, double item_prob, size_t num_observations) {

    ////////////////////////////////////////////////////////////
    // Build the data
    std::vector<std::vector<flexible_type> > train_data;

    train_data.reserve(num_observations);

    random::seed(0);

    // Do one run through with all users and all items;
    for(size_t uid = 0; uid < num_users; ++uid) {
      size_t user = uid;
      size_t item = uid % num_items;
      train_data.push_back( {user, item} );
    }

    do {
      size_t user = random::fast_uniform<size_t>(0, num_users - 1);
      size_t item = random::fast_uniform<size_t>(0, num_items - 1);
      train_data.push_back( {user, item} );

    } while(train_data.size() < num_observations);

    sframe data = make_testing_sframe({"user", "item"},
                                      {flex_type_enum::INTEGER, flex_type_enum::INTEGER},
                                      train_data);

    TS_ASSERT_EQUALS(data.size(), num_observations);

    sframe train_sf, validation_sf;

    std::tie(train_sf, validation_sf) = make_recsys_train_test_split(
        data, "user", "item", val_users, item_prob, 0);

    size_t n_val_users = std::min(val_users, num_users);

    std::vector<size_t> item_val_counts(num_items, 0);
    std::vector<size_t> item_total_counts(num_items, 0);
    std::set<size_t> seen_users;

    for(parallel_sframe_iterator it(validation_sf); !it.done(); ++it) {

      size_t user = it.value(0);
      size_t item = it.value(1);

      seen_users.insert(user);
      ++item_val_counts[item];
      ++item_total_counts[item];
    }

    TS_ASSERT_EQUALS(seen_users.size(), n_val_users);

    for(parallel_sframe_iterator it(train_sf); !it.done(); ++it) {

      size_t user = it.value(0);
      size_t item = it.value(1);

      if(seen_users.count(user)) {
        ++item_total_counts[item];
      }
    }

    double n_val_item = std::accumulate(item_val_counts.begin(), item_val_counts.end(), 0.0);
    double n_tot_item = std::accumulate(item_total_counts.begin(), item_total_counts.end(), 0.0);

    TS_ASSERT_DELTA(n_val_item / n_tot_item, item_prob, 0.05);
  }

  void test_equal_users() {
    run_split_test(100, 100, 100, 0.5, 10000);
  }

  void test_few_users() {
    run_split_test(100, 1000, 1000, 0.5, 10000);
  }

  void test_user_coverage_1() {
    run_split_test(150, 500, 100, 0.5, 20000);
  }

  void test_user_coverage_2() {
    run_split_test(250, 500, 100, 0.5, 30000);
  }

  void test_prob_1() {
    run_split_test(10, 100, 10, 0.1, 10000);
  }

  void test_prob_2() {
    run_split_test(100, 1000, 100, 0.5, 10000);
  }

};

BOOST_FIXTURE_TEST_SUITE(_train_test_split, train_test_split)
BOOST_AUTO_TEST_CASE(test_equal_users) {
  train_test_split::test_equal_users();
}
BOOST_AUTO_TEST_CASE(test_few_users) {
  train_test_split::test_few_users();
}
BOOST_AUTO_TEST_CASE(test_user_coverage_1) {
  train_test_split::test_user_coverage_1();
}
BOOST_AUTO_TEST_CASE(test_user_coverage_2) {
  train_test_split::test_user_coverage_2();
}
BOOST_AUTO_TEST_CASE(test_prob_1) {
  train_test_split::test_prob_1();
}
BOOST_AUTO_TEST_CASE(test_prob_2) {
  train_test_split::test_prob_2();
}
BOOST_AUTO_TEST_SUITE_END()

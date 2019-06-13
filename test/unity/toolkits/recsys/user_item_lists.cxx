/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <model_server/lib/flex_dict_view.hpp>

// ML-Data Utils
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/testing_utils.hpp>
#include <toolkits/recsys/user_item_lists.hpp>

// Testing utils common to all of ml_data_iterator
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>


typedef Eigen::Matrix<double,Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

using namespace turi;
using namespace turi::recsys;
using namespace turi::v2;

struct user_item_lists  {
 public:

  // Test the block iterator by stress-testing a large number of
  // combinations of bounds, threads, sizes, and types

  void run_user_item_lists(size_t n, const std::string& run_string) {

    random::seed(0);

    sframe raw_data;
    v2::ml_data data;

    std::map<std::string, flexible_type> creation_options =
        { {"sort_by_first_two_columns", true} };


    std::tie(raw_data, data) = make_random_sframe_and_ml_data(
        n, run_string, true, creation_options);

    std::map<size_t, std::multimap<size_t, double> > known_user_item_lists_pre_average;
    std::map<size_t, std::map<size_t, double> > known_user_item_lists;

    std::vector<v2::ml_data_entry> x;

    for(auto it = data.get_iterator(0, 1, false); !it.done(); ++it) {
      it.fill_observation(x);

      // Track all items so we can average later
      known_user_item_lists_pre_average[x[0].index].insert({x[1].index, it.target_value()});

      // This gets replaced with the average later on
      known_user_item_lists[x[0].index].insert({x[1].index, 0});
    }

    // Convert all the duplicate values to averages
    for(const auto& p : known_user_item_lists_pre_average) {
      size_t user = p.first;
      const std::multimap<size_t, double>& items = p.second;

      for(const auto& it_p : known_user_item_lists[user]) {

        size_t item = it_p.first;

        auto it_range = items.equal_range(item);

        size_t count = 0;
        double target_total = 0;
        for(auto it = it_range.first; it != it_range.second; ++it) {
          ++count;
          target_total += it->second;
        }

        known_user_item_lists[user][item] = target_total / count;
      }
    }

    auto test_list_sarray = make_user_item_lists(data);

    // Now test it!
    std::vector<std::vector<std::pair<size_t, double> > > test_lists;

    test_list_sarray->get_reader()->read_rows(0, test_list_sarray->size(), test_lists);

    ASSERT_EQ(test_lists.size(), known_user_item_lists.size());

    // Test that the results are approximately equal
    for(size_t user = 0; user < test_lists.size(); ++user) {
      for(const auto& p : test_lists[user]) {
        ASSERT_DELTA(p.second, known_user_item_lists[user][(size_t)p.first], 1e-6);
      }
    }
  }

  void test_small_1() {
    // All unique
    run_user_item_lists(5, "CC");
  }

  void test_small_2() {
    run_user_item_lists(5, "Cb");
  }

  void test_small_3() {
    run_user_item_lists(5, "bC");
  }

  void test_med_1() {
    run_user_item_lists(1000, "ZC");
  }

  void test_med_2() {
    run_user_item_lists(1000, "Zc");
  }

  void test_large() {
    run_user_item_lists(20000, "cZ");
  }

  void test_extra() {
    run_user_item_lists(1000, "ZZduv");
  }

};

BOOST_FIXTURE_TEST_SUITE(_user_item_lists, user_item_lists)
BOOST_AUTO_TEST_CASE(test_small_1) {
  user_item_lists::test_small_1();
}
BOOST_AUTO_TEST_CASE(test_small_2) {
  user_item_lists::test_small_2();
}
BOOST_AUTO_TEST_CASE(test_small_3) {
  user_item_lists::test_small_3();
}
BOOST_AUTO_TEST_CASE(test_med_1) {
  user_item_lists::test_med_1();
}
BOOST_AUTO_TEST_CASE(test_med_2) {
  user_item_lists::test_med_2();
}
BOOST_AUTO_TEST_CASE(test_large) {
  user_item_lists::test_large();
}
BOOST_AUTO_TEST_CASE(test_extra) {
  user_item_lists::test_extra();
}
BOOST_AUTO_TEST_SUITE_END()

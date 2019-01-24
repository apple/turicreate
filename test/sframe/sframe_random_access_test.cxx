/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <sframe/sframe_random_access.hpp>
#include <unity/toolkits/util/random_sframe_generation.hpp>
#include <unity/toolkits/util/sframe_test_util.hpp>
#include <util/basic_types.hpp>
#include <util/string_util.hpp>
#include <util/test_macros.hpp>
#include <util/time_spec.hpp>

using namespace turi;

using std::min;

struct sframe_random_access_test {
  void test_sframe_random_access_conversion() {
    int64_t seed = 0;

    vector<int64_t> n_rows_pool = {
      10,
      1000,
      100000,
    };

    vector<string> column_types_pool = {
      "z",
      "n",
      "r",
      "R",
      "S",
      "X",
      "H",
      "znr",
      "rHnS",
      "zznHrRSXH",
    };

    for (int64_t n_rows : n_rows_pool) {
      for (string column_types : column_types_pool) {
        auto sf1 = _generate_random_sframe(
          n_rows, column_types, seed, false, 0.0);

        auto t0 = now();
        auto sfr = sframe_random_access::from_sframe(sf1);
        auto sf2 = sframe_random_access::to_sframe(sfr);
        auto tr = (now() - t0).val();

        TS_ASSERT(check_equality_gl_sframe(sf1, sf2));
        fmt(cerr,
            "test_sframe_random_access_conversion complete [%v sec]: %v, %v\n",
            cc_sprintf("%5.3f", tr / 1.0e9),
            n_rows,
            column_types);

        ++seed;
      }
    }
  }

  void test_sframe_random_access_groupby() {
    int64_t seed = 0;

    vector<int64_t> n_rows_pool = {
      10,
      1000,
      100000,
    };

    for (int64_t n_rows : n_rows_pool) {
      string column_types;

      {
        column_types = "xznh";
        auto sf1 = _generate_random_sframe(
          n_rows, column_types, seed, false, 0.0);

        auto res_check = sf1.groupby(
          {"X1-x"},
          {{"group_res1", aggregate::SUM("X2-z"),},
           {"group_res2", aggregate::SUM("X3-n"),}}
        );

        auto t0 = now();
        auto sfr = sframe_random_access::from_sframe(sf1);
        auto srr = sfr->group_by(
          {"X1-x"},
          {
            make_pair("group_res1",
                      sframe_random_access::group_by_spec::create_reduce(
                        "SUM",
                        sfr->get_record_at_field_name("X2-z"))),

            make_pair("group_res2",
                      sframe_random_access::group_by_spec::create_reduce(
                        "SUM",
                        sfr->get_record_at_field_name("X3-n"))),
              })->materialize();
        auto res = sframe_random_access::to_sframe(srr);
        auto tr = (now() - t0).val();

        TS_ASSERT(check_equality_gl_sframe(res, res_check, false));
        fmt(cerr,
            "test_sframe_random_access_groupby complete [%v sec]: %v, %v\n",
            cc_sprintf("%5.3f", tr / 1.0e9),
            n_rows,
            column_types);

        ++seed;
      }
    }
  }

  void test_sframe_random_access_join() {
    int64_t seed = 0;

    vector<int64_t> n_rows_pool = {
      10,
      1000,
    };

    vector<string> join_column_type_pool = {
      "H",
      "s",
      "c",
      "x",
      "Z",
      "n",
    };

    for (int64_t n_rows : n_rows_pool) {
      for (string join_column_type : join_column_type_pool) {
        auto sf_base_left_join = _generate_random_sframe(
          n_rows, join_column_type, seed, false, 0.0);
        auto sf_base_left_extra = _generate_random_sframe(
          n_rows, "x", seed + 1, false, 0.0);

        auto sf_base_right = _generate_random_sframe(
          n_rows, join_column_type + "rx", seed, false, 0.0);
        sf_base_right = sf_base_right.sort("X2-r"); // shuffle

        gl_sframe sf_left;
        sf_left.add_column(sf_base_left_extra["X1-x"], "A");
        sf_left.add_column(sf_base_left_join["X1-" + join_column_type], "B");

        gl_sframe sf_right;
        sf_right.add_column(sf_base_right["X1-" + join_column_type], "B");
        sf_right.add_column(sf_base_right["X3-x"], "C");
        auto res_check = sf_left.join(sf_right, {"B"});

        auto t0 = now();
        auto sfr_left = sframe_random_access::from_sframe(sf_left);
        auto sfr_right = sframe_random_access::from_sframe(sf_right);
        auto srr = sfr_left->join_auto(sfr_right)->materialize();
        auto res = sframe_random_access::to_sframe(srr);
        auto tr = (now() - t0).val();

        TS_ASSERT(check_equality_gl_sframe(res, res_check, false));
        fmt(cerr,
            "test_sframe_random_access_join complete [%v sec]: %v, %v\n",
            cc_sprintf("%5.3f", tr / 1.0e9),
            n_rows,
            join_column_type);
      }
    }
  }

  void test_sframe_random_access_filter() {
    int64_t seed = 0;

    vector<int64_t> n_rows_pool = {
      10,
      1000,
      100000,
    };

    for (int64_t n_rows : n_rows_pool) {
      string column_types;

      {
        column_types = "xznh";
        auto sf1 = _generate_random_sframe(
          n_rows, column_types, seed, false, 0.0);

        auto res_check = sf1[sf1["X1-x"] == sf1["X1-x"][0]];
        res_check.materialize();

        auto t0 = now();
        auto sfr = sframe_random_access::from_sframe(sf1);
        auto srr = sfr->at(
          sfr->at_string("X1-x")->equals_value_poly(
            sfr->at_string("X1-x")->at_int(0)))->materialize();
        auto res = sframe_random_access::to_sframe(srr);
        auto tr = (now() - t0).val();

        TS_ASSERT(check_equality_gl_sframe(res, res_check, false));
        fmt(cerr,
            "test_sframe_random_access_filter complete [%v sec]: %v, %v\n",
            cc_sprintf("%5.3f", tr / 1.0e9),
            n_rows,
            column_types);

        ++seed;
      }
    }
  }

  void test_sframe_random_access_filter_multiple() {
    int64_t seed = 0;

    vector<int64_t> n_iter_pool = {
      1,
      100,
    };

    vector<int64_t> n_rows_pool = {
      1000,
      1000000,
    };

    for (int64_t n_rows : n_rows_pool) {
      for (int64_t n_iter : n_iter_pool) {
        n_iter = min<int64_t>(n_iter, n_rows);

        string column_types = "XznnH";
        auto sf1 = _generate_random_sframe(
          n_rows, column_types, seed, false, 0.0);
        auto sf1_col = sf1["X1-X"];
        auto sfr = sframe_random_access::from_sframe(sf1);
        auto sfr_col = sfr->at_string("X1-X")->materialize();

        int64_t t_all = 0;
        int64_t t_all_check = 0;
        for (int64_t iter = 0; iter < n_iter; iter++) {
          auto t0 = now();
          auto srr = sfr->at(
            sfr_col->equals_value_poly(
              sfr_col->at_int(iter)))->materialize();
          auto tr = (now() - t0).val();
          auto res = sframe_random_access::to_sframe(srr);
          t_all += tr;

          auto t0_check = now();
          auto res_check = sf1[sf1_col == sf1_col[iter]];
          res_check.materialize();
          auto tr_check = (now() - t0_check).val();
          t_all_check += tr_check;

          TS_ASSERT(check_equality_gl_sframe(res, res_check, false));
        }

        fmt(cerr,
            "test_sframe_random_access_filter_multiple complete"
            " [%v vs. %v sec]: %v, %v, %v\n",
            cc_sprintf("%5.3f", t_all / 1.0e9),
            cc_sprintf("%5.3f", t_all_check / 1.0e9),
            n_rows,
            n_iter,
            column_types);

        ++seed;
      }
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_sframe_random_access_test, sframe_random_access_test)
BOOST_AUTO_TEST_CASE(test_sframe_random_access_conversion) {
  sframe_random_access_test::test_sframe_random_access_conversion();
}
BOOST_AUTO_TEST_CASE(test_sframe_random_access_groupby) {
  sframe_random_access_test::test_sframe_random_access_groupby();
}
BOOST_AUTO_TEST_CASE(test_sframe_random_access_join) {
  sframe_random_access_test::test_sframe_random_access_join();
}
BOOST_AUTO_TEST_CASE(test_sframe_random_access_filter) {
  sframe_random_access_test::test_sframe_random_access_filter();
}
BOOST_AUTO_TEST_CASE(test_sframe_random_access_filter_multiple) {
  sframe_random_access_test::test_sframe_random_access_filter_multiple();
}
BOOST_AUTO_TEST_SUITE_END()

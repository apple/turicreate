
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <algorithm>
#include <core/util/cityhash_tc.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <model_server/lib/flex_dict_view.hpp>

// ML-Data Utils
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_entry.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>

// Testing utils common to all of ml_data_iterator
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <toolkits/ml_data_2/testing_utils.hpp>

#include <core/util/cityhash_tc.hpp>

#include <core/globals/globals.hpp>

typedef Eigen::Matrix<double,Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

using namespace turi;

namespace std {

// Define this so we can use unordered sets of vectors.
template<>
struct hash<std::vector<turi::v2::ml_data_entry> > {
  inline size_t operator()(const std::vector<turi::v2::ml_data_entry>& s) const {
    return turi::hash64( (const char*)(s.data()), sizeof(turi::v2::ml_data_entry) * s.size());
  }
};

}

struct sorting_and_block_iterator  {
 public:


  // Test the block iterator by stress-testing a large number of
  // combinations of bounds, threads, sizes, and types

  void run_block_check_test(const size_t n, const std::string& run_string, bool target_column) {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);

    ASSERT_GE(run_string.size(), 2);

    std::map<std::string, flexible_type>
        creation_options_1 = { {"sort_by_first_two_columns", true} };

    std::map<std::string, flexible_type>
        creation_options_2 = { {"sort_by_first_two_columns_on_train", true} };

    std::string print_str = run_string;

    if(target_column)
      print_str += ":target";

    random::seed(0);

    struct proc_data {
      v2::ml_data data;
      size_t ref_index;
      bool should_be_sorted;
    };


    sframe raw_data_v[2];
    std::vector<std::vector<flexible_type> > ref_data_v[2];

    std::vector<proc_data> data_v;

    // Always sorted
    {
      v2::ml_data d;
      std::tie(raw_data_v[0], d) = v2::make_random_sframe_and_ml_data(
          n, run_string, target_column, creation_options_1);

      ref_data_v[0] = testing_extract_sframe_data(raw_data_v[0]);

      data_v.push_back(proc_data{d, 0, true} );
    }

    // Sorted only on train time
    {
      v2::ml_data d;
      std::tie(raw_data_v[1], d) = v2::make_random_sframe_and_ml_data(
          n, run_string, target_column, creation_options_2);

      ref_data_v[1] = testing_extract_sframe_data(raw_data_v[1]);

      data_v.push_back(proc_data{d, 1, true} );
    }

    // Saved and loaded versions of the above
    for(size_t i : {0, 1} ) {

      proc_data pd = data_v[i];
      save_and_load_object(pd.data, data_v[i].data);
      data_v.push_back(pd);
    }

    // Saved and loaded metadata, rest reindexed.
    for(size_t i : {0, 1} ) {

      std::shared_ptr<v2::ml_metadata> m_sl;
      save_and_load_object(m_sl, data_v[i].data.metadata());

      proc_data pd = data_v[i];

      pd.data = v2::ml_data(m_sl, false);

      if(target_column)
        pd.data.set_data(raw_data_v[i], "target");
      else
        pd.data.set_data(raw_data_v[i]);

      // If it's the first one, it always gets sorted; otherwise, just
      // on train time.  With the reindexing, it won't be sorted the
      // second time.
      if(pd.ref_index == 0)
        pd.should_be_sorted = true;
      else
        pd.should_be_sorted = false;

      pd.data.fill();
      data_v.push_back(pd);
    }

    // Now, repeat versions of the above, but on predict time, with
    // the other data source.
    size_t n_data_v = data_v.size();
    for(size_t i = 0; i < n_data_v; ++i)
    {
      const proc_data& pd_source = data_v[i];

      proc_data pd;

      pd.data = v2::ml_data(pd_source.data.metadata(), false);

      pd.ref_index = (pd_source.ref_index == 1) ? 0 : 1;

      if(target_column)
        pd.data.set_data(raw_data_v[pd.ref_index], "target");
      else
        pd.data.set_data(raw_data_v[pd.ref_index]);

      pd.data.fill();

      // If it's the first one, it always gets sorted; otherwise, just
      // on train time.
      if(pd_source.ref_index == 0)
        pd.should_be_sorted = true;
      else
        pd.should_be_sorted = false;

      data_v.push_back(pd);
    }

    size_t n_threads_v[] = {1, 3, 13, 79};

    parallel_for(0, data_v.size() * 4 * 4, [&](size_t main_idx) {

        std::vector<v2::ml_data_entry> x;
        std::vector<flexible_type> row_x;

        size_t data_i = main_idx / 16;
        size_t thread_i = (main_idx / 4) % 4;
        size_t segment_i = (main_idx) % 4;

        const auto& data     = data_v[data_i].data;
        const auto& ref_data = ref_data_v[data_v[data_i].ref_index];
        const auto& raw_data = raw_data_v[data_v[data_i].ref_index];

        std::vector<std::pair<size_t, size_t> > row_segments
        { {0, n},
          {0, n / 3},
          {n / 3, 2*n / 3},
          {2*n / 3, n} };

        std::vector<int> hit_row(data.size());
        std::unordered_set<size_t> user_hit;
        std::unordered_map<std::vector<v2::ml_data_entry>, size_t> row_set, reference_row_set;


        bool should_be_sorted = data_v[data_i].should_be_sorted;

        size_t n_threads = n_threads_v[thread_i];
        auto row_segment = row_segments[segment_i];

        ////////////////////////////////////////////////////////////////////////////////
        // Report

        if(thread::cpu_count() == 1) {
          std::cerr << "Case (" << print_str << ":"
                    << data_i << ","
                    << thread_i
                    << ","
                    << segment_i
                    << ")" << std::endl;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Run the actual tests


        size_t row_start = row_segment.first;
        size_t row_end   = row_segment.second;

        v2::ml_data sliced_data = data.slice(row_start, row_end);

        ASSERT_EQ(sliced_data.size(), row_end - row_start);

        if(should_be_sorted) {

          // Just need to verify they are indeed sorted, and all in
          // the correct place. Then we'll make sure that they are
          // blocked correctly

          size_t last_col_1_idx = 0;
          size_t last_col_2_idx = 0;

          for(auto it = sliced_data.get_iterator(); !it.done(); ++it) {
            it.fill_observation(x);

            size_t col_1_idx = x[0].index;
            size_t col_2_idx = x[1].index;

            ASSERT_LE(last_col_1_idx, col_1_idx);

            if(col_1_idx == last_col_1_idx) {
              ASSERT_LE(last_col_2_idx, col_2_idx);
            }

            last_col_1_idx = col_1_idx;
            last_col_2_idx = col_2_idx;

            reference_row_set[x] += 1;
          }
        }

        hit_row.assign(data.size(), false);

        for(size_t thread_idx = 0; thread_idx < n_threads; ++thread_idx) {

          for(auto it = sliced_data.get_block_iterator(thread_idx, n_threads); !it.done(); ++it) {

            ASSERT_LT(it.row_index(), row_end - row_start);
            ASSERT_EQ(it.unsliced_row_index(), row_start + it.row_index());
            ASSERT_LE(row_start, it.unsliced_row_index());
            ASSERT_LT(it.unsliced_row_index(), row_end);

            size_t it_idx = it.unsliced_row_index();

            // std::cout << "row_start = " << row_start
            //           << ", row_end = " << row_end
            //           << ", thread_idx = " << thread_idx
            //           << ", n_threads = " << n_threads
            //           << ", it_idx = " << it_idx << std::endl;

            ASSERT_FALSE(hit_row[it_idx]);

            hit_row[it_idx] = true;

            it.fill_observation(x);
            // std::cerr << "x = " << x << std::endl;

            if(should_be_sorted) {
              if(it.is_start_of_new_block()) {
                size_t user = x[0].index;
                ASSERT_EQ(user_hit.count(user), 0);
                user_hit.insert(user);
              }

              row_set[x] += 1;

            } else {

              row_x = data.translate_row_to_original(x);

              ASSERT_EQ(row_x.size(), run_string.size());

              if(target_column)
                row_x.push_back(size_t(it.target_value()));

              ASSERT_EQ(row_x.size(), raw_data.num_columns());

              ASSERT_EQ(row_x.size(), ref_data.at(it_idx).size());
              for(size_t ri = 0; ri < row_x.size(); ++ri) {
                flexible_type v_rec = row_x.at(ri);
                flexible_type v_ref = ref_data.at(it_idx).at(ri);

                ASSERT_TRUE(v2::ml_testing_equals(v_rec, v_ref));
              }
            }
          }
        }

        // Now go through and make sure that all the entries we are
        // supposed to hit were indeed hit and none of the others were
        // hit.
        for(size_t i = 0; i < hit_row.size(); ++i) {
          if(row_start <= i && i < row_end) {
            ASSERT_TRUE(hit_row[i]);
          } else {
            ASSERT_FALSE(hit_row[i]);
          }
        }

        if(should_be_sorted) {
          ASSERT_TRUE(row_set == reference_row_set);
        }

      });
  }

  void test_block_iter_0_noside() {
    run_block_check_test(5, "bc", false);
  }

  void test_block_iter_0_add() {
    run_block_check_test(5, "bcnnn", false);
  }

  void test_block_iter_0_var_side() {
    run_block_check_test(5, "bcnd", false);
  }

  void test_block_iter_1_noside() {
    run_block_check_test(50, "cc", false);
  }

  void test_block_iter_1_add() {
    run_block_check_test(50, "ccnnn", false);
  }

  void test_block_iter_1_var_side() {
    run_block_check_test(50, "ccnd", false);
  }

  void test_block_iter_2_noside() {
    run_block_check_test(50, "CC", false);
  }

  void test_block_iter_2_add() {
    run_block_check_test(50, "CCnnn", false);
  }

  void test_block_iter_2_var_side() {
    run_block_check_test(50, "CCnd", false);
  }

  void test_block_iter_3_add() {
    run_block_check_test(50, "CCdvs", false);
  }

  void test_block_iter_3_var_side() {
    run_block_check_test(50, "CCdu", false);
  }

  void test_block_iter_4_noside() {
    run_block_check_test(50, "ss", false);
  }

  void test_block_iter_4_add() {
    run_block_check_test(50, "ssnnn", false);
  }

  void test_block_iter_4_var_side() {
    run_block_check_test(50, "ssdv", false);
  }

  void test_block_iter_5_add() {
    run_block_check_test(50, "SSdvs", false);
  }

  void test_block_iter_5_var_side() {
    run_block_check_test(50, "SSdu", false);
  }

  void test_block_iter_large() {
    run_block_check_test(5000, "cC", false);
  }

  void test_block_iter_large_varsize() {
    run_block_check_test(1000, "cCu", false);
  }

  void test_block_iter_very_large() {
    run_block_check_test(127473, "CC", false);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Same as above, but with targets

  void test_block_iter_0_noside_t() {
    run_block_check_test(5, "bc", true);
  }

  void test_block_iter_0_add_t() {
    run_block_check_test(5, "bcnnn", true);
  }

  void test_block_iter_0_var_side_t() {
    run_block_check_test(5, "bcnd", true);
  }

  void test_block_iter_1_noside_t() {
    run_block_check_test(50, "cc", true);
  }

  void test_block_iter_1_add_t() {
    run_block_check_test(50, "ccnnn", true);
  }

  void test_block_iter_1_var_side_t() {
    run_block_check_test(50, "ccnd", true);
  }

  void test_block_iter_2_noside_t() {
    run_block_check_test(50, "CC", true);
  }

  void test_block_iter_2_add_t() {
    run_block_check_test(50, "CCnnn", true);
  }

  void test_block_iter_2_var_side_t() {
    run_block_check_test(50, "CCnd", true);
  }

  void test_block_iter_3_add_t() {
    run_block_check_test(50, "CCdvs", true);
  }

  void test_block_iter_3_var_side_t() {
    run_block_check_test(50, "CCdu", true);
  }

  void test_block_iter_4_noside_t() {
    run_block_check_test(50, "ss", true);
  }

  void test_block_iter_4_add_t() {
    run_block_check_test(50, "ssnnn", true);
  }

  void test_block_iter_4_var_side_t() {
    run_block_check_test(50, "ssdv", true);
  }

  void test_block_iter_5_add_t() {
    run_block_check_test(50, "SSdvs", true);
  }

  void test_block_iter_5_var_side_t() {
    run_block_check_test(50, "SSdu", true);
  }

  void test_block_iter_large_t() {
    run_block_check_test(5000, "cC", true);
  }

  void test_block_iter_large_varsize_t() {
    run_block_check_test(1000, "cCu", true);
  }

  void test_block_iter_very_large_t() {
    run_block_check_test(127473, "CC", true);
  }

};

BOOST_FIXTURE_TEST_SUITE(_sorting_and_block_iterator, sorting_and_block_iterator)
BOOST_AUTO_TEST_CASE(test_block_iter_0_noside) {
  sorting_and_block_iterator::test_block_iter_0_noside();
}
BOOST_AUTO_TEST_CASE(test_block_iter_0_add) {
  sorting_and_block_iterator::test_block_iter_0_add();
}
BOOST_AUTO_TEST_CASE(test_block_iter_0_var_side) {
  sorting_and_block_iterator::test_block_iter_0_var_side();
}
BOOST_AUTO_TEST_CASE(test_block_iter_1_noside) {
  sorting_and_block_iterator::test_block_iter_1_noside();
}
BOOST_AUTO_TEST_CASE(test_block_iter_1_add) {
  sorting_and_block_iterator::test_block_iter_1_add();
}
BOOST_AUTO_TEST_CASE(test_block_iter_1_var_side) {
  sorting_and_block_iterator::test_block_iter_1_var_side();
}
BOOST_AUTO_TEST_CASE(test_block_iter_2_noside) {
  sorting_and_block_iterator::test_block_iter_2_noside();
}
BOOST_AUTO_TEST_CASE(test_block_iter_2_add) {
  sorting_and_block_iterator::test_block_iter_2_add();
}
BOOST_AUTO_TEST_CASE(test_block_iter_2_var_side) {
  sorting_and_block_iterator::test_block_iter_2_var_side();
}
BOOST_AUTO_TEST_CASE(test_block_iter_3_add) {
  sorting_and_block_iterator::test_block_iter_3_add();
}
BOOST_AUTO_TEST_CASE(test_block_iter_3_var_side) {
  sorting_and_block_iterator::test_block_iter_3_var_side();
}
BOOST_AUTO_TEST_CASE(test_block_iter_4_noside) {
  sorting_and_block_iterator::test_block_iter_4_noside();
}
BOOST_AUTO_TEST_CASE(test_block_iter_4_add) {
  sorting_and_block_iterator::test_block_iter_4_add();
}
BOOST_AUTO_TEST_CASE(test_block_iter_4_var_side) {
  sorting_and_block_iterator::test_block_iter_4_var_side();
}
BOOST_AUTO_TEST_CASE(test_block_iter_5_add) {
  sorting_and_block_iterator::test_block_iter_5_add();
}
BOOST_AUTO_TEST_CASE(test_block_iter_5_var_side) {
  sorting_and_block_iterator::test_block_iter_5_var_side();
}
BOOST_AUTO_TEST_CASE(test_block_iter_large) {
  sorting_and_block_iterator::test_block_iter_large();
}
BOOST_AUTO_TEST_CASE(test_block_iter_large_varsize) {
  sorting_and_block_iterator::test_block_iter_large_varsize();
}
BOOST_AUTO_TEST_CASE(test_block_iter_very_large) {
  sorting_and_block_iterator::test_block_iter_very_large();
}
BOOST_AUTO_TEST_CASE(test_block_iter_0_noside_t) {
  sorting_and_block_iterator::test_block_iter_0_noside_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_0_add_t) {
  sorting_and_block_iterator::test_block_iter_0_add_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_0_var_side_t) {
  sorting_and_block_iterator::test_block_iter_0_var_side_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_1_noside_t) {
  sorting_and_block_iterator::test_block_iter_1_noside_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_1_add_t) {
  sorting_and_block_iterator::test_block_iter_1_add_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_1_var_side_t) {
  sorting_and_block_iterator::test_block_iter_1_var_side_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_2_noside_t) {
  sorting_and_block_iterator::test_block_iter_2_noside_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_2_add_t) {
  sorting_and_block_iterator::test_block_iter_2_add_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_2_var_side_t) {
  sorting_and_block_iterator::test_block_iter_2_var_side_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_3_add_t) {
  sorting_and_block_iterator::test_block_iter_3_add_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_3_var_side_t) {
  sorting_and_block_iterator::test_block_iter_3_var_side_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_4_noside_t) {
  sorting_and_block_iterator::test_block_iter_4_noside_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_4_add_t) {
  sorting_and_block_iterator::test_block_iter_4_add_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_4_var_side_t) {
  sorting_and_block_iterator::test_block_iter_4_var_side_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_5_add_t) {
  sorting_and_block_iterator::test_block_iter_5_add_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_5_var_side_t) {
  sorting_and_block_iterator::test_block_iter_5_var_side_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_large_t) {
  sorting_and_block_iterator::test_block_iter_large_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_large_varsize_t) {
  sorting_and_block_iterator::test_block_iter_large_varsize_t();
}
BOOST_AUTO_TEST_CASE(test_block_iter_very_large_t) {
  sorting_and_block_iterator::test_block_iter_very_large_t();
}
BOOST_AUTO_TEST_SUITE_END()

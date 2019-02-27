#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <util/cityhash_tc.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <unity/lib/flex_dict_view.hpp>
#include <random/random.hpp>

// ML-Data Utils
#include <ml_data/ml_data.hpp>

// Testing utils common to all of ml_data_iterator
#include <sframe/testing_utils.hpp>
#include <ml_data/testing_utils.hpp>
#include <util/testing_utils.hpp>

#include <globals/globals.hpp>

using namespace turi;

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

struct test_untranslated_columns_sanity  {
 public:

  void test_basic_1() {
    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 7);

    sframe X = make_integer_testing_sframe( {"C1", "C2"}, { {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4} } );

    ml_data data;

    data.fill(X, "", { {"C2", ml_column_mode::UNTRANSLATED} });

    ASSERT_TRUE(data.metadata()->has_untranslated_columns());
    ASSERT_EQ(data.metadata()->num_untranslated_columns(), 1);
    ASSERT_TRUE(data.metadata()->is_untranslated_column("C2"));
    ASSERT_FALSE(data.metadata()->is_untranslated_column("C1"));

    std::vector<ml_data_entry> x_d;
    std::vector<flexible_type> x_f;

    ////////////////////////////////////////

    for(auto it = data.get_iterator(); !it.done(); ++it) {

      it->fill(x_d);

      ASSERT_EQ(x_d.size(), 1);
      ASSERT_EQ(x_d[0].column_index, 0);
      ASSERT_EQ(x_d[0].index, 0);
      ASSERT_EQ(x_d[0].value, it.row_index());

      it->fill_untranslated_values(x_f);

      ASSERT_EQ(x_f.size(), 1);
      ASSERT_TRUE(x_f[0] == it.row_index());
    }
  }

  void test_basic_2() {
    // globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 7);

    sframe X = make_integer_testing_sframe( {"C1", "C2"}, { {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4} } );

    ml_data data;

    // The next line is the difference from the above version
    data.fill(X, "", { {"C1", ml_column_mode::UNTRANSLATED} });

    ASSERT_TRUE(data.metadata()->has_untranslated_columns());
    ASSERT_EQ(data.metadata()->num_untranslated_columns(), 1);
    ASSERT_FALSE(data.metadata()->is_untranslated_column("C2"));
    ASSERT_TRUE(data.metadata()->is_untranslated_column("C1"));

    std::vector<ml_data_entry> x_d;
    std::vector<flexible_type> x_f;

    ////////////////////////////////////////

    for(auto it = data.get_iterator(); !it.done(); ++it) {

      it->fill(x_d);

      ASSERT_EQ(x_d.size(), 1);
      ASSERT_EQ(x_d[0].column_index, 1);
      ASSERT_EQ(x_d[0].index, 0);
      ASSERT_EQ(x_d[0].value, it.row_index());

      it->fill_untranslated_values(x_f);

      ASSERT_EQ(x_f.size(), 1);
      ASSERT_TRUE(x_f[0] == it.row_index());
    }
  }


};

struct test_untranslated_coulmns  {
 public:

  // Test the block iterator by stress-testing a large number of
  // combinations of bounds, threads, sizes, and types

  void run_check(const std::string& run_string, const std::vector<size_t>& untranslated_columns) {

    // Make sure we are crossing boundaries
    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 11);
    size_t n = 50;

    sframe raw_data = make_random_sframe(n, run_string, false);
    std::vector<std::vector<flexible_type> > ref_data = testing_extract_sframe_data(raw_data);

    std::map<std::string, ml_column_mode> mode_overrides;

    for(size_t c_idx : untranslated_columns)
      mode_overrides[raw_data.column_name(c_idx)] = ml_column_mode::UNTRANSLATED;

    // Several different versions of the ml_data, through various
    // stages of loading and unloading.
    std::array<ml_data, 3> data_v;

    data_v[0].fill(raw_data, "", mode_overrides);

    ASSERT_EQ(data_v[0].size(), raw_data.size());

    // Check that all the column modes are correct
    for(size_t idx : untranslated_columns) {
      ASSERT_TRUE(data_v[0].metadata()->is_untranslated_column(idx));
    }

    // Copied version
    data_v[1] = data_v[0];

    // Version reconstructed from saved metadata
    std::shared_ptr<ml_metadata> m_sl;
    save_and_load_object(m_sl, data_v[0].metadata());

    {
      data_v[2] = ml_data(m_sl);
      data_v[2].fill(raw_data);
    }

    size_t n_threads_v[] = {1, 3, 13, 79};

    std::vector<std::pair<size_t, size_t> > row_segments
    { {0, n},
      {0, n / 3},
      {n / 3, 2*n / 3},
      {2*n / 3, n} };

    parallel_for(0, data_v.size() * 4 * 4, [&](size_t main_idx) {

        std::vector<ml_data_entry> x;
        DenseVector xd;
        Eigen::MatrixXd xdr;
        SparseVector xs;

        std::vector<flexible_type> untranslated_row;

        std::vector<flexible_type> row_x_buffer;
        std::vector<flexible_type> row_x;

        std::vector<int> hit_row(data_v[0].size());

        size_t data_i = main_idx / 16;
        size_t thread_i = (main_idx / 4) % 4;
        size_t segment_i = (main_idx) % 4;

        const auto& data = data_v[data_i];
        size_t n_threads = n_threads_v[thread_i];
        auto row_segment = row_segments[segment_i];

        ////////////////////////////////////////////////////////////////////////////////
        // Report

        if(thread::cpu_count() == 1) {
          std::cerr << "Case (" << run_string << ":"
                    << data_i << ","
                    << thread_i
                    << ","
                    << segment_i
                    << ")" << std::endl;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Run the actual tests

        hit_row.assign(data.size(), false);

        size_t row_start = row_segment.first;
        size_t row_end   = row_segment.second;

        ml_data sliced_data = data.slice(row_start, row_end);

        xd.resize(sliced_data.metadata()->num_dimensions());
        xs.resize(sliced_data.metadata()->num_dimensions());

        xdr.resize(3, sliced_data.metadata()->num_dimensions());
        xdr.setZero();

        ASSERT_EQ(sliced_data.size(), row_end - row_start);

        for(size_t thread_idx = 0; thread_idx < n_threads; ++thread_idx) {

          auto it = sliced_data.get_iterator(thread_idx, n_threads);

          for(; !it.done(); ++it) {

            ASSERT_LT(it.row_index(), row_end - row_start);

            size_t it_idx = row_start + it.row_index();

            ASSERT_FALSE(hit_row[it_idx]);

            hit_row[it_idx] = true;

            for(size_t ref_creation_type : {0, 1}) {

              ml_data_row_reference row_ref;
              
              if(ref_creation_type == 0) {
                row_ref = *it;
              } else {
                // Translate through the single row use case.
                const auto& raw_row = ref_data.at(it_idx);
                DASSERT_EQ(raw_row.size(), raw_data.num_columns()); 

                flex_dict v(raw_row.size());
                for(size_t i = 0; i < raw_row.size(); ++i) {
                  v[i] = {raw_data.column_name(i), raw_row[i]};
                }

                random::shuffle(v);

                row_ref = ml_data_row_reference::from_row(data.metadata(), v);
              }
              
              for(size_t type_idx : {0, 1, 2} ) {

                switch(type_idx) {
                  case 0: {
                    row_ref.fill(x);
                    // std::cerr << "x = " << x << std::endl;
                    row_x_buffer = translate_row_to_original(data.metadata(), x);
                    break;
                  }
                  case 1: {
                    row_ref.fill(xd);
                    row_x_buffer = translate_row_to_original(data.metadata(), xd);
                    // std::cerr << "xd = " << xd.transpose() << std::endl;
                    break;
                  }
                  case 2: {
                    row_ref.fill(xs);
                    row_x_buffer = translate_row_to_original(data.metadata(), xs);
                    break;
                  }
                }

                row_ref.fill_untranslated_values(untranslated_row);

                ASSERT_EQ(untranslated_row.size(), untranslated_columns.size());
              
                // Now recombine the original columns
                row_x.clear();
                size_t uc_idx = 0;
                for(const flexible_type& v : row_x_buffer) {
                  if(v.get_type() == flex_type_enum::UNDEFINED) {
                    row_x.push_back(untranslated_row[uc_idx]);
                    ++uc_idx;
                  } else {
                    row_x.push_back(v);
                  }
                }

                ASSERT_EQ(uc_idx, untranslated_row.size());

                ASSERT_EQ(row_x.size(), run_string.size());
                ASSERT_EQ(row_x.size(), raw_data.num_columns());
                ASSERT_EQ(row_x.size(), ref_data.at(it_idx).size());

                for(size_t ri = 0; ri < row_x.size(); ++ri) {
                  ASSERT_TRUE(ml_testing_equals(row_x.at(ri), ref_data.at(it_idx).at(ri)));
                }
              }
            }
          }
        }
      });
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_untranslated_columns_nn_1() {
    run_check("nn", {1});
  }

  void test_untranslated_columns_nn_2() {
    run_check("nn", {0});
  }

  void test_untranslated_columns_nn_3() {
    run_check("nn", {0, 1});
  }


  void test_untranslated_columns_ssss_1() {
    run_check("ssss", {1,3});
  }

  void test_untranslated_columns_ssss_2() {
    run_check("ssss", {0,1,2,3});
  }

  void test_untranslated_columns_dd_1() {
    run_check("dd", {1});
  }

  void test_untranslated_columns_dd_2() {
    run_check("dd", {0});
  }

  void test_untranslated_columns_dd_3() {
    run_check("dd", {0,1});
  }

  void test_untranslated_columns_v_1() {
    run_check("v", {0});
  }

  void test_untranslated_columns_many_1() {
    run_check("cnsnscsnccccccccncss", {0, 2, 4, 6, 8, 10, 12, 14, 16, 18});
  }

  void test_untranslated_columns_many_2() {
    run_check("cnsnscsnccccccccncss", {19});
  }

  void test_untranslated_columns_many_3() {
    std::string spec = "cnsnscsnccccccccncss";
    std::vector<size_t> x(spec.size() - 1);
    std::iota(x.begin(), x.end(), 0);
    run_check(spec, x);
  }
};

BOOST_FIXTURE_TEST_SUITE(_test_untranslated_columns_sanity, test_untranslated_columns_sanity)
BOOST_AUTO_TEST_CASE(test_basic_1) {
  test_untranslated_columns_sanity::test_basic_1();
}
BOOST_AUTO_TEST_CASE(test_basic_2) {
  test_untranslated_columns_sanity::test_basic_2();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_test_untranslated_coulmns, test_untranslated_coulmns)
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_1) {
  test_untranslated_coulmns::test_untranslated_columns_nn_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_2) {
  test_untranslated_coulmns::test_untranslated_columns_nn_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_3) {
  test_untranslated_coulmns::test_untranslated_columns_nn_3();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_1) {
  test_untranslated_coulmns::test_untranslated_columns_ssss_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_2) {
  test_untranslated_coulmns::test_untranslated_columns_ssss_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_1) {
  test_untranslated_coulmns::test_untranslated_columns_dd_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_2) {
  test_untranslated_coulmns::test_untranslated_columns_dd_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_3) {
  test_untranslated_coulmns::test_untranslated_columns_dd_3();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_v_1) {
  test_untranslated_coulmns::test_untranslated_columns_v_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_1) {
  test_untranslated_coulmns::test_untranslated_columns_many_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_2) {
  test_untranslated_coulmns::test_untranslated_columns_many_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_3) {
  test_untranslated_coulmns::test_untranslated_columns_many_3();
}
BOOST_AUTO_TEST_SUITE_END()

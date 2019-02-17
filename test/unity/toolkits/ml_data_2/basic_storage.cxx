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
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/ml_data_entry.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>

// Testing utils common to all of ml_data_iterator
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>
#include <unity/toolkits/ml_data_2/testing_utils.hpp>

#include <globals/globals.hpp>

using namespace turi;

struct test_basic_storage  {
 public:

  // Test the block iterator by stress-testing a large number of
  // combinations of bounds, threads, sizes, and types

  enum class target_column_type {NONE, NUMERICAL, CATEGORICAL};

  void run_storage_check_test(size_t n, const std::string& run_string, target_column_type target_type) {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);
    globals::set_global("TURI_ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD", 7);

    random::seed(0);

    sframe raw_data;
    std::array<v2::ml_data, 6> data_v;

    std::map<std::string, flexible_type> creation_options;
    bool target_column;

    if(target_type == target_column_type::CATEGORICAL) {
      creation_options["target_column_always_categorical"] = true;
      target_column = true;
    } else if(target_type == target_column_type::NUMERICAL) {
      creation_options["target_column_always_numeric"] = true;
      target_column = true;
    } else {
      target_column = false;
    }

    std::string print_str = run_string;

    if(target_column)
      print_str += ":target";

    std::tie(raw_data, data_v[0]) = v2::make_random_sframe_and_ml_data(
        n, run_string, target_column, creation_options);

    ASSERT_EQ(data_v[0].size(), raw_data.size());

    std::vector<std::vector<flexible_type> > ref_data = testing_extract_sframe_data(raw_data);

    save_and_load_object(data_v[1], data_v[0]);

    data_v[2] = data_v[0];

    std::shared_ptr<v2::ml_metadata> m_sl;
    save_and_load_object(m_sl, data_v[0].metadata());

    {
      data_v[3] = v2::ml_data(m_sl, false);
      if(target_column)
        data_v[3].set_data(raw_data, "target");
      else
        data_v[3].set_data(raw_data);

      data_v[3].fill();
    }

    {
      data_v[4] = v2::ml_data(m_sl, true);
      if(target_column)
        data_v[4].set_data(raw_data, "target");
      else
        data_v[4].set_data(raw_data);

      data_v[4].fill();
    }


    {
      std::vector<std::string> names;

      for(size_t i = 0; i < m_sl->num_columns(false); ++i)
        names.push_back(m_sl->column_name(i));

      std::vector<std::string> names_s = names;
      random::shuffle(names_s.begin(), names_s.end());

      auto reversed_metadata = m_sl->select_columns(names_s);
      auto recovered_metadata = reversed_metadata->select_columns(names);

      data_v[5] = v2::ml_data(recovered_metadata, true);
      if(target_column)
        data_v[5].set_data(raw_data, "target");
      else
        data_v[5].set_data(raw_data);

      data_v[5].fill();
    }


    size_t n_threads_v[] = {1, 3, 13, 79};

    std::vector<std::pair<size_t, size_t> > row_segments
    { {0, n},
      {0, n / 3},
      {n / 3, 2*n / 3},
      {2*n / 3, n} };


    parallel_for(0, data_v.size() * 4 * 4, [&](size_t main_idx) {

        std::vector<v2::ml_data_entry> x, x_alt;
        v2::ml_data::DenseVector xd, xd_alt;
        Eigen::MatrixXd xdr, xdr_alt;
        v2::ml_data::SparseVector xs, xs_alt;
        std::vector<v2::ml_data_entry_global_index> x_gi, x_gi_alt;

        std::vector<flexible_type> row_x;

        std::vector<int> hit_row(data_v[0].size());

        size_t data_i = main_idx / 16;
        size_t thread_i = (main_idx / 4) % 4;
        size_t segment_i = (main_idx) % 4;


        const auto& data = data_v[data_i];
        size_t n_threads = n_threads_v[thread_i];
        auto row_segment = row_segments[segment_i];

        xd.resize(data.metadata()->num_dimensions());
        xd_alt.resize(data.metadata()->num_dimensions());
        xs.resize(data.metadata()->num_dimensions());
        xs_alt.resize(data.metadata()->num_dimensions());

        xdr.resize(3, data.metadata()->num_dimensions());
        xdr.setZero();
        xdr_alt.resize(3, data.metadata()->num_dimensions());
        xdr_alt.setZero();

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

        hit_row.assign(data.size(), false);

        size_t row_start = row_segment.first;
        size_t row_end   = row_segment.second;

        v2::ml_data sliced_data = data.slice(row_start, row_end);

        ASSERT_EQ(sliced_data.size(), row_end - row_start);

        for(size_t thread_idx = 0; thread_idx < n_threads; ++thread_idx) {

          auto it = sliced_data.get_iterator(thread_idx, n_threads);

          for(; !it.done(); ++it) {

            ASSERT_LT(it.row_index(), row_end - row_start);
            ASSERT_EQ(it.unsliced_row_index(), row_start + it.row_index());

            size_t it_idx = it.unsliced_row_index();

            ASSERT_FALSE(hit_row[it_idx]);

            hit_row[it_idx] = true;

            for(size_t type_idx : {0, 1, 2, 3, 4} ) {

              switch(type_idx) {
                case 0: {
                  it.fill_observation(x);

                  it.get_reference().fill(x_alt);
                  ASSERT_TRUE(x_alt == x);

                  // std::cerr << "x = " << x << std::endl;
                  row_x = data.translate_row_to_original(x);
                  break;
                }
                case 1: {
                  it.fill_observation(xd);

                  it.get_reference().fill(xd_alt);
                  ASSERT_TRUE(xd_alt == xd);

                  row_x = data.translate_row_to_original(xd);
                  // std::cerr << "xd = " << xd.transpose() << std::endl;
                  break;
                }
                case 2: {
                  it.fill_observation(xs);

                  it.get_reference().fill(xs_alt);
                  ASSERT_TRUE(xs_alt.toDense() == xs.toDense());

                  row_x = data.translate_row_to_original(xs);
                  break;
                }
                case 3: {
                  it.fill_observation(x_gi);

                  it.get_reference().fill(x_gi_alt);
                  ASSERT_TRUE(x_gi == x_gi_alt);

                  row_x = data.translate_row_to_original(x_gi);
                  break;
                }
                case 4: {
                  it.fill_eigen_row(xdr.row(1));

                  it.get_reference().fill_eigen_row(xdr_alt.row(1));
                  ASSERT_TRUE(xdr == xdr_alt);

                  xd = xdr.row(1);

                  row_x = data.translate_row_to_original(xd);
                  break;
                }
              }

              ASSERT_EQ(row_x.size(), run_string.size());

              if(target_column && target_type == target_column_type::NUMERICAL) {
                row_x.push_back(flex_int(it.target_value()));
              } else if(target_column && target_type == target_column_type::CATEGORICAL) {
                row_x.push_back(data.metadata()->target_indexer()->map_index_to_value(it.target_index()));
              }

              ASSERT_EQ(row_x.size(), raw_data.num_columns());

              ASSERT_EQ(row_x.size(), ref_data.at(it_idx).size());
              for(size_t ri = 0; ri < row_x.size(); ++ri) {
                ASSERT_TRUE(v2::ml_testing_equals(row_x.at(ri), ref_data.at(it_idx).at(ri)));
              }
            }
          }

          // Now, with the same iterator, make sure that the reset works correctly
          {
            it.reset();
            if(!it.done()) {

              size_t it_idx = it.unsliced_row_index();

              it.fill_observation(x);
              row_x = data.translate_row_to_original(x);

              if(target_column && target_type == target_column_type::NUMERICAL) {
                row_x.push_back(flex_int(it.target_value()));
              } else if(target_column && target_type == target_column_type::CATEGORICAL) {
                row_x.push_back(data.metadata()->target_indexer()->map_index_to_value(it.target_index()));
              }

              ASSERT_EQ(row_x.size(), raw_data.num_columns());

              ASSERT_EQ(row_x.size(), ref_data.at(it_idx).size());
              for(size_t ri = 0; ri < row_x.size(); ++ri) {
                ASSERT_TRUE(v2::ml_testing_equals(row_x.at(ri), ref_data.at(it_idx).at(ri)));
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

        // Now, test the random seek function
        if(row_end > row_start) {
          auto it = sliced_data.get_iterator();

          size_t idx = random::fast_uniform<size_t>(0, row_end - 1 - row_start);

          it.seek(idx);
          ASSERT_EQ(it.row_index(), idx);
          ASSERT_EQ(it.unsliced_row_index(), row_start + idx);

          size_t it_idx = it.unsliced_row_index();

          it.fill_observation(x);
          row_x = data.translate_row_to_original(x);

          if(target_column && target_type == target_column_type::NUMERICAL) {
            row_x.push_back(flex_int(it.target_value()));
          } else if(target_column && target_type == target_column_type::CATEGORICAL) {
            row_x.push_back(data.metadata()->target_indexer()->map_index_to_value(it.target_index()));
          }

          ASSERT_EQ(row_x.size(), raw_data.num_columns());

          ASSERT_EQ(row_x.size(), ref_data.at(it_idx).size());
          for(size_t ri = 0; ri < row_x.size(); ++ri) {
            ASSERT_TRUE(v2::ml_testing_equals(row_x.at(ri), ref_data.at(it_idx).at(ri)));
          }
        }
      });
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_storage_000() {
    // All unique
    run_storage_check_test(0, "n", target_column_type::NONE);
  }

  void test_storage_0n() {
    // All unique
    run_storage_check_test(5, "n", target_column_type::NONE);
  }

  void test_storage_0b() {
    run_storage_check_test(5, "b", target_column_type::NONE);
  }

  void test_storage_0c() {
    // All unique
    run_storage_check_test(5, "c", target_column_type::NONE);
  }

  void test_storage_0C() {
    // All unique
    run_storage_check_test(5, "C", target_column_type::NONE);
  }

  void test_storage_1b_unsorted() {
    run_storage_check_test(13, "b", target_column_type::NONE);
  }

  void test_storage_1() {
    run_storage_check_test(13, "bc", target_column_type::NONE);
  }

  void test_storage_2() {
    run_storage_check_test(13, "zc", target_column_type::NONE);
  }

  void test_storage_3() {
    run_storage_check_test(100, "Zc", target_column_type::NONE);
  }

  void test_storage_4() {
    // Pretty much gonna be unique
    run_storage_check_test(100, "Cc", target_column_type::NONE);
  }

  void test_storage_5() {
    // 10 blocks of values.
    run_storage_check_test(1000, "Zc", target_column_type::NONE);
  }

  void test_storage_6() {
    // two large blocks of values
    run_storage_check_test(1000, "bc", target_column_type::NONE);
  }

  void test_storage_10() {
    // Yeah, a corner case
    run_storage_check_test(1, "bc", target_column_type::NONE);
  }

  void test_storage_11() {
    // One with just a lot of stuff
    run_storage_check_test(200, "u", target_column_type::NONE);
  }

  void test_storage_12() {
    // One with just a lot of stuff
    run_storage_check_test(200, "d", target_column_type::NONE);
  }

  void test_storage_13() {
    // One with just a lot of stuff
    run_storage_check_test(1000, "cnv", target_column_type::NONE);
  }

  void test_storage_14() {
    // One with just a lot of stuff
    run_storage_check_test(1000, "du", target_column_type::NONE);
  }

  void test_storage_15() {
    // One with just a lot of stuff
    run_storage_check_test(3, "UDccccV", target_column_type::NONE);
  }

  void test_storage_100() {
    // One with just a lot of stuff
    run_storage_check_test(10, "Zcuvd", target_column_type::NONE);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_storage_000_tn() {
    // All unique
    run_storage_check_test(0, "n", target_column_type::NUMERICAL);
  }

  void test_storage_0n_tn() {
    // All unique
    run_storage_check_test(5, "n", target_column_type::NUMERICAL);
  }

  void test_storage_0C_tn() {
    // All unique
    run_storage_check_test(5, "c", target_column_type::NUMERICAL);
  }

  void test_storage_1_unsorted_tn() {
    run_storage_check_test(5, "b", target_column_type::NUMERICAL);
  }

  void test_storage_0b_tn() {
    // All unique
    run_storage_check_test(13, "C", target_column_type::NUMERICAL);
  }

  void test_storage_1b_unsorted_tn() {
    run_storage_check_test(13, "b", target_column_type::NUMERICAL);
  }

  void test_storage_1_tn() {
    run_storage_check_test(13, "bc", target_column_type::NUMERICAL);
  }

  void test_storage_2_tn() {
    run_storage_check_test(13, "zc", target_column_type::NUMERICAL);
  }

  void test_storage_3_tn() {
    run_storage_check_test(100, "Zc", target_column_type::NUMERICAL);
  }

  void test_storage_4_tn() {
    // Pretty much gonna be unique
    run_storage_check_test(100, "Cc", target_column_type::NUMERICAL);
  }

  void test_storage_5_tn() {
    // 10 blocks of values.
    run_storage_check_test(1000, "Zc", target_column_type::NUMERICAL);
  }

  void test_storage_6_tn() {
    // two large blocks of values
    run_storage_check_test(1000, "bc", target_column_type::NUMERICAL);
  }

  void test_storage_10_tn() {
    // Yeah, a corner case
    run_storage_check_test(1, "bc", target_column_type::NUMERICAL);
  }

  void test_storage_11_tn() {
    // One with just a lot of stuff
    run_storage_check_test(200, "u", target_column_type::NUMERICAL);
  }

  void test_storage_12_tn() {
    // One with just a lot of stuff
    run_storage_check_test(200, "d", target_column_type::NUMERICAL);
  }

  void test_storage_13_tn() {
    // One with just a lot of stuff
    run_storage_check_test(1000, "cnv", target_column_type::NUMERICAL);
  }

  void test_storage_14_tn() {
    // One with just a lot of stuff
    run_storage_check_test(1000, "du", target_column_type::NUMERICAL);
  }

  void test_storage_15_tn() {
    // One with just a lot of stuff
    run_storage_check_test(3, "UDccccV", target_column_type::NUMERICAL);
  }

  void test_storage_100_tn() {
    // One with just a lot of stuff
    run_storage_check_test(10, "Zcuvd", target_column_type::NUMERICAL);
  }

  void test_storage_16_null_tn() {
    // two large blocks of values
    run_storage_check_test(1000, "", target_column_type::NUMERICAL);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_storage_000_tc() {
    // All unique
    run_storage_check_test(0, "n", target_column_type::CATEGORICAL);
  }

  void test_storage_0n_tc() {
    // All unique
    run_storage_check_test(5, "n", target_column_type::CATEGORICAL);
  }

  void test_storage_0C_tc() {
    // All unique
    run_storage_check_test(5, "c", target_column_type::CATEGORICAL);
  }

  void test_storage_1_unsorted_tc() {
    run_storage_check_test(5, "b", target_column_type::CATEGORICAL);
  }

  void test_storage_0b_tc() {
    // All unique
    run_storage_check_test(13, "C", target_column_type::CATEGORICAL);
  }

  void test_storage_1b_unsorted_tc() {
    run_storage_check_test(13, "b", target_column_type::CATEGORICAL);
  }

  void test_storage_1_tc() {
    run_storage_check_test(13, "bc", target_column_type::CATEGORICAL);
  }

  void test_storage_2_tc() {
    run_storage_check_test(13, "zc", target_column_type::CATEGORICAL);
  }

  void test_storage_3_tc() {
    run_storage_check_test(100, "Zc", target_column_type::CATEGORICAL);
  }

  void test_storage_4_tc() {
    // Pretty much gonna be unique
    run_storage_check_test(100, "Cc", target_column_type::CATEGORICAL);
  }

  void test_storage_5_tc() {
    // 10 blocks of values.
    run_storage_check_test(1000, "Zc", target_column_type::CATEGORICAL);
  }

  void test_storage_6_tc() {
    // two large blocks of values
    run_storage_check_test(1000, "bc", target_column_type::CATEGORICAL);
  }

  void test_storage_10_tc() {
    // Yeah, a corner case
    run_storage_check_test(1, "bc", target_column_type::CATEGORICAL);
  }

  void test_storage_11_tc() {
    // One with just a lot of stuff
    run_storage_check_test(200, "u", target_column_type::CATEGORICAL);
  }

  void test_storage_12_tc() {
    // One with just a lot of stuff
    run_storage_check_test(200, "d", target_column_type::CATEGORICAL);
  }

  void test_storage_13_tc() {
    // One with just a lot of stuff
    run_storage_check_test(1000, "cnv", target_column_type::CATEGORICAL);
  }

  void test_storage_14_tc() {
    // One with just a lot of stuff
    run_storage_check_test(1000, "du", target_column_type::CATEGORICAL);
  }

  void test_storage_15_tc() {
    // One with just a lot of stuff
    run_storage_check_test(3, "UDccccV", target_column_type::CATEGORICAL);
  }

  void test_storage_100_tc() {
    // One with just a lot of stuff
    run_storage_check_test(10, "Zcuvd", target_column_type::CATEGORICAL);
  }

  void test_storage_16_null_tc() {
    // two large blocks of values
    run_storage_check_test(1000, "", target_column_type::CATEGORICAL);
  }

};

BOOST_FIXTURE_TEST_SUITE(_test_basic_storage, test_basic_storage)
BOOST_AUTO_TEST_CASE(test_storage_000) {
  test_basic_storage::test_storage_000();
}
BOOST_AUTO_TEST_CASE(test_storage_0n) {
  test_basic_storage::test_storage_0n();
}
BOOST_AUTO_TEST_CASE(test_storage_0b) {
  test_basic_storage::test_storage_0b();
}
BOOST_AUTO_TEST_CASE(test_storage_0c) {
  test_basic_storage::test_storage_0c();
}
BOOST_AUTO_TEST_CASE(test_storage_0C) {
  test_basic_storage::test_storage_0C();
}
BOOST_AUTO_TEST_CASE(test_storage_1b_unsorted) {
  test_basic_storage::test_storage_1b_unsorted();
}
BOOST_AUTO_TEST_CASE(test_storage_1) {
  test_basic_storage::test_storage_1();
}
BOOST_AUTO_TEST_CASE(test_storage_2) {
  test_basic_storage::test_storage_2();
}
BOOST_AUTO_TEST_CASE(test_storage_3) {
  test_basic_storage::test_storage_3();
}
BOOST_AUTO_TEST_CASE(test_storage_4) {
  test_basic_storage::test_storage_4();
}
BOOST_AUTO_TEST_CASE(test_storage_5) {
  test_basic_storage::test_storage_5();
}
BOOST_AUTO_TEST_CASE(test_storage_6) {
  test_basic_storage::test_storage_6();
}
BOOST_AUTO_TEST_CASE(test_storage_10) {
  test_basic_storage::test_storage_10();
}
BOOST_AUTO_TEST_CASE(test_storage_11) {
  test_basic_storage::test_storage_11();
}
BOOST_AUTO_TEST_CASE(test_storage_12) {
  test_basic_storage::test_storage_12();
}
BOOST_AUTO_TEST_CASE(test_storage_13) {
  test_basic_storage::test_storage_13();
}
BOOST_AUTO_TEST_CASE(test_storage_14) {
  test_basic_storage::test_storage_14();
}
BOOST_AUTO_TEST_CASE(test_storage_15) {
  test_basic_storage::test_storage_15();
}
BOOST_AUTO_TEST_CASE(test_storage_100) {
  test_basic_storage::test_storage_100();
}
BOOST_AUTO_TEST_CASE(test_storage_000_tn) {
  test_basic_storage::test_storage_000_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_0n_tn) {
  test_basic_storage::test_storage_0n_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_0C_tn) {
  test_basic_storage::test_storage_0C_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_1_unsorted_tn) {
  test_basic_storage::test_storage_1_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_0b_tn) {
  test_basic_storage::test_storage_0b_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_1b_unsorted_tn) {
  test_basic_storage::test_storage_1b_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_1_tn) {
  test_basic_storage::test_storage_1_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_2_tn) {
  test_basic_storage::test_storage_2_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_3_tn) {
  test_basic_storage::test_storage_3_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_4_tn) {
  test_basic_storage::test_storage_4_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_5_tn) {
  test_basic_storage::test_storage_5_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_6_tn) {
  test_basic_storage::test_storage_6_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_10_tn) {
  test_basic_storage::test_storage_10_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_11_tn) {
  test_basic_storage::test_storage_11_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_12_tn) {
  test_basic_storage::test_storage_12_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_13_tn) {
  test_basic_storage::test_storage_13_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_14_tn) {
  test_basic_storage::test_storage_14_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_15_tn) {
  test_basic_storage::test_storage_15_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_100_tn) {
  test_basic_storage::test_storage_100_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_16_null_tn) {
  test_basic_storage::test_storage_16_null_tn();
}
BOOST_AUTO_TEST_CASE(test_storage_000_tc) {
  test_basic_storage::test_storage_000_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_0n_tc) {
  test_basic_storage::test_storage_0n_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_0C_tc) {
  test_basic_storage::test_storage_0C_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_1_unsorted_tc) {
  test_basic_storage::test_storage_1_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_0b_tc) {
  test_basic_storage::test_storage_0b_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_1b_unsorted_tc) {
  test_basic_storage::test_storage_1b_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_1_tc) {
  test_basic_storage::test_storage_1_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_2_tc) {
  test_basic_storage::test_storage_2_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_3_tc) {
  test_basic_storage::test_storage_3_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_4_tc) {
  test_basic_storage::test_storage_4_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_5_tc) {
  test_basic_storage::test_storage_5_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_6_tc) {
  test_basic_storage::test_storage_6_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_10_tc) {
  test_basic_storage::test_storage_10_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_11_tc) {
  test_basic_storage::test_storage_11_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_12_tc) {
  test_basic_storage::test_storage_12_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_13_tc) {
  test_basic_storage::test_storage_13_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_14_tc) {
  test_basic_storage::test_storage_14_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_15_tc) {
  test_basic_storage::test_storage_15_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_100_tc) {
  test_basic_storage::test_storage_100_tc();
}
BOOST_AUTO_TEST_CASE(test_storage_16_null_tc) {
  test_basic_storage::test_storage_16_null_tc();
}
BOOST_AUTO_TEST_SUITE_END()

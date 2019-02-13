#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <util/cityhash_gl.hpp>
#include <cxxtest/TestSuite.h>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <unity/lib/flex_dict_view.hpp>
#include <random/random.hpp>

// ML-Data Utils
#include <ml_data/ml_data.hpp>
#include <unity/dml/distributed_ml_data.hpp>

// Testing utils common to all of ml_data_iterator
#include <util/testing_utils.hpp>
#include <ml_data/testing_utils.hpp>
#include <sframe/testing_utils.hpp>

#include <globals/globals.hpp>

#include <string>
#include <iostream>
#include <distributed/distributed_context.hpp>
#include <rpc/dc_global.hpp>
#include <rpc/dc.hpp>

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;

using namespace turi;

enum class target_column_type {NONE, NUMERICAL, CATEGORICAL};

void run_reconcile_test(size_t n, const std::string& run_string,
                        target_column_type target_type, bool categorical_is_sorted) {

  auto dc_ptr = distributed_control_global::get_instance();

  random::seed(dc_ptr->procid());

  // Just so they don't all have the same number of entries
  n += random::fast_uniform<size_t>(0, std::max<size_t>(0, n / 2));

  globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);
  globals::set_global("TURI_ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD", 7);

  bool target_column;

  if(target_type == target_column_type::CATEGORICAL) {
    target_column = true;
  } else if(target_type == target_column_type::NUMERICAL) {
    target_column = true;
  } else {
    target_column = false;
  }

  std::string print_str = run_string;

  if(target_column)
    print_str += ":target";

  bool target_column_categorical = (target_type == target_column_type::CATEGORICAL);

  sframe raw_data;
  ml_data data;
  std::vector<std::vector<flexible_type> > ref_data;

  std::tie(raw_data, data) = make_random_sframe_and_ml_data(
      n, run_string, target_column, target_column_categorical);

  ref_data = testing_extract_sframe_data(raw_data);

  std::vector<std::string> sorted_columns;

  if(categorical_is_sorted) {
    for(size_t i = 0; i < data.metadata()->num_columns(); ++i) {
      if(data.metadata()->column_mode(i) == ml_column_mode::CATEGORICAL) {
        sorted_columns.push_back(data.metadata()->column_name(i));
      }
    }

    if(target_type == target_column_type::CATEGORICAL) {
      sorted_columns.push_back(data.metadata()->target_column_name());
    }
  }

  // The big deal function.
  reconcile_distributed_ml_data(data, sorted_columns);

  if(categorical_is_sorted) {
    for(size_t i = 0; i < data.metadata()->num_columns(); ++i) {
      if(data.metadata()->column_mode(i) == ml_column_mode::CATEGORICAL) {
        auto m = data.metadata()->indexer(i);
        for(size_t i = 0; i + 1 < m->indexed_column_size(); ++i) {
          ASSERT_TRUE(!(m->map_index_to_value(i) > m->map_index_to_value(i+1)));
        }
      }
      if(target_type == target_column_type::CATEGORICAL) {
        auto m = data.metadata()->target_indexer();
        for(size_t i = 0; i + 1 < m->indexed_column_size(); ++i) {
          ASSERT_TRUE(!(m->map_index_to_value(i) > m->map_index_to_value(i+1)));
        }
      }
    }
  }

  // Now, just go through and make sure that everything works.

  std::vector<ml_data_entry> x;
  DenseVector xd;
  Eigen::MatrixXd xdr;
  SparseVector xs;
  std::vector<ml_data_entry_global_index> x_gi;
  std::vector<flexible_type> row_x;

  xd.resize(data.metadata()->num_dimensions());
  xs.resize(data.metadata()->num_dimensions());

  xdr.resize(3, data.metadata()->num_dimensions());
  xdr.setZero();

  ////////////////////////////////////////////////////////////////////////////////
  // Run the actual tests

  for(auto it = data.get_iterator(); !it.done(); ++it) {

    size_t it_idx = it.row_index();

    for(size_t type_idx : {0, 1, 2, 3, 4} ) {

      switch(type_idx) {
        case 0: {
          it->fill(x);

          // std::cerr << "x = " << x << std::endl;
          row_x = translate_row_to_original(data.metadata(), x);
          break;
        }
        case 1: {
          it->fill(xd);

          row_x = translate_row_to_original(data.metadata(), xd);
          // std::cerr << "xd = " << xd.transpose() << std::endl;
          break;
        }
        case 2: {
          it->fill(xs);

          row_x = translate_row_to_original(data.metadata(), xs);
          break;
        }
        case 3: {
          it->fill(x_gi);

          row_x = translate_row_to_original(data.metadata(), x_gi);
          break;
        }
        case 4: {
          it->fill(xdr.row(1));

          xd = xdr.row(1);

          row_x = translate_row_to_original(data.metadata(), xd);
          break;
        }
      }

      ASSERT_EQ(row_x.size(), run_string.size());

      if(target_column && target_type == target_column_type::NUMERICAL) {
        row_x.push_back(flex_int(it->target_value()));
      } else if(target_column && target_type == target_column_type::CATEGORICAL) {
        row_x.push_back(data.metadata()->target_indexer()->map_index_to_value(it->target_index()));
      }

      ASSERT_EQ(row_x.size(), raw_data.num_columns());

      ASSERT_EQ(row_x.size(), ref_data.at(it_idx).size());
      for(size_t ri = 0; ri < row_x.size(); ++ri) {
        ASSERT_TRUE(ml_testing_equals(row_x.at(ri), ref_data.at(it_idx).at(ri)));
      }
    }
  }
}

#define CREATE_DISTRIBUTED_TEST(n, run_str, target, cat_sorted)         \
  void test_distributed_ml_data_##n##_##run_str##_##target##_withsort##cat_sorted() { \
    std::cerr << "RUNNING: n=" << #n << "; run_string = " << #run_str   \
              << "; target=" << #target << "; with sorting = " << #cat_sorted << std::endl; \
    auto& ctx = get_distributed_context();                              \
    ctx.distributed_exec(run_reconcile_test, n, #run_str, target_column_type::target, cat_sorted); \
  }

extern "C" {
  CREATE_DISTRIBUTED_TEST(5, n, NONE, false);
  CREATE_DISTRIBUTED_TEST(5, b, NONE, false);
  CREATE_DISTRIBUTED_TEST(5, c, NONE, false);
  CREATE_DISTRIBUTED_TEST(5, C, NONE, false);
  CREATE_DISTRIBUTED_TEST(13, b, NONE, false);
  CREATE_DISTRIBUTED_TEST(13, bc, NONE, false);
  CREATE_DISTRIBUTED_TEST(13, zc, NONE, false);
  CREATE_DISTRIBUTED_TEST(30, C, NONE, false);
  CREATE_DISTRIBUTED_TEST(3000, C, NONE, false);
  CREATE_DISTRIBUTED_TEST(100, Zc, NONE, false);
  CREATE_DISTRIBUTED_TEST(100, Cc, NONE, false);
  CREATE_DISTRIBUTED_TEST(1000, Zc, NONE, false);
  CREATE_DISTRIBUTED_TEST(1000, bc, NONE, false);
  CREATE_DISTRIBUTED_TEST(1, bc, NONE, false);
  CREATE_DISTRIBUTED_TEST(200, u, NONE, false);
  CREATE_DISTRIBUTED_TEST(200, d, NONE, false);
  CREATE_DISTRIBUTED_TEST(1000, cnv, NONE, false);
  CREATE_DISTRIBUTED_TEST(1000, du, NONE, false);
  CREATE_DISTRIBUTED_TEST(3, UDccccV, NONE, false);
  CREATE_DISTRIBUTED_TEST(10, Zcuvd, NONE, false);
  CREATE_DISTRIBUTED_TEST(0, n, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(5, n, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(5, c, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(5, b, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(13, C, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(13, b, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(13, bc, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(13, zc, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(30, C, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(100, Zc, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(100, Cc, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, Zc, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, bc, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(1, bc, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(200, u, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(200, d, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, cnv, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, du, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(3, UDccccV, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(10, Zcuvd, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, n, NUMERICAL, false);
  CREATE_DISTRIBUTED_TEST(0, n, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(5, n, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(5, c, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(5, b, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(13, C, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(13, b, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(13, bc, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(13, zc, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(30, C, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(100, Zc, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(100, Cc, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, Zc, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, bc, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(1, bc, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(200, u, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(200, d, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, cnv, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, du, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(3, UDccccV, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(10, Zcuvd, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(1000, n, CATEGORICAL, false);
  CREATE_DISTRIBUTED_TEST(5, n, NONE, true);
  CREATE_DISTRIBUTED_TEST(5, b, NONE, true);
  CREATE_DISTRIBUTED_TEST(5, c, NONE, true);
  CREATE_DISTRIBUTED_TEST(5, C, NONE, true);
  CREATE_DISTRIBUTED_TEST(13, b, NONE, true);
  CREATE_DISTRIBUTED_TEST(13, bc, NONE, true);
  CREATE_DISTRIBUTED_TEST(13, zc, NONE, true);
  CREATE_DISTRIBUTED_TEST(30, C, NONE, true);
  CREATE_DISTRIBUTED_TEST(3000, C, NONE, true);
  CREATE_DISTRIBUTED_TEST(100, Zc, NONE, true);
  CREATE_DISTRIBUTED_TEST(100, Cc, NONE, true);
  CREATE_DISTRIBUTED_TEST(1000, Zc, NONE, true);
  CREATE_DISTRIBUTED_TEST(1000, bc, NONE, true);
  CREATE_DISTRIBUTED_TEST(1, bc, NONE, true);
  CREATE_DISTRIBUTED_TEST(200, u, NONE, true);
  CREATE_DISTRIBUTED_TEST(200, d, NONE, true);
  CREATE_DISTRIBUTED_TEST(1000, cnv, NONE, true);
  CREATE_DISTRIBUTED_TEST(1000, du, NONE, true);
  CREATE_DISTRIBUTED_TEST(3, UDccccV, NONE, true);
  CREATE_DISTRIBUTED_TEST(10, Zcuvd, NONE, true);
  CREATE_DISTRIBUTED_TEST(0, n, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(5, n, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(5, c, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(5, b, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(13, C, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(13, b, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(13, bc, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(13, zc, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(30, C, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(100, Zc, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(100, Cc, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, Zc, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, bc, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(1, bc, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(200, u, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(200, d, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, cnv, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, du, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(3, UDccccV, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(10, Zcuvd, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, n, NUMERICAL, true);
  CREATE_DISTRIBUTED_TEST(0, n, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(5, n, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(5, c, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(5, b, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(13, C, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(13, b, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(13, bc, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(13, zc, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(30, C, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(100, Zc, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(100, Cc, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, Zc, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, bc, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(1, bc, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(200, u, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(200, d, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, cnv, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, du, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(3, UDccccV, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(10, Zcuvd, CATEGORICAL, true);
  CREATE_DISTRIBUTED_TEST(1000, n, CATEGORICAL, true);
}

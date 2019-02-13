
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <algorithm>
#include <util/cityhash_tc.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <unity/lib/flex_dict_view.hpp>

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


using namespace turi;
struct ml_data_numeric_iteration_test  {

  // Answers
  std::vector<std::vector<flexible_type> > raw_x;
  std::vector<std::vector<flexible_type> > raw_y;
  sframe X;
  sframe y;
  v2::ml_data data;
  v2::ml_data saved_data;
  std::shared_ptr<v2::ml_metadata> metadata;
  std::vector<std::vector<std::vector<double>>> ml_data_entry_x;
  std::vector<std::vector<double>> dense_vector_x;
  std::vector<std::vector<double>> dense_vector_reference_x;
  size_t total_size;
  size_t total_size_reference;
  std::vector<size_t> index;


 public:

  ml_data_iterator_test() {

    // Int-Double dictionary
    std::vector<std::vector<std::vector<flexible_type>>> raw_int_dbl = \
        {{{0,2.0}},
         {{1,1.0}},
         {{2,1.0}},
         {{2,1.0}, {3,2.0}},
         {{4,1.0}, {5,2.0}},
         {{6,1.0}, {7,2.0}},
         {{2,1.0}, {3,2.0}},
         {{2,2.0}, {5,1.0}},
         {{1,1.0}, {2,2.0}, {0,2.0}},
         {{1,1.0}, {3,2.0}, {0,2.0}}};

    std::vector<std::vector<std::pair<flexible_type,flexible_type>>> int_dbl;
    std::vector<std::vector<std::pair<flexible_type,flexible_type>>> str_dbl;
    for (const auto& row : raw_int_dbl){
      std::vector<std::pair<flexible_type,flexible_type>> m;
      std::vector<std::pair<flexible_type,flexible_type>> str_m;
      for (const auto& entry : row){
        m.push_back(std::make_pair(entry[0],entry[1]));
        str_m.push_back(std::make_pair(std::to_string((size_t)entry[0]),entry[1]));
      }
      int_dbl.push_back(m);
      str_dbl.push_back(str_m);
    }

    // Step 1: Make the raw data.
    // ---------------------------------------------------------------------
    std::vector<std::vector<flexible_type> > raw_x = \
        {{"0",10,10.0, {1.0,10.1}, int_dbl[0], str_dbl[0]},
         {"1",11,21.0, {1.1,21.1}, int_dbl[1], str_dbl[1]},
         {"2",22,22.0, {2.2,22.1}, int_dbl[2], str_dbl[2]},
         {"0",33,23.0, {3.3,23.1}, int_dbl[3], str_dbl[3]},
         {"1",44,24.0, {4.4,24.1}, int_dbl[4], str_dbl[4]},
         {"2",55,25.0, {5.5,25.1}, int_dbl[5], str_dbl[5]},
         {"0",26,26.0, {2.6,26.1}, int_dbl[6], str_dbl[6]},
         {"1",27,27.0, {2.7,27.1}, int_dbl[7], str_dbl[7]},
         {"2",28,28.0, {2.8,28.1}, int_dbl[8], str_dbl[8]},
         {"3",39,49.0, {3.9,49.1}, int_dbl[9], str_dbl[9]} };

    std::vector<size_t> index = {4,1,1,2,8,8};

    std::vector<std::vector<flexible_type> > raw_y = \
        {{0},
         {1},
         {2},
         {3},
         {4},
         {5},
         {2},
         {2},
         {2},
         {3} };

    std::vector<std::vector<std::vector<double>>> ml_data_entry_x = \
        {{{0,0,1.0} , {1,0,10.0}, {2,0,10.0}, {3,0,1.0}, {3,1,10.1},
          {4,0,2.0}, {5,0,2.0}},
         {{0,1,1.0} , {1,0,11.0}, {2,0,21.0}, {3,0,1.1}, {3,1,21.1},
          {4,1,1.0}, {5,1,1.0}},
         {{0,2,1.0} , {1,0,22.0}, {2,0,22.0}, {3,0,2.2}, {3,1,22.1},
          {4,2,1.0}, {5,2,1.0}},
         {{0,0,1.0} , {1,0,33.0}, {2,0,23.0}, {3,0,3.3}, {3,1,23.1},
          {4,2,1.0}, {4,3,2.0}, {5,2,1.0}, {5,3,2.0}},
         {{0,1,1.0} , {1,0,44.0}, {2,0,24.0}, {3,0,4.4}, {3,1,24.1},
          {4,4,1.0}, {4,5,2.0}, {5,4,1.0}, {5,5,2.0}},
         {{0,2,1.0} , {1,0,55.0}, {2,0,25.0}, {3,0,5.5}, {3,1,25.1},
          {4,6,1.0}, {4,7,2.0}, {5,6,1.0}, {5,7,2.0}},
         {{0,0,1.0} , {1,0,26.0}, {2,0,26.0}, {3,0,2.6}, {3,1,26.1},
          {4,2,1.0}, {4,3,2.0}, {5,2,1.0}, {5,3,2.0}},
         {{0,1,1.0} , {1,0,27.0}, {2,0,27.0}, {3,0,2.7}, {3,1,27.1},
          {4,2,2.0}, {4,5,1.0},
          {5,2,2.0}, {5,5,1.0}},
         {{0,2,1.0} , {1,0,28.0}, {2,0,28.0}, {3,0,2.8}, {3,1,28.1},
          {4,0,2.0}, {4,1,1.0}, {4,2,2.0},
          {5,0,2.0}, {5,1,1.0}, {5,2,2.0} },
         {{0,3,1.0} , {1,0,39.0}, {2,0,49.0}, {3,0,3.9}, {3,1,49.1},
          {4,0,2.0}, {4,1,1.0}, {4,3,2.0},
          {5,0,2.0}, {5,1,1.0}, {5,3,2.0}}};

    std::vector<std::vector<double>> dense_vector_reference_x = \
        {{0.0, 0.0, 0.0, 10.0, 10.0, 1.0, 10.1, 2.0, 0.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
         {1.0, 0.0, 0.0, 11.0, 21.0, 1.1, 21.1, 0.0, 1.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
         {0.0, 1.0, 0.0, 22.0, 22.0, 2.2, 22.1, 0.0, 0.0, 1.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
         {0.0, 0.0, 0.0, 33.0, 23.0, 3.3, 23.1, 0.0, 0.0, 1.0, 2.0, 0.0,
          0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0},
         {1.0, 0.0, 0.0, 44.0, 24.0, 4.4, 24.1, 0.0, 0.0, 0.0, 0.0, 1.0,
          2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0},
         {0.0, 1.0, 0.0, 55.0, 25.0, 5.5, 25.1, 0.0, 0.0, 0.0, 0.0, 0.0,
          0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0},
         {0.0, 0.0, 0.0, 26.0, 26.0, 2.6, 26.1, 0.0, 0.0, 1.0, 2.0, 0.0,
          0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0},
         {1.0, 0.0, 0.0, 27.0, 27.0, 2.7, 27.1, 0.0, 0.0, 2.0, 0.0, 0.0,
          1.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 1.0, 0.0, 0.0},
         {0.0, 1.0, 0.0, 28.0, 28.0, 2.8, 28.1, 2.0, 1.0, 2.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 2.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0},
         {0.0, 0.0, 1.0, 39.0, 49.0, 3.9, 49.1, 2.0, 1.0, 0.0, 2.0, 0.0,
          0.0, 0.0, 0.0, 2.0, 1.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0}};

    std::vector<std::vector<double>> dense_vector_x = \
        {{1.0, 0.0, 0.0, 0.0, 10.0, 10.0, 1.0, 10.1, 2.0, 0.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
         {0.0, 1.0, 0.0, 0.0, 11.0, 21.0, 1.1, 21.1, 0.0, 1.0, 0.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
         {0.0, 0.0, 1.0, 0.0, 22.0, 22.0, 2.2, 22.1, 0.0, 0.0, 1.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
         {1.0, 0.0, 0.0, 0.0, 33.0, 23.0, 3.3, 23.1, 0.0, 0.0, 1.0, 2.0, 0.0,
          0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0},
         {0.0, 1.0, 0.0, 0.0, 44.0, 24.0, 4.4, 24.1, 0.0, 0.0, 0.0, 0.0, 1.0,
          2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0},
         {0.0, 0.0, 1.0, 0.0, 55.0, 25.0, 5.5, 25.1, 0.0, 0.0, 0.0, 0.0, 0.0,
          0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0},
         {1.0, 0.0, 0.0, 0.0, 26.0, 26.0, 2.6, 26.1, 0.0, 0.0, 1.0, 2.0, 0.0,
          0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0},
         {0.0, 1.0, 0.0, 0.0, 27.0, 27.0, 2.7, 27.1, 0.0, 0.0, 2.0, 0.0, 0.0,
          1.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 1.0, 0.0, 0.0},
         {0.0, 0.0, 1.0, 0.0, 28.0, 28.0, 2.8, 28.1, 2.0, 1.0, 2.0, 0.0, 0.0,
          0.0, 0.0, 0.0, 2.0, 1.0, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0},
         {0.0, 0.0, 0.0, 1.0, 39.0, 49.0, 3.9, 49.1, 2.0, 1.0, 0.0, 2.0, 0.0,
          0.0, 0.0, 0.0, 2.0, 1.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0}};

    sframe X = make_testing_sframe(
        {"string","int","float","vector","int-dbl-dict","str-dbl-dict"},
        {flex_type_enum::STRING,
              flex_type_enum::INTEGER,
              flex_type_enum::FLOAT,
              flex_type_enum::VECTOR,
              flex_type_enum::DICT,
              flex_type_enum::DICT},
        raw_x);

    sframe y = make_testing_sframe(
        {"response"},
        {flex_type_enum::FLOAT},
        raw_y);

    // Step 2: Convert to ML-Data
    // ---------------------------------------------------------------------

    v2::ml_data data;
    data.set_data(X, y);
    data.fill();

    this->data = data;
    this->metadata = data.metadata();
    this->total_size = 24;
    this->total_size_reference = 23;
    this->X = X;
    this->y = y;
    this->index = index;
    this->raw_y = raw_y;
    this->raw_x = raw_x;
    this->ml_data_entry_x = ml_data_entry_x;
    this->dense_vector_x = dense_vector_x;
    this->dense_vector_reference_x = dense_vector_reference_x;
  }

  /**
   * Make sure row_index() is correct.
   */
  void test_row_index(){
    size_t idx=0;
    for(auto it = data.get_iterator(); !it.done(); ++it){
      TS_ASSERT_EQUALS(idx,it.row_index());
      idx++;
    }
  }

  // /**
  //  * Make sure size() is correct.
  //  */
  //  void test_total_size(){
  //    ml_data_iterator_initializer it_init(data);
  //    ml_data_iterator it(data);
  //    TS_ASSERT_EQUALS(total_size,data.size());
  //  }

  //  /**
  //  * Make sure size() is correct.
  //  */
  //  void test_total_size_reference(){
  //    ml_data_iterator_initializer it_init(data);
  //    ml_data_iterator it(data);
  //    TS_ASSERT_EQUALS(total_size_reference,data.size_reference());
  //  }

  /**
   * Target Value
   */
  void test_target_value(){
    for(size_t num_threads : {1, 3, 8}) {
      size_t idx=0;
      for(size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {

        for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it){
          TS_ASSERT_EQUALS((double)raw_y[idx][0],it.target_value());
          idx++;
        }
      }
    }
  }


  /**
   * Fill observation (ml_data_entry)
   */
  void test_fill_observation_ml_data_entry(){
    for(size_t num_threads : {1, 3, 8}) {
      size_t idx=0;
      for(size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {

        for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it){

          std::vector<v2::ml_data_entry> x;
          it.fill_observation(x);
          TS_ASSERT_EQUALS(x.size(),ml_data_entry_x[idx].size());
          for(size_t cid=0; cid < x.size(); cid++){
            ASSERT_EQ(size_t(ml_data_entry_x[idx][cid][0]),x[cid].column_index);
            ASSERT_EQ(size_t(ml_data_entry_x[idx][cid][1]),x[cid].index);
            ASSERT_EQ(size_t(ml_data_entry_x[idx][cid][2]),x[cid].value);
            // TS_ASSERT_EQUALS(ml_data_entry_x[idx][cid][0],x[cid].column_index);
            // TS_ASSERT_EQUALS(ml_data_entry_x[idx][cid][1],x[cid].index);
            // TS_ASSERT_EQUALS(ml_data_entry_x[idx][cid][2],x[cid].value);
          }
          idx++;
        }
      }
    }
  }

  /**
   * Fill observation (dense_vector)
   */
  void test_fill_observation_dense_vector(){
    DenseVector x(total_size);

    for(size_t num_threads : {1, 3, 8}) {
      size_t idx=0;
      for(size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {

        for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it){

          it.fill_observation(x);
          Eigen::Map<DenseVector> vec(&dense_vector_x[idx][0], total_size);

          for(int i = 0; i < vec.size(); ++i)
            ASSERT_EQ(vec[i], x[i]);

          idx++;
        }
      }
    }
  }

  /**
   * Fill observation (sparse_vector)
   */
  void test_fill_observation_sparse_vector(){
    SparseVector x(total_size);
    SparseVector vec(total_size);
    for(size_t num_threads : {1, 3, 8}) {
      size_t idx=0;
      for(size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {

        for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it){
          it.fill_observation(x);
          Eigen::Map<DenseVector> dense_vec(&dense_vector_x[idx][0],total_size);
          vec = dense_vec.sparseView(0,0);
          TS_ASSERT(vec.isApprox(x));
          idx++;
        }
      }
    }
  }

  /**
   * Fill observation (dense_vector) with reference categories.
   */
  void test_fill_observation_reference_dense_vector(){
    DenseVector x(total_size_reference);
    DenseVector vec(total_size_reference);
    for(size_t num_threads : {1, 3, 8}) {
      size_t idx=0;
      for(size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {

        for(auto it = data.get_iterator(thread_idx, num_threads, false, true); !it.done(); ++it){
          it.fill_observation(x);
          Eigen::Map<DenseVector>
              vec(&dense_vector_reference_x[idx][0],total_size_reference);
          TS_ASSERT(vec.isApprox(x));
          idx++;
        }
      }
    }
  }

  /**
   * Fill observation (sparse_vector) with reference categories.
   */
  void test_fill_observation_reference_sparse_vector(){
    SparseVector x(total_size_reference);
    SparseVector vec(total_size_reference);
    for(size_t num_threads : {1, 3, 8}) {
      size_t idx=0;
      for(size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {

        for(auto it = data.get_iterator(thread_idx, num_threads, false, true); !it.done(); ++it){
          it.fill_observation(x);
          Eigen::Map<DenseVector>
              dense_vec(&dense_vector_reference_x[idx][0],total_size_reference);
          vec = dense_vec.sparseView(0,0);
          TS_ASSERT(vec.isApprox(x));
          idx++;
        }
      }
    }
  }

  // Big test to make sure all ordering is fine
  void test_large_ordering() {

    size_t n = 3235;

    std::vector<std::vector<flexible_type> > data_v(n);

    for(size_t i = 0; i < n; ++i) {
      flex_vec vv(3);
      for(size_t j = 0; j < 3; ++j)
        vv[j] = i + j;

      std::sort(vv.begin(), vv.end());

      size_t s1 = hash64(i) % 10;
      flex_list vr(s1);
      for(size_t j = 0; j < s1; ++j)
        vr[j] = std::to_string((i % 100) + j);

      std::sort(vr.begin(), vr.end());

      size_t s2 = hash64(i) % 5;
      flex_dict dv(s2);
      for(size_t j = 0; j < s2; ++j)
        dv[j] = {std::to_string(j), j};

      std::sort(dv.begin(), dv.end());

      data_v[i] = {int(i), vv, vr, dv};
    }

    sframe raw_data = make_testing_sframe({"id", "vec", "rec", "dict"}, data_v);

    v2::ml_data data;
    data.fill(raw_data);

    random::seed(0);

    // Make sure that iterator retrieves everything, and in the correct order

    for(size_t i = 0; i < 20; ++i) {

      std::vector<int> seen_value(n);

      size_t row_start = random::fast_uniform<size_t>(0, n-1);
      size_t row_end = random::fast_uniform<size_t>(0, n-1);

      if(row_start > row_end) {
        if(i % 2 == 0)
          row_start = 0;
        else
          row_end = -1;
      }

      for(size_t num_threads : {1, 5, 13, 37}) {

        seen_value.assign(n, 0);

        size_t current_row = row_start;

        v2::ml_data sliced_data = data.slice(row_start, row_end);

        for(size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {

          for(auto it = sliced_data.get_iterator(thread_idx, num_threads);
              !it.done(); ++it, ++current_row) {

            ASSERT_EQ(current_row, it.row_index());
            ASSERT_EQ(seen_value[current_row], 0);
            seen_value[current_row] = 1;

            std::vector<flexible_type> row = it._testing_extract_current_row();

            ASSERT_TRUE(data_v[current_row][0].get_type() == flex_type_enum::INTEGER);
            ASSERT_TRUE(data_v[current_row][1].get_type() == flex_type_enum::VECTOR);
            ASSERT_TRUE(data_v[current_row][2].get_type() == flex_type_enum::LIST);
            ASSERT_TRUE(data_v[current_row][3].get_type() == flex_type_enum::DICT);

            ASSERT_TRUE(row[0].get_type() == flex_type_enum::FLOAT);
            ASSERT_TRUE(row[1].get_type() == flex_type_enum::VECTOR);
            ASSERT_TRUE(row[2].get_type() == flex_type_enum::LIST);
            ASSERT_TRUE(row[3].get_type() == flex_type_enum::DICT);

            ASSERT_TRUE(row[0] == data_v[current_row][0]);
            ASSERT_TRUE(row[1] == data_v[current_row][1]);

            flex_list e2 = row[2].get<flex_list>();
            std::sort(e2.begin(), e2.end());

            flex_list e2_ref = data_v[current_row][2];

            ASSERT_TRUE(e2 == e2_ref);


            flex_dict e3 = row[3].get<flex_dict>();

            std::sort(e3.begin(), e3.end());

            flex_dict e3_ref = data_v[current_row][3].get<flex_dict>();

            ASSERT_TRUE(e3 == e3_ref);
          }
        }

        ASSERT_EQ(current_row, std::min(row_end, data.size()));

        for(size_t i = 0; i < n; ++i)
          ASSERT_EQ(seen_value[i], (row_start <= i && i < row_end) ? 1 : 0);
      }
    }
  }

  // Big test to make sure all ordering is fine
  void test_large_ordering_in_parallel() {

    size_t n = 3235;

    std::vector<std::vector<flexible_type> > data_v(n);

    for(size_t i = 0; i < n; ++i) {
      flex_vec vv(3);
      for(size_t j = 0; j < 3; ++j)
        vv[j] = i + j;

      std::sort(vv.begin(), vv.end());

      size_t s1 = hash64(i) % 10;
      flex_list vr(s1);
      for(size_t j = 0; j < s1; ++j)
        vr[j] = std::to_string((i % 100) + j);

      std::sort(vr.begin(), vr.end());

      size_t s2 = hash64(i) % 5;
      flex_dict dv(s2);
      for(size_t j = 0; j < s2; ++j)
        dv[j] = {std::to_string(j), j};

      std::sort(dv.begin(), dv.end());

      data_v[i] = {int(i), vv, vr, dv};
    }

    sframe raw_data = make_testing_sframe({"id", "vec", "rec", "dict"}, data_v);

    v2::ml_data data;
    data.fill(raw_data);

    random::seed(0);

    // Make sure that iterator retrieves everything, and in the correct order

    for(size_t i = 0; i < 100; ++i) {

      std::vector<int> seen_value(n);

      size_t row_start = random::fast_uniform<size_t>(0, n-1);
      size_t row_end = random::fast_uniform<size_t>(0, n-1);

      if(row_start > row_end) {
        if(i % 2 == 0)
          row_start = 0;
        else
          row_end = -1;
      }

      seen_value.assign(n, 0);

      v2::ml_data sliced_data = data.slice(row_start, row_end);

      in_parallel([&](size_t thread_idx, size_t num_threads) {

          for(auto it = sliced_data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
            size_t current_row = it.row_index();
            ASSERT_EQ(seen_value[current_row], 0);
            seen_value[current_row] = 1;

            std::vector<flexible_type> row = it._testing_extract_current_row();

            ASSERT_TRUE(data_v[current_row][0].get_type() == flex_type_enum::INTEGER);
            ASSERT_TRUE(data_v[current_row][1].get_type() == flex_type_enum::VECTOR);
            ASSERT_TRUE(data_v[current_row][2].get_type() == flex_type_enum::LIST);
            ASSERT_TRUE(data_v[current_row][3].get_type() == flex_type_enum::DICT);

            ASSERT_TRUE(row[0].get_type() == flex_type_enum::FLOAT);
            ASSERT_TRUE(row[1].get_type() == flex_type_enum::VECTOR);
            ASSERT_TRUE(row[2].get_type() == flex_type_enum::LIST);
            ASSERT_TRUE(row[3].get_type() == flex_type_enum::DICT);

            ASSERT_TRUE(row[0] == data_v[current_row][0]);
            ASSERT_TRUE(row[1] == data_v[current_row][1]);

            flex_list e2 = row[2].get<flex_list>();
            std::sort(e2.begin(), e2.end());

            flex_list e2_ref = data_v[current_row][2];

            ASSERT_TRUE(e2 == e2_ref);

            flex_dict e3 = row[3].get<flex_dict>();

            std::sort(e3.begin(), e3.end());

            flex_dict e3_ref = data_v[current_row][3].get<flex_dict>();

            ASSERT_TRUE(e3 == e3_ref);
          }
        });

      for(size_t i = 0; i < n; ++i)
        ASSERT_EQ(seen_value[i], (row_start <= i && i < row_end) ? 1 : 0);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Test categorical targets
  void test_categorical_targets() {

    std::vector<std::vector<flexible_type> > raw_y = \
        {{"0"},
         {"1"},
         {"2"},
         {"3"},
         {"4"},
         {"0"},
         {"2"},
         {"2"},
         {"2"},
         {"0"} };

    v2::ml_data data({{"target_column_always_numeric", false}});

    sframe raw_data = make_testing_sframe({"response"}, {flex_type_enum::STRING}, raw_y);

    data.set_data(raw_data);

    data.fill();

    for(auto it = data.get_iterator(); !it.done(); ++it) {
      ASSERT_TRUE(data.metadata()->target_indexer()->map_index_to_value(it.target_index())
                  == raw_y[it.row_index()][0]);
    }
  }
};

#endif

BOOST_FIXTURE_TEST_SUITE(_ml_data_numeric_iteration_test, ml_data_numeric_iteration_test)
BOOST_AUTO_TEST_CASE(test_row_index) {
  ml_data_numeric_iteration_test::test_row_index();
}
BOOST_AUTO_TEST_CASE(test_target_value) {
  ml_data_numeric_iteration_test::test_target_value();
}
BOOST_AUTO_TEST_CASE(test_fill_observation_ml_data_entry) {
  ml_data_numeric_iteration_test::test_fill_observation_ml_data_entry();
}
BOOST_AUTO_TEST_CASE(test_fill_observation_dense_vector) {
  ml_data_numeric_iteration_test::test_fill_observation_dense_vector();
}
BOOST_AUTO_TEST_CASE(test_fill_observation_sparse_vector) {
  ml_data_numeric_iteration_test::test_fill_observation_sparse_vector();
}
BOOST_AUTO_TEST_CASE(test_fill_observation_reference_dense_vector) {
  ml_data_numeric_iteration_test::test_fill_observation_reference_dense_vector();
}
BOOST_AUTO_TEST_CASE(test_fill_observation_reference_sparse_vector) {
  ml_data_numeric_iteration_test::test_fill_observation_reference_sparse_vector();
}
BOOST_AUTO_TEST_CASE(test_large_ordering) {
  ml_data_numeric_iteration_test::test_large_ordering();
}
BOOST_AUTO_TEST_CASE(test_large_ordering_in_parallel) {
  ml_data_numeric_iteration_test::test_large_ordering_in_parallel();
}
BOOST_AUTO_TEST_CASE(test_categorical_targets) {
  ml_data_numeric_iteration_test::test_categorical_targets();
}
BOOST_AUTO_TEST_SUITE_END()

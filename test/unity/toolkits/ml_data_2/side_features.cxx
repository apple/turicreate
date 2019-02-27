#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <vector>
#include <string>
#include <map>


#include <random/random.hpp>

#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/testing_utils.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>

#include <globals/globals.hpp>

#define tI flex_type_enum::INTEGER
#define tD flex_type_enum::DICT
#define tV flex_type_enum::VECTOR

using namespace turi;


// The main testing function translates the data through the ml_data
// with side_data classes, then translates it back to make sure it
// gets the right answer. If it does, all aspects of the translation
// process, including the side_data class is correct.  Each of the
// individual tests below ensure correctness on a different part of
// this process.


/// Tests the consistency of a join against reference data.
void test_consistency(const v2::ml_data& data,
                      const std::vector<std::vector<flexible_type> >& full_joined_data,
                      bool test_eigen) {

  globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);

  std::vector<v2::ml_data_entry> x, x_alt;
  v2::ml_data::SparseVector xs, xs_alt;
  v2::ml_data::DenseVector xd, xd_alt;
  std::vector<v2::ml_data_entry_global_index> x_gi, x_gi_alt;
  Eigen::MatrixXd xdr, xdr_alt;

  xd.resize(data.metadata()->num_dimensions());
  xd_alt.resize(data.metadata()->num_dimensions());
  xs.resize(data.metadata()->num_dimensions());
  xs_alt.resize(data.metadata()->num_dimensions());

  xdr.resize(3, data.metadata()->num_dimensions());
  xdr.setZero();
  xdr_alt.resize(3, data.metadata()->num_dimensions());
  xdr_alt.setZero();

  size_t idx = 0;
  for(auto it = data.get_iterator(); !it.done(); ++it, ++idx) {

    std::vector<flexible_type> ref_row = full_joined_data[idx];
    std::vector<flexible_type> joined_row;

    for(size_t type_idx : {0, 1, 2, 3, 4, 5, 6} ) {

      switch(type_idx) {
        case 0: {
          it.fill_observation(x);

          it.get_reference().fill(x_alt);
          ASSERT_TRUE(x_alt == x);

          // std::cerr << "x = " << x << std::endl;
          joined_row = data.translate_row_to_original(x);
          break;
        }
        case 1: {
          it.fill_observation(xs);

          it.get_reference().fill(xs_alt);
          ASSERT_TRUE(xs_alt.toDense() == xs.toDense());

          joined_row = data.translate_row_to_original(xs);
          break;
        }
        case 2: {
          it.fill_observation(xd);

          it.get_reference().fill(xd_alt);
          ASSERT_TRUE(xd_alt == xd);

          joined_row = data.translate_row_to_original(xd);
          // std::cerr << "xd = " << xd.transpose() << std::endl;
          break;
        }
        case 3: {
          it.fill_observation(x_gi);

          it.get_reference().fill(x_gi_alt);
          ASSERT_TRUE(x_gi == x_gi_alt);

          joined_row = data.translate_row_to_original(x_gi);
          // std::cerr << "xd = " << xd.transpose() << std::endl;
          break;
        }
        case 4: {
          it.fill_observation(x);

          // Strip and replace the features associated with one of the columns
          size_t idx = random::fast_uniform<size_t>(0, data.metadata()->num_columns(false) - 1);

          data.get_side_features()->strip_side_features_from_row(idx, x);
          data.get_side_features()->add_partial_side_features_to_row(x, idx);

          // std::cerr << "x = " << x << std::endl;
          joined_row = data.translate_row_to_original(x);
          break;
        }
        case 5: {
          it.fill_observation(x_gi);

          // Strip and replace the features associated with one of the columns
          size_t idx = random::fast_uniform<size_t>(0, data.metadata()->num_columns(false) - 1);

          data.get_side_features()->strip_side_features_from_row(idx, x_gi);
          data.get_side_features()->add_partial_side_features_to_row(x_gi, idx);

          // std::cerr << "x = " << x << std::endl;
          joined_row = data.translate_row_to_original(x_gi);
          break;
        }

        case 6: {
          it.fill_eigen_row(xdr.row(1));

          it.get_reference().fill_eigen_row(xdr_alt.row(1));
          ASSERT_TRUE(xdr == xdr_alt);

          xd = xdr.row(1);

          joined_row = data.translate_row_to_original(xd);
          break;
        }

      }

      // Test to make sure these fill without error, but they cannot
      // handle new indices beyond those given at train time, so they
      // will fail the test (sometimes).
      if(!test_eigen && type_idx >= 1)
        continue;

      ASSERT_EQ(ref_row.size(), joined_row.size());

      for(size_t i = 0; i < joined_row.size(); ++i) {
        if(ref_row[i] != FLEX_UNDEFINED) {
          ASSERT_TRUE(v2::ml_testing_equals(ref_row[i], joined_row[i]));
        }
      }
    }
  }
}


void test_join_sf(

    // The main data.  This is given as a vector of vectors of integers.
    const sframe& initial_data_sf,

    // The side data.  This is given as a vector of maps, one for each
    // column in initial_data.  The maps are from feature value in the
    // appropriate columne of initial_data to a row of side
    // information. All values are done with integers.
    std::vector<std::unordered_map<flexible_type, std::vector<flexible_type> > > side_data) {

  // This test does a comprehensive test of the joining capabilities
  // of the side information.  There are two aspects of this side
  // information that is important.
  //
  // First, that basic functionality is correct.  That is, that all
  // the columns are joined properly.
  //
  // Second, that adding additional information is
  // handled properly; i.e. adding a small amount of additional
  // information does not delete any of the information currently
  // present, but may overwrite conflicting values or add additional
  // entries that were not part of the original map.

  // Make sure that side data is the right size and contains enough
  // side information.
  size_t num_columns = initial_data_sf.num_columns();
  DASSERT_LE(side_data.size(), num_columns);
  side_data.resize(num_columns);

  // Build an index of the side column sizes and offsets.  When an
  // observation vector has all the columns joined in, this gives
  // where in that vector the different blocks of side information are
  // located.
  std::vector<size_t> side_column_sizes(num_columns);
  std::vector<size_t> side_column_offsets(num_columns);

  size_t offset = num_columns;

  for(size_t i = 0; i < num_columns; ++i) {
    size_t size = 0;
    for(const auto& p : side_data[i]) {
      if(size != 0)
        DASSERT_EQ(p.second.size(), size);
      else
        size = p.second.size();
    }
    side_column_sizes[i] = size;
    side_column_offsets[i] = offset;
    offset += size;
  }

  size_t total_columns = offset;

  DASSERT_EQ(side_column_sizes.size(), num_columns);
  DASSERT_EQ(side_data.size(), num_columns);

  auto initial_data = testing_extract_sframe_data(initial_data_sf);

  ////////////////////////////////////////////////////////////////////////////////

  // Create the reference data.  We do this by explicitly
  // instantiating the complete join; We will thus test
  // the filled observations against full_joined_data.
  std::vector<std::vector<flexible_type> > full_joined_data;

  for(const auto& v : initial_data)
    full_joined_data.emplace_back(v.begin(), v.end());

  for(auto& row : full_joined_data)
    row.resize(total_columns, FLEX_UNDEFINED);

  size_t start_idx = num_columns;

  for(size_t i = 0; i < num_columns; ++i) {

    for(size_t j = 0; j < full_joined_data.size(); ++j) {

      auto it = side_data[i].find(initial_data[j][i]);

      if(it != side_data[i].end()) {

        DASSERT_EQ(it->second.size(), side_column_sizes[i]);

        for(size_t k = 0; k < it->second.size(); ++k)
          full_joined_data[j][start_idx + k] = it->second[k];
      }
    }

    start_idx += side_column_sizes[i];
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Now build the sframes needed to use the side_information class.

  //////////////////////////////////////////////////

  // Now, create vectors of sframes for the side information.

  // Simultaneously, we create alternative versions of each side data
  // sframe to test the ability of the side features class to
  // overwrite and extend the initial data.  These alternative sframes
  // are only of length 2, giving a small change.  Thus we can test
  // that only appropriate side rows were overwritten.  As part of
  // this, we also have to create an alternative version of the full
  // joined data to make sure everything is still correct.

  std::vector<sframe> side_data_sf, alt_side_data_sf;
  std::vector<std::vector<flexible_type> > alt_full_joined_data = full_joined_data;

  DASSERT_EQ(side_data.size(), num_columns);

  for(size_t column_index = 0; column_index < side_data.size(); ++column_index) {
    if(side_data[column_index].empty())
      continue;

    std::vector<std::string> names;

    names.push_back(std::string("C") + std::to_string(column_index));

    size_t n_side_columns = 1 + side_data[column_index].begin()->second.size();

    for(size_t j = 1; j < n_side_columns; ++j)
      names.push_back(std::string("S-") + std::to_string(column_index) + "-" + std::to_string(j-1));

    std::vector<std::vector<flexible_type> > data;

    for(const auto& p : side_data[column_index]) {
      DASSERT_EQ(names.size(), p.second.size() + 1);

      std::vector<flexible_type> row = { p.first };

      row.insert(row.end(), p.second.begin(), p.second.end());
      data.push_back(std::move(row));
    }


    side_data_sf.push_back(make_testing_sframe(names, data));

    // Now, create the alt version.
    if(data.size() >= 2) {

      data.resize(2);

      for(size_t k = 1; k < data[0].size(); ++k) {
        if(data[0][k].get_type() != flex_type_enum::DICT)
          data[0][k] += 1000000;
      }

      for(size_t k = 1; k < data[1].size(); ++k) {
        if(data[0][k].get_type() != flex_type_enum::DICT)
          data[1][k] += 2000000;
      }

      alt_side_data_sf.push_back(make_testing_sframe(names, data));

      // Put these modifications into the alt_full_joined_data
      for(size_t data_idx : {0,1}) {
        for(size_t k = 0; k < alt_full_joined_data.size(); ++k) {
          if(alt_full_joined_data[k][column_index] == data[data_idx][0]) {
            for(size_t j = 0; j < data[data_idx].size() - 1; ++j) {
              alt_full_joined_data[k][side_column_offsets[column_index] + j] = data[data_idx][j + 1];
            }
          }
        }
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Now, create the ml_data_side_features class.

  v2::ml_data initial_data_ml({{"integer_columns_categorical_by_default", true}});
  initial_data_ml.set_data(initial_data_sf);

  for(const sframe& S : side_data_sf)
    initial_data_ml.add_side_data(S);

  initial_data_ml.fill();

  ////////////////////////////////////////////////////////////////////////////////
  // Now check everything in the main data.
  test_consistency(initial_data_ml, full_joined_data, true);

  // Now check that the metadata preserves all of these things.
  {
    // Copy the original into a new one.  We'll overwrite things in this one.
    v2::ml_data alt_data(initial_data_ml.metadata());

    alt_data.set_data(initial_data_sf);
    alt_data.fill();

    test_consistency(alt_data, full_joined_data, true);

    v2::ml_data alt_data_v2;
    save_and_load_object(alt_data_v2, alt_data);
    test_consistency(alt_data_v2, full_joined_data, true);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Now check everything in the main data again to make sure nothing
  // has changed.

  test_consistency(initial_data_ml, full_joined_data, true);


  ////////////////////////////////////////////////////////////////////////////////
  // Now check to see if things still work well when you add additional information.
  {
    // Copy the original into a new one.  We'll overwrite things in this one.
    v2::ml_data alt_data(initial_data_ml.metadata(), false);

    alt_data.set_data(initial_data_sf);
    for(const sframe& S : alt_side_data_sf)
      alt_data.add_side_data(S);

    alt_data.fill();
    test_consistency(alt_data, alt_full_joined_data, false);

    v2::ml_data alt_data_v2;
    save_and_load_object(alt_data_v2, alt_data);
    test_consistency(alt_data_v2, alt_full_joined_data, false);
  }


  // Now check to see if things still work well when you save and load the metadata
  {
    std::shared_ptr<v2::ml_metadata> m_sl;
    save_and_load_object(m_sl, initial_data_ml.metadata());
    v2::ml_data alt_data(m_sl, false);

    alt_data.set_data(initial_data_sf);
    alt_data.fill();
    test_consistency(alt_data, full_joined_data, true);

    v2::ml_data alt_data_v2;
    save_and_load_object(alt_data_v2, alt_data);
    test_consistency(alt_data_v2, full_joined_data, true);
  }

  {
    v2::ml_data data_sl;
    save_and_load_object(data_sl, initial_data_ml);

    test_consistency(initial_data_ml, full_joined_data, true);
  }

}


void test_join(

    // The main data.  This is given as a vector of vectors of integers.
    const std::vector<std::vector<size_t> >& initial_data,

    // The side data.  This is given as a vector of maps, one for each
    // column in initial_data.  The maps are from feature value in the
    // appropriate columne of initial_data to a row of side
    // information. All values are done with integers.
    std::vector<std::unordered_map<flexible_type, std::vector<flexible_type> > > side_data) {


  // First, turn the main data into an ml_data object.
  sframe initial_data_sf;

  {
    std::vector<std::string> names;

    for(size_t i = 0; i < initial_data[0].size(); ++i)
      names.push_back(std::string("C") + std::to_string(i));

    initial_data_sf = make_integer_testing_sframe(names, initial_data);
  }

  test_join_sf(initial_data_sf, side_data);
}



struct side_feature_basic_test  {
 public:
  void test_sanity() {
    test_join({ {0} },
              { { { 0, {290} } } });
  }

  void test_1_column_small() {
    test_join({ {0}, {1} },
              { { { 0, {2} },
                  { 1, {3} } } });
  }

  void test_1_column_small_missing_values() {
    test_join({ {0}, {1}, {2} },
              { { { 0, {2} },
                  { 1, {3} } } });
  }

  void test_2_column_small_1_side() {
    test_join({ {0, 1}, {1, 2} },
              { { { 0, {2} },
                  { 1, {3} } } });
  }

  void test_2_column_small() {
    test_join({ {0, 1}, {1, 2} },
              { { { 0, {2} },
                  { 1, {3} } },
                { { 1, {4} },
                  { 2, {5} } }});
  }

  void test_2_column_small_superfulous_sides() {
    test_join({ {0, 1}, {1, 2} },
              { { { 0, {2} },
                  { 3, {3} } },
                { { 1, {4} },
                  { 4, {5} } }});
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Now, let's test vectors and dictionaries

  void test_vector_sanity() {
    test_join({ {0} },
              { { { 0, {flex_vec{1,2,3}} } } });
  }

  void test_2_column_vector_partial() {
    test_join({ {0, 1}, {1, 2} },
              { { { 0, {flex_vec{5,5,7}} },
                  { 1, {flex_vec{6,5,6}} } } });
  }

  void test_vector_sanity_type_mix() {
    test_join({ {0}, {1} },
              { { { 0, {7, flex_vec{1,2,3}} },
                  { 1, {8, flex_vec{4,5,6}} }    } });
  }

  void test_vector_sanity_missing_sides() {
    test_join({ {0}, {2} },
              { { { 0, {7, flex_vec{1,2,3}} },
                  { 1, {8, flex_vec{4,5,6}} }    } });
  }

  void test_vector_empty_vectors() {
    test_join({ {0}, {2} },
              { { { 0, {7, flex_vec()} },
                  { 1, {8, flex_vec()} }    } });
  }

  void test_2_column_vector_partial_multicolumn() {
    test_join({ {0, 1}, {1, 2} },
              { { { 0, {flex_vec{1,5,7}} },
                  { 1, {flex_vec{2,5,6}} } },
                { { 1, {flex_vec{3,5,7}} },
                  { 2, {flex_vec{4,5,6}} } }
              });
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Dictionaries

  void test_dict_sanity() {
    test_join({ {0} },
              { { { 0, {flex_dict{{1,5},{2,3}}} } } });
  }

  void test_2_column_dict_partial() {
    test_join({ {0, 1}, {1, 2} },
              { { { 0, {flex_dict{{1,5},{5,7}}} },
                  { 1, {flex_dict{{1,6},{5,6}}} } } });
  }

  void test_dict_sanity_type_mix() {
    test_join({ {0}, {1} },
              { { { 0, {7, flex_dict{{1,1},{2,3}}} },
                  { 1, {8, flex_dict{{1,4},{5,6}}} }    } });
  }

  void test_dict_size_mix() {
    test_join({ {0}, {1} },
              { { { 0, {flex_dict{{1,1}}} },
                  { 1, {flex_dict{{1,4},{5,6}}} }    } });
  }

  void test_2_column_dict_partial_multicolumn() {
    test_join({ {0, 1}, {1, 2} },
              { { { 0, {flex_dict{{1,1},{5,7}}} },
                  { 1, {flex_dict{{1,2},{5,6}}} } },
                { { 1, {flex_dict{{1,3},{5,7}}} },
                  { 2, {flex_dict{{1,4},{5,6}}} } }
              });
  }

};

void test_consistency(const v2::sframe_and_side_info& info) {
  const v2::ml_data& data = info.data;

  std::vector<v2::ml_data_entry> x;
  v2::ml_data::DenseVector xd;
  v2::ml_data::SparseVector xs;
  Eigen::MatrixXd xdr;

  xd.resize(data.metadata()->num_dimensions());
  xs.resize(data.metadata()->num_dimensions());

  xdr.resize(3, data.metadata()->num_dimensions());
  xdr.setZero();

  for(auto it = data.get_iterator(); !it.done(); ++it) {

    const std::vector<flexible_type>& ref_row = info.joined_data[it.row_index()];
    std::vector<flexible_type> joined_row;

    for(size_t type_idx : {0, 1, 2, 3} ) {

      switch(type_idx) {
        case 0: {
          it.fill_observation(x);
          // std::cerr << "x = " << x << std::endl;
          joined_row = data.translate_row_to_original(x);
          break;
        }
        case 1: {
          it.fill_observation(xs);
          joined_row = data.translate_row_to_original(xs);
          break;
        }
        case 2: {
          it.fill_observation(xd);
          joined_row = data.translate_row_to_original(xd);
          // std::cerr << "xd = " << xd.transpose() << std::endl;
          break;
        }
        case 3: {
          it.fill_eigen_row(xdr.row(1));

          xd = xdr.row(1);

          joined_row = data.translate_row_to_original(xd);
          break;
        }
      }

      ASSERT_EQ(ref_row.size(), joined_row.size());

      for(size_t i = 0; i < joined_row.size(); ++i)
        ASSERT_TRUE(v2::ml_testing_equals(ref_row[i], joined_row[i]));
    }
  }
}

void run_random_test(size_t n,
                     const std::string& main_string,
                     const std::vector<std::pair<size_t, std::string> >& run_strings,
                     bool use_target_column) {

  globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);

  auto info = v2::make_ml_data_with_side_data(n, main_string, run_strings, use_target_column);

  test_consistency(info);
}


struct side_feature_random_test  {
 public:

  ////////////////////////////////////////////////////////////////////////////////
  // Cases with no target

  void test_side_random_1c() {
    run_random_test(25, "c", { {5, "n"} }, false);
  }

  void test_side_random_2c() {
    run_random_test(25, "cC", { {5, "n"}, {100, "n"} }, false);
  }

  void test_side_random_1s() {
    run_random_test(25, "s", { {5, "n"} }, false );
  }

  void test_side_random_2s() {
    run_random_test(25, "sS", { {5, "n"}, {100, "n"} }, false);
  }

  void test_side_random_3s_a() {
    run_random_test(25, "sss", { {5, "n"}, {100, "n"} }, false);
  }

  void test_side_random_3s_b() {
    run_random_test(25, "sss", { {5, "n"}, {100, "n"}, {100, "n"} }, false);
  }

  void test_side_random_1s_c() {
    run_random_test(25, "s", { {5, "nsdv"} }, false);
  }

  void test_side_random_4s_large() {
    run_random_test(25, "csCSnnvd", { {100, "nsv"}, {100, "ndu"}, {100, "ncn"} }, false);
  }

  void test_side_random_fixed_main_nonfixed_side() {
    run_random_test(25, "ccnnv", { {100, "nnn"}, {100, "d"} }, false);
  }

  void test_side_large() {
    run_random_test(500, "cCnn", { {100, "nc"}, {500, "nbbbbbb"} }, false);
  }

  void test_side_large_gap() {
    run_random_test(500, "ccccccc", { {200, "nc"}, {0, ""}, {10, "cv"}, {0, ""}, {20, "V"} }, false);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Cases with target

  void test_side_random_1c_t() {
    run_random_test(25, "c", { {5, "n"} }, true);
  }

  void test_side_random_2c_t() {
    run_random_test(25, "cC", { {5, "n"}, {100, "n"} }, true);
  }

  void test_side_random_1s_t() {
    run_random_test(25, "s", { {5, "n"} }, true );
  }

  void test_side_random_2s_t() {
    run_random_test(25, "sS", { {5, "n"}, {100, "n"} }, true);
  }

  void test_side_random_3s_a_t() {
    run_random_test(25, "sss", { {5, "n"}, {100, "n"} }, true);
  }

  void test_side_random_3s_b_t() {
    run_random_test(25, "sss", { {5, "n"}, {100, "n"}, {100, "n"} }, true);
  }

  void test_side_random_1s_c_t() {
    run_random_test(25, "s", { {5, "nsdv"} }, true);
  }

  void test_side_random_4s_large_t() {
    run_random_test(25, "csCSnnvd", { {100, "nsv"}, {100, "ndu"}, {100, "ncn"} }, true);
  }

  void test_side_random_fixed_main_nonfixed_side_t() {
    run_random_test(25, "ccnnv", { {100, "nnn"}, {100, "d"} }, true);
  }

  void test_side_large_t() {
    run_random_test(500, "cCnn", { {100, "nc"}, {500, "nbbbbbb"} }, true);
  }

  void test_side_large_gap_t() {
    run_random_test(500, "ccccccc", { {200, "nc"}, {0, ""}, {10, "cv"}, {0, ""}, {20, "V"} }, true);
  }
};

struct side_feature_metadata_consistency_test  {
 public:

  void run_schema_test(const std::string& main_string,
                       const std::vector<std::string>& run_strings) {

    // If we have vectors, we have to skip some of the tests, in
    // particular those with no data.  It will still cause issues.
    bool has_vectors = false;

    if(main_string.find('v') != std::string::npos
       || main_string.find('V') != std::string::npos) {
      has_vectors = true;
    }

    std::vector<std::pair<size_t, std::string> > run_spec_1(run_strings.size());
    std::vector<std::pair<size_t, std::string> > run_spec_2(run_strings.size());
    std::vector<std::pair<size_t, std::string> > run_spec_3(run_strings.size());

    for(size_t i = 0; i < run_strings.size(); ++i) {

      if(run_strings[i].find('v') != std::string::npos
         || run_strings[i].find('V') != std::string::npos) {
        has_vectors = true;
      }

      run_spec_1[i] = {10, run_strings[i]};
      run_spec_2[i] = {5, run_strings[i]};
      run_spec_3[i] = {0, run_strings[i]};
    }

    std::map<std::string, flexible_type> options =
        {{"integer_columns_categorical_by_default", true}};

    std::vector<v2::sframe_and_side_info> info_v;

    for(bool uniquify_side_column_names : {false, true} ) {

      if(uniquify_side_column_names)
        options["uniquify_side_column_names"] = true;
      else
        options["uniquify_side_column_names"] = false;


      info_v.push_back(
          v2::make_ml_data_with_side_data(11, main_string, run_spec_1, false, options));

      info_v.push_back(
          v2::make_ml_data_with_side_data(11, main_string, run_spec_1, false, options));

      info_v.push_back(
          v2::make_ml_data_with_side_data(11, main_string, run_spec_1, false, options));

      info_v.push_back(
          v2::make_ml_data_with_side_data(0,  main_string, run_spec_3, false, options));

      // Do ones with same data, but new metadata and explicit join columns
      for(size_t i = 0; i < 4; ++i) {
        v2::sframe_and_side_info info = info_v[i];

        info.data = v2::ml_data(options);

        info.data.set_data(info.main_sframe);
        for(const sframe& side_sframe : info.side_sframes) {
          sframe side_new = side_sframe;

          // Set the names so that they conflict with the main column names
          for(size_t j = 1; j < std::min(info.main_sframe.num_columns()+1, side_sframe.num_columns()); ++j) {
            std::string main_name = info.main_sframe.column_name(j-1);
            if(main_name != side_sframe.column_name(0)) {
              std::string new_column_name = main_name;

              if(!side_sframe.contains_column(new_column_name))
                side_new.set_column_name(j, new_column_name);
            }
          }

          // The first column is the one that the join is performed
          // on.
          info.data.add_side_data(side_sframe, side_sframe.column_name(0));
        }

        info.data.fill();

        // Check uniqueness if that's what we've asked for.
        if(uniquify_side_column_names) {
          std::vector<std::string> column_names = info.data.metadata()->column_names();
          std::set<std::string> column_name_set(column_names.begin(), column_names.end());

          ASSERT_EQ(column_names.size(), column_name_set.size());
        }

        info_v.push_back(info);
      }
    }

    // Now go and add in the data again, but reindexing things and saving and loading the metadata
    size_t n_columns = info_v.size();
    info_v.resize(2*n_columns);

    for(size_t i = 0; i < n_columns; ++i) {

      info_v[n_columns + i] = info_v[i];

      auto& info = info_v[n_columns + i];

      std::shared_ptr<v2::ml_metadata> m_sl_1, m_sl_2;

      save_and_load_object(m_sl_1, info.data.metadata());
      save_and_load_object(m_sl_2, info.data.metadata());

      v2::ml_data d(m_sl_1, true);

      d.set_data(info.main_sframe);
      for(const sframe& side_sframe : info.side_sframes) {
        d.add_side_data(side_sframe);
      }

      d.fill();

      info.data = d;

      // Now, possibly, see if
      size_t other_index = 4 * (i / 4);

      if(info_v[other_index].data.metadata()->get_current_options() == m_sl_1->get_current_options()
         && (!has_vectors || (
             // Need to check these cases if
             info_v[i].main_sframe.num_rows() != 0
             && info_v[i].side_sframes.back().num_rows() != 0))) {

        auto info = info_v[other_index];

        // Index the other one with this metadata, but that one's
        // data.  It should still work okay -- as the schema is
        // exactly the same -- but it won't be quite the same.

        {
          v2::ml_data d(m_sl_1, false);

          d.set_data(info.main_sframe);
          for(const sframe& side_sframe : info.side_sframes) {
            d.add_side_data(side_sframe);
          }

          d.fill();
        }

        {
          // Index the other one with this metadata.  It shouldn't throw an  error (regression test).

          v2::ml_data d2(m_sl_2, true);

          d2.set_data(info.main_sframe);
          for(const sframe& side_sframe : info.side_sframes) {
            d2.add_side_data(side_sframe);
          }

          d2.fill();
        }
      }
    }

    // Now go through and test the consistency of each of existing
    // ones. Can do this in parallel.
    parallel_for(size_t(0), info_v.size(), [&](size_t i) {
        test_consistency(info_v[i]);
      });
  }


  ////////////////////////////////////////////////////////////////////////////////
  // Cases with no target

  void test_schema_1() {
    run_schema_test("cc", {"c", "c"});
  }

  void test_schema_2() {
    run_schema_test("cc", {"ccc", "ccc"});
  }

  void test_schema_3() {
    run_schema_test("CCCC", {"ccc", "ccc", "ccc", "ccc"});
  }

  void test_schema_4() {
    run_schema_test("cccc", {"d", "d", "d", "d"});
  }

  // And finally one with everything
  void test_schema_5() {
    run_schema_test("Cscscs", {"cccccdv", "cnvn", "css", "scnu", "c", "n"});
  }

  void test_schema_no_additional_info() {
    run_schema_test("cc", {"c", ""});
  }

};

BOOST_FIXTURE_TEST_SUITE(_side_feature_basic_test, side_feature_basic_test)
BOOST_AUTO_TEST_CASE(test_sanity) {
  side_feature_basic_test::test_sanity();
}
BOOST_AUTO_TEST_CASE(test_1_column_small) {
  side_feature_basic_test::test_1_column_small();
}
BOOST_AUTO_TEST_CASE(test_1_column_small_missing_values) {
  side_feature_basic_test::test_1_column_small_missing_values();
}
BOOST_AUTO_TEST_CASE(test_2_column_small_1_side) {
  side_feature_basic_test::test_2_column_small_1_side();
}
BOOST_AUTO_TEST_CASE(test_2_column_small) {
  side_feature_basic_test::test_2_column_small();
}
BOOST_AUTO_TEST_CASE(test_2_column_small_superfulous_sides) {
  side_feature_basic_test::test_2_column_small_superfulous_sides();
}
BOOST_AUTO_TEST_CASE(test_vector_sanity) {
  side_feature_basic_test::test_vector_sanity();
}
BOOST_AUTO_TEST_CASE(test_2_column_vector_partial) {
  side_feature_basic_test::test_2_column_vector_partial();
}
BOOST_AUTO_TEST_CASE(test_vector_sanity_type_mix) {
  side_feature_basic_test::test_vector_sanity_type_mix();
}
BOOST_AUTO_TEST_CASE(test_vector_sanity_missing_sides) {
  side_feature_basic_test::test_vector_sanity_missing_sides();
}
BOOST_AUTO_TEST_CASE(test_vector_empty_vectors) {
  side_feature_basic_test::test_vector_empty_vectors();
}
BOOST_AUTO_TEST_CASE(test_2_column_vector_partial_multicolumn) {
  side_feature_basic_test::test_2_column_vector_partial_multicolumn();
}
BOOST_AUTO_TEST_CASE(test_dict_sanity) {
  side_feature_basic_test::test_dict_sanity();
}
BOOST_AUTO_TEST_CASE(test_2_column_dict_partial) {
  side_feature_basic_test::test_2_column_dict_partial();
}
BOOST_AUTO_TEST_CASE(test_dict_sanity_type_mix) {
  side_feature_basic_test::test_dict_sanity_type_mix();
}
BOOST_AUTO_TEST_CASE(test_dict_size_mix) {
  side_feature_basic_test::test_dict_size_mix();
}
BOOST_AUTO_TEST_CASE(test_2_column_dict_partial_multicolumn) {
  side_feature_basic_test::test_2_column_dict_partial_multicolumn();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_side_feature_random_test, side_feature_random_test)
BOOST_AUTO_TEST_CASE(test_side_random_1c) {
  side_feature_random_test::test_side_random_1c();
}
BOOST_AUTO_TEST_CASE(test_side_random_2c) {
  side_feature_random_test::test_side_random_2c();
}
BOOST_AUTO_TEST_CASE(test_side_random_1s) {
  side_feature_random_test::test_side_random_1s();
}
BOOST_AUTO_TEST_CASE(test_side_random_2s) {
  side_feature_random_test::test_side_random_2s();
}
BOOST_AUTO_TEST_CASE(test_side_random_3s_a) {
  side_feature_random_test::test_side_random_3s_a();
}
BOOST_AUTO_TEST_CASE(test_side_random_3s_b) {
  side_feature_random_test::test_side_random_3s_b();
}
BOOST_AUTO_TEST_CASE(test_side_random_1s_c) {
  side_feature_random_test::test_side_random_1s_c();
}
BOOST_AUTO_TEST_CASE(test_side_random_4s_large) {
  side_feature_random_test::test_side_random_4s_large();
}
BOOST_AUTO_TEST_CASE(test_side_random_fixed_main_nonfixed_side) {
  side_feature_random_test::test_side_random_fixed_main_nonfixed_side();
}
BOOST_AUTO_TEST_CASE(test_side_large) {
  side_feature_random_test::test_side_large();
}
BOOST_AUTO_TEST_CASE(test_side_large_gap) {
  side_feature_random_test::test_side_large_gap();
}
BOOST_AUTO_TEST_CASE(test_side_random_1c_t) {
  side_feature_random_test::test_side_random_1c_t();
}
BOOST_AUTO_TEST_CASE(test_side_random_2c_t) {
  side_feature_random_test::test_side_random_2c_t();
}
BOOST_AUTO_TEST_CASE(test_side_random_1s_t) {
  side_feature_random_test::test_side_random_1s_t();
}
BOOST_AUTO_TEST_CASE(test_side_random_2s_t) {
  side_feature_random_test::test_side_random_2s_t();
}
BOOST_AUTO_TEST_CASE(test_side_random_3s_a_t) {
  side_feature_random_test::test_side_random_3s_a_t();
}
BOOST_AUTO_TEST_CASE(test_side_random_3s_b_t) {
  side_feature_random_test::test_side_random_3s_b_t();
}
BOOST_AUTO_TEST_CASE(test_side_random_1s_c_t) {
  side_feature_random_test::test_side_random_1s_c_t();
}
BOOST_AUTO_TEST_CASE(test_side_random_4s_large_t) {
  side_feature_random_test::test_side_random_4s_large_t();
}
BOOST_AUTO_TEST_CASE(test_side_random_fixed_main_nonfixed_side_t) {
  side_feature_random_test::test_side_random_fixed_main_nonfixed_side_t();
}
BOOST_AUTO_TEST_CASE(test_side_large_t) {
  side_feature_random_test::test_side_large_t();
}
BOOST_AUTO_TEST_CASE(test_side_large_gap_t) {
  side_feature_random_test::test_side_large_gap_t();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_side_feature_metadata_consistency_test, side_feature_metadata_consistency_test)
BOOST_AUTO_TEST_CASE(test_schema_1) {
  side_feature_metadata_consistency_test::test_schema_1();
}
BOOST_AUTO_TEST_CASE(test_schema_2) {
  side_feature_metadata_consistency_test::test_schema_2();
}
BOOST_AUTO_TEST_CASE(test_schema_3) {
  side_feature_metadata_consistency_test::test_schema_3();
}
BOOST_AUTO_TEST_CASE(test_schema_4) {
  side_feature_metadata_consistency_test::test_schema_4();
}
BOOST_AUTO_TEST_CASE(test_schema_5) {
  side_feature_metadata_consistency_test::test_schema_5();
}
BOOST_AUTO_TEST_CASE(test_schema_no_additional_info) {
  side_feature_metadata_consistency_test::test_schema_no_additional_info();
}
BOOST_AUTO_TEST_SUITE_END()

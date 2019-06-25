#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <core/util/cityhash_tc.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <model_server/lib/flex_dict_view.hpp>
#include <core/random/random.hpp>

// ML-Data Utils
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/row_slicing_utilities.hpp>

// Testing utils common to all of ml_data_iterator
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <toolkits/ml_data_2/testing_utils.hpp>

#include <core/globals/globals.hpp>

using namespace turi;

struct test_composite_rows  {
 public:

  void test_composite_rows_simple() {

    sframe X = make_integer_testing_sframe( {"C0", "C1", "C2"}, { {1,2,3}, {4,5,6} } );

    v2::ml_data data;

    // Set column "C0" to be untranslated.
    data.set_data(X, "", {}, { {"C0", v2::ml_column_mode::UNTRANSLATED} });

    data.fill();

    auto row_spec = std::make_shared<v2::composite_row_specification>(data.metadata());

    // Add one dense subrow formed from columns 1 and 2
    size_t dense_row_index_1  = row_spec->add_dense_subrow( {1, 2} );

    // Add a sparse subrow formed from column 2
    size_t sparse_row_index   = row_spec->add_sparse_subrow( {2} );

    // Add an untranslated row formed from column 0
    size_t flex_row_index     = row_spec->add_flex_type_subrow( {0} );

    // Add another dense subrow formed from column 1
    size_t dense_row_index_2  = row_spec->add_dense_subrow( {1} );

    v2::composite_row_container crc(row_spec);

    ////////////////////////////////////////


    auto it = data.get_iterator();

    {
      it.fill_observation(crc);

      // The 1st dense component; two numerical columns.
      const auto& vd = crc.dense_subrows[dense_row_index_1];

      ASSERT_EQ(vd.size(), 2);
      ASSERT_EQ(size_t(vd[0]), 2);  // First row, 2nd column
      ASSERT_EQ(size_t(vd[1]), 3);  // First row, 3nd column

      // The 2nd dense component; one numerical columns.
      const auto& vd2 = crc.dense_subrows[dense_row_index_2];

      ASSERT_EQ(vd2.size(), 1);
      ASSERT_EQ(size_t(vd2[0]), 2);  // First row, 2nd column

      // The sparse component: one numerical column
      const auto& vs = crc.sparse_subrows[sparse_row_index];

      ASSERT_EQ(vs.size(), 1);
      ASSERT_EQ(size_t(vs.coeff(0)), 3);  // First row, 3nd column

      // The untranslated column.
      const auto& vf = crc.flex_subrows[flex_row_index];

      ASSERT_EQ(vf.size(), 1);
      ASSERT_TRUE(vf[0] == 1);   // First row, 1st column
    }

    ++it;

    {
      it.fill_observation(crc);

      // The 1st dense component; two numerical columns.
      const auto& vd = crc.dense_subrows[dense_row_index_1];

      ASSERT_EQ(vd.size(), 2);
      ASSERT_EQ(size_t(vd[0]), 5);  // First row, 2nd column
      ASSERT_EQ(size_t(vd[1]), 6);  // First row, 3nd column

      // The 2nd dense component; one numerical columns.
      const auto& vd2 = crc.dense_subrows[dense_row_index_2];

      ASSERT_EQ(vd2.size(), 1);
      ASSERT_EQ(size_t(vd2[0]), 5);  // First row, 2nd column

      const auto& vs = crc.sparse_subrows[sparse_row_index];

      // One numerical column
      ASSERT_EQ(vs.size(), 1);
      ASSERT_EQ(size_t(vs.coeff(0)), 6);  // First row, 3nd column

      // The untranslated column.
      const auto& vf = crc.flex_subrows[flex_row_index];

      ASSERT_EQ(vf.size(), 1);
      ASSERT_TRUE(vf[0] == 4);   // First row, 1st column
    }

    ++it;
    ASSERT_TRUE(it.done());
  }

  void test_large_deal() {

    // Construct a complicated one and make sure we get the same
    // results as the row slicers do.

    size_t n = 100;
    std::string run_string = "CSncvdnsss";

    sframe raw_data = make_random_sframe(n, run_string, false);

    v2::ml_data data;

    // Set columns 0 and 3 as untranslated; make these special cased for that
    data.set_data(raw_data, "", {},
                  { {raw_data.column_name(0),  v2::ml_column_mode::UNTRANSLATED},
                    {raw_data.column_name(3),  v2::ml_column_mode::UNTRANSLATED} });


    data.fill();


    // Now set up a bunch of random row slicers on the sparse and dense compononents
    auto comp_spec = std::make_shared<v2::composite_row_specification>(data.metadata());

    std::vector<v2::row_slicer> row_slicers;

    std::vector<size_t> sparse_row_indices;
    std::vector<size_t> dense_row_indices;

    std::vector<std::vector<size_t> > column_sets = {
        {1, 2, 6, 7},
        {8, 9},
        {5},
        {5,6,7,8,9},
        {1,4,7,9}};

    for(std::vector<size_t> columns : column_sets) {

      row_slicers.emplace_back(data.metadata(), columns);
      sparse_row_indices.push_back(comp_spec->add_sparse_subrow(columns));
      dense_row_indices.push_back(comp_spec->add_dense_subrow(columns));
    }

    // Now do the same with the untranslated columns

    std::vector<v2::row_slicer> flex_row_slicers;
    std::vector<size_t> flex_row_indices;

    std::vector<std::vector<size_t> > flex_column_sets = { {0}, {3}, {0,3} };

    for(std::vector<size_t> columns : flex_column_sets) {

      flex_row_slicers.emplace_back(data.metadata(), columns);
      flex_row_indices.push_back(comp_spec->add_flex_type_subrow(columns));
    }

    v2::composite_row_container crc(comp_spec);

    std::vector<v2::ml_data_entry> x_t;
    std::vector<flexible_type> x_u;

    v2::dense_vector vd;
    v2::sparse_vector vs;
    std::vector<flexible_type> vf;


    for(auto it = data.get_iterator(); !it.done(); ++it) {

      it.fill_observation(x_t);
      it.fill_untranslated_values(x_u);

      it.fill_observation(crc);

      // Now, test them!  Are they the same?
      for(size_t i = 0; i < row_slicers.size(); ++i) {

        row_slicers[i].slice(vd, x_t, x_u);
        ASSERT_TRUE(vd == crc.dense_subrows[dense_row_indices[i]]);

        row_slicers[i].slice(vs, x_t, x_u);
        ASSERT_TRUE(vs.toDense() == crc.sparse_subrows[sparse_row_indices[i]].toDense());
      }

      // Now, do the same with the untranslated columns
      for(size_t i = 0; i < flex_row_slicers.size(); ++i) {

        flex_row_slicers[i].slice(vf, x_t, x_u);
        ASSERT_TRUE(vf == crc.flex_subrows[flex_row_indices[i]]);
      }

    }

  }
};

BOOST_FIXTURE_TEST_SUITE(_test_composite_rows, test_composite_rows)
BOOST_AUTO_TEST_CASE(test_composite_rows_simple) {
  test_composite_rows::test_composite_rows_simple();
}
BOOST_AUTO_TEST_CASE(test_large_deal) {
  test_composite_rows::test_large_deal();
}
BOOST_AUTO_TEST_SUITE_END()

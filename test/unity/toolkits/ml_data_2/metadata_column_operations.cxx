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
#include <toolkits/ml_data_2/ml_data_entry.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>

// Testing utils common to all of ml_data_iterator
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <toolkits/ml_data_2/testing_utils.hpp>

#include <core/globals/globals.hpp>

using namespace turi;

struct test_metadata_column_selection  {
 public:

  void test_basic_1() {
    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 7);

    sframe X = make_integer_testing_sframe( {"C1", "C2"}, { {0, 1}, {2, 3}, {4, 5} } );

    v2::ml_data data;
    data.fill(X);

    v2::ml_data data_2(data.metadata()->select_columns({"C2", "C1"}));
    data_2.fill(X);

    std::vector<v2::ml_data_full_entry> x1, x2;

    ASSERT_EQ(data_2.metadata()->num_dimensions(),
              data.metadata()->index_size("C2") + data.metadata()->index_size("C1"));

    ASSERT_EQ(data_2.metadata()->num_untranslated_columns(), 0);

    ////////////////////////////////////////

    auto it_1 = data.get_iterator();
    auto it_2 = data_2.get_iterator();


    for(; !it_1.done(); ++it_1, ++it_2) {

      it_1.fill_observation(x1);

      ASSERT_EQ(x1.size(), 2);
      ASSERT_EQ(x1[0].column_index, 0);
      ASSERT_EQ(x1[1].column_index, 1);

      ASSERT_EQ(x1[0].global_index, 0);
      ASSERT_EQ(x1[1].global_index, 1);

      it_2.fill_observation(x2);

      ASSERT_EQ(x2.size(), 2);
      ASSERT_EQ(x2[0].column_index, 0);
      ASSERT_EQ(x2[1].column_index, 1);

      // These are reversed
      ASSERT_EQ(x2[0].global_index, 1);
      ASSERT_EQ(x2[1].global_index, 0);

      ASSERT_EQ(x2[0].value, x1[1].value);
      ASSERT_EQ(x2[1].value, x1[0].value);
    }

    ASSERT_TRUE(it_1.done());
    ASSERT_TRUE(it_2.done());
  }



  void test_basic_2_side_features() {
    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 7);

    sframe X = make_integer_testing_sframe( {"C1", "C2"}, { {0, 1}, {2, 3}, {4, 5} } );

    sframe X2 = make_integer_testing_sframe( {"C1", "S1"}, { {0, 0}, {2, 20}, {4, 40} } );

    sframe X3 = make_integer_testing_sframe( {"C2", "S2"}, { {1, 11}, {3, 13}, {5, 15} } );

    v2::ml_data data({{"integer_columns_categorical_by_default", true}});
    data.set_data(X);
    data.add_side_data(X2);
    data.add_side_data(X3);
    data.fill();

    v2::ml_data data_2(data.metadata()->select_columns({"C2", "C1"}));
    data_2.fill(X);

    std::vector<v2::ml_data_full_entry> x1, x2;

    ASSERT_EQ(data_2.metadata()->num_dimensions(),
              data.metadata()->index_size("C2")
              + data.metadata()->index_size("C1")
              + data.metadata()->index_size("S1")
              + data.metadata()->index_size("S2"));

    ASSERT_EQ(data_2.metadata()->num_untranslated_columns(), 0);

    ////////////////////////////////////////

    auto it_1 = data.get_iterator();
    auto it_2 = data_2.get_iterator();


    for(size_t i = 0; !it_1.done(); ++it_1, ++it_2, ++i) {

      it_1.fill_observation(x1);

      ASSERT_EQ(x1.size(), 4);
      ASSERT_EQ(x1[0].column_index, 0);
      ASSERT_EQ(x1[1].column_index, 1);
      ASSERT_EQ(x1[2].column_index, 2);
      ASSERT_EQ(x1[3].column_index, 3);

      it_2.fill_observation(x2);

      ASSERT_EQ(x2.size(), 4);
      ASSERT_EQ(x2[0].column_index, 0);
      ASSERT_EQ(x2[1].column_index, 1);
      ASSERT_EQ(x2[2].column_index, 2);
      ASSERT_EQ(x2[3].column_index, 3);

      // These are reversed
      ASSERT_EQ(x1[0].global_index, x2[1].global_index);
      ASSERT_EQ(x1[1].global_index, x2[0].global_index);
      ASSERT_EQ(x1[2].global_index, x2[3].global_index);
      ASSERT_EQ(x1[3].global_index, x2[2].global_index);
    }

    ASSERT_TRUE(it_1.done());
    ASSERT_TRUE(it_2.done());
  }

};

BOOST_FIXTURE_TEST_SUITE(_test_metadata_column_selection, test_metadata_column_selection)
BOOST_AUTO_TEST_CASE(test_basic_1) {
  test_metadata_column_selection::test_basic_1();
}
BOOST_AUTO_TEST_CASE(test_basic_2_side_features) {
  test_metadata_column_selection::test_basic_2_side_features();
}
BOOST_AUTO_TEST_SUITE_END()

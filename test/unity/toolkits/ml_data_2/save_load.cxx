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
#include <cmath>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <model_server/lib/flex_dict_view.hpp>

// ML-Data Utils
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <core/storage/sframe_data/sframe_iterators.hpp>
#include <toolkits/ml_data_2/ml_data_entry.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>

// Testing utils common to all of ml_data
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>

// Testing utils common to all of ml_data
#include <toolkits/ml_data_2/testing_utils.hpp>

using namespace turi;

struct test_save_load  {

 public:
  ////////////////////////////////////////////////////////////////////////////////

  void _run_save_load_test(size_t n, const std::string& run_string) {

    sframe raw_data;
    v2::ml_data data;

    std::tie(raw_data, data) = v2::make_random_sframe_and_v2::ml_data(n, run_string);

    ASSERT_EQ(n, raw_data.size());
    ASSERT_EQ(n, data.size());

    ////////////////////////////////////////
    v2::ml_data saved_data;







    saved_data.metadata = data.metadata;
    saved_data.target_metadata = data.target_metadata;


    {
      // Save it
      dir_archive archive_write;
      archive_write.open_directory_for_write("ml_data_test");

      turi::oarchive oarc(archive_write);

      oarc << data;

      archive_write.close();

      // Load it
      dir_archive archive_read;
      archive_read.open_directory_for_read("ml_data_test");

      turi::iarchive iarc(archive_read);

      iarc >> saved_data;
    }

    ASSERT_EQ(saved_data.size(), data.size());

    v2::ml_data_iterator it_1(data);
    v2::ml_data_iterator it_2(saved_data);
    parallel_sframe_iterator it_raw(raw_data);

    for(; !it_1.done(); ++it_1, ++it_2, ++it_raw) {
      std::vector<v2::ml_data_entry> x_1;
      std::vector<v2::ml_data_entry> x_2;

      it_1.fill_observation(x_1, false);
      it_2.fill_observation(x_2, false);

      DASSERT_TRUE(x_1 == x_2);

      std::vector<flexible_type> row_1 = it_1._testing_extract_current_row();
      ASSERT_EQ(row_1.size(), run_string.size());

      for(size_t i = 0; i < run_string.size(); ++i) {
        ASSERT_TRUE(row_1[i] == it_raw.value(i));
      }

      std::vector<flexible_type> row_2 = it_2._testing_extract_current_row();
      ASSERT_EQ(row_2.size(), run_string.size());

      for(size_t i = 0; i < run_string.size(); ++i) {
        ASSERT_TRUE(row_2[i] == it_raw.value(i));
      }
    }

    DASSERT_TRUE(it_2.done());
  }

  void test_save_and_load_1() {
    _run_save_load_test(3, "c");
  }

  void test_save_and_load_2() {
    _run_save_load_test(100, "c");
  }

  void test_save_and_load_3() {
    _run_save_load_test(10, "cdD");
  }

  void test_save_and_load_4() {
    _run_save_load_test(1000, "n");
  }

  void test_save_and_load_5() {
    _run_save_load_test(0, "cccccccccccccccccccccccccccc");
  }

  void test_save_load_4() {
    // One more than is needed for deterministic adding
    _run_save_load_test(50001, "Cc");
  }

  void test_save_load_5d() {
    // Varying rows -- dict
    _run_save_load_test(500, "ccD");
  }

  void test_save_load_5v() {
    // Varying rows -- vect
    _run_save_load_test(500, "ccv");
  }

  void test_save_load_5u() {
    // Varying rows -- cat vect
    _run_save_load_test(500, "ccu");
  }

  void test_save_load_5Dvu() {
    // Large, varying rows
    _run_save_load_test(500, "ccDvu");
  }

  void test_save_load_5vud() {
    // Varying rows -- varying all
    _run_save_load_test(500, "ccvuD");
  }

};

BOOST_FIXTURE_TEST_SUITE(_test_save_load, test_save_load)
BOOST_AUTO_TEST_CASE(test_save_and_load_1) {
  test_save_load::test_save_and_load_1();
}
BOOST_AUTO_TEST_CASE(test_save_and_load_2) {
  test_save_load::test_save_and_load_2();
}
BOOST_AUTO_TEST_CASE(test_save_and_load_3) {
  test_save_load::test_save_and_load_3();
}
BOOST_AUTO_TEST_CASE(test_save_and_load_4) {
  test_save_load::test_save_and_load_4();
}
BOOST_AUTO_TEST_CASE(test_save_and_load_5) {
  test_save_load::test_save_and_load_5();
}
BOOST_AUTO_TEST_CASE(test_save_load_4) {
  test_save_load::test_save_load_4();
}
BOOST_AUTO_TEST_CASE(test_save_load_5d) {
  test_save_load::test_save_load_5d();
}
BOOST_AUTO_TEST_CASE(test_save_load_5v) {
  test_save_load::test_save_load_5v();
}
BOOST_AUTO_TEST_CASE(test_save_load_5u) {
  test_save_load::test_save_load_5u();
}
BOOST_AUTO_TEST_CASE(test_save_load_5Dvu) {
  test_save_load::test_save_load_5Dvu();
}
BOOST_AUTO_TEST_CASE(test_save_load_5vud) {
  test_save_load::test_save_load_5vud();
}
BOOST_AUTO_TEST_SUITE_END()

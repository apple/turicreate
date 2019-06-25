#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>
#include <random>
#include <core/data/sframe/gl_sarray.hpp>
#include <toolkits/feature_engineering/content_interpretation.cpp>
#include <core/data/flexible_type/flexible_type.hpp>

using namespace turi;
using namespace turi::feature_engineering;

flex_list long_text_data = {
  "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.",
  "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."};


flex_list short_text_data = {
  "Lorem ipsum dolor sit amet",
  "consectetur adipiscing elit",
  "sed do eiusmod tempor incididunt",
  "ut labore et dolore magna aliqua."
  "Ut enim ad minim veniam, quis"
  "nostrud exercitation ullamco"
  "laboris nisi ut aliquip ex"
  "ea commodo consequat.",
  "Duis aute irure dolor in reprehenderit"
  "in voluptate velit esse cillum dolore"
  "eu fugiat nulla pariatur."
  "Excepteur sint occaecat"
  "cupidatat non proident's"
  "sunt in culpa qui officia"
  "deserunt mollit anim id est laborum."};

flex_list categorical_text_data = {
  "Series",
  "Series",
  "MadeForTV",
  "Series",
  "Movie",
  "MadeForTV",
  "MadeForTV",
  "Drama",
  "Drama",
  "Movie",
  "Movie",
  "BajoranHolodeckNoir"};

flex_list dict_data = {flex_dict{{"one", 1},
                                 {"two", 2},
                        {"three", 3}},
                       flex_dict{{"one", 1},
                                 {"four", 4}}};

flex_list categorical_list_data = {
    flex_list{"cat1", "cat2"},
    flex_list{"cat5e"},
    flex_list{"mycat", "cat2"},
    flex_list{"lion", "cat6"} };

flex_list vector_data = {
  flex_vec{0.1, 0.2, 0.3, 42},
  flex_vec{0.1, 0.2, 0.3, 43},
  flex_vec{0.1, 0.3, 0.3, 42},
  flex_vec{0.1, 0.5, 0.3, 44}};



struct content_interpretation_test  {
 public:
  void test_long_text_1() {
    std::string true_interpretation = "long_text";

    gl_sarray data_1(long_text_data);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_1), true_interpretation);

    auto long_text_data_copy = long_text_data;
    long_text_data_copy.push_back(FLEX_UNDEFINED);

    gl_sarray data_2(long_text_data_copy);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_2), true_interpretation);
  }

  void test_short_text_1() {
    std::string true_interpretation = "short_text";

    gl_sarray data_1(short_text_data);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_1), true_interpretation);

    auto short_text_data_copy = short_text_data;
    short_text_data_copy.push_back(FLEX_UNDEFINED);

    gl_sarray data_2(short_text_data_copy);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_2), true_interpretation);
  }

  void test_categorical_text_data_1() {
    std::string true_interpretation = "categorical";

    gl_sarray data_1(categorical_text_data);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_1), true_interpretation);

    auto categorical_text_data_copy = categorical_text_data;
    categorical_text_data_copy.push_back(FLEX_UNDEFINED);

    gl_sarray data_2(categorical_text_data_copy);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_2), true_interpretation);
  }

  void test_dict_1() {
    std::string true_interpretation = "sparse_vector";

    gl_sarray data_1(dict_data);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_1), true_interpretation);

    auto dict_data_copy = dict_data;
    dict_data_copy.push_back(FLEX_UNDEFINED);

    gl_sarray data_2(dict_data_copy);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_2), true_interpretation);
  }

  void test_categorical_list_1() {
    std::string true_interpretation = "categorical";

    gl_sarray data_1(categorical_list_data);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_1), true_interpretation);

    auto categorical_list_data_copy = categorical_list_data;
    categorical_list_data_copy.push_back(FLEX_UNDEFINED);

    gl_sarray data_2(categorical_list_data_copy);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_2), true_interpretation);
  }

  void test_vector_1() {
    std::string true_interpretation = "vector";

    gl_sarray data_1(vector_data);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_1), true_interpretation);

    auto vector_data_copy = vector_data;
    vector_data_copy.push_back(FLEX_UNDEFINED);

    gl_sarray data_2(vector_data_copy);
    TS_ASSERT_EQUALS(infer_content_interpretation(data_2), true_interpretation);
  }


};

BOOST_FIXTURE_TEST_SUITE(_content_interpretation_test, content_interpretation_test)
BOOST_AUTO_TEST_CASE(test_long_text_1) {
  content_interpretation_test::test_long_text_1();
}
BOOST_AUTO_TEST_CASE(test_short_text_1) {
  content_interpretation_test::test_short_text_1();
}
BOOST_AUTO_TEST_CASE(test_categorical_text_data_1) {
  content_interpretation_test::test_categorical_text_data_1();
}
BOOST_AUTO_TEST_CASE(test_dict_1) {
  content_interpretation_test::test_dict_1();
}
BOOST_AUTO_TEST_CASE(test_categorical_list_1) {
  content_interpretation_test::test_categorical_list_1();
}
BOOST_AUTO_TEST_CASE(test_vector_1) {
  content_interpretation_test::test_vector_1();
}
BOOST_AUTO_TEST_SUITE_END()

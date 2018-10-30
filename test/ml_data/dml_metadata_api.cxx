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


// SFrame and Flex type
#include <unity/lib/flex_dict_view.hpp>
#include <random/random.hpp>

// ML-Data Utils
#include <ml_data/ml_data.hpp>

// Testing utils common to all of ml_data_iterator
#include <ml_data/testing_utils.hpp>
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>

#include <globals/globals.hpp>

using namespace turi;

BOOST_TEST_DONT_PRINT_LOG_VALUE(turi::ml_column_mode)

struct test_metadata  {
 public:

  void test_basic_1() {

    std::vector<std::vector<flexible_type> > raw_data =
        {{"0",
          "ut0",
          10,
          10.0,
          flex_vec{1.0, 10.1},
          flex_list{"1", "2"},
          flex_dict{ {"8", 1}, {"3", 2} },
          flex_nd_vec({1,2,3,4}, {}, 5)
         },

         {"1",
          "ut1",
          11,
          11.0,
          flex_vec{2.0, 11.1},
          flex_list{"2", "3"},
          flex_dict{ {"8", 1}, {"4", 2}},
          flex_nd_vec({1,2,3,4}, {}, 6)
         }
        };

    std::vector<std::string> names = {
      "string",
      "untranslated_string",
      "int",
      "float",
      "vec",
      "list",
      "dict",
      "ndarray"};

    sframe data_sf = make_testing_sframe(names, raw_data);

    ml_data X;
    X.fill(data_sf, "", { {"untranslated_string", ml_column_mode::UNTRANSLATED} });

    std::shared_ptr<ml_metadata> m1 = X.metadata();

    // Also try a saved and loaded one.
    std::shared_ptr<ml_metadata> m2;
    save_and_load_object(m2, m1);

    m1->_debug_is_equal(m2);


    for(auto m : {m1, m2}) {

      // Check to make sure that everything is correct

      TS_ASSERT(!m->has_target());

      TS_ASSERT_EQUALS(m->num_columns(), 8);
      TS_ASSERT_EQUALS(m->num_untranslated_columns(), 1);
      TS_ASSERT(m->has_untranslated_columns());

      TS_ASSERT_EQUALS(m->column_names().size(), 8);

      {
        bool include_untranslated_columns = true;
        TS_ASSERT_EQUALS(m->num_columns(include_untranslated_columns), 8);
      }

      {
        bool include_untranslated_columns = false;
        TS_ASSERT_EQUALS(m->num_columns(include_untranslated_columns), 7);
      }

      size_t global_index_offset = 0;

      {
        // The string column
        TS_ASSERT(m->column_name(0) == "string");
        TS_ASSERT(m->column_names()[0] == "string");
        TS_ASSERT(m->column_index("string") == 0);

        TS_ASSERT(m->is_indexed(0));
        TS_ASSERT(m->is_indexed("string"));

        TS_ASSERT(m->indexer("string").get() == m->indexer(0).get());

        TS_ASSERT(m->statistics("string").get() == m->statistics(0).get());

        // 2 unique entries
        TS_ASSERT_EQUALS(m->index_size(0), 2);
        TS_ASSERT_EQUALS(m->index_size("string"), 2);
        global_index_offset += 2;

        // 2 unique entries
        TS_ASSERT_EQUALS(m->global_index_offset(0), 0);
        TS_ASSERT_EQUALS(m->global_index_offset("string"), 0);

        TS_ASSERT(m->is_categorical(0));
        TS_ASSERT(m->is_categorical("string"));

        TS_ASSERT(!m->is_untranslated_column(0));
        TS_ASSERT(!m->is_untranslated_column("string"));

        TS_ASSERT_EQUALS(m->column_mode(0), ml_column_mode::CATEGORICAL);
        TS_ASSERT_EQUALS(m->column_mode("string"), ml_column_mode::CATEGORICAL);

        TS_ASSERT_EQUALS(m->column_type(0), flex_type_enum::STRING);
        TS_ASSERT_EQUALS(m->column_type("string"), flex_type_enum::STRING);
      }

      {
        // The untranslated string column
        TS_ASSERT(m->column_name(1) == "untranslated_string");
        TS_ASSERT(m->column_names()[1] == "untranslated_string");
        TS_ASSERT(m->column_index("untranslated_string") == 1);

        TS_ASSERT(!m->is_indexed(1));
        TS_ASSERT(!m->is_indexed("untranslated_string"));

        // nothing here; it's untranslated
        TS_ASSERT_EQUALS(m->index_size(1), 0);
        TS_ASSERT_EQUALS(m->index_size("untranslated_string"), 0);
        global_index_offset += 0;

        TS_ASSERT(!m->is_categorical(1));
        TS_ASSERT(!m->is_categorical("untranslated_string"));

        TS_ASSERT(m->is_untranslated_column(1));
        TS_ASSERT(m->is_untranslated_column("untranslated_string"));

        TS_ASSERT_EQUALS(m->column_mode(1), ml_column_mode::UNTRANSLATED);
        TS_ASSERT_EQUALS(m->column_mode("untranslated_string"), ml_column_mode::UNTRANSLATED);

        TS_ASSERT_EQUALS(m->column_type(1), flex_type_enum::STRING);
        TS_ASSERT_EQUALS(m->column_type("untranslated_string"), flex_type_enum::STRING);
      }

      {
        // The int column
        TS_ASSERT(m->column_name(2) == "int");
        TS_ASSERT(m->column_names()[2] == "int");
        TS_ASSERT(m->column_index("int") == 2);

        TS_ASSERT(!m->is_indexed(2));
        TS_ASSERT(!m->is_indexed("int"));

        TS_ASSERT(m->indexer("int").get() == m->indexer(2).get());

        TS_ASSERT(m->statistics("int").get() == m->statistics(2).get());

        // 1 category
        TS_ASSERT_EQUALS(m->index_size(2), 1);
        TS_ASSERT_EQUALS(m->index_size("int"), 1);

        TS_ASSERT_EQUALS(m->global_index_offset(2), global_index_offset);
        TS_ASSERT_EQUALS(m->global_index_offset("int"), global_index_offset);

        global_index_offset += 1;

        TS_ASSERT(!m->is_categorical(2));
        TS_ASSERT(!m->is_categorical("int"));

        TS_ASSERT(!m->is_untranslated_column(2));
        TS_ASSERT(!m->is_untranslated_column("int"));

        TS_ASSERT_EQUALS(m->column_mode(2), ml_column_mode::NUMERIC);
        TS_ASSERT_EQUALS(m->column_mode("int"), ml_column_mode::NUMERIC);

        TS_ASSERT_EQUALS(m->column_type(2), flex_type_enum::INTEGER);
        TS_ASSERT_EQUALS(m->column_type("int"), flex_type_enum::INTEGER);
      }

      {
        // The float column
        TS_ASSERT(m->column_name(3) == "float");
        TS_ASSERT(m->column_names()[3] == "float");
        TS_ASSERT(m->column_index("float") == 3);

        TS_ASSERT(!m->is_indexed(3));
        TS_ASSERT(!m->is_indexed("float"));

        TS_ASSERT(m->indexer("float").get() == m->indexer(3).get());

        TS_ASSERT(m->statistics("float").get() == m->statistics(3).get());

        // 1 category
        TS_ASSERT_EQUALS(m->index_size(3), 1);
        TS_ASSERT_EQUALS(m->index_size("float"), 1);

        TS_ASSERT_EQUALS(m->global_index_offset(3), global_index_offset);
        TS_ASSERT_EQUALS(m->global_index_offset("float"), global_index_offset);

        global_index_offset += 1;

        TS_ASSERT(!m->is_categorical(3));
        TS_ASSERT(!m->is_categorical("float"));

        TS_ASSERT(!m->is_untranslated_column(3));
        TS_ASSERT(!m->is_untranslated_column("float"));

        TS_ASSERT_EQUALS(m->column_mode(3), ml_column_mode::NUMERIC);
        TS_ASSERT_EQUALS(m->column_mode("float"), ml_column_mode::NUMERIC);

        TS_ASSERT_EQUALS(m->column_type(3), flex_type_enum::FLOAT);
        TS_ASSERT_EQUALS(m->column_type("float"), flex_type_enum::FLOAT);
      }

      {
        // The vec column
        TS_ASSERT(m->column_name(4) == "vec");
        TS_ASSERT(m->column_names()[4] == "vec");
        TS_ASSERT(m->column_index("vec") == 4);

        TS_ASSERT(!m->is_indexed(4));
        TS_ASSERT(!m->is_indexed("vec"));

        TS_ASSERT(m->indexer("vec").get() == m->indexer(4).get());

        TS_ASSERT(m->statistics("vec").get() == m->statistics(4).get());

        // 2 dimensional array
        TS_ASSERT_EQUALS(m->index_size(4), 2);
        TS_ASSERT_EQUALS(m->index_size("vec"), 2);

        // 4 unique entries
        TS_ASSERT_EQUALS(m->global_index_offset(4), global_index_offset);
        TS_ASSERT_EQUALS(m->global_index_offset("vec"), global_index_offset);

        global_index_offset += 2;

        TS_ASSERT(!m->is_categorical(4));
        TS_ASSERT(!m->is_categorical("vec"));

        TS_ASSERT(!m->is_untranslated_column(4));
        TS_ASSERT(!m->is_untranslated_column("vec"));

        TS_ASSERT_EQUALS(m->column_mode(4), ml_column_mode::NUMERIC_VECTOR);
        TS_ASSERT_EQUALS(m->column_mode("vec"), ml_column_mode::NUMERIC_VECTOR);

        TS_ASSERT_EQUALS(m->column_type(4), flex_type_enum::VECTOR);
        TS_ASSERT_EQUALS(m->column_type("vec"), flex_type_enum::VECTOR);
      }

      {
        // The list column
        TS_ASSERT(m->column_name(5) == "list");
        TS_ASSERT(m->column_names()[5] == "list");
        TS_ASSERT(m->column_index("list") == 5);

        TS_ASSERT(m->is_indexed(5));
        TS_ASSERT(m->is_indexed("list"));

        TS_ASSERT(m->indexer("list").get() == m->indexer(5).get());

        TS_ASSERT(m->statistics("list").get() == m->statistics(5).get());

        // 3 unique categorical keys
        TS_ASSERT_EQUALS(m->index_size(5), 3);
        TS_ASSERT_EQUALS(m->index_size("list"), 3);

        // 5 unique entries
        TS_ASSERT_EQUALS(m->global_index_offset(5), global_index_offset);
        TS_ASSERT_EQUALS(m->global_index_offset("list"), global_index_offset);

        global_index_offset += 3;

        TS_ASSERT(m->is_categorical(5));
        TS_ASSERT(m->is_categorical("list"));

        TS_ASSERT(!m->is_untranslated_column(5));
        TS_ASSERT(!m->is_untranslated_column("list"));

        TS_ASSERT_EQUALS(m->column_mode(5), ml_column_mode::CATEGORICAL_VECTOR);
        TS_ASSERT_EQUALS(m->column_mode("list"), ml_column_mode::CATEGORICAL_VECTOR);

        TS_ASSERT_EQUALS(m->column_type(5), flex_type_enum::LIST);
        TS_ASSERT_EQUALS(m->column_type("list"), flex_type_enum::LIST);
      }

      {
        // The dict column
        TS_ASSERT(m->column_name(6) == "dict");
        TS_ASSERT(m->column_names()[6] == "dict");
        TS_ASSERT(m->column_index("dict") == 6);

        TS_ASSERT(m->is_indexed(6));
        TS_ASSERT(m->is_indexed("dict"));

        TS_ASSERT(m->indexer("dict").get() == m->indexer(6).get());

        TS_ASSERT(m->statistics("dict").get() == m->statistics(6).get());

        // 3 unique categorical keys
        TS_ASSERT_EQUALS(m->index_size(6), 3);
        TS_ASSERT_EQUALS(m->index_size("dict"), 3);

        // 6 unique entries
        TS_ASSERT_EQUALS(m->global_index_offset(6), global_index_offset);
        TS_ASSERT_EQUALS(m->global_index_offset("dict"), global_index_offset);

        global_index_offset += 3;

        // Dicts are not categoricals
        TS_ASSERT(!m->is_categorical(6));
        TS_ASSERT(!m->is_categorical("dict"));

        TS_ASSERT(!m->is_untranslated_column(6));
        TS_ASSERT(!m->is_untranslated_column("dict"));

        TS_ASSERT_EQUALS(m->column_mode(6), ml_column_mode::DICTIONARY);
        TS_ASSERT_EQUALS(m->column_mode("dict"), ml_column_mode::DICTIONARY);

        TS_ASSERT_EQUALS(m->column_type(6), flex_type_enum::DICT);
        TS_ASSERT_EQUALS(m->column_type("dict"), flex_type_enum::DICT);
      }
      {
        // The dict column
        TS_ASSERT(m->column_name(7) == "ndarray");
        TS_ASSERT(m->column_names()[7] == "ndarray");
        TS_ASSERT(m->column_index("ndarray") == 7);

        TS_ASSERT(!m->is_indexed(7));
        TS_ASSERT(!m->is_indexed("ndarray"));

        TS_ASSERT(m->indexer("ndarray").get() == m->indexer(7).get());

        TS_ASSERT(m->statistics("ndarray").get() == m->statistics(7).get());

        // 3 unique categorical keys
        TS_ASSERT_EQUALS(m->index_size(7), 1*2*3*4);
        TS_ASSERT_EQUALS(m->index_size("ndarray"), 1*2*3*4);

        // 6 unique entries
        TS_ASSERT_EQUALS(m->global_index_offset(7), global_index_offset);
        TS_ASSERT_EQUALS(m->global_index_offset("ndarray"), global_index_offset);

        global_index_offset += (1*2*3*4);

        // Dicts are not categoricals
        TS_ASSERT(!m->is_categorical(7));
        TS_ASSERT(!m->is_categorical("ndarray"));

        TS_ASSERT(!m->is_untranslated_column(7));
        TS_ASSERT(!m->is_untranslated_column("ndarray"));

        TS_ASSERT_EQUALS(m->column_mode(7), ml_column_mode::NUMERIC_ND_VECTOR);
        TS_ASSERT_EQUALS(m->column_mode("ndarray"), ml_column_mode::NUMERIC_ND_VECTOR);

        TS_ASSERT_EQUALS(m->column_type(7), flex_type_enum::ND_VECTOR);
        TS_ASSERT_EQUALS(m->column_type("ndarray"), flex_type_enum::ND_VECTOR);

        auto shape = flex_nd_vec::index_range_type{1, 2, 3, 4};
        TS_ASSERT(m->nd_column_shape(7) == shape);
        TS_ASSERT(m->nd_column_shape("ndarray") == shape);
      }
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_test_metadata, test_metadata)
BOOST_AUTO_TEST_CASE(test_basic_1) {
  test_metadata::test_basic_1();
}
BOOST_AUTO_TEST_SUITE_END()

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <unistd.h>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <toolkits/feature_engineering/dict_transform_utils.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <iostream>

using namespace turi;

/**
 * Main test driver
 * -----------------------------------------------------------------------------
 */
struct dictionary_flatting_test  {

 public:

  void _test_equality(const flexible_type& in, const flexible_type& true_out) {

    flexible_type out = to_flat_dict(in, ".", "__undefined__","ignore", "ignore");

    if(out != true_out) {
      std::cout << "\nout = " << out << std::endl;
      std::cout << "true_out = " << true_out << std::endl;

    }

    DASSERT_TRUE(out == true_out);
  }

  /**
   * Test that models are initialized properly with the default settings.
   */
  void test_unity_preserving() {
    flex_dict d = { {"a", 1}, {"b", 2} };
    _test_equality(d, d);
  }

  void test_nested_1() {
    flex_dict in = { {"a", flex_dict{ {"b", 3} } }, {"c", 2} };
    flex_dict out = { {"a.b", 3}, {"c", 2} };
    _test_equality(in, out);
  }

  void test_nested_2() {
    flex_dict in = { {"a", flex_dict{ {"b", 3}, {"c", 2.5} } }, {"c", 2} };
    flex_dict out = { {"a.b", 3}, {"a.c", 2.5}, {"c", 2} };
    _test_equality(in, out);
  }

  void test_nested_vect() {
    flex_dict in = { {"a", flex_vec{1,2,4} }, {"c", 2} };
    flex_dict out = { {"a.0", 1}, {"a.1", 2},{"a.2", 4}, {"c", 2} };
    _test_equality(in, out);
  }

  void test_nested_string() {
    flex_dict in = { {"a", "b"}, {"c", 2} };
    flex_dict out = { {"a.b", 1}, {"c", 2} };
    _test_equality(in, out);
  }

  void test_nested_undefined() {
    flex_dict in = { {"a", FLEX_UNDEFINED}, {"c", 2} };
    flex_dict out = { {"a.__undefined__", 1}, {"c", 2} };
    _test_equality(in, out);
  }

  void test_nested_list() {
    flex_dict in = { {"a", flex_list{"a", "b", 1.5} }, {"c", 2} };
    flex_dict out = { {"a.0.a", 1}, {"a.1.b", 1}, {"a.2", 1.5}, {"c", 2} };
    _test_equality(in, out);
  }

  void test_string() {
    flex_string in = "a";
    flex_dict out = { {"a", 1} };
    _test_equality(in, out);
  }

  void test_integer() {
    flex_int in = 1;
    flex_dict out = { {"0", 1} };
    _test_equality(in, out);
  }

  void test_datetime_ignore() {
    flex_dict in = { {"a", flex_list{"a", "b", 1.5} }, {"d", flex_date_time()}, {"c", 2} };
    flex_dict out = { {"a.0.a", 1}, {"a.1.b", 1}, {"a.2", 1.5}, {"c", 2} };
    _test_equality(in, out);
  }

  void test_datetime_ignore_2() {
    flex_dict in = { {"a", flex_list{"a", "b", flex_date_time(), 1.5} }, {"c", 2} };
    flex_dict out = { {"a.0.a", 1}, {"a.1.b", 1}, {"a.3", 1.5}, {"c", 2} };
    _test_equality(in, out);
  }


  void test_nesting_1() {
    flex_dict in = { {"a", flex_dict{ {"b", flex_vec{0, 1} } } } };
    flex_dict out = { {"a.b.0", 0}, {"a.b.1", 1} };
    _test_equality(in, out);
  }

  void test_nesting_2() {
    flex_dict in = { {"a", flex_dict{ {"b", flex_list{flex_string{"c"}, 1} } } } };
    flex_dict out = { {"a.b.0.c", 1}, {"a.b.1", 1} };
    _test_equality(in, out);
  }

  void test_nesting_3() {
    flex_dict in = { {"a", flex_list{5} },
                     {"c", flex_dict{ {"d", 4} } } };
    flex_dict out = { {"a.0", 5},
                      {"c.d", 4} };

    _test_equality(in, out);
  }

  void test_deep_full_recursion() {

    flex_dict s = { {"a", 1} };
    flex_string key = "a";

    for(size_t i = 0; i < 64; ++i) {
      s = flex_dict{ {"a",  s} };
      key = "a." + key;
    }

    flex_dict out = { {key, 1} };

    _test_equality(s, out);
  }

  flexible_type _add_nested_component(flex_dict& final_out, std::string key_root, size_t depth) {

    size_t t = random::fast_uniform<int>(0, 6);

    if(depth < 1) {
      t = t % 4;
    }

    switch(t) {
      case 0: {
        // integer
        flex_int n = random::fast_uniform<int>(0, 100);
        final_out.push_back( {key_root, n});
        return n;
      }
      case 1: {
        // float
        flex_float n = random::fast_uniform<double>(0, 1);
        final_out.push_back( {key_root, n});
        return n;
      }
      case 2: {
        // string
        flex_string n = std::to_string(random::fast_uniform<int>(0, 1000));
        final_out.push_back( {key_root + "." + n, 1});
        return n;
      }
      case 3: {
        // vector
        size_t length = random::fast_uniform<size_t>(0, 10);
        flex_vec v(length);

        for(size_t i = 0; i < length; ++i) {
          v[i] = random::fast_uniform<double>(0, 1);
          final_out.push_back({key_root + "." + std::to_string(i), v[i]});
        }

        return v;
      }

      case 4: {
        // list
        size_t length = random::fast_uniform<size_t>(0, 10);
        flex_list v(length);

        for(size_t i = 0; i < length; ++i) {
          v[i] = _add_nested_component(final_out, key_root + "." + std::to_string(i), depth - 1);
        }

        return v;
      }

      case 5: {
        // dict
        size_t length = random::fast_uniform<size_t>(0, 10);
        flex_dict d(length);

        for(size_t i = 0; i < length; ++i) {
          flex_string s = "key-" + std::to_string(i) + "-"
              + std::to_string(random::fast_uniform<int>(0,1000));

          d[i] = {s, _add_nested_component(final_out, key_root + "." + s, depth - 1)};
        }

        return d;
      }
      case 6:
      default: {
        final_out.push_back({key_root + ".__undefined__", 1});
        return FLEX_UNDEFINED;
      }
    }
  }

  void run_deep_random_test(size_t depth) {
    random::seed(0);

    flex_dict true_out;

    flex_dict d(20);
    for(size_t i = 0; i < 20; ++i) {
      auto& p = d[i];
      p.first = "a" + std::to_string(i) + "-"
          + std::to_string(random::fast_uniform<int>(0,1000000));
      p.second = _add_nested_component(true_out, p.first, depth);
    }

    _test_equality(d, true_out);
  }

  void test_random_1() {
    run_deep_random_test(1);
  }

  void test_random_2() {
    run_deep_random_test(4);
  }

  void test_random_3() {
    run_deep_random_test(10);
  }

  // Run the above on the gl_sarray versions
  void test_gl_sarray() {
    random::seed(0);

    flex_dict true_out;

    flex_dict d(10);
    for(auto& p : d) {
      p.first = "a" + std::to_string(random::fast_uniform<int>(0,1000000));
      p.second = _add_nested_component(true_out, p.first, 3);
    }

    gl_sarray sa({d, d, d});

    gl_sarray sa_out
        = to_sarray_of_flat_dictionaries(sa, ".", "__undefined__", "ignore", "ignore");

    for(size_t i = 0; i < 3; ++i) {
      _test_equality(sa_out[i], true_out);
    }
  }

};

BOOST_FIXTURE_TEST_SUITE(_dictionary_flatting_test, dictionary_flatting_test)
BOOST_AUTO_TEST_CASE(test_unity_preserving) {
  dictionary_flatting_test::test_unity_preserving();
}
BOOST_AUTO_TEST_CASE(test_nested_1) {
  dictionary_flatting_test::test_nested_1();
}
BOOST_AUTO_TEST_CASE(test_nested_2) {
  dictionary_flatting_test::test_nested_2();
}
BOOST_AUTO_TEST_CASE(test_nested_vect) {
  dictionary_flatting_test::test_nested_vect();
}
BOOST_AUTO_TEST_CASE(test_nested_string) {
  dictionary_flatting_test::test_nested_string();
}
BOOST_AUTO_TEST_CASE(test_nested_undefined) {
  dictionary_flatting_test::test_nested_undefined();
}
BOOST_AUTO_TEST_CASE(test_nested_list) {
  dictionary_flatting_test::test_nested_list();
}
BOOST_AUTO_TEST_CASE(test_string) {
  dictionary_flatting_test::test_string();
}
BOOST_AUTO_TEST_CASE(test_integer) {
  dictionary_flatting_test::test_integer();
}
BOOST_AUTO_TEST_CASE(test_datetime_ignore) {
  dictionary_flatting_test::test_datetime_ignore();
}
BOOST_AUTO_TEST_CASE(test_datetime_ignore_2) {
  dictionary_flatting_test::test_datetime_ignore_2();
}
BOOST_AUTO_TEST_CASE(test_nesting_1) {
  dictionary_flatting_test::test_nesting_1();
}
BOOST_AUTO_TEST_CASE(test_nesting_2) {
  dictionary_flatting_test::test_nesting_2();
}
BOOST_AUTO_TEST_CASE(test_nesting_3) {
  dictionary_flatting_test::test_nesting_3();
}
BOOST_AUTO_TEST_CASE(test_deep_full_recursion) {
  dictionary_flatting_test::test_deep_full_recursion();
}
BOOST_AUTO_TEST_CASE(test_random_1) {
  dictionary_flatting_test::test_random_1();
}
BOOST_AUTO_TEST_CASE(test_random_2) {
  dictionary_flatting_test::test_random_2();
}
BOOST_AUTO_TEST_CASE(test_random_3) {
  dictionary_flatting_test::test_random_3();
}
BOOST_AUTO_TEST_CASE(test_gl_sarray) {
  dictionary_flatting_test::test_gl_sarray();
}
BOOST_AUTO_TEST_SUITE_END()

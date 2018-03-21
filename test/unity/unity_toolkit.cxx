#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>
#include <cmath>
#include <boost/variant/apply_visitor.hpp>
#include <boost/range/combine.hpp>

#include <unity/lib/variant.hpp>
#include <unity/lib/variant_converter.hpp>
#include <unity/lib/simple_model.hpp>
#include <unity/lib/unity_sarray.hpp>
// #include <unity/lib/api/model_interface.hpp>
// The above gave a fatal error when I tried to make
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
using namespace turi;


struct variant_equality_visitor {
  typedef bool result_type;
  template <typename T>
  bool operator()(T a, T b) const {
    return a == b;
  }
  template <typename S, typename T>
  bool operator()(S a, T b) const {
    return false;
  }
  bool operator()(std::shared_ptr<unity_sframe_base> a, std::shared_ptr<unity_sframe_base> b) const {
    return true;
  }
  bool operator()(std::shared_ptr<unity_sarray_base> a, std::shared_ptr<unity_sarray_base> b) const {
    return true;
  }
  bool operator()(std::shared_ptr<unity_sgraph_base> a, std::shared_ptr<unity_sgraph_base> b) const {
    return true;
  }
  bool operator()(std::shared_ptr<model_base> a, std::shared_ptr<model_base> b) const {
    return true;
  }
  bool operator()(function_closure_info a, function_closure_info b) const {
    return true;
  }
  bool operator()(dataframe_t a, dataframe_t b) const {
    // for completeness. We are not handling the dataframe case
    return false;
  }
  bool operator()(std::map<std::string, variant_type> a,
                  std::map<std::string, variant_type> b) const {
    if (a.size() != b.size()) return false;
    bool equal = true;
    for (const auto& val: a) {
      if (b.count(val.first) == 0) return false;
      equal &= boost::apply_visitor(variant_equality_visitor(),
                                    val.second,
                                    b.at(val.first));
    }
    return equal;
  }

  bool operator()(std::vector<variant_type> a,
                  std::vector<variant_type> b) const {
    if (a.size() != b.size()) return false;
    bool equal = true;
    for (size_t i = 0;i < a.size(); ++i) {
      equal &= boost::apply_visitor(variant_equality_visitor(), a[i], b[i]);
    }
    return equal;
  }
};

struct unity_toolkit_test {
 public:

  // convert value to a flexible_type
  // convert it back to the type
  // convert it back to flexible_type
  // check flexible_type for equality
  template <typename T>
  void converter_test(T value) {
    static_assert(variant_converter<T>::value, "bad");
    TS_ASSERT(variant_converter<T>::value == true);
    variant_type fval = variant_converter<T>().set(T(value));
    T val = variant_converter<T>().get(fval);
    variant_type fval2 = variant_converter<T>().set(val);
    bool values_equal = boost::apply_visitor(variant_equality_visitor(), fval, fval2);
    TS_ASSERT(values_equal);
  }

  std::shared_ptr<unity_sarray> make_sarray() {
    return std::make_shared<unity_sarray>();
  }

  std::shared_ptr<unity_sframe> make_sframe() {
    return std::make_shared<unity_sframe>();
  }

  std::shared_ptr<unity_sgraph> make_sgraph() {
    return std::make_shared<unity_sgraph>();
  }
  std::shared_ptr<simple_model> make_model() {
    return std::make_shared<simple_model>();
  }

  void test_variant() {
    // case 1
    converter_test<flexible_type>(flexible_type(1.0));
    converter_test<flexible_type>(flexible_type(flex_vec{1.0,2.0,3.0}));
    converter_test<std::vector<flexible_type>>(flex_list{1.0, "hello world"});
    converter_test<std::tuple<size_t, std::vector<bool>>>(
        std::tuple<size_t, std::vector<bool>>{1,std::vector<bool>{true, false}});
    converter_test<std::tuple<size_t, int, double>>(std::tuple<size_t, int, double>{1, -1, 3.0});
    converter_test<std::tuple<double, int, int>>(std::tuple<double, int, int>{1.0, 1, 2});
    converter_test<std::vector<std::vector<std::string>>>({{"hello"},{"world"}});
    converter_test<std::map<std::string, std::string>>({{"hello","world"}, {"pika","chu"}});
    converter_test<std::pair<std::string, bool>>({"hello",true});

    // case 2
    converter_test<std::shared_ptr<unity_sarray_base>>(make_sarray());
    converter_test<std::shared_ptr<unity_sframe_base>>(make_sframe());
    converter_test<std::shared_ptr<unity_sgraph_base>>(make_sgraph());
    converter_test<std::shared_ptr<model_base>>(make_model());
    converter_test<std::vector<variant_type>>({variant_type()});
    converter_test<std::vector<variant_type>>(
        std::vector<variant_type>{flexible_type("hello"),
                                  flexible_type(1.0),
                                  std::static_pointer_cast<unity_sarray_base>(make_sarray()),
                                  std::static_pointer_cast<model_base>(make_model())});
    converter_test<std::map<std::string, variant_type>>({{"hello world", variant_type()}});

    // case 3
    converter_test<variant_type>(variant_type());

    // case 4
    converter_test<std::shared_ptr<unity_sarray>>(make_sarray());

    // case 5
    converter_test<std::shared_ptr<unity_sframe>>(make_sframe());

    // case 6
    converter_test<std::shared_ptr<unity_sgraph>>(make_sgraph());

    // case 7
    converter_test<std::shared_ptr<simple_model>>(make_model());

    // case 8
    converter_test<std::vector<std::shared_ptr<unity_sarray>>>({make_sarray(), make_sarray()});
    converter_test<std::vector<std::shared_ptr<unity_sgraph>>>({make_sgraph(), make_sgraph()});
    converter_test<std::vector<variant_vector_type>>(
        std::vector<variant_vector_type>{{flexible_type("hello")},
                                        {flexible_type(1.0), to_variant(make_sgraph())},
                                        {to_variant(make_sarray())},
                                        {to_variant(make_model()), to_variant(make_sframe())}});
    // case 9
    converter_test<std::map<std::string, variant_vector_type>>(
        std::map<std::string, variant_vector_type>{{"hello world", {variant_type()}}});
    converter_test<std::map<std::string, std::shared_ptr<unity_sarray>>>({{"hello", make_sarray()},
                                                          {"world", make_sarray()}});
    converter_test<std::map<std::string, std::vector<std::shared_ptr<unity_sarray>>>>
                                                  ({{"hello", {make_sarray()}},
                                                    {"world", {make_sarray()}}});
    // this technically will fall in the flexible_type case.
    // But it should disambiguate well.
    converter_test<std::map<std::string, std::map<std::string, flexible_type>>>
                                                  ({{"hello", {{"world", 123}}},
                                                    {"world", {{"world", 456}}}});
    // case 10
    converter_test<std::unordered_map<std::string, variant_vector_type>>(
        std::unordered_map<std::string, variant_vector_type>{{"hello world", {variant_type()}}});
    converter_test<std::unordered_map<std::string, std::shared_ptr<unity_sarray>>>(
                                              {{"hello", make_sarray()},
                                               {"world", make_sarray()}});
    converter_test<std::unordered_map<std::string, std::vector<std::shared_ptr<unity_sarray>>>>
                                                  ({{"hello", {make_sarray()}},
                                                    {"world", {make_sarray()}}});
    // this technically will fall in the flexible_type case.
    // But it should disambiguate well.
    converter_test<std::unordered_map<std::string, std::unordered_map<std::string, flexible_type>>>
                                                  ({{"hello", {{"world", 123}}},
                                                    {"world", {{"world", 456}}}});
    // case 11
    converter_test<std::pair<size_t, std::shared_ptr<unity_sarray>>>({1.0, make_sarray()});
    converter_test<std::pair<std::shared_ptr<unity_sgraph>, std::shared_ptr<unity_sarray>>>({make_sgraph(), make_sarray()});
    // flexible_type case. but should disambiguate
    converter_test<std::pair<size_t, int>>({1,2});

    // case 12
    converter_test<std::tuple<size_t, std::shared_ptr<unity_sarray>, bool>>(
        std::tuple<size_t, std::shared_ptr<unity_sarray>,bool>{1.0, make_sarray(), true});
    converter_test<std::tuple<std::shared_ptr<unity_sgraph>, std::shared_ptr<unity_sarray>, bool>>(
        std::tuple<std::shared_ptr<unity_sgraph>,std::shared_ptr<unity_sarray>,bool>{make_sgraph(), make_sarray(), false});
    // flexible_type case. but should disambiguate
    converter_test<std::tuple<size_t, int>>(std::tuple<size_t,int>{1,2});
  }
};

BOOST_FIXTURE_TEST_SUITE(_unity_toolkit_test, unity_toolkit_test)
BOOST_AUTO_TEST_CASE(test_variant) {
  unity_toolkit_test::test_variant();
}
BOOST_AUTO_TEST_SUITE_END()

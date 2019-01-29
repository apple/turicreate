/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <vector>
#include <iostream>
#include <typeinfo>       // operator typeid


#include <flexible_type/flexible_type.hpp>
#include <flexible_type/flexible_type_converter.hpp>
#include <flexible_type/flexible_type_spirit_parser.hpp>


using namespace turi;


struct new_flexible_type_test  {

  public:
    
    void test_storage() {
      // test assignment from integer and copy of integer
      flexible_type f, f2;
      f = 1;
      TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(f.get<flex_int>(), 1);
      f2 = f;
      TS_ASSERT_EQUALS(f2.get_type(), flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(f2.get<flex_int>(), 1);

      

      // test assignment from flex_float and copy of flex_float
      f = 1.1;
      TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::FLOAT);
      TS_ASSERT_DELTA(f.get<flex_float>(), 1.1, 1E-6);
      f2 = f;
      TS_ASSERT_EQUALS(f2.get_type(), flex_type_enum::FLOAT);
      TS_ASSERT_DELTA(f2.get<flex_float>(), 1.1, 1E-6);
      TS_ASSERT_DELTA(f2[0], 1.1, 1E-6);

      // test assignment from char*
      f = "hello world";
      TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::STRING);
      TS_ASSERT_EQUALS(f.get<flex_string>(), "hello world");

      // test assignment from string and copy of string
      f = flex_string("hello world");
      TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::STRING);
      TS_ASSERT_EQUALS(f.get<flex_string>(), "hello world");
      f2 = f;
      TS_ASSERT_EQUALS(f2.get_type(), flex_type_enum::STRING);
      TS_ASSERT_EQUALS(f2.get<flex_string>(), "hello world");

      // test assignment from vector and copy of vector
      f = flex_vec{1.1, 2.2, 3.3};
      TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::VECTOR);
      TS_ASSERT_EQUALS(f.size(), 3);
      TS_ASSERT_DELTA(f[0], 1.1, 1E-6);
      TS_ASSERT_DELTA(f[1], 2.2, 1E-6);
      TS_ASSERT_DELTA(f[2], 3.3, 1E-6);
      f2 = f;
      TS_ASSERT_EQUALS(f2.get_type(), flex_type_enum::VECTOR);
      TS_ASSERT_EQUALS(f2.size(), 3);
      TS_ASSERT_DELTA(f2[0], 1.1, 1E-6);
      TS_ASSERT_DELTA(f2[1], 2.2, 1E-6);
      TS_ASSERT_DELTA(f2[2], 3.3, 1E-6);

      // test release of vector back to integer
      f = 1;
      TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(f.get<flex_int>(), 1);
      f2 = 1;
      TS_ASSERT_EQUALS(f2.get_type(), flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(f2.get<flex_int>(), 1);
    }

    void test_comparison_stability() {
      flex_float f = 0.1;
      flexible_type g = 0.1;
      if (f < g) {
        TS_ASSERT(false);
      }

      if ((float)f < (float)g) {
        TS_ASSERT(false);
      }
    }

    void test_mutating_operators() {
      flexible_type f = 1;
      flexible_type f2 = 2;
      f += f2;
      TS_ASSERT_EQUALS(f, 3);

      f += 1;
      TS_ASSERT_EQUALS(f, 4);

      // convert to flex_float
      f = float(f);
      f += 2.5;
      TS_ASSERT_EQUALS((float)f, 6.5);
      f -= 1.5;
      TS_ASSERT_EQUALS((float)f, 5.0);

      // convert to string
      f = std::string(f);
      TS_ASSERT_EQUALS(f, "5");
      f += "hello";
      TS_ASSERT_EQUALS(f, "5hello");

      // vector test
      f = {1.1, 2.2};
      TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::VECTOR);
      for (flexible_type i = 0; i < 10; i++) {
        f.push_back(i);
      }
      std::vector<double> vvec;
      vvec = f;
      TS_ASSERT_DELTA(f[0], 1.1, 1E-6);
      TS_ASSERT_DELTA(f[1], 2.2, 1E-6);
      TS_ASSERT_DELTA(vvec[0], 1.1, 1E-6);
      TS_ASSERT_DELTA(vvec[1], 2.2, 1E-6);
      for (flexible_type i = 2;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], (i - 2));
        TS_ASSERT_EQUALS(vvec[i], (i - 2));
      }

      // test aliasing
#ifdef __clang__
#pragma clang diagnostic push
#if defined(__has_warning) && __has_warning("-Wself-assign-overloaded")
/*
 * This new warning in Xcode 10.2 beta causes an error in our build (since
 * we have -Werror on), but the thing we're trying to test only makes
 * sense in the presence of compilers that don't have this error/warning.
 * So for now, let's just suppress the warning in this test.
 */
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif // __has_warning
#endif // __clang__
      f = f;
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__
      TS_ASSERT_DELTA(f[0], 1.1, 1E-6);
      TS_ASSERT_DELTA(f[1], 2.2, 1E-6);
      for (flexible_type i = 2;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], (i - 2));
      }

      // vector addition
      f = f + f;
      TS_ASSERT_DELTA(f[0], 2.2, 1E-6);
      TS_ASSERT_DELTA(f[1], 4.4, 1E-6);
      for (flexible_type i = 2;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], 2 * (i - 2));
      }


      // vector scalar addition
      f = f + 1;
      TS_ASSERT_DELTA(f[0], 3.2, 1E-6);
      TS_ASSERT_DELTA(f[1], 5.4, 1E-6);
      for (flexible_type i = 2;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], 1 + 2 * (i - 2));
      }



      // vector scalar subtraction
      f = f - 1;
      TS_ASSERT_DELTA(f[0], 2.2, 1E-6);
      TS_ASSERT_DELTA(f[1], 4.4, 1E-6);
      for (flexible_type i = 2;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], 2 * (i - 2));
      }


      // vector scalar division
      f = f / 2;
      TS_ASSERT_DELTA(f[0], 1.1, 1E-6);
      TS_ASSERT_DELTA(f[1], 2.2, 1E-6);
      for (flexible_type i = 2;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], (i - 2));
      }


      // vector scalar multiplication
      f = 2 * f;
      TS_ASSERT_DELTA(f[0], 2.2, 1E-6);
      TS_ASSERT_DELTA(f[1], 4.4, 1E-6);
      for (flexible_type i = 2;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], 2 * (i - 2));
      }


      // vector scalar division
      f = f * 0.5;
      TS_ASSERT_DELTA(f[0], 1.1, 1E-6);
      TS_ASSERT_DELTA(f[1], 2.2, 1E-6);
      for (flexible_type i = 2;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], (i - 2));
      }


      // vector scalar division
      f = -f;
      TS_ASSERT_DELTA(f[0], -1.1, 1E-6);
      TS_ASSERT_DELTA(f[1], -2.2, 1E-6);
      for (flexible_type i = 2;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], -(i - 2));
      }

      // vector subtraction
      f = f - f;
      for (flexible_type i = 0;i < 12; ++i) {
        TS_ASSERT_EQUALS(f[i], 0.0);
      }

      // cast to integer
      f = 0;
      // integer addition
      f = f + 5;
      TS_ASSERT_EQUALS(f, 5);

      // integer addition
      f = f + 5.6;
      // truncates
      TS_ASSERT_EQUALS(f, 10);

      // integer subtraction
      f = f - 1;
      TS_ASSERT_EQUALS(f, 9);

      // integer subtraction
      f = 1 - f;
      TS_ASSERT_EQUALS(f, -8);

      // integer product
      f = f * 2;
      TS_ASSERT_EQUALS(f, -16);

      // integer division
      f = f / 2;
      TS_ASSERT_EQUALS(f, -8);

      // integer negation and product
      f = -f * 2.5;
      TS_ASSERT_EQUALS(f, 20);

      // integer product on the left side
      f = 2 * f;
      TS_ASSERT_EQUALS(f, 40);

      // integer division on the left side
      f = 20 / f;
      TS_ASSERT_EQUALS(f, 0);

      // make it a double
      f = 1.1;
      // product with integer on the left side
      f = 2 * f;
      TS_ASSERT_DELTA(f, 2.2, 1E-6);
      // product with float on the left side
      f = 2.0 * f;
      TS_ASSERT_DELTA(f, 4.4, 1E-6);
      // division with integer on the left side
      f = 8 / f;
      TS_ASSERT_DELTA(f, 1.818181818, 1E-6);

      // soemthing extremely fun
      f = {1.0, 2.0, 3.0};
      f += {2.0, 3.0, 4.0};
      TS_ASSERT_EQUALS(f[0], 3);
      TS_ASSERT_EQUALS(f[1], 5);
      TS_ASSERT_EQUALS(f[2], 7);
    }

    void test_compilation() {
      // This currently does not compile.
      std::map< flexible_type, std::vector<flexible_type> > map;
      // Using unordered map would work
      // std::unordered_map< flexible_type, std::vector<flexible_type> > map;
      flexible_type key("foo");
      map[key] = std::vector<flexible_type>{"a", "b", "c"};
      auto& x = map[key];
      TS_ASSERT_EQUALS(x.size(), 3);
      map[key].push_back("d");
      TS_ASSERT_EQUALS(x.size(), 4);
    }

    void test_parser() {
      flexible_type_parser parser;
      std::string s = "1"; const char* c = &s[0];
      auto ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(ret.first, 1);

      s = "1.0"; c = &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::FLOAT);
      TS_ASSERT_EQUALS((flex_float)ret.first, 1.0);

      s = "[1,2,3,4]"; c = &s[0];
      ret = parser.vector_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::VECTOR);
      TS_ASSERT_EQUALS(ret.first.size(), 4);
      for (size_t i = 0;i < 4; ++i) TS_ASSERT_EQUALS(ret.first[i], i + 1);

      s = "[1, 2, 3 , 4]"; c = &s[0];
      ret = parser.vector_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::VECTOR);
      TS_ASSERT_EQUALS(ret.first.size(), 4);
      for (size_t i = 0;i < 4; ++i) TS_ASSERT_EQUALS(ret.first[i], i + 1);

      s = "[1,2; 3   4]"; c= &s[0];
      ret = parser.vector_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::VECTOR);
      TS_ASSERT_EQUALS(ret.first.size(), 4);
      for (size_t i = 0;i < 4; ++i) TS_ASSERT_EQUALS(ret.first[i], i + 1);


      s = "[]"; c= &s[0];
      ret = parser.vector_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::VECTOR);
      TS_ASSERT_EQUALS(ret.first.size(), 0);


      s = "[[]]"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::LIST);
      TS_ASSERT_EQUALS(ret.first.size(), 1);
      TS_ASSERT_EQUALS(ret.first.array_at(0).get_type(), flex_type_enum::LIST);
      TS_ASSERT_EQUALS(ret.first.array_at(0).size(), 0);

      
      s = "[{}]"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::LIST);
      TS_ASSERT_EQUALS(ret.first.size(), 1);
      TS_ASSERT_EQUALS(ret.first.array_at(0).get_type(), flex_type_enum::DICT);
      TS_ASSERT_EQUALS(ret.first.array_at(0).size(), 0);


      s = "{a:b c:d , 1:2}"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::DICT);
      TS_ASSERT_EQUALS(ret.first.size(), 3);
      TS_ASSERT_EQUALS(ret.first.dict_at("a").get_type(), flex_type_enum::STRING);
      TS_ASSERT_EQUALS((flex_string)ret.first.dict_at("a"), "b");
      TS_ASSERT_EQUALS(ret.first.dict_at("c").get_type(), flex_type_enum::STRING);
      TS_ASSERT_EQUALS((flex_string)ret.first.dict_at("c"), "d");
      TS_ASSERT_EQUALS(ret.first.dict_at(1).get_type(), flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(ret.first.dict_at(1), 2);

      s = "[{a:b c:d , 1:2}]"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);


      s = "[abc,123,def]"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::LIST);
      TS_ASSERT_EQUALS(ret.first.size(), 3);
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(0), "abc");
      TS_ASSERT_EQUALS(ret.first.array_at(1), 123);
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(2), "def");

      s = "[abc , 123 , def]"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::LIST);
      TS_ASSERT_EQUALS(ret.first.size(), 3);
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(0), "abc");
      TS_ASSERT_EQUALS(ret.first.array_at(1), 123);
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(2), "def");

      s = "[abc,1abc , def]"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::LIST);
      TS_ASSERT_EQUALS(ret.first.size(), 3);
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(0), "abc");
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(1), "1abc");
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(2), "def");

      s = "[abc,123 456, def]"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::LIST);
      TS_ASSERT_EQUALS(ret.first.size(), 3);
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(0), "abc");
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(1), "123 456");
      TS_ASSERT_EQUALS((flex_string)ret.first.array_at(2), "def");

      s = "{abc:def 1abc:2def,2abc:3}"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::DICT);
      TS_ASSERT_EQUALS(ret.first.size(), 3);
      TS_ASSERT_EQUALS((flex_string)ret.first.dict_at("abc"), "def");
      TS_ASSERT_EQUALS((flex_string)ret.first.dict_at("1abc"), "2def");
      TS_ASSERT_EQUALS(ret.first.dict_at("2abc"), 3);


      s = "{:}"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::DICT);
      TS_ASSERT_EQUALS(ret.first.size(), 1);
      TS_ASSERT_EQUALS(ret.first.dict_at(FLEX_UNDEFINED).get_type(), flex_type_enum::UNDEFINED);


      s = "[,]"; c= &s[0];
      ret = parser.general_flexible_type_parse(&c, s.length());
      TS_ASSERT(ret.second);
      TS_ASSERT_EQUALS(ret.first.get_type(), flex_type_enum::LIST);
      TS_ASSERT_EQUALS(ret.first.size(), 2);
      TS_ASSERT_EQUALS(ret.first.array_at(0).get_type(), flex_type_enum::UNDEFINED);
      TS_ASSERT_EQUALS(ret.first.array_at(1).get_type(), flex_type_enum::UNDEFINED);
    }

    // convert value to a flexible_type
    // convert it back to the type
    // convert it back to flexible_type
    // check flexible_type for equality
    template <typename T>
    void converter_test(T value) {  
      static_assert(flexible_type_converter<T>::value, "bad");
      TS_ASSERT(flexible_type_converter<T>::value == true);
      flexible_type fval = flexible_type_converter<T>().set(T(value));
      T val = flexible_type_converter<T>().get(fval);
      flexible_type fval2 = flexible_type_converter<T>().set(val);
      TS_ASSERT(fval == fval2);
    }

    void test_flexible_type_converters() {
      // we try to enumerate all cases... in flexible_type/flexible_type_converter.hpp
      // Case 1:
      converter_test<flex_string>("hello world");
      converter_test<flex_vec>({1,2,3});
      converter_test<flex_list>({1.0,"hello world",2});
      converter_test<flex_dict>({{1.0,"hello world"},{2, "pika"}});

      // case 2
      converter_test<flexible_type>(flexible_type(1.0));

      // case 3
      converter_test<flex_int>(1);
      converter_test<flex_float>(2.0);
      converter_test<int>(3);
      converter_test<float>(4.0);
      converter_test<bool>(true);
      converter_test<uint32_t>(5);

      // case 4
      converter_test<std::vector<int>>({-4,3,-2,1,0});
      converter_test<std::vector<float>>({-4.0,3.0,-2.0,1.0,0.0});
      converter_test<std::vector<double>>({-4.0,3.0,-2.0,1.0,0.0});
      converter_test<std::vector<bool>>({true, false, true});
      converter_test<std::vector<bool>>({true, false, true});
      // case 5:
      converter_test<std::vector<std::string>>({"hello", "world"});
      converter_test<std::vector<flexible_type>>({flexible_type("hello"), flexible_type("world")});
      converter_test<std::vector<std::vector<std::string>>>({{"hello"},{"world"}});

      // case 6:
      converter_test<std::map<std::string, std::string>>({{"hello","world"}, {"pika","chu"}});
      converter_test<std::map<std::string, std::vector<std::string>>>({{"hello",{"world"}}, {"pika",{"chu"}}});
      converter_test<std::map<std::string, bool>>({{"hello",true}, {"pika",false}});

      // case 7:
      converter_test<std::unordered_map<std::string, std::string>>({{"hello","world"}, {"pika","chu"}});
      converter_test<std::unordered_map<std::string, std::vector<std::string>>>({{"hello",{"world"}}, {"pika",{"chu"}}});
      converter_test<std::unordered_map<std::string, bool>>({{"hello",true}, {"pika",false}});

      // case 8:
      converter_test<std::pair<std::string, std::string>>({"hello","world"});
      converter_test<std::pair<std::string, std::vector<std::string>>>({"hello",{"world"}});
      converter_test<std::pair<std::string, bool>>({"hello",true});

      // case 9:
      converter_test<std::pair<size_t, int>>({1, -1});
      converter_test<std::pair<double, int>>({1.0, 1});

      // case 10:
      converter_test<std::tuple<std::string, std::string, std::vector<std::string>>>(
          std::tuple<std::string, std::string, std::vector<std::string>>
            {"hello","world",std::vector<std::string>{"pika"}});
      converter_test<std::tuple<size_t, std::vector<bool>>>(
          std::tuple<size_t, std::vector<bool>>{1,std::vector<bool>{true, false}});

      // case 11:
      converter_test<std::tuple<size_t, int, double>>(std::tuple<size_t, int, double>{1, -1, 3.0});
      converter_test<std::tuple<double, int, int>>(std::tuple<double,int,int>{1.0, 1, 2});
    }
};

BOOST_FIXTURE_TEST_SUITE(_new_flexible_type_test, new_flexible_type_test)
BOOST_AUTO_TEST_CASE(test_storage) {
  new_flexible_type_test::test_storage();
}
BOOST_AUTO_TEST_CASE(test_comparison_stability) {
  new_flexible_type_test::test_comparison_stability();
}
BOOST_AUTO_TEST_CASE(test_mutating_operators) {
  new_flexible_type_test::test_mutating_operators();
}
BOOST_AUTO_TEST_CASE(test_compilation) {
  new_flexible_type_test::test_compilation();
}
BOOST_AUTO_TEST_CASE(test_parser) {
  new_flexible_type_test::test_parser();
}
BOOST_AUTO_TEST_CASE(test_flexible_type_converters) {
  new_flexible_type_test::test_flexible_type_converters();
}
BOOST_AUTO_TEST_SUITE_END()

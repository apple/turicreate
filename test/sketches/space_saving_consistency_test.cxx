#define ENABLE_SKETCH_CONSISTENCY_CHECKS

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <iostream>
#include <set>
#include <cmath>
#include <unordered_map>
#include <core/data/flexible_type/flexible_type.hpp>
#include <ml/sketches/space_saving.hpp>
#include <ml/sketches/space_saving_flextype.hpp>
#include <core/random/random.hpp>
#include <core/util/cityhash_tc.hpp>

using namespace turi;
using namespace turi::sketches;

struct space_saving_consistency_test {
 public:
  template <typename SketchType>
  void run_test_simple() { 

    std::vector<int> el = {0,1,2,3,0,1,2,3};

    SketchType g;

    for(int v : el) {
      g.add(v);
    }

    DASSERT_EQ(g.size(), 8); 

    auto ret = g.frequent_items();
    
    std::sort(ret.begin(), ret.end());

    DASSERT_EQ(ret.size(), 4);

    DASSERT_EQ(ret[0].first, 0);
    DASSERT_EQ(ret[0].second, 2);
    DASSERT_EQ(ret[1].first, 1);
    DASSERT_EQ(ret[1].second, 2);
    DASSERT_EQ(ret[2].first, 2);
    DASSERT_EQ(ret[2].second, 2);
    DASSERT_EQ(ret[3].first, 3);
    DASSERT_EQ(ret[3].second, 2);
  }

  void  test_simple_a() {
    run_test_simple<space_saving<int> >();
  }
  
  void  test_simple_b() {
    run_test_simple<space_saving<flexible_type> >();
  }
  
  void  test_simple_c() {
    run_test_simple<space_saving_flextype >();
  }

  ////////////////////////////////////////////////////////////////////////////////
  
  template <typename SketchType>
  void run_test_simple_2() { 

    std::vector<int> el(100);

    for(int i = 0; i < 100; ++i) {
      el[i] = i % 10;
    }

    SketchType g(0.1);

    for(int v : el) {
      g.add(v);
    }

    DASSERT_EQ(g.size(), 100); 

    auto ret = g.frequent_items();

    std::sort(ret.begin(), ret.end());

    DASSERT_EQ(ret.size(), 10);

    for(int i = 0; i < 10; ++i) {
      DASSERT_EQ(ret[i].first, i);
      DASSERT_EQ(ret[i].second, 10);
    }
  }

  void test_simple_2_a() {
    run_test_simple_2<space_saving<int> >();
  }

  void test_simple_2_b() {
    run_test_simple_2<space_saving<flexible_type> >();
  }

  void test_simple_2_c() {
    run_test_simple_2<space_saving_flextype>();
  }
  
  ////////////////////////////////////////////////////////////////////////////////

  template <typename SketchType>
  void run_test_simple_3() { 

    std::vector<int> el(2000);

    for(int i = 0; i < 1000; ++i) {
      el[i] = i % 20;
    }

    for(int i = 100; i < 2000; ++i) {
      el[i] = 100;
    }
  
    SketchType g(0.01);

    for(int v : el) {
      g.add(v);
    }

    DASSERT_EQ(g.size(), 2000); 

    auto ret = g.frequent_items();

    std::sort(ret.begin(), ret.end());

    DASSERT_EQ(ret.back().first, 100);
    DASSERT_EQ(ret.back().second, 1900);
  }


  void test_simple_3_a() {
    run_test_simple_3<space_saving<int> >();
  }

  void test_simple_3_b() {
    run_test_simple_3<space_saving<flexible_type> >();
  }

  void test_simple_3_c() {
    run_test_simple_3<space_saving_flextype>();
  }

  ////////////////////////////////////////////////////////////////////////////////
  
  template <typename SketchType> 
  void run_test_big() { 
    
    SketchType g(0.1);

    for(int i = 0; i < 4000; ++i) {
      g.add(hash64(i) % 4);
    }

    DASSERT_EQ(g.size(), 4000); 
  }

  void test_big_a() {
    run_test_big<space_saving<int> >();
  }

  void test_big_b() {
    run_test_big<space_saving<flexible_type> >();
  }

  void test_big_c() {
    run_test_big<space_saving_flextype>();
  }

  ////////////////////////////////////////////////////////////////////////////////
  
  template <typename SketchType> 
  void run_test_combine_1() { 

    std::vector<int> el = {0,1,2,3,0,1,2,3};

    SketchType g1, g2;

    for(int v : el) {
      if( v % 2 == 0)
        g1.add(v);
      else
        g2.add(v); 
    }

    DASSERT_EQ(g1.size() + g2.size(), 8);

    g1.combine(g2);
  
    DASSERT_EQ(g1.size(), 8);

    auto ret = g1.frequent_items();

    std::sort(ret.begin(), ret.end());

    DASSERT_EQ(ret.size(), 4);

    DASSERT_EQ(ret[0].first, 0);
    DASSERT_EQ(ret[0].second, 2);
    DASSERT_EQ(ret[1].first, 1);
    DASSERT_EQ(ret[1].second, 2);
    DASSERT_EQ(ret[2].first, 2);
    DASSERT_EQ(ret[2].second, 2);
    DASSERT_EQ(ret[3].first, 3);
    DASSERT_EQ(ret[3].second, 2);
  }

  void test_combine_1_a() {
    run_test_combine_1<space_saving<int> >();
  }

  void test_combine_1_b() {
    run_test_combine_1<space_saving<flexible_type> >();
  }

  void test_combine_1_c() {
    run_test_combine_1<space_saving_flextype>();
  }

  ////////////////////////////////////////////////////////////////////////////////
  
  template <typename SketchType> 
  void run_test_combine_2() { 

    std::vector<int> el = {0,1,2,3,0,1,2,3};

    SketchType g1, g2;

    for(int v : el) {
      g1.add(v);
      g2.add(v); 
    }

    DASSERT_EQ(g1.size() + g2.size(), 16);

    g1.combine(g2);
  
    DASSERT_EQ(g1.size(), 16);

    auto ret = g1.frequent_items();

    std::sort(ret.begin(), ret.end());

    DASSERT_EQ(ret.size(), 4);

    DASSERT_EQ(ret[0].first, 0);
    DASSERT_EQ(ret[0].second, 4);
    DASSERT_EQ(ret[1].first, 1);
    DASSERT_EQ(ret[1].second, 4);
    DASSERT_EQ(ret[2].first, 2);
    DASSERT_EQ(ret[2].second, 4);
    DASSERT_EQ(ret[3].first, 3);
    DASSERT_EQ(ret[3].second, 4);
  }

  void test_combine_2_a() {
    run_test_combine_2<space_saving<int> >();
  }

  void test_combine_2_b() {
    run_test_combine_2<space_saving<flexible_type> >();
  }

  void test_combine_2_c() {
    run_test_combine_2<space_saving_flextype>();
  }

  ////////////////////////////////////////////////////////////////////////////////

  template <typename SketchType> 
  void run_test_combine_3() { 

    std::vector<int> el(2000);

    for(int i = 0; i < 100; ++i) {
      el[i] = i % 20;
    }

    for(int i = 100; i < 2000; ++i) {
      el[i] = 100;
    }
  
    SketchType g1(0.01);
    SketchType g2(0.01);

    for(size_t i = 0; i < el.size(); ++i) {
      int v = el[i];
      if((i % 3) == 0)
        g1.add(v);
      else
        g2.add(v);
    }

    DASSERT_EQ(g1.size() + g2.size(), 2000); 

    g1.combine(g2); 
  
    auto ret = g1.frequent_items();

    std::sort(ret.begin(), ret.end());

    DASSERT_EQ(ret.back().first, 100);
    DASSERT_EQ(ret.back().second, 1900);
  }

  void test_combine_3_a() {
    run_test_combine_3<space_saving<int> >();
  }

  void test_combine_3_b() {
    run_test_combine_3<space_saving<flexible_type> >();
  }

  void test_combine_3_c() {
    run_test_combine_3<space_saving_flextype>();
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_combine_4() { 

    std::vector<int> el(4000);

    for(int i = 0; i < 1000; ++i) {
      el[i] = i % 2000;
    }

    for(int i = 2000; i < 4000; ++i) {
      el[i] =  i % 20;
    }
  
    space_saving_flextype g1(0.01);
    space_saving_flextype g2(0.01);

    for(size_t i = 0; i < el.size(); ++i) {
      flexible_type v = (i % 5 == 0) ? flexible_type(float(el[i])) : flexible_type(el[i]);
      
      if((i % 3) == 0)
        g1.add(v);
      else
        g2.add(v);
    }

    DASSERT_EQ(g1.size() + g2.size(), 4000); 

    g1.combine(g2); 
  
    auto ret_1 = g1.frequent_items();
    std::sort(ret_1.begin(), ret_1.end());

    g2.clear();
    g1.combine(g2);

    auto ret_2 = g1.frequent_items();
    std::sort(ret_2.begin(), ret_2.end());

    DASSERT_TRUE(ret_1 == ret_2);
  }

  ////////////////////////////////////////////////////////////////////////////////
  
  template <typename SketchType> 
  void run_test_combine_5() { 

    std::vector<int> el(4000);

    for(int i = 0; i < 2000; ++i) {
      el[i] = i % 20;
    }

    for(int i = 100; i < 2000; ++i) {
      el[i] = i % 50;
    }
  
    SketchType g1(0.001);
    SketchType g2(0.001);
    SketchType g3(0.001);

    for(size_t i = 0; i < el.size(); ++i) {
      int v = el[i];
      if((i % 3) == 0)
        g1.add(v);
      else
        g2.add(v);

      g3.add(v);
      
      if(i % 1000 == 0) {
        g1.combine(g2);
        g2.clear();
      }
    }

    DASSERT_EQ(g1.size() + g2.size(), 4000); 

    g1.combine(g2); 
    g2.clear();

    auto ret_1 = g1.frequent_items();
    std::sort(ret_1.begin(), ret_1.end());

    auto ret_2 = g3.frequent_items();
    std::sort(ret_2.begin(), ret_2.end());
    
    DASSERT_TRUE(ret_1 == ret_2);

    g1.combine(g2);
    g2.clear();
    g2.combine(g1); 
    g1.clear();
    g1.combine(g2);
    g2.clear();
    
    auto ret_3 = g1.frequent_items();
    std::sort(ret_3.begin(), ret_3.end());

    auto ret_4 = g3.frequent_items();
    std::sort(ret_4.begin(), ret_4.end());
    
    DASSERT_TRUE(ret_3 == ret_4);

  }

  void test_combine_5_a() {
    run_test_combine_5<space_saving<int> >();
  }

  void test_combine_5_b() {
    run_test_combine_5<space_saving<flexible_type> >();
  }

  void test_combine_5_c() {
    run_test_combine_5<space_saving_flextype>();
  }
  
  ////////////////////////////////////////////////////////////////////////////////

  void run_test_simple_flextype_NAN() { 

    std::vector<flexible_type> el = {
      flexible_type(0),
      flexible_type(1),
      flexible_type(2),
      flexible_type(3),
      flexible_type(0),
      flexible_type(1),
      flexible_type(2),
      flexible_type(3),
      FLEX_UNDEFINED };

    space_saving_flextype g;

    for(size_t i = 0; i < 1000; ++i) {
      for(auto v : el) {
        g.add(v);
      }
    }

    DASSERT_EQ(g.size(), 8); 

    auto ret = g.frequent_items();
    
    std::sort(ret.begin(), ret.end());

    DASSERT_EQ(ret.size(), 5);

    DASSERT_EQ(ret[0].first, 0);
    DASSERT_EQ(ret[0].second, 2000);
    DASSERT_EQ(ret[1].first, 1);
    DASSERT_EQ(ret[1].second, 2000);
    DASSERT_EQ(ret[2].first, 2);
    DASSERT_EQ(ret[2].second, 2000);
    DASSERT_EQ(ret[3].first, 3);
    DASSERT_EQ(ret[3].second, 2000);
    DASSERT_EQ(ret[4].first, FLEX_UNDEFINED);
    DASSERT_EQ(ret[4].second, 1000);
  }

  ////////////////////////////////////////////////////////////////////////////////
  
  void run_test_flextype_combine_NAN() { 

    std::vector<flexible_type> el(2000);

    for(int i = 0; i < 2000; ++i) {
      el[i] = (i % 37 != 0) ? flexible_type(double(i % 200)) : flexible_type(NAN);
    }
  
    space_saving_flextype g1(0.01);
    space_saving_flextype g2(0.01);

    for(size_t i = 0; i < el.size(); ++i) {
      int v = el[i];
      if((i % 3) == 0)
        g1.add(v);
      else
        g2.add(v);
    }

    DASSERT_EQ(g1.size() + g2.size(), 2000); 

    g1.combine(g2); 
  
    auto ret = g1.frequent_items();

    DASSERT_EQ(ret.front().first, NAN);
  }

  void run_test_NAN_Inf() { 

    space_saving_flextype g;

    g.add(flexible_type(1.0));
    g.add(flexible_type(NAN));
    g.add(flexible_type(INFINITY));
    g.add(flexible_type(2.0));

    DASSERT_EQ(g.size(), 4); 

    g.frequent_items();
  }

  
};

BOOST_FIXTURE_TEST_SUITE(_space_saving_consistency_test, space_saving_consistency_test)
BOOST_AUTO_TEST_CASE(test_simple_a) {
  space_saving_consistency_test::test_simple_a();
}
BOOST_AUTO_TEST_CASE(test_simple_b) {
  space_saving_consistency_test::test_simple_b();
}
BOOST_AUTO_TEST_CASE(test_simple_c) {
  space_saving_consistency_test::test_simple_c();
}
BOOST_AUTO_TEST_CASE(test_simple_2_a) {
  space_saving_consistency_test::test_simple_2_a();
}
BOOST_AUTO_TEST_CASE(test_simple_2_b) {
  space_saving_consistency_test::test_simple_2_b();
}
BOOST_AUTO_TEST_CASE(test_simple_2_c) {
  space_saving_consistency_test::test_simple_2_c();
}
BOOST_AUTO_TEST_CASE(test_simple_3_a) {
  space_saving_consistency_test::test_simple_3_a();
}
BOOST_AUTO_TEST_CASE(test_simple_3_b) {
  space_saving_consistency_test::test_simple_3_b();
}
BOOST_AUTO_TEST_CASE(test_simple_3_c) {
  space_saving_consistency_test::test_simple_3_c();
}
BOOST_AUTO_TEST_CASE(test_big_a) {
  space_saving_consistency_test::test_big_a();
}
BOOST_AUTO_TEST_CASE(test_big_b) {
  space_saving_consistency_test::test_big_b();
}
BOOST_AUTO_TEST_CASE(test_big_c) {
  space_saving_consistency_test::test_big_c();
}
BOOST_AUTO_TEST_CASE(test_combine_1_a) {
  space_saving_consistency_test::test_combine_1_a();
}
BOOST_AUTO_TEST_CASE(test_combine_1_b) {
  space_saving_consistency_test::test_combine_1_b();
}
BOOST_AUTO_TEST_CASE(test_combine_1_c) {
  space_saving_consistency_test::test_combine_1_c();
}
BOOST_AUTO_TEST_CASE(test_combine_2_a) {
  space_saving_consistency_test::test_combine_2_a();
}
BOOST_AUTO_TEST_CASE(test_combine_2_b) {
  space_saving_consistency_test::test_combine_2_b();
}
BOOST_AUTO_TEST_CASE(test_combine_2_c) {
  space_saving_consistency_test::test_combine_2_c();
}
BOOST_AUTO_TEST_CASE(test_combine_3_a) {
  space_saving_consistency_test::test_combine_3_a();
}
BOOST_AUTO_TEST_CASE(test_combine_3_b) {
  space_saving_consistency_test::test_combine_3_b();
}
BOOST_AUTO_TEST_CASE(test_combine_3_c) {
  space_saving_consistency_test::test_combine_3_c();
}
BOOST_AUTO_TEST_CASE(test_combine_4) {
  space_saving_consistency_test::test_combine_4();
}
BOOST_AUTO_TEST_CASE(test_combine_5_a) {
  space_saving_consistency_test::test_combine_5_a();
}
BOOST_AUTO_TEST_CASE(test_combine_5_b) {
  space_saving_consistency_test::test_combine_5_b();
}
BOOST_AUTO_TEST_CASE(test_combine_5_c) {
  space_saving_consistency_test::test_combine_5_c();
}
BOOST_AUTO_TEST_SUITE_END()

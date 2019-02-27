/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>
#include <unistd.h>
#include <sgraph/sgraph.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/unity_sframe.hpp>
using namespace turi;

struct unity_graph_test {

  /*
   * Creates a sparse dataframe with 4 fields
   * a: int
   * c: float
   * d: string
   * e: vector
   * the dataframe has 60 rows with incrementing a from 0 to 59
   * first 20 rows have float 0-19
   * next 20 rows have a string "20" to "39"
   * next 20 rows have a vector of length 1 containing the value 40 to 59
   */
  void create_test_dataframe_a(dataframe_t& df) {
    std::vector<flexible_type> a;
    std::vector<flexible_type> c;
    std::vector<flexible_type> d;
    std::vector<flexible_type> e;
    for (size_t i = 0;i < 20; ++i) {
      a.push_back(i);
      c.push_back((double)i);
      d.push_back(FLEX_UNDEFINED);
      e.push_back(FLEX_UNDEFINED);
    }
    for (size_t i = 0;i < 20; ++i) {
      a.push_back(i + 20);
      c.push_back(FLEX_UNDEFINED);
      d.push_back(std::to_string(i + 20));
      e.push_back(FLEX_UNDEFINED);
    }
    for (size_t i = 0;i < 20; ++i) {
      a.push_back(i + 40);
      c.push_back(FLEX_UNDEFINED);
      d.push_back(FLEX_UNDEFINED);
      std::vector<double> vec;
      vec.push_back(i + 40);
      e.push_back(vec);
    }
    df.set_column("a", a, flex_type_enum::INTEGER);
    df.set_column("c", c, flex_type_enum::FLOAT);
    df.set_column("d", d, flex_type_enum::STRING);
    df.set_column("e", e, flex_type_enum::VECTOR);
  }


  /*
   * Creates a sparse dataframe with 5 fields
   * a: int
   * b: int
   * c: float
   * d: string
   * e: vector
   * the dataframe has 60 rows with incrementing a from 0 to 59, and b from 1 to 60
   * first 20 rows have float 0-19
   * next 20 rows have a string "20" to "39"
   * next 20 rows have a vector of length 1 containing the value 40 to 59
   */
  void create_test_dataframe_b(dataframe_t& df) {
    std::vector<flexible_type> a;
    std::vector<flexible_type> b;
    std::vector<flexible_type> c;
    std::vector<flexible_type> d;
    std::vector<flexible_type> e;

    for (size_t i = 0;i < 20; ++i) {
      a.push_back(i);
      b.push_back(i + 1);
      c.push_back((double)i);
      d.push_back(FLEX_UNDEFINED);
      e.push_back(FLEX_UNDEFINED);
    }
    for (size_t i = 0;i < 20; ++i) {
      a.push_back(i + 20);
      b.push_back(i + 21);
      c.push_back(FLEX_UNDEFINED);
      d.push_back(std::to_string(i + 20));
      e.push_back(FLEX_UNDEFINED);
    }
    for (size_t i = 0;i < 20; ++i) {
      a.push_back(i + 40);
      b.push_back(i + 41);
      c.push_back(FLEX_UNDEFINED);
      d.push_back(FLEX_UNDEFINED);
      std::vector<double> vec;
      vec.push_back(i + 40);
      e.push_back(vec);
    }
    df.set_column("a", a, flex_type_enum::INTEGER);
    df.set_column("b", b, flex_type_enum::INTEGER);
    df.set_column("c", c, flex_type_enum::FLOAT);
    df.set_column("d", d, flex_type_enum::STRING);
    df.set_column("e", e, flex_type_enum::VECTOR);
  }

  /*
   * Creates a dense dataframe with 5 fields
   * a: int
   * b: int
   * c: float
   * d: string
   * e: vector
   * the dataframe has 60 rows with incrementing a from 0 to 59, and b from 1 to 60
   * column c,d,e will be the corresponding type cast of column a.
   */
  void create_test_dataframe_c(dataframe_t& d) {
    std::vector<flexible_type> col_a;
    std::vector<flexible_type> col_b;
    std::vector<flexible_type> col_c;
    std::vector<flexible_type> col_d;
    std::vector<flexible_type> col_e;
    for (size_t i = 0; i < 60; ++i) {
      col_a.push_back(flex_int(i));
      col_b.push_back(flex_int(i+1));
      col_c.push_back(flex_float(i));
      col_d.push_back(flex_string(std::to_string(i)));
      col_e.push_back(flex_vec{flex_float(i)});
    }
    d.set_column("a", col_a, flex_type_enum::INTEGER);
    d.set_column("b", col_b, flex_type_enum::INTEGER);
    d.set_column("c", col_c, flex_type_enum::FLOAT);
    d.set_column("d", col_d, flex_type_enum::STRING);
    d.set_column("e", col_e, flex_type_enum::VECTOR);
  }


 public:
  void test_insertion() {
    dataframe_t df;
    create_test_dataframe_c(df);
    std::shared_ptr<unity_sframe_base> sfb(new unity_sframe);
    sfb->construct_from_dataframe(df); // contains column a,b,c,d,e
    df.remove_column("b");
    std::shared_ptr<unity_sframe_base> sfa(new unity_sframe); // contains colum a,c,d,e
    sfa->construct_from_dataframe(df);
    size_t group, groupa, groupb;
    group = groupa = groupb = 0;

    for (size_t i = 0; i < 7; ++i) { 
      std::shared_ptr<unity_sgraph_base> graph1(new unity_sgraph);
      std::shared_ptr<unity_sgraph_base> graph2(graph1->add_vertices(sfa, "a", group));
      std::shared_ptr<unity_sgraph_base> graph3(graph1->add_edges(sfb, "a", "b", groupa, groupb));
      std::shared_ptr<unity_sgraph_base> graph4(graph2->add_edges(sfb, "a", "b", groupa, groupb));
      if (i & 1) {
        TS_ASSERT_EQUALS((size_t)graph2->summary()["num_vertices"], 60);
        TS_ASSERT_EQUALS((size_t)graph2->summary()["num_edges"], 0);
        std::vector<std::string> vfields = graph2->get_vertex_fields(group);
        std::vector<std::string> efields = graph2->get_edge_fields(groupa, groupb);
        std::set<std::string> vf(vfields.begin(), vfields.end());
        std::set<std::string> ef(efields.begin(), efields.end());
        std::set<std::string> expected_vfields{"__id", "c", "d", "e"};
        std::set<std::string> expected_efields{"__src_id", "__dst_id"};
        BOOST_TEST(vf == expected_vfields);
        BOOST_TEST(ef == expected_efields);
      }
      if (i & 2) {
        TS_ASSERT_EQUALS((size_t)graph3->summary()["num_vertices"], 61);
        TS_ASSERT_EQUALS((size_t)graph3->summary()["num_edges"], 60);
        std::vector<std::string> vfields = graph3->get_vertex_fields(group);
        std::vector<std::string> efields = graph3->get_edge_fields(groupa, groupb);
        std::set<std::string> vf(vfields.begin(), vfields.end());
        std::set<std::string> ef(efields.begin(), efields.end());
        std::set<std::string> expected_vfields{"__id"};
        std::set<std::string> expected_efields{"__src_id", "__dst_id", "c", "d", "e"};
        BOOST_TEST(vf == expected_vfields);
        BOOST_TEST(ef == expected_efields);
      }
      if (i & 4) {
        TS_ASSERT_EQUALS((size_t)graph4->summary()["num_vertices"], 61);
        TS_ASSERT_EQUALS((size_t)graph4->summary()["num_edges"], 60);
        std::vector<std::string> vfields = graph4->get_vertex_fields(group);
        std::vector<std::string> efields = graph4->get_edge_fields(groupa, groupb);
        std::set<std::string> vf(vfields.begin(), vfields.end());
        std::set<std::string> ef(efields.begin(), efields.end());
        std::set<std::string> expected_vfields{"__id", "c", "d", "e"};
        std::set<std::string> expected_efields{"__src_id", "__dst_id", "c", "d", "e"};
        BOOST_TEST(vf == expected_vfields);
        BOOST_TEST(ef == expected_efields);
      }
    }
  }

  void test_field_manipulation() {
    dataframe_t dfa;
    dataframe_t dfb;
    create_test_dataframe_a(dfa);
    create_test_dataframe_b(dfb);
    std::shared_ptr<unity_sframe_base> sfa(new unity_sframe); // contains colum a,c,d,e
    sfa->construct_from_dataframe(dfa);
    std::shared_ptr<unity_sframe_base> sfb(new unity_sframe); // contains colum a,b,c,d,e
    sfb->construct_from_dataframe(dfb);
    size_t group, groupa, groupb;
    group = groupa = groupb = 0;

    std::shared_ptr<unity_sgraph_base> graph1(new unity_sgraph);
    std::shared_ptr<unity_sgraph_base> graph2(graph1->add_vertices(sfa, "a", group));
    std::shared_ptr<unity_sgraph_base> graph3(graph2->add_edges(sfb, "a", "b", groupa, groupb));
    std::shared_ptr<unity_sgraph_base> graph4(graph3->select_vertex_fields({"d","e"}, group));
    std::shared_ptr<unity_sgraph_base> graph5(graph4->copy_edge_field("e","g", groupa, groupb));
    std::shared_ptr<unity_sgraph_base> graph6(graph5->delete_edge_field("g", groupa, groupb));

    TS_ASSERT_EQUALS((size_t)graph6->summary()["num_vertices"], 61);
    TS_ASSERT_EQUALS((size_t)graph6->summary()["num_edges"], 60);
    std::vector<std::string> vfields = graph6->get_vertex_fields(group);
    std::set<std::string> vf(vfields.begin(), vfields.end());
    TS_ASSERT_EQUALS(vf.size(), 3);
    TS_ASSERT_EQUALS(vf.count("__id"), 1);
    TS_ASSERT_EQUALS(vf.count("d"), 1);
    TS_ASSERT_EQUALS(vf.count("e"), 1);

    std::vector<std::string> efields = graph6->get_edge_fields(groupa, groupb);
    std::set<std::string> ef(efields.begin(), efields.end());
    TS_ASSERT_EQUALS(ef.size(), 5);
    TS_ASSERT_EQUALS(ef.count("__src_id"), 1);
    TS_ASSERT_EQUALS(ef.count("__dst_id"), 1);
    TS_ASSERT_EQUALS(ef.count("c"), 1);
    TS_ASSERT_EQUALS(ef.count("d"), 1);
    TS_ASSERT_EQUALS(ef.count("e"), 1);

    sgraph::options_map_t empty_constraint;
    dataframe_t vt = graph6->get_vertices({}, empty_constraint, group)->_head(size_t(-1));

    TS_ASSERT_EQUALS(vt.nrows(), 61);
    TS_ASSERT_EQUALS(vt.ncols(), 3);
    for (size_t i = 0; i < vt.nrows(); ++i) {
      auto id = vt.values["__id"][i];
      auto str = vt.values["d"][i];
      auto vec = vt.values["e"][i];
      if (id < 20) {
        TS_ASSERT_EQUALS(str.get_type(), flex_type_enum::UNDEFINED);
        TS_ASSERT_EQUALS(vec.get_type(), flex_type_enum::UNDEFINED);
      } else if (id < 40) {
        TS_ASSERT_EQUALS(str.get_type(), flex_type_enum::STRING);
        TS_ASSERT_EQUALS(vec.get_type(), flex_type_enum::UNDEFINED);
        std::cerr << "got string: " << str << " and should be " << std::to_string((int)id) << std::endl;
        TS_ASSERT_EQUALS(str, std::to_string((int)id));
      } else if (id < 60) {
        TS_ASSERT_EQUALS(str.get_type(), flex_type_enum::UNDEFINED);
        TS_ASSERT_EQUALS(vec.get_type(), flex_type_enum::VECTOR);
        TS_ASSERT_EQUALS(vec.size(), 1);
        TS_ASSERT_EQUALS(vec[0], id);
      } else {
        TS_ASSERT_EQUALS(str.get_type(), flex_type_enum::UNDEFINED);
        TS_ASSERT_EQUALS(vec.get_type(), flex_type_enum::UNDEFINED);
      }
    }

    vt = graph6->get_edges({}, {}, empty_constraint, groupa, groupb)->_head(size_t(-1));

    TS_ASSERT_EQUALS(vt.nrows(), 60);
    TS_ASSERT_EQUALS(vt.ncols(), 5);
    for (size_t i = 0; i < vt.nrows(); ++i) {
      auto srcid = vt.values["__src_id"][i];
      auto dstid = vt.values["__dst_id"][i];
      auto flt = vt.values["c"][i];
      auto str = vt.values["d"][i];
      auto vec = vt.values["e"][i];
      if (srcid < 20) {
        TS_ASSERT_EQUALS(flt.get_type(), flex_type_enum::FLOAT);
        TS_ASSERT_EQUALS(str.get_type(), flex_type_enum::UNDEFINED);
        TS_ASSERT_EQUALS(vec.get_type(), flex_type_enum::UNDEFINED);
        TS_ASSERT_EQUALS(flt, flex_float(srcid));
      } else if (srcid < 40) {
        TS_ASSERT_EQUALS(flt.get_type(), flex_type_enum::UNDEFINED);
        TS_ASSERT_EQUALS(str.get_type(), flex_type_enum::STRING);
        TS_ASSERT_EQUALS(vec.get_type(), flex_type_enum::UNDEFINED);
        TS_ASSERT_EQUALS(str, std::to_string((int)srcid));
      } else if (srcid < 60) {
        TS_ASSERT_EQUALS(flt.get_type(), flex_type_enum::UNDEFINED);
        TS_ASSERT_EQUALS(str.get_type(), flex_type_enum::UNDEFINED);
        TS_ASSERT_EQUALS(vec.get_type(), flex_type_enum::VECTOR);
        TS_ASSERT_EQUALS(vec.size(), 1);
        TS_ASSERT_EQUALS(vec[0], srcid);
      }
    }
    // try the constrained get_edges
    vt = graph6->get_edges({1}, {flex_undefined()}, empty_constraint, groupa, groupb)->_head(size_t(-1));
    TS_ASSERT_EQUALS(vt.nrows(), 1);
    TS_ASSERT_EQUALS(vt.values["__src_id"][0], 1);
    TS_ASSERT_EQUALS(vt.values["__dst_id"][0], 2);

    vt = graph6->get_edges({flex_undefined()}, {5}, empty_constraint, groupa, groupb)->_head(size_t(-1));
    TS_ASSERT_EQUALS(vt.nrows(), 1);
    TS_ASSERT_EQUALS(vt.values["__src_id"][0], 4);
    TS_ASSERT_EQUALS(vt.values["__dst_id"][0], 5);

    vt = graph6->get_edges({1, flex_undefined()}, {flex_undefined(), 5}, 
                           empty_constraint, groupa, groupb)->_head(size_t(-1));
    TS_ASSERT_EQUALS(vt.nrows(), 2);
  }

  void test_errors() {
    size_t group, groupa, groupb;
    group = groupa = groupb = 0;
    dataframe_t dfa;
    create_test_dataframe_a(dfa);
    std::shared_ptr<unity_sframe_base> sfa(new unity_sframe);
    sfa->construct_from_dataframe(dfa);

    std::shared_ptr<unity_sgraph_base> graph1(new unity_sgraph);
    std::shared_ptr<unity_sgraph_base> graph2(graph1->add_vertices(sfa, "a", group));
    // try to add vertices again with different field types
    // change c to an integer
    for (size_t i = 0;i < dfa.values["c"].size(); ++i) {
      dfa.values["c"][i] = static_cast<flex_int>(dfa.values["c"][i]);
    }
    dfa.types["c"] = flex_type_enum::INTEGER;

    // attempting to add vertices of mismatch type
    bool exception = false;
    std::shared_ptr<unity_sgraph_base> graph3;
    try {
      std::shared_ptr<unity_sframe_base> tmp(new unity_sframe);
      tmp->construct_from_dataframe(dfa);
      graph3 = graph2->add_vertices(tmp, "a", group);
      graph3->summary();
    } catch(...) {
      exception = true;
    }
    ASSERT_TRUE(exception);


    dfa.types["__moo"] = flex_type_enum::INTEGER;
    dfa.values["__moo"] = std::vector<flexible_type>(dfa.values["c"].size(), 
                                                     flexible_type(flex_type_enum::INTEGER));


    // attempting to add vertices with a reserved field name
    exception = false;
    try {
      std::shared_ptr<unity_sframe_base> tmp(new unity_sframe);
      tmp->construct_from_dataframe(dfa);
      graph3 = graph2->add_vertices(tmp, "a", group);
      graph3->summary();
    } catch(...) {
      exception = true;
    }
    ASSERT_TRUE(exception);

    // attempt to add edges of inconsistent src/target type
    dataframe_t dfb;
    create_test_dataframe_b(dfb);
    for (size_t i = 0;i < dfb.values["b"].size(); ++i) {
      dfb.values["b"][i] = (flex_string)dfb.values["b"][i];
    }
    dfb.types["b"] = flex_type_enum::STRING;
    exception = false;
    try {
      std::shared_ptr<unity_sframe_base> tmp(new unity_sframe);
      tmp->construct_from_dataframe(dfb);
      graph3 = graph2->add_edges(tmp, "a", "b", groupa, groupb);
      graph3->summary();
    } catch(...) {
      exception = true;
    }
    ASSERT_TRUE(exception);


    // attempt to add edges of src/target type which is different from when I 
    // add_vertices
    create_test_dataframe_b(dfb);
    for (size_t i = 0;i < dfb.values["b"].size(); ++i) {
      dfb.values["a"][i] = (flex_string)dfb.values["a"][i];
      dfb.values["b"][i] = (flex_string)dfb.values["b"][i];
    }
    dfb.types["a"] = flex_type_enum::STRING;
    dfb.types["b"] = flex_type_enum::STRING;
    exception = false;
    try {
      std::shared_ptr<unity_sframe_base> tmp(new unity_sframe);
      tmp->construct_from_dataframe(dfb);
      graph3 = graph2->add_edges(tmp, "a", "b", groupa, groupb);
      graph3->summary();
    } catch(...) {
      exception = true;
    }
    ASSERT_TRUE(exception);
  }

};

BOOST_FIXTURE_TEST_SUITE(_unity_graph_test, unity_graph_test)
BOOST_AUTO_TEST_CASE(test_insertion) {
  unity_graph_test::test_insertion();
}
BOOST_AUTO_TEST_CASE(test_field_manipulation) {
  unity_graph_test::test_field_manipulation();
}
BOOST_AUTO_TEST_CASE(test_errors) {
  unity_graph_test::test_errors();
}
BOOST_AUTO_TEST_SUITE_END()

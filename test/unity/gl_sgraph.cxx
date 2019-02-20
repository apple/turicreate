/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <boost/range/combine.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/gl_sgraph.hpp>
#include <unity/lib/gl_gframe.hpp>

using namespace turi;

const flexible_type None = FLEX_UNDEFINED;

struct gl_sgraph_test {
public:
  void test_constructor() {
    gl_sgraph g;
    TS_ASSERT_EQUALS(g.num_vertices(), 0);
    TS_ASSERT_EQUALS(g.num_edges(), 0);

    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,2,3}}, {"__dst_id", {2,3,1}}};
    gl_sgraph g2(vertices, edges, "__id", "__src_id", "__dst_id");

    _assert_sframe_equals(vertices, g2.get_vertices().sort("__id"));
    _assert_sframe_equals(edges, g2.get_edges().sort("__src_id"));
  }

  void test_copy() {
    gl_sgraph g;
    gl_sgraph g2(g);
    g2.vertices()["x"] = 0;

    TS_ASSERT_EQUALS(g.get_vertex_fields().size(), 1);
    TS_ASSERT_EQUALS(g2.get_vertex_fields().size(), 2);

    gl_sgraph g3 = g2;
    g3.vertices().remove_column("x");
    TS_ASSERT_EQUALS(g3.get_vertex_fields().size(), 1);
  }

  void test_field_querys() {
    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,2,3}}, {"__dst_id", {2,3,1}}};

    vertices.add_column("v", "v_str");
    vertices.add_column(0, "v_int");
    vertices.add_column(0., "v_float");

    edges.add_column("e", "e_str");
    edges.add_column(1, "e_int");
    edges.add_column(1.0, "e_float");

    // reference graph
    gl_sgraph g(vertices, edges, "__id", "__src_id", "__dst_id");

    TS_ASSERT_EQUALS(g.num_vertices(), 3);
    _assert_vec_equals(g.get_vertex_fields(), {"__id", "v_str", "v_int", "v_float"});
    _assert_vec_equals(g.get_vertex_field_types(), 
     {flex_type_enum::INTEGER,
      flex_type_enum::STRING,
      flex_type_enum::INTEGER,
      flex_type_enum::FLOAT});

    TS_ASSERT_EQUALS(g.num_edges(), 3);
    _assert_vec_equals(g.get_edge_fields(), {"__src_id", "__dst_id", "e_str", "e_int", "e_float"});
    _assert_vec_equals(g.get_edge_field_types(),
     {flex_type_enum::INTEGER,
      flex_type_enum::INTEGER,
      flex_type_enum::STRING,
      flex_type_enum::INTEGER,
      flex_type_enum::FLOAT});
  }

  void test_get_vertices() {
    flexible_type none = FLEX_UNDEFINED;
    std::vector<flexible_type> empty;
    gl_sframe vertices{
      {"__id", {1,2,3}},
      {"vdata", {0,1,none}},
    };
    gl_sframe edges{{"__src_id", {1,2,3}}, {"__dst_id", {2,3,1}}};
    // reference graph
    gl_sgraph g(vertices, edges);

    // get all vertices 
    _assert_sframe_equals(g.get_vertices().sort("__id"), vertices);

    // get vid in {1,2}
    _assert_sframe_equals(g.get_vertices({1,2}).sort("__id"), vertices.head(2));

    // get vdata == 0
    _assert_sframe_equals(g.get_vertices(empty, {{"vdata", 0}}).sort("__id"), vertices.head(1));
  }

  void test_get_edges() {
    flexible_type none = FLEX_UNDEFINED;
    std::vector<std::pair<flexible_type, flexible_type>> empty;
    gl_sframe vertices{ {"__id", {1,2,3}} };
    gl_sframe edges{
      {"__src_id", {1,2,3}},
      {"__dst_id", {2,3,1}},
      {"edata", {0,1,none}},
    };
    // reference graph
    gl_sgraph g(vertices, edges);

    // get all edges
    _assert_sframe_equals(g.get_edges().sort({"__src_id", "__dst_id"}), edges);

    // get edges with src in {1,2}
    _assert_sframe_equals(g.get_edges({ {1,none}, {2,none} }).sort("__src_id"), edges.head(2));

    // get edges with dst in {2,3}
    _assert_sframe_equals(g.get_edges({ {none, 2},{none, 3} }).sort("__src_id"), edges.head(2));

    // get edata == 0
    _assert_sframe_equals(g.get_edges(empty, { {"edata", 0} }), edges.head(1));
  }

  void test_add_vertices() {
    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,2,3}}, {"__dst_id", {2,3,1}}};
    gl_sgraph g(vertices, edges, "__id", "__src_id", "__dst_id");

    gl_sframe new_vertices{{"__id", {4,5,6}}};
    gl_sgraph g2 = g.add_vertices(new_vertices, "__id");
    _assert_sframe_equals(vertices.append(new_vertices), g2.get_vertices().sort("__id"));
  }

  void test_add_edges() {
    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,1}}, {"__dst_id", {2,3}}};
    gl_sgraph g(vertices, edges, "__id", "__src_id", "__dst_id");

    gl_sframe new_edges{{"__src_id", {2,2}}, {"__dst_id", {1,3}}};

    gl_sgraph g2 = g.add_edges(new_edges, "__src_id", "__dst_id");
    _assert_sframe_equals(edges.append(new_edges),
                          g2.get_edges().sort(std::vector<std::string>{"__src_id", "__dst_id"}));
  }

  void test_select_fields() {
    // reference graph
    gl_sframe vertices{{"__id", {1,2,3}}, {"zeros", {0,0,0}}, {"id_copy", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,1}}, {"__dst_id", {2,3}}, {"ones", {1,1}}, {"dst_copy", {2,3}}};
    gl_sgraph g(vertices, edges);

    _assert_sgraph_equals(g.select_vertex_fields(std::vector<std::string>()),
                          gl_sgraph(vertices[std::vector<std::string>{"__id",}], edges));

    _assert_sgraph_equals(g.select_edge_fields(std::vector<std::string>()),
                          gl_sgraph(vertices, edges[{"__src_id", "__dst_id"}]));

    _assert_sgraph_equals(g.select_fields({"zeros", "ones"}),
                          gl_sgraph(vertices[{"__id", "zeros"}],
                                    edges[{"__src_id", "__dst_id", "ones"}]));
  }

  void test_vertex_field_mutation() {
    // reference graph
    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,1}}, {"__dst_id", {2,3}}};
    gl_sgraph g(vertices, edges);

    // add vertex field
    g.add_vertex_field(g.vertices()["__id"], "id_copy");
    vertices.add_column(vertices["__id"], "id_copy");
    _assert_sgraph_equals(g, gl_sgraph(vertices, edges));

    // add const vertex field
    g.add_vertex_field(0, "zeros");
    vertices.add_column(0, "zeros");
    _assert_sgraph_equals(g, gl_sgraph(vertices, edges));

    // delete vertex field
    g.remove_vertex_field("id_copy");
    vertices.remove_column("id_copy");
    _assert_sgraph_equals(g, gl_sgraph(vertices, edges));

    // rename vertex field
    g.rename_vertex_fields({"zeros"}, {"__zeros"});
    vertices.rename({{"zeros", "__zeros"}});
    _assert_sgraph_equals(g, gl_sgraph(vertices, edges));
  }

  void test_edge_field_mutation() {
    // reference graph
    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,1}}, {"__dst_id", {2,3}}};
    gl_sgraph g(vertices, edges);

    // add edge field
    g.add_edge_field(g.edges()["__dst_id"], "dst_copy");
    edges.add_column(edges["__dst_id"], "dst_copy");
    _assert_sgraph_equals(g, gl_sgraph(vertices, edges));

    // add const edge field
    g.add_edge_field(1, "ones");
    edges.add_column(1, "ones");
    _assert_sgraph_equals(g, gl_sgraph(vertices, edges));

    // delete edge field
    g.remove_edge_field("dst_copy");
    edges.remove_column("dst_copy");
    _assert_sgraph_equals(g, gl_sgraph(vertices, edges));

    // rename edge field
    g.rename_edge_fields({"ones"}, {"__ones"});
    edges.rename({{"ones", "__ones"}});
    _assert_sgraph_equals(g, gl_sgraph(vertices, edges));
  }

  void test_triple_apply() {
    // reference graph
    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {2,3}}, {"__dst_id", {1,1}}, {"weight", {0.5, 0.5}}};
    gl_sgraph g(vertices, edges);

    // degree count
    auto deg_count_fn = [](edge_triple& triple)->void {
      triple.source["deg"] += 1;
      triple.target["deg"] += 1;
    };

    g.add_vertex_field(0, "deg");
    g = g.triple_apply(deg_count_fn, {"deg"});
    vertices["deg"] = gl_sarray({2,1,1});
    _assert_sframe_equals(g.get_vertices().sort("__id"), vertices);

    // weighted sum
    auto weighted_sum = [](edge_triple& triple)->void {
      triple.target["sum"] += triple.source["sum"] * triple.edge["weight"];
    };

    g.add_vertex_field(1., "sum");
    g = g.triple_apply(weighted_sum, {"sum"});
    vertices["sum"] = gl_sarray({2., 1., 1.});
    _assert_sframe_equals(g.get_vertices().sort("__id"), vertices);
  }

    template<typename T>
    void _assert_vec_equals(const std::vector<T>& sa,
                            const std::vector<T>& sb) {
      TS_ASSERT_EQUALS(sa.size(), sb.size());
      for (size_t i = 0;i < sa.size() ;++i) {
        TS_ASSERT_EQUALS(sa[i], sb[i]);
      }
    }

    std::vector<std::vector<flexible_type> > _to_vec(gl_sframe sa) {
      std::vector<std::vector<flexible_type> > ret;
      for (auto& v: sa.range_iterator()) { ret.push_back(v); }
      return ret;
    }

    void _assert_sframe_equals(gl_sframe sa, gl_sframe sb) {
      TS_ASSERT_EQUALS(sa.size(), sb.size());
      TS_ASSERT_EQUALS(sa.num_columns(), sb.num_columns());
      auto a_cols = sa.column_names();
      auto b_cols = sb.column_names();
      std::sort(a_cols.begin(), a_cols.end());
      std::sort(b_cols.begin(), b_cols.end());
      for (size_t i = 0;i < a_cols.size(); ++i) TS_ASSERT_EQUALS(a_cols[i], b_cols[i]);
      sb = sb[sa.column_names()];
      for (size_t i = 0; i < sa.size(); ++i) {
        _assert_vec_equals(sa[i], sb[i]);
      }
    }

  void _assert_sgraph_equals(gl_sgraph g, gl_sframe vertices, gl_sframe edges) {
    _assert_sframe_equals(g.get_vertices().sort("__id"), vertices.sort("__id"));
    _assert_sframe_equals(g.get_edges().sort({"__src_id", "__dst_id"}),
                          edges.sort({"__src_id", "__dst_id"}));
  }

  void _assert_sgraph_equals(gl_sgraph g, gl_sgraph g2) {
    _assert_sframe_equals(g.get_vertices().sort("__id"), g2.get_vertices().sort("__id"));
    _assert_sframe_equals(g.get_edges().sort({"__src_id", "__dst_id"}),
                          g2.get_edges().sort({"__src_id", "__dst_id"}));
  }

};

BOOST_FIXTURE_TEST_SUITE(_gl_sgraph_test, gl_sgraph_test)
BOOST_AUTO_TEST_CASE(test_constructor) {
  gl_sgraph_test::test_constructor();
}
BOOST_AUTO_TEST_CASE(test_copy) {
  gl_sgraph_test::test_copy();
}
BOOST_AUTO_TEST_CASE(test_field_querys) {
  gl_sgraph_test::test_field_querys();
}
BOOST_AUTO_TEST_CASE(test_get_vertices) {
  gl_sgraph_test::test_get_vertices();
}
BOOST_AUTO_TEST_CASE(test_get_edges) {
  gl_sgraph_test::test_get_edges();
}
BOOST_AUTO_TEST_CASE(test_add_vertices) {
  gl_sgraph_test::test_add_vertices();
}
BOOST_AUTO_TEST_CASE(test_add_edges) {
  gl_sgraph_test::test_add_edges();
}
BOOST_AUTO_TEST_CASE(test_select_fields) {
  gl_sgraph_test::test_select_fields();
}
BOOST_AUTO_TEST_CASE(test_vertex_field_mutation) {
  gl_sgraph_test::test_vertex_field_mutation();
}
BOOST_AUTO_TEST_CASE(test_edge_field_mutation) {
  gl_sgraph_test::test_edge_field_mutation();
}
BOOST_AUTO_TEST_CASE(test_triple_apply) {
  gl_sgraph_test::test_triple_apply();
}
BOOST_AUTO_TEST_SUITE_END()

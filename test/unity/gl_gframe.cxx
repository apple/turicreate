/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <boost/range/combine.hpp>
#include <core/data/sframe/gl_gframe.hpp>
#include <core/data/sframe/gl_sgraph.hpp>
#include <core/data/sframe/gl_sframe.hpp>

using namespace turi;

const flexible_type None = FLEX_UNDEFINED;

struct gl_gframe_test {

public:
  void test_empty_constructor() {
    gl_sgraph g;
    gl_sframe vertices = g.vertices();
    gl_sframe edges = g.edges();

    std::vector<flexible_type> empty;
    gl_sframe vertices_expected{{"__id", empty}};
    gl_sframe edges_expected{{"__src_id", empty}, {"__dst_id", empty}};

    _assert_sframe_equals(vertices, vertices_expected);
    _assert_sframe_equals(edges, edges_expected);
  }

  void test_constructor() {
    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,2,3}}, {"__dst_id", {2,3,1}}};
    gl_sgraph g(vertices, edges, "__id", "__src_id", "__dst_id");

    gl_gframe gf_vertices = g.vertices();
    gl_gframe gf_edges = g.edges();

    _assert_sframe_equals(vertices, gf_vertices.sort("__id"));
    _assert_sframe_equals(edges, gf_edges.sort("__src_id"));
  }

  void test_vertex_gframe_binding() {
    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,2,3}}, {"__dst_id", {2,3,1}}};

    gl_sgraph g(vertices, edges, "__id", "__src_id", "__dst_id");

    gl_gframe gf_vertices = g.vertices();
    gl_gframe gf_edges = g.edges();

    // add vertex field to graph 
    g.add_vertex_field(0, "zeros");
    _assert_sframe_equals(gf_vertices.sort("__id"), g.get_vertices().sort("__id"));

    // remove vertex field from graph 
    g.remove_vertex_field("zeros");
    _assert_sframe_equals(gf_vertices.sort("__id"), g.get_vertices().sort("__id"));

    // add a column to vertex gframe affects graph
    gf_vertices.add_column(1, "ones");
    _assert_sframe_equals(gf_vertices.sort("__id"), g.get_vertices().sort("__id"));

    // remove column from vertex gframe
    gf_vertices.remove_column("ones");
    _assert_sframe_equals(gf_vertices.sort("__id"), g.get_vertices().sort("__id"));

    // assign by sarray ref
    gf_vertices["id_copy"] = gf_vertices["__id"];
    vertices["id_copy"] = vertices["__id"];
    _assert_sframe_equals(gf_vertices.sort("__id"), vertices);
    _assert_sframe_equals(gf_vertices.sort("__id"), g.get_vertices().sort("__id"));

    // rename
    gf_vertices.rename({{"id_copy", "__id_copy"}});
    vertices.rename({{"id_copy", "__id_copy"}});
    _assert_sframe_equals(gf_vertices.sort("__id"), vertices);
    _assert_sframe_equals(gf_vertices.sort("__id"), g.get_vertices().sort("__id"));
  }

  void test_edge_gframe_binding() {
    gl_sframe vertices{{"__id", {1,2,3}}};
    gl_sframe edges{{"__src_id", {1,2,3}}, {"__dst_id", {2,3,1}}};

    gl_sgraph g(vertices, edges, "__id", "__src_id", "__dst_id");

    gl_gframe gf_vertices = g.vertices();
    gl_gframe gf_edges = g.edges();

    // add vertex field to graph 
    g.add_edge_field(0, "zeros");
    _assert_sframe_equals(gf_edges.sort({"__src_id", "__dst_id"}),
        g.get_edges().sort({"__src_id", "__dst_id"}));

    // remove vertex field from graph 
    g.remove_edge_field("zeros");
    _assert_sframe_equals(gf_edges.sort({"__src_id", "__dst_id"}),
        g.get_edges().sort({"__src_id", "__dst_id"}));

    // add a column to edge gframe affects graph 
    gf_edges.add_column(1, "ones");
    _assert_sframe_equals(gf_edges.sort({"__src_id", "__dst_id"}),
        g.get_edges().sort({"__src_id", "__dst_id"}));

    // remove column from edge gframe
    gf_edges.remove_column("ones");
    _assert_sframe_equals(gf_edges.sort({"__src_id", "__dst_id"}),
        g.get_edges().sort({"__src_id", "__dst_id"}));

    // assign by sarray ref
    gf_edges["id_copy"] = gf_edges["__src_id"];
    edges["id_copy"] = edges["__src_id"];
    _assert_sframe_equals(gf_edges.sort("__src_id"), edges);
    _assert_sframe_equals(gf_edges.sort({"__src_id", "__dst_id"}),
        g.get_edges().sort({"__src_id", "__dst_id"}));

    // rename
    gf_edges.rename({{"id_copy", "__src_id_copy"}});
    edges.rename({{"id_copy", "__src_id_copy"}});
    _assert_sframe_equals(gf_edges.sort("__src_id"), edges);
    _assert_sframe_equals(gf_edges.sort({"__src_id", "__dst_id"}),
        g.get_edges().sort({"__src_id", "__dst_id"}));
  }

  void _assert_flexvec_equals(const std::vector<flexible_type>& sa, 
      const std::vector<flexible_type>& sb) {
    TS_ASSERT_EQUALS(sa.size(), sb.size());
    for (size_t i = 0;i < sa.size() ;++i) {
      TS_ASSERT_EQUALS(sa[i], sb[i]);
    }
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
      _assert_flexvec_equals(sa[i], sb[i]);
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_gl_gframe_test, gl_gframe_test)
BOOST_AUTO_TEST_CASE(test_empty_constructor) {
  gl_gframe_test::test_empty_constructor();
}
BOOST_AUTO_TEST_CASE(test_constructor) {
  gl_gframe_test::test_constructor();
}
BOOST_AUTO_TEST_CASE(test_vertex_gframe_binding) {
  gl_gframe_test::test_vertex_gframe_binding();
}
BOOST_AUTO_TEST_CASE(test_edge_gframe_binding) {
  gl_gframe_test::test_edge_gframe_binding();
}
BOOST_AUTO_TEST_SUITE_END()

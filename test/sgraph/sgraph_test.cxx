#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/sgraph_data/sgraph.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include "sgraph_test_util.hpp"

using namespace turi;

struct sgraph_test  {
 public:
  void test_empty_graph() {
    // Empty graph
    sgraph g(4);
    TS_ASSERT(g.empty());
    TS_ASSERT_EQUALS(g.num_vertices(), 0);
    TS_ASSERT_EQUALS(g.num_edges(), 0);
    TS_ASSERT_EQUALS(g.get_num_groups(), 1);
    TS_ASSERT_EQUALS(g.vertex_id_type(), flex_type_enum::UNDEFINED);
    TS_ASSERT_EQUALS(g.get_num_partitions(), 4);
    TS_ASSERT_EQUALS(g.get_vertices().num_rows(), 0);
    TS_ASSERT_EQUALS(g.get_edges().num_rows(), 0);
  }

  void test_add_vertices() {
    size_t n_vertex = 20;
    std::vector<size_t> n_partitions = {2,4,8};
    for (auto& n_partition : n_partitions) {
      sgraph g(n_partition);
      std::vector<flexible_type> data; // first 10 vertices 0,1,2,3,4,...,9
      std::vector<flexible_type> data2; // next 10 vertices 10,11, ..., 19
      for (size_t i = 0; i < n_vertex; ++i) {
        data.push_back(i);
        data2.push_back(i + n_vertex);
      }

      // Let's add vertex 0 to 9, with no data.
      column id_column {"id", flex_type_enum::INTEGER, data};
      sframe id_only_data = create_sframe({id_column});
      g.add_vertices(id_only_data, "id", 0);
      TS_ASSERT(!g.empty());
      TS_ASSERT_EQUALS(g.num_vertices(), n_vertex);
      TS_ASSERT_EQUALS(g.num_edges(), 0);
      TS_ASSERT_EQUALS(g.get_num_groups(), 1);
      TS_ASSERT_EQUALS(g.vertex_id_type(), flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(g.get_num_partitions(), n_partition);
      sframe expected = id_only_data;
      expected.set_column_name(0, sgraph::VID_COLUMN_NAME);
      TS_ASSERT(test_frame_equal(g.get_vertices(), expected, {0}));
      // g.get_vertices().debug_print();

      // Add the same vertex data again, and nothing should change.
      g.add_vertices(id_only_data, "id", 0);
      TS_ASSERT_EQUALS(g.num_vertices(), n_vertex);
      TS_ASSERT_EQUALS(g.get_vertices().num_rows(), n_vertex);
      TS_ASSERT(test_frame_equal(g.get_vertices(), expected, {0}));
      // g.get_vertices().debug_print();

      // Add one data column to the same vertices, data column is the same as id.
      column data_column{"data", flex_type_enum::INTEGER, data};
      sframe vertex_data1 = create_sframe({id_column, data_column});
      g.add_vertices(vertex_data1, "id", 0);
      TS_ASSERT_EQUALS(g.num_vertices(), n_vertex);
      expected = vertex_data1;
      expected.set_column_name(0, sgraph::VID_COLUMN_NAME);
      TS_ASSERT(test_frame_equal(g.get_vertices(), expected, {0}));
      // g.get_vertices().debug_print();

      // Add the next 10 vertices (10, ... 19) with a new data column
      column id_column_2 {"id", flex_type_enum::INTEGER, data2};
      column data_column_2{"data2", flex_type_enum::INTEGER, data2};
      sframe vertex_data2 = create_sframe({id_column_2, data_column_2});
      g.add_vertices(vertex_data2, "id", 0);
      TS_ASSERT_EQUALS(g.num_vertices(), n_vertex * 2);

      std::vector<flexible_type> expected_id_column = data;
      expected_id_column.insert(expected_id_column.end(), data2.begin(), data2.end());
      std::vector<flexible_type> expected_data1 = expected_id_column;
      std::vector<flexible_type> expected_data2 = expected_id_column;
      for (size_t i = 0; i < n_vertex; ++i) {
        expected_data2[i] = FLEX_UNDEFINED;
        expected_data1[n_vertex + i] = FLEX_UNDEFINED;
      }
      expected = create_sframe(
      {
        {"id", flex_type_enum::INTEGER, expected_id_column},
        {"data", flex_type_enum::INTEGER, expected_data1},
        {"data2", flex_type_enum::INTEGER, expected_data2}
      });
      expected.set_column_name(0, sgraph::VID_COLUMN_NAME);
      TS_ASSERT(test_frame_equal(g.get_vertices(), expected, {0}));
      // g.get_vertices().debug_print();
    }
  }

  void test_add_edges() {
    size_t n_vertex = 20;
    std::vector<size_t> n_partitions = {2,4,8};
    for (auto& n_partition : n_partitions) {
      sgraph g(n_partition);
      std::vector<flexible_type> sources;
      std::vector<flexible_type> targets;
      std::vector<flexible_type> data;
      for (size_t i = 0; i < n_vertex; ++i) {
        sources.push_back(i);
        targets.push_back((i + 1) % n_vertex);;
        data.push_back(0.0);
      }
      column source_col = {
        "source",
        flex_type_enum::INTEGER,
        sources
      };
      column target_col = {
        "target",
        flex_type_enum::INTEGER,
        targets
      };
      column data_col = {
        "data",
        flex_type_enum::FLOAT,
        data
      };

      sframe edge_data = create_sframe({source_col, target_col, data_col});

      // Add one direction
      g.add_edges(edge_data, "source", "target", 0, 0);
      TS_ASSERT(!g.empty());
      TS_ASSERT_EQUALS(g.num_vertices(), n_vertex);
      TS_ASSERT_EQUALS(g.num_edges(), n_vertex);
      TS_ASSERT_EQUALS(g.get_num_groups(), 1);
      TS_ASSERT_EQUALS(g.vertex_id_type(), flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(g.get_num_partitions(), n_partition);
      TS_ASSERT_EQUALS(g.get_vertices().num_rows(), n_vertex);
      sframe expected = edge_data;
      expected.set_column_name(0, sgraph::SRC_COLUMN_NAME);
      expected.set_column_name(1, sgraph::DST_COLUMN_NAME);
      TS_ASSERT(test_frame_equal(g.get_edges(), expected, {0, 1}));
      // g.get_edges().debug_print();
      // expected.debug_print();

      // Add the other direction
      g.add_edges(edge_data, "target", "source", 0, 0);
      TS_ASSERT(!g.empty());
      TS_ASSERT_EQUALS(g.num_vertices(), n_vertex);
      TS_ASSERT_EQUALS(g.num_edges(), n_vertex * 2);
      TS_ASSERT_EQUALS(g.get_num_groups(), 1);
      TS_ASSERT_EQUALS(g.vertex_id_type(), flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS(g.get_num_partitions(), n_partition);
      TS_ASSERT_EQUALS(g.get_vertices().num_rows(), n_vertex);

      std::vector<flexible_type> expected_src = sources;
      std::vector<flexible_type> expected_dst = targets;
      std::vector<flexible_type> expected_data(2*n_vertex, 0.0);
      expected_src.insert(expected_src.end(), targets.begin(), targets.end());
      expected_dst.insert(expected_dst.end(), sources.begin(), sources.end());
      expected = create_sframe({
        {sgraph::SRC_COLUMN_NAME, flex_type_enum::INTEGER, expected_src},
        {sgraph::DST_COLUMN_NAME, flex_type_enum::INTEGER, expected_dst},
        {"data", flex_type_enum::FLOAT, expected_data}
      });
      TS_ASSERT(test_frame_equal(g.get_edges(), expected, {0, 1}));
      // g.get_edges().debug_print();
      // expected.debug_print();
    }
  }

  void test_add_edges_cross_group() {
    size_t n_vertex = 20;
    size_t n_partition = 8;
    sgraph g(n_partition);
    std::vector<flexible_type> sources;
    std::vector<flexible_type> targets;
    for (size_t i = 0; i < n_vertex; ++i) {
      sources.push_back(i);
      targets.push_back((i + 1) % n_vertex);;
    }
    column source_col = {
      "source",
      flex_type_enum::INTEGER,
      sources
    };
    column target_col = {
      "target",
      flex_type_enum::INTEGER,
      targets
    };

    sframe edge_data = create_sframe({source_col, target_col});

    sgraph::options_map_t empty_constraint;
    // Add edges from group 0 to group 1
    g.add_edges(edge_data, "source", "target", 0, 1);
    TS_ASSERT(!g.empty());
    TS_ASSERT_EQUALS(g.num_vertices(0), n_vertex);
    TS_ASSERT_EQUALS(g.num_vertices(1), n_vertex);
    TS_ASSERT_EQUALS(g.num_vertices(), 2 * n_vertex);
    TS_ASSERT_EQUALS(g.num_edges(0, 1), n_vertex);
    TS_ASSERT_EQUALS(g.num_edges(), n_vertex);
    TS_ASSERT_EQUALS(g.get_num_groups(), 2);
    TS_ASSERT_EQUALS(g.vertex_id_type(), flex_type_enum::INTEGER);
    TS_ASSERT_EQUALS(g.get_num_partitions(), n_partition);
    TS_ASSERT_EQUALS(g.get_vertices({}, empty_constraint, 0).num_rows(), n_vertex);
    TS_ASSERT_EQUALS(g.get_vertices({}, empty_constraint, 1).num_rows(), n_vertex);
    TS_ASSERT_EQUALS(g.get_edges({}, {}, empty_constraint, 0, 1).num_rows(), n_vertex);
    TS_ASSERT_EQUALS(g.get_edges({}, {}, empty_constraint, 1, 0).num_rows(), 0);

    // Add edges from group 1 to group 0
    g.add_edges(edge_data, "source", "target", 1, 0);
    TS_ASSERT(!g.empty());
    TS_ASSERT_EQUALS(g.num_vertices(0), n_vertex);
    TS_ASSERT_EQUALS(g.num_vertices(1), n_vertex);
    TS_ASSERT_EQUALS(g.num_vertices(), 2 * n_vertex);
    TS_ASSERT_EQUALS(g.num_edges(0, 1), n_vertex);
    TS_ASSERT_EQUALS(g.num_edges(1, 0), n_vertex);
    TS_ASSERT_EQUALS(g.num_edges(), 2 * n_vertex);
    TS_ASSERT_EQUALS(g.get_num_groups(), 2);
    TS_ASSERT_EQUALS(g.vertex_id_type(), flex_type_enum::INTEGER);
    TS_ASSERT_EQUALS(g.get_num_partitions(), n_partition);
    TS_ASSERT_EQUALS(g.get_vertices({}, empty_constraint, 0).num_rows(), n_vertex);
    TS_ASSERT_EQUALS(g.get_vertices({}, empty_constraint, 1).num_rows(), n_vertex);
    TS_ASSERT_EQUALS(g.get_edges({}, {}, empty_constraint, 0, 1).num_rows(), n_vertex);
    TS_ASSERT_EQUALS(g.get_edges({}, {}, empty_constraint, 1, 0).num_rows(), n_vertex);
  }

  void test_ring_graph() {
    std::vector<size_t> npartitions = {4,8};
    std::vector<size_t> nvertices = {100, 1000};
    std::vector<int> bidirectional = {0, 1};
    for (auto& nparts : npartitions) {
      for (auto& nverts: nvertices) {
        for (auto& bidir : bidirectional) {
          create_ring_graph(nverts, nparts, bidir, true /* validate=true */);
        }
      }
    }
  }

  void test_star_graph() {
    std::vector<size_t> npartitions = {4,8};
    std::vector<size_t> nvertices = {100, 1000};
    std::vector<int> bidirectional = {0, 1};
    for (auto& nparts : npartitions) {
      for (auto& nverts: nvertices) {
        for (auto& bidir : bidirectional) {
          create_star_graph(nverts, nparts, bidir, true /* validate=true */);
        }
      }
    }
  }

  template<typename T>
  void assert_vector_equals(
      const std::vector<T>& a,
      const std::vector<T>& b) {
    TS_ASSERT_EQUALS(a.size(), b.size());
    for (size_t i = 0; i < a.size(); ++i)
      TS_ASSERT_EQUALS(a[i], b[i]);
  }

  void test_graph_field_query() {
    sgraph g;
    sframe vertices = create_sframe(
        {
          {"vid", flex_type_enum::STRING, {'a', 'b', 'c'}},
          {"vdata", flex_type_enum::INTEGER, {1, 2, 3}},
        });

    sframe edges = create_sframe(
      {
        {"src_id", flex_type_enum::STRING, {'a', 'b', 'c'}},
        {"dst_id", flex_type_enum::STRING, {'b', 'c', 'a'}},
        {"edata", flex_type_enum::FLOAT, {1.0, 2.0, 3.0}}
      });
    g.add_vertices(vertices, "vid");
    g.add_edges(edges, "src_id", "dst_id");

    std::vector<std::string> expected_vfields{"__id", "vdata"};
    std::vector<std::string> expected_efields{"__src_id", "__dst_id", "edata"};
    std::vector<flex_type_enum> expected_vfield_types{
      flex_type_enum::STRING, flex_type_enum::INTEGER
    };
    std::vector<flex_type_enum> expected_efield_types{
      flex_type_enum::STRING, flex_type_enum::STRING, flex_type_enum::FLOAT
    };
    assert_vector_equals(expected_vfields, g.get_vertex_fields());
    assert_vector_equals(expected_efields, g.get_edge_fields());
    assert_vector_equals(expected_vfield_types, g.get_vertex_field_types());
    assert_vector_equals(expected_efield_types, g.get_edge_field_types());
  }
};

BOOST_FIXTURE_TEST_SUITE(_sgraph_test, sgraph_test)
BOOST_AUTO_TEST_CASE(test_empty_graph) {
  sgraph_test::test_empty_graph();
}
BOOST_AUTO_TEST_CASE(test_add_vertices) {
  sgraph_test::test_add_vertices();
}
BOOST_AUTO_TEST_CASE(test_add_edges) {
  sgraph_test::test_add_edges();
}
BOOST_AUTO_TEST_CASE(test_add_edges_cross_group) {
  sgraph_test::test_add_edges_cross_group();
}
BOOST_AUTO_TEST_CASE(test_ring_graph) {
  sgraph_test::test_ring_graph();
}
BOOST_AUTO_TEST_CASE(test_star_graph) {
  sgraph_test::test_star_graph();
}
BOOST_AUTO_TEST_CASE(test_graph_field_query) {
  sgraph_test::test_graph_field_query();
}
BOOST_AUTO_TEST_SUITE_END()

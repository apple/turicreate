#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <unity/lib/variant.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/gl_sgraph.hpp>

#include <unity/dml/dml_class_registry.hpp>
#include <unity/dml/dml_toolkit_runner.hpp>

#include <sframe/testing_utils.hpp>
#include "graph_testing_utils.hpp"

using namespace turi;
using namespace turi::graph_testing_utils;


struct graph_ingress_test {
 public:

  void test_random_graph_small() {
    size_t v_size = 100;
    size_t e_size = 500;

    std::vector<flexible_type> vid(v_size);
    std::generate(vid.begin(), vid.end(), std::rand);

    std::vector<flexible_type> src_id(e_size);
    std::vector<flexible_type> dst_id(e_size);

    std::generate(src_id.begin(), src_id.end(), [&](void){ return vid[std::rand() % v_size];});
    std::generate(dst_id.begin(), dst_id.end(), [&](void){ return vid[std::rand() % v_size];});

    gl_sframe vertex_data{{"vid", vid}};
    gl_sframe edge_data{{"src", src_id},
                        {"dst", dst_id}};

    size_t num_partitions = 2;
    size_t n = runner.get_default_num_workers_from_env();
    test_impl(vertex_data, edge_data, "vid", "src", "dst", num_partitions, n);
  }

  void test_empty_graph() {
    gl_sarray empty_sa(std::vector<flexible_type>(), flex_type_enum::INTEGER);
    gl_sframe vertex_data{{"vid", empty_sa}};
    gl_sframe edge_data{{"src", empty_sa}, {"dst", empty_sa}};

    size_t num_partitions = 2;
    test_impl(vertex_data, edge_data, "vid", "src", "dst", num_partitions, num_partitions);
  }

  void test_vertex_data_only() {
    size_t v_size = 100;

    std::vector<flexible_type> vid(v_size);
    std::generate(vid.begin(), vid.end(), std::rand);
    gl_sframe vertex_data{{"vid", vid}};

    gl_sarray empty_sa(std::vector<flexible_type>(), flex_type_enum::INTEGER);
    gl_sframe edge_data{{"src", empty_sa}, {"dst", empty_sa}};

    size_t num_partitions = 2;
    test_impl(vertex_data, edge_data, "vid", "src", "dst", num_partitions, 2);
  }

  void test_edge_data_only() {
    size_t e_size = 500;

    gl_sarray empty_sa(std::vector<flexible_type>(), flex_type_enum::INTEGER);
    gl_sframe vertex_data{{"vid", empty_sa}};

    std::vector<flexible_type> src_id(e_size);
    std::vector<flexible_type> dst_id(e_size);

    std::generate(src_id.begin(), src_id.end(), [&](void){ return std::rand(); });
    std::generate(dst_id.begin(), dst_id.end(), [&](void){ return std::rand(); });

    gl_sframe edge_data{{"src", src_id},
                        {"dst", dst_id}};

    size_t num_partitions = 2;
    test_impl(vertex_data, edge_data, "vid", "src", "dst", num_partitions, 2);
  }

  void setup() {
    runner.set_library("libdistributed_graph_analytics.so");
    working_dir = turi::get_temp_name();
    fileio::create_directory(working_dir);
  }

  void teardown() {
    fileio::delete_path_recursive(working_dir);
  }

  void test_impl(gl_sframe vertex_data,
                 gl_sframe edge_data,
                 flexible_type vid_field,
                 flexible_type src_id_field,
                 flexible_type dst_id_field,
                 size_t num_partitions,
                 size_t num_workers) {
    setup();

    try {
      // Init
      variant_map_type args;
      args["vertex_data"] = vertex_data;
      args["edge_data"] = edge_data;
      args["vid_field"] = vid_field;
      args["src_field"] = src_id_field;
      args["dst_field"] = dst_id_field;
      args["num_partitions"] = num_partitions;
      args["output_path"] = working_dir + "/saved_graph";

      // Act
      runner.run("distributed_graph_ingress", args, working_dir, num_workers);

      // Assert
      {
        gl_sgraph actual_g(working_dir + "/saved_graph");
        gl_sgraph expect_g(vertex_data, edge_data, vid_field, src_id_field, dst_id_field);
        ASSERT_EQ(actual_g.num_vertices(), expect_g.num_vertices());
        ASSERT_EQ(actual_g.num_edges(), expect_g.num_edges());

        if (vertex_data.size() > 0) {
          gl_sframe vdata = actual_g.vertices().sort("__id");
          gl_sframe vdata_expected = expect_g.vertices().sort("__id");

          ASSERT_EQ(vdata.size(), vdata_expected.size());
          ASSERT_EQ((vdata["__id"] - vdata_expected["__id"]).sum(), 0);
          ASSERT_EQ((vdata["__id"] - vdata_expected["__id"]).sum(), 0);
        }

        if (edge_data.size() > 0) {
          gl_sframe edata = actual_g.edges().sort({"__src_id", "__dst_id"});
          gl_sframe edata_expected = expect_g.edges().sort({"__src_id", "__dst_id"});

          ASSERT_EQ(edata.size(), edata_expected.size());
          ASSERT_EQ((edata["__src_id"] - edata_expected["__src_id"]).sum(), 0);
          ASSERT_EQ((edata["__dst_id"] - edata_expected["__dst_id"]).sum(), 0);
        }
      }
    } catch (...) {
      teardown();
      throw;
    }
    teardown();
  }

  dml_toolkit_runner runner;
  std::string working_dir;
};

BOOST_FIXTURE_TEST_SUITE(_graph_ingress_test, graph_ingress_test)
BOOST_AUTO_TEST_CASE(test_random_graph_small) {
  graph_ingress_test::test_random_graph_small();
}
BOOST_AUTO_TEST_CASE(test_empty_graph) {
  graph_ingress_test::test_empty_graph();
}
BOOST_AUTO_TEST_CASE(test_vertex_data_only) {
  graph_ingress_test::test_vertex_data_only();
}
BOOST_AUTO_TEST_CASE(test_edge_data_only) {
  graph_ingress_test::test_edge_data_only();
}
BOOST_AUTO_TEST_SUITE_END()

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <unity/lib/variant.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/simple_model.hpp>
#include <unity/dml/dml_class_registry.hpp>

#include "graph_testing_utils.hpp"
#include <unity/dml/dml_toolkit_runner.hpp>

using namespace turi;
using namespace turi::graph_testing_utils;

/**
 * Test suite for distributed linear regression. 
 */
struct label_propagation_test  {
 public:

  void test_default() {
    std::map<std::string, flexible_type> options = { 
      {"label_field", "label"},
      {"threshold", 0.0001},
      {"self_weight", 1.0},
      {"undirected", 1},
      {"max_iterations", 30},
      {"weight_field", ""}
    };

    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(options, n);
  }

  void test_weighted() {
    std::map<std::string, flexible_type> options = { 
      {"label_field", "label"},
      {"threshold", 0.0001},
      {"self_weight", 1.0},
      {"undirected", 1},
      {"max_iterations", 30},
      {"weight_field", "data"} 
    };

    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(options, n);
  }

  void setup() {
    runner.set_library("libdistributed_graph_analytics.so");
    g = create_zachary_dataset();
    dml_class_registry::get_instance().register_model<simple_model>();
    working_dir = turi::get_temp_name();
    fileio::create_directory(working_dir);
  }

  void teardown() {
    fileio::delete_path_recursive(working_dir);
  }

  void test_impl(std::map<std::string,
                 flexible_type> opts,
                 size_t num_workers) {
    setup();

    try {
      // Init
      std::shared_ptr<unity_sgraph> ug(new unity_sgraph(std::make_shared<sgraph>(g)));
      variant_map_type args;
      args["graph"] = to_variant(ug);

      for (auto& kv: opts) {
        args[kv.first] = to_variant(kv.second);
      }

      variant_type ret = runner.run("distributed_labelprop", args, working_dir, num_workers);

      // Act
      auto m = variant_get_value<std::shared_ptr<simple_model>>(ret);

      // Assert 
      {
        std::shared_ptr<unity_sgraph> result_g;
        result_g = variant_get_value<std::shared_ptr<unity_sgraph>>(m->params.at("graph"));

        auto vertex_data = *(std::static_pointer_cast<unity_sframe>(
                              result_g->get_vertices())->get_underlying_sframe());
        TS_ASSERT(vertex_data.contains_column("predicted_label"));
        TS_ASSERT(!vertex_data.contains_column("expected"));

        // Get both columsn from SFrame
        std::vector<flexible_type> preds;
        std::shared_ptr<sarray<flexible_type>> preds_sa = vertex_data.select_column("predicted_label");

        auto original_vertex_data = *(std::static_pointer_cast<unity_sframe>(
                                      ug->get_vertices())->get_underlying_sframe());
        std::vector<flexible_type> expected;
        std::shared_ptr<sarray<flexible_type>> expected_sa = original_vertex_data.select_column("expected");

        size_t N = vertex_data.num_rows();
        auto pred_reader = preds_sa->get_reader();
        pred_reader->read_rows(0, N, preds);
        auto expected_reader = expected_sa->get_reader();
        expected_reader->read_rows(0, N, expected);
        for (size_t i = 0; i < N; ++i) {
          TS_ASSERT_EQUALS((int)preds[i], (int)expected[i]);
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
  sgraph g;

};

BOOST_FIXTURE_TEST_SUITE(_label_propagation_test, label_propagation_test)
BOOST_AUTO_TEST_CASE(test_default) {
  label_propagation_test::test_default();
}
BOOST_AUTO_TEST_CASE(test_weighted) {
  label_propagation_test::test_weighted();
}
BOOST_AUTO_TEST_SUITE_END()

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


struct pagerank_test {
 public:

  void test_default() {
    std::map<std::string, flexible_type> options = {
      {"threshold", 1e-2},
      {"max_iterations", 20},
      {"reset_probability", 0.15},
    };
    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(options, 9.95996, n);
  }

  void test_advanced() {
    std::map<std::string, flexible_type> options = {
      {"threshold", 1e-10},
      {"max_iterations", 20},
      {"reset_probability", 0.3},
    };
    size_t n = runner.get_default_num_workers_from_env(); 
    test_impl(options, 17.68454, n);
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
                 double expected_total_pr,
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

      variant_type ret = runner.run("distributed_pagerank", args, working_dir, num_workers);

      // Act
      auto m = variant_get_value<std::shared_ptr<simple_model>>(ret);

      // Assert 
      {
        auto delta = variant_get_value<double>(m->params.at("delta"));
        auto threshold = variant_get_value<double>(m->params.at("threshold"));
        TS_ASSERT(delta < threshold);

        ug = variant_get_value<std::shared_ptr<unity_sgraph>>(m->params.at("graph"));
        auto vertex_data = std::static_pointer_cast<unity_sframe>(ug->get_vertices());
        double total_pagerank = vertex_data->select_column("pagerank")->sum();
        double total_delta = vertex_data->select_column("delta")->sum();
        TS_ASSERT_DELTA(delta, total_delta, 1e-5);
        TS_ASSERT_DELTA(total_pagerank, expected_total_pr, 1e-5);
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

BOOST_FIXTURE_TEST_SUITE(_pagerank_test, pagerank_test)
BOOST_AUTO_TEST_CASE(test_default) {
  pagerank_test::test_default();
}
BOOST_AUTO_TEST_CASE(test_advanced) {
  pagerank_test::test_advanced();
}
BOOST_AUTO_TEST_SUITE_END()

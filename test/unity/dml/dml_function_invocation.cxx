#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>
#include <unistd.h>

#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/simple_model.hpp>

#include <sframe/testing_utils.hpp>
#include <unity/dml/dml_function_invocation.hpp>

#include <toolkits/supervised_learning/linear_regression.hpp>
#include <fileio/temp_files.hpp>
#include <fileio/fs_utils.hpp>


using namespace turi::supervised;
using namespace turi;
using namespace turi::fileio;
using namespace turi::dml;

struct dml_function_invocation_test  {
 
 public:
  std::string working_dir;

  dml_function_invocation_test() {
    working_dir = turi::get_temp_name();
    create_directory(working_dir);
  }

  ~dml_function_invocation_test() {
    delete_path_recursive(working_dir);
  }

  void test_flex_types() {

    // Arrange.
    std::map<std::string, turi::flexible_type> options = {
      {"convergence_threshold", 1e-2},
      {"step_size", 1.0},
      {"lbfgs_memory_level", 3},
      {"max_iterations", 10},
      {"solver", "auto"},
      {"l1_penalty", 0.0},
      {"l2_penalty", 0.0},
      {"quoted_string", "\"hello world\""}
    };

    variant_map_type params;
    for (const auto& kvp: options) {
      params[kvp.first] = to_variant(kvp.second);
    }

    // Act & Assert: From dict.
    dml_function_invocation args;
    args.from_dict(params, working_dir);
    for (const auto& kvp: options) {
      TS_ASSERT(args.exists(kvp.first));
      TS_ASSERT_EQUALS(variant_get_value<flexible_type>(args.get_value(kvp.first)),
                       kvp.second);
      TS_ASSERT_EQUALS(args.get_type(kvp.first), std::string("flexible_type"));
    }

    // Act & Assert: To dict. 
    variant_map_type args_dict = args.to_dict();
    for (const auto& kvp: args_dict) {
      TS_ASSERT_EQUALS(variant_get_value<flexible_type>(kvp.second),
                       variant_get_value<flexible_type>(params.at(kvp.first)));
    }


    // Act & Assert: To string 
    std::string str_args = args.to_str();
    std::string ans = "{\"convergence_threshold\":[\"flexible_type\",0.01], "
                       "\"l1_penalty\":[\"flexible_type\",0], "
                       "\"l2_penalty\":[\"flexible_type\",0], "
                       "\"lbfgs_memory_level\":[\"flexible_type\",3], "
                       "\"max_iterations\":[\"flexible_type\",10], "
                       "\"quoted_string\":[\"flexible_type\",\"\\\"hello world\\\"\"], "
                       "\"solver\":[\"flexible_type\",\"auto\"], "
                       "\"step_size\":[\"flexible_type\",1]}";
    TS_ASSERT_EQUALS(str_args, ans);


    // Act & Assert: From string 
    dml_function_invocation args2;
    args2.from_str(str_args);
    for (const auto& kvp: options) {
      TS_ASSERT(args.exists(kvp.first));
      TS_ASSERT_EQUALS(variant_get_value<flexible_type>(args.get_value(kvp.first)), kvp.second);
      TS_ASSERT_EQUALS(args.get_type(kvp.first), std::string("flexible_type"));
    }
    TS_ASSERT(!args.exists(std::string("cricket")));
    

  }


  void test_stypes() {

    // Arrange.
    std::vector<flexible_type> test_data;
    std::shared_ptr<unity_sframe> sf(new unity_sframe());
    std::shared_ptr<unity_sarray> sa(new unity_sarray());
    std::shared_ptr<unity_sgraph> sg(new unity_sgraph());

    sa->construct_from_vector(test_data, flex_type_enum::INTEGER);
    sf->add_column(sa, "a");

    variant_map_type params;
    params["sf"] = to_variant(sf);
    params["sa"] = to_variant(sa);
    params["sg"] = to_variant(sg);

    // Act & Assert: From dict.
    dml_function_invocation args;
    args.from_dict(params, working_dir);
    TS_ASSERT(args.exists("sf"));
    TS_ASSERT_EQUALS(args.get_type("sf"), std::string("SFrame"));
    TS_ASSERT_EQUALS(get_variant_which_name(args.get_value("sf").which()), 
                     std::string("SFrame"));
    TS_ASSERT(args.exists("sa"));
    TS_ASSERT_EQUALS(args.get_type("sa"), std::string("SArray"));
    TS_ASSERT_EQUALS(get_variant_which_name(args.get_value("sa").which()), 
                     std::string("SArray"));
    TS_ASSERT(args.exists("sg"));
    TS_ASSERT_EQUALS(args.get_type("sg"), std::string("SGraph"));
    TS_ASSERT_EQUALS(get_variant_which_name(args.get_value("sg").which()), 
                     std::string("SGraph"));

    // Act & Assert: To dict. 
    variant_map_type args_dict = args.to_dict();
    for (const auto& kvp: args_dict) {
      TS_ASSERT(params.find(kvp.first) != args_dict.end());
    }

    // Act & Assert: To string 
    std::string cwd = working_dir;
    std::string str_args = args.to_str();
    std::string ans = "{\"sa\":[\"SArray\",\"" + cwd + "/sa\"], " +
                       "\"sf\":[\"SFrame\",\"" + cwd + "/sf\"], " +
                       "\"sg\":[\"SGraph\",\"" + cwd + "/sg\"]}";
    TS_ASSERT_EQUALS(str_args, ans);


    // Act & Assert: From string 
    dml_function_invocation args2;
    args2.from_str(str_args);
    for (const auto& kvp: params) {
      TS_ASSERT(args.exists(kvp.first));
      TS_ASSERT_EQUALS(args.get_type(kvp.first), args2.get_type(kvp.first));
    }

  }

  void test_models() {

    // Arrange
    // --------------------------------------------------------------
    size_t features = 10;
    size_t examples = 100;

    // Generate some data.
    std::string feature_types;
    for(size_t i=0; i < features; i++) feature_types += "n";
    sframe data = make_random_sframe(examples, feature_types, true);
    sframe y = data.select_columns({"target"});
    sframe X = data;
    X = X.remove_column(X.column_index("target"));

    // Setup the arguments. 
    std::map<std::string, flexible_type> options = { 
      {"convergence_threshold", 1e-2},
      {"step_size", 1.0},
      {"lbfgs_memory_level", 3},
      {"max_iterations", 10},
      {"solver", "auto"},
      {"l1_penalty", 0.0},
      {"l2_penalty", 0.0}
    };
 
    // Train the model. 
    std::shared_ptr<linear_regression> model;
    model.reset(new linear_regression);
    model->init(X,y);
    model->init_options(options);
    model->train();

    variant_map_type params;
    params["model"] = model;
    
    // Act & Assert: From dict.
    // --------------------------------------------------------------
    dml_function_invocation args;
    args.from_dict(params, working_dir);
    TS_ASSERT(args.exists("model"));
    std::map<std::string, flexible_type> _options = 
         variant_get_value<std::shared_ptr<linear_regression>>(
                    args.get_value("model"))->get_current_options();
    for (auto& kvp: options){
      TS_ASSERT(_options[kvp.first] == kvp.second);
    }
    TS_ASSERT(model->is_trained() == true);
    
    // Act & Assert: To dict. 
    variant_map_type args_dict = args.to_dict();
    for (const auto& kvp: args_dict) {
      TS_ASSERT(params.find(kvp.first) != args_dict.end());
    }

    // Act & Assert: To string 
    std::string cwd = working_dir;
    std::string str_args = args.to_str();
    std::string ans = "{\"model\":[\"Model\",\"" + cwd + "/model\"]}";
    TS_ASSERT_EQUALS(str_args, ans);


    // Act & Assert: From string 
    dml_function_invocation args2;
    args2.from_str(str_args);
    for (const auto& kvp: params) {
      TS_ASSERT(args.exists(kvp.first));
      TS_ASSERT_EQUALS(args.get_type(kvp.first), args2.get_type(kvp.first));
    }
  }

  void test_graph_models() {
    // Arrange
    // --------------------------------------------------------------
    auto model = std::make_shared<simple_model>();
    auto g = std::make_shared<unity_sgraph>();
    model->params["graph"] = to_variant(g);
    variant_map_type params;
    params["model"] = model;
    
    // Act & Assert: From dict.
    // --------------------------------------------------------------
    dml_function_invocation args;
    args.from_dict(params, working_dir);
    TS_ASSERT(args.exists("model"));
    auto m2 = variant_get_value<std::shared_ptr<simple_model>>(args.get_value("model"));
    TS_ASSERT(m2->params.count("graph") > 0);
    auto g2 = variant_get_value<std::shared_ptr<unity_sgraph>>(m2->params["graph"]);
    
    // Act & Assert: To dict. 
    variant_map_type args_dict = args.to_dict();
    for (const auto& kvp: args_dict) {
      TS_ASSERT(params.find(kvp.first) != args_dict.end());
    }

    // Act & Assert: To string 
    std::string cwd = working_dir;
    std::string str_args = args.to_str();
    std::string ans = "{\"model\":[\"Model\",\"" + cwd + "/model\"]}";
    TS_ASSERT_EQUALS(str_args, ans);


    // Act & Assert: From string 
    dml_function_invocation args2;
    args2.from_str(str_args);
    for (const auto& kvp: params) {
      TS_ASSERT(args.exists(kvp.first));
      TS_ASSERT_EQUALS(args.get_type(kvp.first), args2.get_type(kvp.first));
    }
  }

};

BOOST_FIXTURE_TEST_SUITE(_dml_function_invocation_test, dml_function_invocation_test)
BOOST_AUTO_TEST_CASE(test_flex_types) {
  dml_function_invocation_test::test_flex_types();
}
BOOST_AUTO_TEST_CASE(test_stypes) {
  dml_function_invocation_test::test_stypes();
}
BOOST_AUTO_TEST_CASE(test_models) {
  dml_function_invocation_test::test_models();
}
BOOST_AUTO_TEST_CASE(test_graph_models) {
  dml_function_invocation_test::test_graph_models();
}
BOOST_AUTO_TEST_SUITE_END()

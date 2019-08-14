#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>

#include <random>

// ML-Data Utils
#include <ml/ml_data/ml_data.hpp>

// Models
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/boosted_trees.hpp>

#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::supervised;
using namespace turi::supervised::xgboost;

void run_boosted_trees_classifier_test(
    std::map<std::string, flexible_type> test_opts,
    std::map<std::string, flexible_type> model_opts = std::map<std::string, flexible_type>()) {

  size_t examples = test_opts.at("examples");
  size_t features = test_opts.at("features");
  std::string target_column_name = "target";
  std::vector<std::string> column_names = {"user", "item"};

  // Answers
  // -----------------------------------------------------------------------
  DenseVector coefs(features+1);
  coefs.setRandom();

  // Feature names
  std::vector<std::string> feature_names;
  std::vector<flex_type_enum> feature_types;
  for(size_t i=0; i < features; i++){
    feature_names.push_back(std::to_string(i));
    feature_types.push_back(flex_type_enum::FLOAT);
  }

  // Data
  std::vector<std::vector<flexible_type>> y_data;
  std::vector<std::vector<flexible_type>> X_data;
  for(size_t i=0; i < examples; i++){
    DenseVector x(features);
    x.setRandom();
    std::vector<flexible_type> x_tmp;
    for(size_t k=0; k < features; k++){
      x_tmp.push_back(x(k));
    }

    // Compute the prediction for this
    double t = x.dot(coefs.segment(0, features)) + coefs(features);
    t = 1.0/(1.0+exp(-1.0*t));
    int c = turi::random::bernoulli(t);
    std::vector<flexible_type> y_tmp;
    y_tmp.push_back(c);

    X_data.push_back(x_tmp);
    y_data.push_back(y_tmp);
  }

  // Options
  std::map<std::string, flexible_type> options = model_opts;

  // Make the data
  sframe X = make_testing_sframe(feature_names, feature_types, X_data);
  sframe y = make_testing_sframe({"target"}, {flex_type_enum::INTEGER}, y_data);

  // Train model
  std::shared_ptr<boosted_trees_classifier> model;
  model.reset(new boosted_trees_classifier);
  model->init(X,y);
  model->init_options(options);
  if (test_opts.count("external_memory")) {
    model->_set_storage_mode(storage_mode_enum::EXT_MEMORY);
    model->_set_num_batches(8);
  }
  model->train();

  // Check options
  // ----------------------------------------------------------------------
  std::map<std::string, flexible_type> _options;
  _options = model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }
  TS_ASSERT(model->is_trained() == true);


  // Check predictions
  // ----------------------------------------------------------------------

  // Construct the ml_data
  ml_data data = model->construct_ml_data_using_current_metadata(X, y);

  std::vector<flexible_type> pred_class;
  std::shared_ptr<sarray<flexible_type>> _pred_class
    = model->predict(data, "class");


  // Check that we can train a model when providing a validation set
  std::vector<std::vector<flexible_type>> y_v;
  std::vector<std::vector<flexible_type>> X_v;
  for(size_t i=0; i < 5; i++){
    DenseVector x(features);
    x.setRandom();
    std::vector<flexible_type> x_tmp;
    for(size_t k=0; k < features; k++){
      x_tmp.push_back(x(k));
    }

    // Compute the prediction for this
    double t = x.dot(coefs.segment(0, features)) + coefs(features);
    t = 1.0/(1.0+exp(-1.0*t));
    int c = turi::random::bernoulli(t);
    std::vector<flexible_type> y_tmp;
    y_tmp.push_back(1 + c);

    X_v.push_back(x_tmp);
    y_v.push_back(y_tmp);
  }

  sframe Xv = make_testing_sframe(feature_names, feature_types, X_v);
  sframe yv = make_testing_sframe({"target"}, {flex_type_enum::INTEGER}, y_v);

  model.reset(new boosted_trees_classifier);
  model->init(X, y, Xv, yv);
  model->init_options(options);
  if (test_opts.count("external_memory")) {
    model->_set_storage_mode(storage_mode_enum::EXT_MEMORY);
    model->_set_num_batches(8);
  }

  model->train();

  ml_data valid_data = model->construct_ml_data_using_current_metadata(Xv, yv);
  auto ret = model->evaluate(valid_data, "accuracy");
  for (auto kv : ret) {
    logprogress_stream << kv.first << ": "
                       << variant_get_value<flexible_type>(kv.second)
                       << std::endl;
  }
  ret = model->evaluate(valid_data, "rmse");
  for (auto kv : ret) {
    logprogress_stream << kv.first << ": "
                       << variant_get_value<flexible_type>(kv.second)
                       << std::endl;
  }
}

struct boosted_trees_classifier_test  {
  public:
  void test_boosted_trees_classifier_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 100},
      {"features", 1}};
    run_boosted_trees_classifier_test(opts);
  }

  void test_boosted_trees_classifier_small() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000},
      {"features", 10}};
    run_boosted_trees_classifier_test(opts);
  }

  void test_boosted_trees_classifier_with_insufficient_column_subsample() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000},
      {"features", 10}
    };
    std::map<std::string, flexible_type> model_opts = {
      {"column_subsample", 0.01}
    };
    run_boosted_trees_classifier_test(opts, model_opts);
  }

  void test_boosted_trees_classifier_external_memory() {
    std::map<std::string, flexible_type> opts = {
      {"examples", 1000},
      {"features", 10},
      {"external_memory", 1},
      {"row_subsample", .5},
      {"column_subsample", .5},
    };
    run_boosted_trees_classifier_test(opts);
  }
};

struct boosted_trees_stress_test  {

  void run_stress(size_t n, const std::string& run_string,
                  const std::string& target_column_str) {

    random::seed(0);

    size_t ntest = 10;

    sframe X = make_random_sframe(n, run_string, false);
    sframe X2 = make_random_sframe(ntest, run_string, false);
    sframe y = make_random_sframe(n, target_column_str, false);
    sframe y2 = make_random_sframe(ntest, target_column_str, false);
    y.set_column_name(0, "target");
    y2.set_column_name(0, "target");

    std::shared_ptr<xgboost_model> model;

    for (auto storage_mode : {storage_mode_enum::IN_MEMORY, storage_mode_enum::EXT_MEMORY}) {
      // Skip external memory stress test for large target columns
      if (storage_mode == storage_mode_enum::EXT_MEMORY &&
          (target_column_str[0] == 'S' || target_column_str[0] == 's'))
        continue;

      if(target_column_str == "n")
        model.reset(new boosted_trees_regression);
      else
        model.reset(new boosted_trees_classifier);

      // Options
      std::map<std::string, flexible_type> options = {
        {"max_iterations", 3},
      };

      model->init(X,y);
      model->init_options(options);
      model->_set_storage_mode(storage_mode);
      if (storage_mode == storage_mode_enum::EXT_MEMORY)
        model->_set_num_batches(8);
      model->train();

      // Construct the ml_data
      ml_data data = model->construct_ml_data_using_current_metadata(X2, y2);
      size_t num_classes = model->num_classes();

      // Check prediction API
      // ----------------------------------------------------------------------
      if (model->is_classifier()) {

        std::cout << "Check predict class" << std::endl;
        // predict
        auto preds = model->predict(data, "class");
        TS_ASSERT_EQUALS(preds->size(), data.num_rows());

        std::cout << "Check predict prob vector" << std::endl;

        // predict all
        auto preds_vec = model->predict(data, "probability_vector");
        TS_ASSERT_EQUALS(preds->size(), data.num_rows());

        std::cout << "Check classify " << std::endl;
        // classify
        auto classify_vec = model->classify(data);
        TS_ASSERT_EQUALS(preds->size(), data.num_rows());

        // predict topk
        std::vector<size_t> topk{1, 2, num_classes};
        for (size_t k : topk) {
          std::cout << "Check predict topk=" <<  k << std::endl;
          auto topk_preds = model->predict_topk(data, "probability", k);
          TS_ASSERT_EQUALS(topk_preds.size(), data.num_rows() * k);
          std::vector<flexible_type> probs;
          topk_preds.select_column("probability")->get_reader()->read_rows(0, topk_preds.size(), probs);
          for (auto& p : probs) {
            TS_ASSERT(p <= 1.0 && p >= 0.0);
          }
        }
      } else {
        auto preds = model->predict(data);
        TS_ASSERT_EQUALS(preds->size(), data.num_rows());
      }
    }
  }

 public:

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_boosted_tree_stress000_tn() {
    // All unique
    run_stress(1, "n", "n");
  }

  void test_boosted_tree_stress0n_tn() {
    // All unique
    run_stress(5, "n", "n");
  }

  void test_boosted_tree_stress0S_tn() {
    // All unique
    run_stress(5, "s", "n");
  }

  void test_boosted_tree_stress1_unsorted_tn() {
    run_stress(5, "b", "n");
  }

  void test_boosted_tree_stress0b_tn() {
    // All unique
    run_stress(13, "S", "n");
  }

  void test_boosted_tree_stress1b_unsorted_tn() {
    run_stress(13, "b", "n");
  }

  void test_boosted_tree_stress1_tn() {
    run_stress(13, "bs", "n");
  }

  void test_boosted_tree_stress2_tn() {
    run_stress(13, "zs", "n");
  }

  void test_boosted_tree_stress3_tn() {
    run_stress(100, "Zs", "n");
  }

  void test_boosted_tree_stress4_tn() {
    // Pretty mush gonna be unique
    run_stress(100, "Ss", "n");
  }

  void test_boosted_tree_stress5_tn() {
    // 10 blocks of values.
    run_stress(1000, "Zs", "n");
  }

  void test_boosted_tree_stress6_tn() {
    // two large blocks of values
    run_stress(1000, "bs", "n");
  }

  void test_boosted_tree_stress10_tn() {
    // Yeah, a corner sase
    run_stress(1, "bc", "n");
  }

  void test_boosted_tree_stress11_tn() {
    // One with just a lot of stuff
    run_stress(200, "u", "n");
  }

  void test_boosted_tree_stress12_tn() {
    // One with just a lot of stuff
    run_stress(200, "d", "n");
  }

  void test_boosted_tree_stress13_tn() {
    // One with just a lot of stuff
    run_stress(1000, "snv", "n");
  }

  void test_boosted_tree_stress14_tn() {
    // One with just a lot of stuff
    run_stress(1000, "du", "n");
  }

  void test_boosted_tree_stress15_tn() {
    // One with just a lot of stuff
    run_stress(3, "UDssssV", "n");
  }

  void test_boosted_tree_stress15b_tn() {
    // One with just a lot of stuff
    run_stress(35, "UDssssV", "n");
  }

  void test_boosted_tree_stress15c_tn() {
    // One with just a lot of stuff
    run_stress(500, "UDsssV", "n");
  }

  void test_boosted_tree_stress100_tn() {
    // One with just a lot of stuff
    run_stress(10, "Zsuvd", "n");
  }

  void test_boosted_tree_stress16_null_tn() {
    // two large blocks of values
    run_stress(1000, "S", "n");
  }


  void test_boosted_tree_stress000_tc() {
    // All unique
    run_stress(2, "n", "s");
  }

  void test_boosted_tree_stress0n_tc() {
    // All unique
    run_stress(5, "n", "s");
  }

  void test_boosted_tree_stress0S_tc() {
    // All unique
    run_stress(5, "s", "s");
  }

  void test_boosted_tree_stress1_unsorted_tc() {
    run_stress(5, "b", "s");
  }

  void test_boosted_tree_stress0b_tc() {
    // All unique
    run_stress(13, "S", "s");
  }

  void test_boosted_tree_stress1b_unsorted_tc() {
    run_stress(13, "b", "s");
  }

  void test_boosted_tree_stress1_tc() {
    run_stress(13, "bs", "s");
  }

  void test_boosted_tree_stress2_tc() {
    run_stress(13, "zs", "s");
  }

  void test_boosted_tree_stress3_tc() {
    run_stress(100, "Zs", "s");
  }

  void test_boosted_tree_stress4_tc() {
    // Pretty mush gonna be unique
    run_stress(100, "Ss", "s");
  }

  void test_boosted_tree_stress5_tc() {
    // 10 blocks of values.
    run_stress(1000, "Zs", "s");
  }

  void test_boosted_tree_stress6_tc() {
    // two large blocks of values
    run_stress(1000, "bs", "s");
  }

  void test_boosted_tree_stress10_tc() {
    // Yeah, a corner sase
    run_stress(2, "bc", "s");
  }

  void test_boosted_tree_stress11_tc() {
    // One with just a lot of stuff
    run_stress(200, "u", "s");
  }

  void test_boosted_tree_stress12_tc() {
    // One with just a lot of stuff
    run_stress(200, "d", "s");
  }

  void test_boosted_tree_stress13_tc() {
    // One with just a lot of stuff
    run_stress(1000, "snv", "s");
  }

  void test_boosted_tree_stress14_tc() {
    // One with just a lot of stuff
    run_stress(1000, "du", "s");
  }

  void test_boosted_tree_stress15_tc() {
    // One with just a lot of stuff
    run_stress(3, "UDssssV", "s");
  }

  void test_boosted_tree_stress15b_tc() {
    // One with just a lot of stuff
    run_stress(35, "UDssssV", "s");
  }

  void test_boosted_tree_stress15c_tc() {
    // One with just a lot of stuff
    run_stress(500, "UDsssV", "s");
  }

  void test_boosted_tree_stress100_tc() {
    // One with just a lot of stuff
    run_stress(10, "Zsuvd", "s");
  }

  void test_boosted_tree_stress16_null_tc() {
    // two large blocks of values
    run_stress(1000, "S", "s");
  }

  //////////////////////////////////////////////////////////////////////////////

  void test_boosted_tree_stress000_tC() {
    // All unique
    run_stress(2, "n", "S");
  }

  void test_boosted_tree_stress0n_tC() {
    // All unique
    run_stress(5, "n", "S");
  }

  void test_boosted_tree_stress0S_tC() {
    // All unique
    run_stress(5, "s", "S");
  }

  void test_boosted_tree_stress1_unsorted_tC() {
    run_stress(5, "b", "S");
  }

  void test_boosted_tree_stress0b_tC() {
    // All unique
    run_stress(13, "S", "S");
  }

  void test_boosted_tree_stress1b_unsorted_tC() {
    run_stress(13, "b", "S");
  }

  void test_boosted_tree_stress1_tC() {
    run_stress(13, "bs", "S");
  }

  void test_boosted_tree_stress2_tC() {
    run_stress(13, "zs", "S");
  }

  void test_boosted_tree_stress3_tC() {
    run_stress(100, "Zs", "S");
  }

  void test_boosted_tree_stress4_tC() {
    // Pretty mush gonna be unique
    run_stress(100, "Ss", "S");
  }

  void test_boosted_tree_stress5_tC() {
    // 10 blocks of values.
    run_stress(1000, "Zs", "S");
  }

  void test_boosted_tree_stress6_tC() {
    // two large blocks of values
    run_stress(1000, "bs", "S");
  }

  void test_boosted_tree_stress10_tC() {
    // Yeah, a corner sase
    run_stress(2, "bc", "S");
  }

  void test_boosted_tree_stress11_tC() {
    // One with just a lot of stuff
    run_stress(200, "u", "S");
  }

  void test_boosted_tree_stress12_tC() {
    // One with just a lot of stuff
    run_stress(200, "d", "S");
  }

  void test_boosted_tree_stress13_tC() {
    // One with just a lot of stuff
    run_stress(1000, "snv", "S");
  }

  void test_boosted_tree_stress14_tC() {
    // One with just a lot of stuff
    run_stress(1000, "du", "S");
  }

  void test_boosted_tree_stress15_tC() {
    // One with just a lot of stuff
    run_stress(3, "UDssssV", "S");
  }

  void test_boosted_tree_stress15b_tC() {
    // One with just a lot of stuff
    run_stress(35, "UDssssV", "S");
  }

  void test_boosted_tree_stress15c_tC() {
    // One with just a lot of stuff
    run_stress(500, "UDsssV", "S");
  }

  void test_boosted_tree_stress100_tC() {
    // One with just a lot of stuff
    run_stress(10, "Zsuvd", "S");
  }

  void test_boosted_tree_stress16_null_tC() {
    // two large blocks of values
    run_stress(1000, "S", "S");
  }

};

BOOST_FIXTURE_TEST_SUITE(_boosted_trees_classifier_test, boosted_trees_classifier_test)
BOOST_AUTO_TEST_CASE(test_boosted_trees_classifier_basic_2d) {
  boosted_trees_classifier_test::test_boosted_trees_classifier_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_boosted_trees_classifier_small) {
  boosted_trees_classifier_test::test_boosted_trees_classifier_small();
}
BOOST_AUTO_TEST_CASE(test_boosted_trees_classifier_with_insufficient_column_subsample) {
  boosted_trees_classifier_test::test_boosted_trees_classifier_with_insufficient_column_subsample();
}
BOOST_AUTO_TEST_CASE(test_boosted_trees_classifier_external_memory) {
  boosted_trees_classifier_test::test_boosted_trees_classifier_external_memory();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_boosted_trees_stress_test, boosted_trees_stress_test)
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress000_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress000_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress0n_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress0n_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress0S_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress0S_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress1_unsorted_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress1_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress0b_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress0b_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress1b_unsorted_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress1b_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress1_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress1_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress2_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress2_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress3_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress3_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress4_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress4_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress5_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress5_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress6_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress6_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress10_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress10_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress11_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress11_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress12_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress12_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress13_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress13_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress14_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress14_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress15_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress15_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress15b_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress15b_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress15c_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress15c_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress100_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress100_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress16_null_tn) {
  boosted_trees_stress_test::test_boosted_tree_stress16_null_tn();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress000_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress000_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress0n_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress0n_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress0S_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress0S_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress1_unsorted_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress1_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress0b_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress0b_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress1b_unsorted_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress1b_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress1_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress1_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress2_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress2_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress3_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress3_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress4_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress4_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress5_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress5_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress6_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress6_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress10_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress10_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress11_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress11_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress12_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress12_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress13_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress13_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress14_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress14_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress15_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress15_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress15b_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress15b_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress15c_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress15c_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress100_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress100_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress16_null_tc) {
  boosted_trees_stress_test::test_boosted_tree_stress16_null_tc();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress000_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress000_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress0n_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress0n_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress0S_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress0S_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress1_unsorted_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress1_unsorted_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress0b_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress0b_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress1b_unsorted_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress1b_unsorted_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress1_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress1_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress2_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress2_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress3_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress3_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress4_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress4_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress5_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress5_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress6_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress6_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress10_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress10_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress11_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress11_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress12_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress12_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress13_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress13_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress14_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress14_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress15_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress15_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress15b_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress15b_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress15c_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress15c_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress100_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress100_tC();
}
BOOST_AUTO_TEST_CASE(test_boosted_tree_stress16_null_tC) {
  boosted_trees_stress_test::test_boosted_tree_stress16_null_tC();
}
BOOST_AUTO_TEST_SUITE_END()

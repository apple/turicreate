#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <vector>
#include <set>
#include <string>
#include <random>

#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <toolkits/clustering/kmeans.hpp>

using namespace turi;


void run_kmeans_test(std::map<std::string, flexible_type> opts) {

  size_t num_examples = opts.at("num_examples");
  size_t num_clusters = opts.at("num_clusters");
  bool custom_centers = opts.at("custom_centers");
  size_t max_iterations = opts.at("max_iterations");
  std::string feature_column_types = opts.at("feature_column_types");
  bool has_target_column = false;

  sframe raw_data;
  raw_data = make_random_sframe(
      num_examples, feature_column_types, has_target_column);
  sframe init_centers;
  if (custom_centers) {
    init_centers  = make_random_sframe(
        num_clusters, feature_column_types, has_target_column);
  }

  // Define options
  std::map<std::string, flexible_type> options = { 
    {"num_clusters", num_clusters},
    {"max_iterations", max_iterations}
  };

  // Train the model
  std::shared_ptr<kmeans::kmeans_model> model;
  model.reset(new kmeans::kmeans_model);
  model->init_options(options);
  model->train(raw_data, init_centers, "elkan");

  // Get predictions with the model.
  sframe predictions = model->predict(raw_data);


  // Test save and load
  // Record thing to test
  std::map<std::string, flexible_type> _options;
  _options = model->get_current_options();

  // Save it
  dir_archive archive_write;
  archive_write.open_directory_for_write("kmeans_cxx_test");
  turi::oarchive oarc(archive_write);
  oarc << *model;
  archive_write.close(); 

  // Load it
  dir_archive archive_read;
  archive_read.open_directory_for_read("kmeans_cxx_test");
  turi::iarchive iarc(archive_read);
  iarc >> *model;

  // Check that stuff in the loaded model is correct
  TS_ASSERT(model->is_trained());
  
  for (auto& kvp: options){
    TS_ASSERT(_options[kvp.first] == kvp.second);
  }
}


/**
 *  Check kmeans model
*/
struct kmeans_test  {
 
  public:

  void test_kmeans_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"num_examples", 3},
      {"num_clusters", 2},
      {"max_iterations", 10},
      {"custom_centers", false},
      {"feature_column_types", "nn"}}; 
    run_kmeans_test(opts);
  }

  void test_kmeans_custom_centers() {
      std::map<std::string, flexible_type> opts = {
      {"num_examples", 10},
      {"num_clusters", 2},
      {"max_iterations", 0},
      {"custom_centers", true},
      {"feature_column_types", "nn"}}; 
    run_kmeans_test(opts);
  }

  void test_kmeans_no_iters() {
      std::map<std::string, flexible_type> opts = {
      {"num_examples", 10},
      {"num_clusters", 2},
      {"max_iterations", 0},
      {"custom_centers", false},
      {"feature_column_types", "nn"}}; 
    run_kmeans_test(opts);
  }
  
  void test_kmeans_dict_input() {
    std::map<std::string, flexible_type> opts = {
      {"num_examples", 20},
      {"num_clusters", 3},
      {"max_iterations", 10},
      {"custom_centers", false},
      {"feature_column_types", "d"}}; 
    run_kmeans_test(opts);    
  }

  void test_kmeans_vector_input() {
    std::map<std::string, flexible_type> opts = {
      {"num_examples", 20},
      {"num_clusters", 3},
      {"max_iterations", 10},
      {"custom_centers", false},
      {"feature_column_types", "v"}}; 
    run_kmeans_test(opts);    
  }
};


BOOST_FIXTURE_TEST_SUITE(_kmeans_test, kmeans_test)
BOOST_AUTO_TEST_CASE(test_kmeans_basic_2d) {
  kmeans_test::test_kmeans_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_kmeans_custom_centers) {
  kmeans_test::test_kmeans_custom_centers();
}
BOOST_AUTO_TEST_CASE(test_kmeans_no_iters) {
  kmeans_test::test_kmeans_no_iters();
}
BOOST_AUTO_TEST_CASE(test_kmeans_dict_input) {
  kmeans_test::test_kmeans_dict_input();
}
BOOST_AUTO_TEST_CASE(test_kmeans_vector_input) {
  kmeans_test::test_kmeans_vector_input();
}
BOOST_AUTO_TEST_SUITE_END()

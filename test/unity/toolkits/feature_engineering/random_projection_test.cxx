
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <unistd.h>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <toolkits/feature_engineering/dimension_reduction.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>

using namespace turi;
using namespace turi::sdk_model::feature_engineering;


/**
 * Generate synthetic data for random projection testing.
 */
gl_sframe make_random_projection_data() {
  size_t num_examples = 10;
  std::string column_type_code = "nnnn";  // four numeric columns
  sframe raw_data = make_random_sframe(num_examples, column_type_code, false);
  return gl_sframe(raw_data);
}


/**
 * Generic check for correctness of the members of a random projection instance.
 */
void check_model_attributes(std::shared_ptr<random_projection> model,
                           std::map<std::string, flexible_type> correct_state) {

  // Define answers that never change.
  std::map<std::string, flexible_type> correct_default_options {
    {"output_column_name", "embedded_features"},
    {"random_seed", FLEX_UNDEFINED},
    {"embedding_dimension", 2}};


  // Check the default options of the model.
  std::map<std::string, flexible_type> model_default_options
    = model->get_default_options();

  TS_ASSERT_EQUALS(correct_default_options.size(),
                   model_default_options.size());

  for (auto& kvp : correct_default_options)
    TS_ASSERT_EQUALS(model_default_options[kvp.first], kvp.second);

  // Check that the current options of the default model are actually the
  // defaults. Use the `correct_default_options` to iterate, but check the
  // values against the `correct_state_parameter`.
  std::map<std::string, flexible_type> model_options
    = model->get_current_options();

  TS_ASSERT_EQUALS(correct_default_options.size(), model_options.size());

  for (auto& kvp : correct_default_options)
    TS_ASSERT_EQUALS(model_options[kvp.first], correct_state[kvp.first]);

  // Check model's retrievable fields, both names (i.e. list fields) and
  // actual values in model state.
  std::vector<std::string> model_state_names = model->list_fields();

  TS_ASSERT_EQUALS(correct_state.size(), model_state_names.size());

  for (auto& kvp : correct_state) {

    // make the sure the field name is in `list_fields`.
    TS_ASSERT(
      std::find(model_state_names.begin(), model_state_names.end(), kvp.first)
      != model_state_names.end());

    // make sure the value is correct.
    flexible_type model_state_value = variant_get_value<flexible_type>(
                                      model->get_value_from_state(kvp.first));
    TS_ASSERT_EQUALS(model_state_value, correct_state[kvp.first]);
  }
}


/**
 * Check that the values in two gl_sframe objects are equal.
 */
void check_gl_sframe_equality(gl_sframe& sf_a, gl_sframe& sf_b) {
  TS_ASSERT_EQUALS(sf_a.size(), sf_b.size());
  TS_ASSERT_EQUALS(sf_a.num_columns(), sf_b.num_columns());

  for (size_t i = 0; i < sf_a.size(); ++i) {
    std::vector<flexible_type> row_a = sf_a[i];
    std::vector<flexible_type> row_b = sf_b[i];

    for (size_t j = 0; j < sf_a.num_columns(); ++j) {
      TS_ASSERT_EQUALS(row_a[j], row_b[j]);
    }
  }
}


/**
 * Main test driver
 * -----------------------------------------------------------------------------
 */
struct random_projection_test  {

 public:

  /**
   * Test that models are initialized properly with the default settings.
   */
  void test_default_model_initialization() {
    logprogress_stream << "Testing model initialization" << std::endl;

    std::map<std::string, flexible_type> user_opts = {
      {"features", FLEX_UNDEFINED},
      {"exclude", false}};

    std::map<std::string, flexible_type> correct_state {
      {"features", FLEX_UNDEFINED},
      {"excluded_features", FLEX_UNDEFINED},
      {"original_dimension", FLEX_UNDEFINED},
      {"is_fitted", false},
      {"output_column_name", "embedded_features"},
      {"random_seed", FLEX_UNDEFINED},
      {"embedding_dimension", 2}};

    // Construct the model with default parameters.
    std::shared_ptr<random_projection> projector;
    projector.reset(new random_projection);
    projector->init_transformer(user_opts);

    // Check basic model attributes.
    check_model_attributes(projector, correct_state);
  }


  /**
   * Test that models are initialized properly with user-defined options.
   */
  void test_custom_model_initialization() {
    logprogress_stream << "Testing user-defined arguments to RP." << std::endl;

    std::vector<flexible_type> features = {flexible_type("x.0"),
                                           flexible_type("x.1"),
                                           flexible_type("x.2"),
                                           flexible_type("x.3")};

    std::map<std::string, flexible_type> user_opts = {
      {"features", features},
      {"exclude", false},
      {"embedding_dimension", 3},
      {"output_column_name", "data_out"},
      {"random_seed", 192}};

    std::map<std::string, flexible_type> correct_state {
      {"features", features},
      {"excluded_features", FLEX_UNDEFINED},
      {"original_dimension", FLEX_UNDEFINED},
      {"is_fitted", false},
      {"output_column_name", "data_out"},
      {"random_seed", 192},
      {"embedding_dimension", 3}};

    // Construct the model with default parameters.
    std::shared_ptr<random_projection> projector;
    projector.reset(new random_projection);
    projector->init_transformer(user_opts);

    // Check basic model attributes.
    check_model_attributes(projector, correct_state);
  }


  /**
   * Verify that fitting a random projection model changes the model's members
   * correctly.
   */
  void test_model_fit() {
    logprogress_stream << "Testing that fitting updates model attributes "
                       << "correctly." << std::endl;

    // Create the model and fit to synthetic data.
    std::map<std::string, flexible_type> user_opts = {
      {"features", FLEX_UNDEFINED},
      {"exclude", false},
      {"embedding_dimension", 3},
      {"output_column_name", "data_out"},
      {"random_seed", 193}};

    std::shared_ptr<random_projection> projector;
    projector.reset(new random_projection);
    projector->init_transformer(user_opts);

    gl_sframe data = make_random_projection_data();
    projector->fit(data);

    // Check model attributes
    std::vector<flexible_type> feature_names;

    for (size_t j = 0; j < data.num_columns(); ++j) {
      feature_names.push_back(flexible_type(data.column_names()[j]));
    }

    std::map<std::string, flexible_type> correct_state {
      {"features", feature_names},
      {"excluded_features", FLEX_UNDEFINED},
      {"original_dimension", 4},
      {"is_fitted", true},
      {"output_column_name", "data_out"},
      {"random_seed", 193},
      {"embedding_dimension", 3}};

    check_model_attributes(projector, correct_state);

    // Check that the random seed is different each time we call fit, if not
    // specified by the user. Because the random seed is set to be the epoch
    // seconds (if not specified), we need to sleep for about 1.5 seconds in
    // between calls to `fit`.
    user_opts = { {"features", FLEX_UNDEFINED},
                  {"exclude", false},
                  {"embedding_dimension", 3},
                  {"output_column_name", "data_out"},
                  {"random_seed", FLEX_UNDEFINED} };

    projector.reset(new random_projection);
    projector->init_transformer(user_opts);
    projector->fit(data);
    size_t seed1 = variant_get_value<size_t>(
                                projector->get_value_from_state("random_seed"));

    usleep(1.5e6); // sleep for 1.5 seconds

    projector.reset(new random_projection);
    projector->init_transformer(user_opts);
    projector->fit(data);
    size_t seed2 = variant_get_value<size_t>(
                                projector->get_value_from_state("random_seed"));

    TS_ASSERT(seed1 != seed2);
  }


  /**
   * Check that the random projection model behaves correctly when doing
   * transformations.
   */
  void test_transform_logistics() {
    logprogress_stream << "Testing that RP transform works correctly."
                       << std::endl;

    // Create the model and fit to data.
    std::map<std::string, flexible_type> user_opts = {
      {"features", FLEX_UNDEFINED},
      {"exclude", false},
      {"embedding_dimension", 3},
      {"output_column_name", "data_out"},
      {"random_seed", 194}};

    std::shared_ptr<random_projection> projector;
    projector.reset(new random_projection);
    projector->init_transformer(user_opts);

    gl_sframe data = make_random_projection_data();
    projector->fit(data);

    // Check that `transform` doesn't change the model attributes at all.
    std::vector<flexible_type> feature_names;

    for (size_t j = 0; j < data.num_columns(); ++j) {
      feature_names.push_back(flexible_type(data.column_names()[j]));
    }

    std::map<std::string, flexible_type> correct_state {
      {"features", feature_names},
      {"excluded_features", FLEX_UNDEFINED},
      {"original_dimension", 4},
      {"is_fitted", true},
      {"output_column_name", "data_out"},
      {"random_seed", 194},
      {"embedding_dimension", 3}};

    gl_sframe sf_embed = projector->transform(data);
    gl_sframe sf_embed_unpacked = sf_embed.unpack("data_out");
    check_model_attributes(projector, correct_state);

    // Check that the dimensions of the output are correct.
    TS_ASSERT_EQUALS(sf_embed.size(), data.size());
    TS_ASSERT_EQUALS(sf_embed.num_columns(), 1);
    TS_ASSERT_EQUALS(sf_embed_unpacked.num_columns(),
                     static_cast<size_t>(user_opts.at("embedding_dimension")));

    // Check that transforming repeatedly with the same projection matrix yields
    // the same result.
    gl_sframe sf_embed2 = projector->transform(data);
    check_gl_sframe_equality(sf_embed, sf_embed2);

    // Make sure two models with same random seed yield the same transformation
    // results.
    projector.reset(new random_projection);
    projector->init_transformer(user_opts);
    projector->fit(data);
    gl_sframe sf_embed3 = projector->transform(data);
    check_gl_sframe_equality(sf_embed, sf_embed3);

    // Make sure `fit_transform` does the same thing as `fit` and `transform`.
    gl_sframe sf_embed4 = projector->fit_transform(data);
    check_gl_sframe_equality(sf_embed, sf_embed4);


    // Make sure two models with a different random seed yield different
    // transformation results.
    user_opts["random_seed"] = 195;

    projector.reset(new random_projection);
    projector->init_transformer(user_opts);
    projector->fit(data);
    gl_sframe sf_embed5 = projector->transform(data);
    sf_embed5 = sf_embed5.unpack("data_out");

    TS_ASSERT_DIFFERS(sf_embed_unpacked.select_column("X.0")[0],
                      sf_embed5.select_column("X.0")[0]);
  }


  /**
   * Make sure the results of the random projection are correct.
   */
  void test_transform_correctness() {
    logprogress_stream << "Testing that RP transform is correct." << std::endl;

    // Test that the same data points project to the same place. Create a
    // synthetic SFrame and append it to itself so there are two copies of the
    // data in one SFrame. The top and bottom halves of the result should be
    // equal as well.
    gl_sframe sf{ {"a", {1, 2, 3}},
                  {"b", {7, 8, 4}},
                  {"c", {6, 9, 7}} };

    sf = sf.append(sf);

    std::map<std::string, flexible_type> user_opts = {
      {"features", FLEX_UNDEFINED},
      {"exclude", false},
      {"embedding_dimension", 2},
      {"output_column_name", "data_out"},
      {"random_seed", FLEX_UNDEFINED}};

    std::shared_ptr<random_projection> projector;
    projector.reset(new random_projection);
    projector->init_transformer(user_opts);
    gl_sframe sf_embed = projector->fit_transform(sf);

    gl_sframe sf_embed_a = sf_embed[{0, 3}];
    gl_sframe sf_embed_b = sf_embed[{3, 6}];
    check_gl_sframe_equality(sf_embed_a, sf_embed_b);
  }


  /**
   * Make sure save and loading a model doesn't corrupt the model's attributes.
   */
  void test_save_and_load() {

    // Create the model and fit to synthetic data.
    std::map<std::string, flexible_type> user_opts = {
      {"features", FLEX_UNDEFINED},
      {"exclude", false},
      {"embedding_dimension", 3},
      {"output_column_name", "data_out"},
      {"random_seed", 195}};

    std::shared_ptr<random_projection> projector;
    projector.reset(new random_projection);
    projector->init_transformer(user_opts);

    gl_sframe data = make_random_projection_data();
    projector->fit(data);
    gl_sframe sf_embed_orig = projector->transform(data);

    // Define the correct state.
    std::vector<flexible_type> feature_names;

    for (size_t j = 0; j < data.num_columns(); ++j) {
      feature_names.push_back(flexible_type(data.column_names()[j]));
    }

    std::map<std::string, flexible_type> correct_state {
      {"features", feature_names},
      {"excluded_features", FLEX_UNDEFINED},
      {"original_dimension", 4},
      {"is_fitted", true},
      {"output_column_name", "data_out"},
      {"random_seed", 195},
      {"embedding_dimension", 3}};

    // Check that the original model has the correct state.
    check_model_attributes(projector, correct_state);

    // Save the model
    dir_archive archive_write;
    archive_write.open_directory_for_write("random_projection_cxx_test");
    turi::oarchive oarc(archive_write);
    oarc << *projector;
    archive_write.close();


    // Load the model back.
    dir_archive archive_read;
    archive_read.open_directory_for_read("random_projection_cxx_test");
    turi::iarchive iarc(archive_read);
    iarc >> *projector;

    // Check that the loaded model has the correct state.
    check_model_attributes(projector, correct_state);

    // Check that the transformation is the same after loading.
    gl_sframe sf_embed_loaded = projector->transform(data);
    check_gl_sframe_equality(sf_embed_orig, sf_embed_loaded);
  }
};

BOOST_FIXTURE_TEST_SUITE(_random_projection_test, random_projection_test)
BOOST_AUTO_TEST_CASE(test_default_model_initialization) {
  random_projection_test::test_default_model_initialization();
}
BOOST_AUTO_TEST_CASE(test_custom_model_initialization) {
  random_projection_test::test_custom_model_initialization();
}
BOOST_AUTO_TEST_CASE(test_model_fit) {
  random_projection_test::test_model_fit();
}
BOOST_AUTO_TEST_CASE(test_transform_logistics) {
  random_projection_test::test_transform_logistics();
}
BOOST_AUTO_TEST_CASE(test_transform_correctness) {
  random_projection_test::test_transform_correctness();
}
BOOST_AUTO_TEST_CASE(test_save_and_load) {
  random_projection_test::test_save_and_load();
}
BOOST_AUTO_TEST_SUITE_END()

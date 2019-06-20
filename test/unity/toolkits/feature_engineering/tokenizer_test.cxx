#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <stdlib.h>
#include <vector>
#include <string>
#include <functional>
#include <random>
#include <sstream>
#include <iterator>
#include <iostream>
#include <algorithm>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/feature_engineering/tokenizer.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>

#include <cfenv>

using namespace turi;
using namespace turi::sdk_model::feature_engineering;

/**
 * Construct a model from data and options.
 */
std::shared_ptr<tokenizer> init_model(const gl_sframe& data, 
  std::map<std::string, flexible_type>& opts) {

  std::shared_ptr<tokenizer> model;
  model.reset(new tokenizer);
  std::map<std::string, flexible_type> options;
  for (const auto& kvp: opts){
    if (kvp.first != "examples" && kvp.first != "features") {
      options[kvp.first] = kvp.second;
    }
  }
  options["features"] = FLEX_UNDEFINED;
  model->init_transformer(options);
  model->fit(data);
  return model;
}

/**
 * Save the model and load it back.
 */
std::shared_ptr<tokenizer> save_and_load_model(
                                std::shared_ptr<tokenizer> model) {

  std::shared_ptr<tokenizer> loaded_model;
  loaded_model.reset(new tokenizer);

  dir_archive archive_write;
  archive_write.open_directory_for_write("tokenizer_tests");
  turi::oarchive oarc(archive_write);
  oarc << *model;
  archive_write.close();

  dir_archive archive_read;
  archive_read.open_directory_for_read("tokenizer_tests");
  turi::iarchive iarc(archive_read);
  iarc >> *loaded_model;
  return loaded_model;
}

void _assert_vector_equals(std::vector<flexible_type> result, 
                           std::vector<flexible_type> expected){
  TS_ASSERT_EQUALS(result.size(), expected.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    TS_ASSERT_EQUALS(result[i], expected[i]);
  }
}
  
void check_model(std::shared_ptr<tokenizer> model,
                 const gl_sframe& data, 
                 const gl_sarray& expected,
                 std::map<std::string, flexible_type>& opts) { 
  std::map<std::string, flexible_type> _options;
  std::vector<std::string> _list_fields;
  flexible_type _get;

  // Answers.
  std::map<std::string, flexible_type> default_options = { 
    {"to_lower", false},
    {"output_column_prefix", flex_undefined()},
    {"delimiters", flex_list({"\r", "\v", "\n", "\f", "\t", " "})}
  };
  std::vector<std::string> _list_fields_ans = {
        "features",
        "excluded_features",
        "to_lower",
        "output_column_prefix",
        "delimiters"
  };
  std::map<std::string, flexible_type> options;
  for (const auto& kvp: default_options){
    if (opts.count(kvp.first) == 0) { 
      options[kvp.first] = kvp.second;
    } else {
      options[kvp.first] = opts[kvp.first];
    }
  }

  // Check the model
  // ----------------------------------------------------------------------
  TS_ASSERT(data.column_names() == variant_get_value<std::vector<std::string>>(
                            model->get_value_from_state("features")));

  // Check options
  _options = model->get_current_options();
  for (auto& kvp: options){
    TS_ASSERT_EQUALS(_options[kvp.first],  kvp.second);
  }
  TS_ASSERT_EQUALS(_options.size(), options.size());

  // Default options
  _options = model->get_default_options();
  for (auto& kvp: default_options){
    TS_ASSERT_EQUALS(_options[kvp.first],  kvp.second);
  }
  TS_ASSERT_EQUALS(_options.size(), default_options.size());

  // Check list_fields
  _list_fields = model->list_fields();
  for(const auto& f: _list_fields_ans){
    TS_ASSERT(std::find(_list_fields.begin(), _list_fields.end(), f) 
                                                    != _list_fields.end());
  }
  TS_ASSERT_EQUALS(_list_fields.size(), _list_fields_ans.size());

  // Check that transformations work.
  // ----------------------------------------------------------------
  gl_sarray result = model->transform(data)["docs"];

  TS_ASSERT_EQUALS(result.size(), expected.size());

  for (size_t i = 0; i < result.size(); ++i){
    _assert_vector_equals(result[i], expected[i]);
  }
}

void run_tokenizer_test(std::map<std::string, flexible_type>& opts) {
  gl_sframe sf{{"docs", {
    std::string("\"Oh, no,\" she's saying, \"our $400 blender can't handle") + 
      std::string(" something this hard & grainy!\""),
    "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "Welcome to RegExr v2.0 by gskinner.com!",
    "0123456789 +-.,!@#$%^&*();\\/|<>\"'",
    "12345 -98.7 3.141 .6180 9,000 +42",
    "555.123.4567    +1-(800)-555-2468",
    "foo@demo.net    bar.ba@test.co.uk",
    "www.demo.com    http://foo.co.uk/",
    "http://regexr.com/foo.html?q=bar",
    "She's leaving home. I've got nothing to say.",
    "I'm just trying to test contraction tokenization."
  }}};
  const gl_sarray expected({
    flex_list{"\"", "Oh", ",", "no", ",", "\"", "she", 
     "'s", "saying", ",", "\"", "our", "$", 
     "400", "blender", "ca", "n't", "handle",
     "something", "this", "hard", "&", "grainy", "!", "\""},
    flex_list{"abcdefghijklmnopqrstuvwxyz", 
     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"},
    flex_list{"Welcome", "to", "RegExr", "v2.0", "by", 
     "gskinner.com", "!"},
    flex_list{"0123456789", "+", "-", ".", ",", "!", "@", "#", "$", 
     "%", "^", "&", "*", "(", ")", ";", "\\", "/", "|", "<", 
     ">", "\"", "'"},
    flex_list{"12345", "-98.7", "3.141", ".6180", "9,000", 
     "+42"},
    flex_list{"555.123.4567", "+1-(800)-555-2468"},
    flex_list{"foo@demo.net", "bar.ba@test.co.uk"},
    flex_list{"www.demo.com", "http://foo.co.uk", "/"},
    flex_list{"http://regexr.com/foo.html?q=bar"},
    flex_list{"She", "'s", "leaving", "home", ".", "I", 
     "'ve", "got", "nothing", "to", "say", "."},
    flex_list{"I", "'m", "just", "trying", "to", 
     "test", "contraction", "tokenization", "."}
  });

  std::shared_ptr<tokenizer> model = init_model(sf, opts);
  std::shared_ptr<tokenizer> loaded_model = save_and_load_model(model);
  check_model(model, sf, expected, opts);
  check_model(loaded_model, sf, expected, opts);
}

/**
 *  Run tests.
*/
struct tokenizer_test  {

  public:

  void test_tokenizer() {
    std::map<std::string, flexible_type> opts = {
      {"to_lower", false},
      {"delimiters", flex_undefined()},
      {"exclude", false}};
    run_tokenizer_test(opts);
  }
};

BOOST_FIXTURE_TEST_SUITE(_tokenizer_test, tokenizer_test)
BOOST_AUTO_TEST_CASE(test_tokenizer) {
  tokenizer_test::test_tokenizer();
}
BOOST_AUTO_TEST_SUITE_END()

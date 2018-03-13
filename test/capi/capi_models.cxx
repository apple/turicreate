#define BOOST_TEST_MODULE capi_models
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <capi/TuriCore.h>
#include <algorithm>
#include <image/image_type.hpp>
#include <vector>
#include "capi_utils.hpp"

BOOST_AUTO_TEST_CASE(test_boosted_trees_double) {
  for (const char* model_name :
       {"boosted_trees_regression", "decision_tree_regression",
        "regression_linear_regression", "auto"}) {

    bool is_auto = std::string(model_name) == "auto"; 

    tc_error* error = NULL;

    tc_initialize("/tmp/", &error);
    TS_ASSERT(error == NULL);

    std::vector<std::pair<std::string, std::vector<double> > > data = {
        {"col1", {1., 1., 2., 2.}},
        {"col2", {1., 2., 1., 2.}},
        {"target", {0., 1., 2., 3.}}};

    tc_sframe* sf_1 = make_sframe_double(data);

    tc_sframe* sf_2 = tc_sframe_append(sf_1, sf_1, &error);
    TS_ASSERT(error == NULL);
    tc_sframe_destroy(sf_1);

    tc_sframe* sf_3 = tc_sframe_append(sf_2, sf_2, &error);
    TS_ASSERT(error == NULL);
    tc_sframe_destroy(sf_2);

    tc_sframe* sf = tc_sframe_append(sf_3, sf_3, &error);
    TS_ASSERT(error == NULL);
    tc_sframe_destroy(sf_3);

    tc_parameters* args = tc_parameters_create_empty(&error);

    TS_ASSERT(error == NULL);

    // Add in the sframe; then destroy it when we're done with it.
    {
      tc_parameters_add_sframe(args, "data", sf, &error);
      TS_ASSERT(error == NULL);
    }

    // Set the target column
    {
      tc_flexible_type* ft_name = tc_ft_create_from_cstring("target", &error);
      TS_ASSERT(error == NULL);
      tc_parameters_add_flexible_type(args, "target", ft_name, &error);
      TS_ASSERT(error == NULL);
      tc_ft_destroy(ft_name);
    }

    // Set the validation data to something empty
    {
      tc_sframe* ft_empty_sframe = tc_sframe_create_empty(&error);
      TS_ASSERT(error == NULL);

      tc_parameters_add_sframe(args, "validation_data", ft_empty_sframe, &error);
      TS_ASSERT(error == NULL);

      tc_sframe_destroy(ft_empty_sframe);
    }

    // Set the options
    {
      tc_flex_dict* fd = tc_flex_dict_create(&error); 
      TS_ASSERT(error == NULL);

      tc_parameters_add_flex_dict(args, "options", fd, &error);
      TS_ASSERT(error == NULL);


      tc_flex_dict_destroy(fd);
    }

    tc_model* model; 

    if(is_auto) {
      tc_variant* var_m =
          tc_function_call("_supervised_learning.create_automatic_regression_model", args, &error);

      TS_ASSERT(error == NULL);

      model = tc_variant_model(var_m, &error);
      TS_ASSERT(error == NULL);

      tc_variant_destroy(var_m);
    } else {
      // We now have enough to create the model.
      model = tc_model_new(model_name, &error);
      TS_ASSERT(error == NULL);

      tc_model_call_method(model, "train", args, &error);
      TS_ASSERT(error == NULL);
    }

    tc_parameters_destroy(args);

    TS_ASSERT(model != NULL);

    std::string ret_name = tc_model_name(model, &error);

    if(!is_auto) {
      TS_ASSERT(ret_name == model_name);
    }

    // Test predictions on the same data.  Should be almost completely
    // accurate...
    {
      tc_sframe* sf_2 = tc_sframe_create_copy(sf, &error);
      TS_ASSERT(error == NULL);

      tc_sframe_remove_column(sf_2, "target", &error);
      TS_ASSERT(error == NULL);

      tc_parameters* p_args = tc_parameters_create_empty(&error);

      tc_parameters_add_sframe(p_args, "data", sf_2, &error);
      TS_ASSERT(error == NULL);
      tc_sframe_destroy(sf_2);

      tc_variant* ret = tc_model_call_method(model, "predict", p_args, &error);
      TS_ASSERT(error == NULL);
      tc_parameters_destroy(p_args);

#ifdef TC_VARIANT_FUNCTIONS_DEFINED
      int is_sarray = tc_variant_is_sarray(ret);
      TS_ASSERT(is_sarray);

      tc_sarray* sa = tc_variant_sarray(ret, &error);
      TS_ASSERT(error == NULL);

      const auto& target_values = data.back().second;

      for (size_t i = 0; i < target_values.size(); ++i) {
        tc_flexible_type* ft = tc_sarray_extract_element(sa, i, &error);
        TS_ASSERT(error == NULL);

        double v = tc_ft_double(ft, &error);
        TS_ASSERT(error == NULL);
        tc_ft_destroy(ft);

        // Make sure they are close -- on a tiny dataset like this the default
        // setting
        TS_ASSERT_DELTA(v, target_values[i], 0.5);
      }
#endif

      {
        tc_parameters* export_args = tc_parameters_create_empty(&error);
        TS_ASSERT(error == NULL);

        // Set the l2 regression
        {
          tc_flexible_type* ft_name = tc_ft_create_from_cstring(
              "coreml_export_test_1_tmp.mlmodel", &error);
          TS_ASSERT(error == NULL);
          tc_parameters_add_flexible_type(export_args, "filename", ft_name,
                                          &error);
          TS_ASSERT(error == NULL);
          tc_ft_destroy(ft_name);
        }

        tc_variant* ret_2 = tc_model_call_method(model, "export_to_coreml",
                                                 export_args, &error);
        TS_ASSERT(error == NULL);
        tc_parameters_destroy(export_args);
      }
    }

    // Test saving and loading the model.
    {
      constexpr char MODEL_PATH[] = "save_test_1_tmp";
      tc_model_save(model, MODEL_PATH, &error);
      TS_ASSERT(error == nullptr);

      tc_model* loaded_model = tc_model_load(MODEL_PATH, &error);
      TS_ASSERT(error == nullptr);
      TS_ASSERT(loaded_model != nullptr);

      std::string ret_name = tc_model_name(loaded_model, &error);
      TS_ASSERT(error == nullptr);
      if(!is_auto) {
        TS_ASSERT(ret_name == model_name);
      }

      tc_model_destroy(loaded_model);
    }

    tc_model_destroy(model);
  }
}

BOOST_AUTO_TEST_CASE(test_auto_classification) {
    tc_error* error = NULL;

    tc_initialize("/tmp/", &error);
    TS_ASSERT(error == NULL);

    std::vector<std::pair<std::string, std::vector<double> > > features = {
        {"col1", {1., 1., 10., 10.}},
        {"col2", {2., 2., 20., 20.}}};
    tc_sframe* data = make_sframe_double(features);

    tc_flex_list* target_value = make_flex_list_string({"A", "A", "B", "B"});
    tc_sarray* target_sarray = tc_sarray_create_from_list(target_value, &error);
    TS_ASSERT(error == NULL);

    tc_sframe_add_column(data, "target", target_sarray, &error);
    TS_ASSERT(error == NULL);

    tc_parameters* args = tc_parameters_create_empty(&error);
    TS_ASSERT(error == NULL);

    { // Populate args
      tc_parameters_add_sframe(args, "data", data, &error);
      TS_ASSERT(error == NULL);

      // Set the target column
      tc_flexible_type* ft_name = tc_ft_create_from_cstring("target", &error);
      TS_ASSERT(error == NULL);
      tc_parameters_add_flexible_type(args, "target", ft_name, &error);
      TS_ASSERT(error == NULL);
      tc_ft_destroy(ft_name);

      // Set the validation data to something empty
      tc_sframe* ft_empty_sframe = tc_sframe_create_empty(&error);
      TS_ASSERT(error == NULL);

      tc_parameters_add_sframe(args, "validation_data", ft_empty_sframe, &error);
      TS_ASSERT(error == NULL);

      tc_sframe_destroy(ft_empty_sframe);

      // Set the options
      tc_flex_dict* fd = tc_flex_dict_create(&error);
      TS_ASSERT(error == NULL);

      tc_parameters_add_flex_dict(args, "options", fd, &error);
      TS_ASSERT(error == NULL);

      tc_flex_dict_destroy(fd);
    }

    { // Model selection without validation data
      tc_variant* var_m =
        tc_function_call("_supervised_learning.create_automatic_classifier_model", args, &error);
      TS_ASSERT(error == NULL);

      tc_model* model = tc_variant_model(var_m, &error);
      TS_ASSERT(error == NULL);
      TS_ASSERT(model != NULL);

      tc_model_destroy(model);
      tc_variant_destroy(var_m);
    }

    { // Model selection with validation data
      tc_parameters_add_sframe(args, "validation_data", data, &error);
      TS_ASSERT(error == NULL);

      tc_variant* var_m =
        tc_function_call("_supervised_learning.create_automatic_classifier_model", args, &error);
      TS_ASSERT(error == NULL);

      tc_model* model = tc_variant_model(var_m, &error);
      TS_ASSERT(error == NULL);
      TS_ASSERT(model != NULL);

      tc_model_destroy(model);
      tc_variant_destroy(var_m);
    }

    tc_sframe_destroy(data);
    tc_parameters_destroy(args);
}

/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE capi_models
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <capi/TuriCreate.h>
#include <algorithm>
#include <image/image_type.hpp>
#include <vector>
#include <fileio/fileio_constants.hpp>
#include <util/fs_util.hpp>

#include "capi_utils.hpp"

BOOST_AUTO_TEST_CASE(test_boosted_trees_double) {
  for (const char* model_name :
       {"boosted_trees_regression", "decision_tree_regression",
        "regression_linear_regression", "auto"}) {

    bool is_auto = std::string(model_name) == "auto"; 

    tc_error* error = NULL;

    CAPI_CHECK_ERROR(error);

    std::vector<std::pair<std::string, std::vector<double> > > data = {
        {"col1", {1., 1., 2., 2.}},
        {"col2", {1., 2., 1., 2.}},
        {"target", {0., 1., 2., 3.}}};

    tc_sframe* sf_1 = make_sframe_double(data);

    tc_sframe* sf_2 = tc_sframe_append(sf_1, sf_1, &error);

    CAPI_CHECK_ERROR(error);
    tc_release(sf_1);

    tc_sframe* sf_3 = tc_sframe_append(sf_2, sf_2, &error);
    CAPI_CHECK_ERROR(error);
    tc_release(sf_2);

    tc_sframe* sf = tc_sframe_append(sf_3, sf_3, &error);
    CAPI_CHECK_ERROR(error);
    tc_release(sf_3);

    tc_parameters* args = tc_parameters_create_empty(&error);

    CAPI_CHECK_ERROR(error);

    // Add in the sframe; then destroy it when we're done with it.
    {
      tc_parameters_add_sframe(args, "data", sf, &error);
      CAPI_CHECK_ERROR(error);
    }

    // Set the target column
    {
      tc_flexible_type* ft_name = tc_ft_create_from_cstring("target", &error);
      CAPI_CHECK_ERROR(error);
      tc_parameters_add_flexible_type(args, "target", ft_name, &error);
      CAPI_CHECK_ERROR(error);
      tc_release(ft_name);
    }

    // Set the validation data to something empty
    {
      tc_sframe* ft_empty_sframe = tc_sframe_create_empty(&error);
      CAPI_CHECK_ERROR(error);

      tc_parameters_add_sframe(args, "validation_data", ft_empty_sframe, &error);
      CAPI_CHECK_ERROR(error);

      tc_release(ft_empty_sframe);
    }

    // Set the options
    {
      tc_flex_dict* fd = tc_flex_dict_create(&error); 
      CAPI_CHECK_ERROR(error);

      tc_parameters_add_flex_dict(args, "options", fd, &error);
      CAPI_CHECK_ERROR(error);


      tc_release(fd);
    }

    tc_model* model; 

    if(is_auto) {
      tc_variant* var_m =
          tc_function_call("_supervised_learning.create_automatic_regression_model", args, &error);

      CAPI_CHECK_ERROR(error);

      model = tc_variant_model(var_m, &error);
      CAPI_CHECK_ERROR(error);

      tc_release(var_m);
    } else {
      // We now have enough to create the model.
      model = tc_model_new(model_name, &error);
      CAPI_CHECK_ERROR(error);

      tc_variant* ret = tc_model_call_method(model, "train", args, &error);
      CAPI_CHECK_ERROR(error);
      TS_ASSERT(ret != NULL);

      tc_release(ret);
    }

    tc_release(args);

    TS_ASSERT(model != NULL);

    std::string ret_name = tc_model_name(model, &error);

    if(!is_auto) {
      TS_ASSERT(ret_name == model_name);
    }

    // Test predictions on the same data.  Should be almost completely
    // accurate...
    {
      tc_sframe* sf_2 = tc_sframe_create_copy(sf, &error);
      CAPI_CHECK_ERROR(error);

      tc_sframe_remove_column(sf_2, "target", &error);
      CAPI_CHECK_ERROR(error);

      tc_parameters* p_args = tc_parameters_create_empty(&error);

      tc_parameters_add_sframe(p_args, "data", sf_2, &error);
      tc_release(sf_2);
      CAPI_CHECK_ERROR(error);

      {
        tc_variant* ret_2 = tc_model_call_method(model, "predict", p_args, &error);
        CAPI_CHECK_ERROR(error);

        tc_release(p_args);

        int is_sarray = tc_variant_is_sarray(ret_2);
      TS_ASSERT(is_sarray);

        tc_sarray* sa = tc_variant_sarray(ret_2, &error);
        CAPI_CHECK_ERROR(error);

      const auto& target_values = data.back().second;

      for (size_t i = 0; i < target_values.size(); ++i) {
        tc_flexible_type* ft = tc_sarray_extract_element(sa, i, &error);
          CAPI_CHECK_ERROR(error);

        double v = tc_ft_double(ft, &error);

          CAPI_CHECK_ERROR(error);
          tc_release(ft);

        // Make sure they are close -- on a tiny dataset like this the default
        // setting
        TS_ASSERT_DELTA(v, target_values[i], 0.5);
      }
        tc_release(ret_2);
      }

      {
        tc_parameters* export_args = tc_parameters_create_empty(&error);
        CAPI_CHECK_ERROR(error);

        // Set the l2 regression
        {
          std::string url = turi::fs_util::system_temp_directory_unique_path(
            "", "_coreml_export_test_1_tmp.mlmodel");
          tc_flexible_type* ft_name = tc_ft_create_from_cstring(url.c_str(), &error);
          CAPI_CHECK_ERROR(error);
          tc_parameters_add_flexible_type(export_args, "filename", ft_name,
                                          &error);
          CAPI_CHECK_ERROR(error);
          tc_release(ft_name);
        }

        tc_model_call_method(model, "export_to_coreml",
                             export_args, &error);
        CAPI_CHECK_ERROR(error);
        tc_release(export_args);
      }
    }

    // Test saving and loading the model.
    {
      std::string model_path =
        turi::fs_util::system_temp_directory_unique_path("", "_save_test_1_tmp_model");

      tc_model_save(model, model_path.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_model* loaded_model = tc_model_load(model_path.c_str(), &error);
      CAPI_CHECK_ERROR(error);
      TS_ASSERT(loaded_model != nullptr);

      std::string ret_name = tc_model_name(loaded_model, &error);
      CAPI_CHECK_ERROR(error);
      if(!is_auto) {
        TS_ASSERT(ret_name == model_name);
      }

      tc_release(loaded_model);
    }

    tc_release(model);
  }
}

BOOST_AUTO_TEST_CASE(test_auto_classification) {
    tc_error* error = NULL;

    CAPI_CHECK_ERROR(error);

    std::vector<std::pair<std::string, std::vector<double> > > features = {
        {"col1", {1., 1., 10., 10.}},
        {"col2", {2., 2., 20., 20.}}};
    tc_sframe* data = make_sframe_double(features);

    tc_flex_list* target_value = make_flex_list_string({"A", "A", "B", "B"});
    tc_sarray* target_sarray = tc_sarray_create_from_list(target_value, &error);
    CAPI_CHECK_ERROR(error);

    tc_sframe_add_column(data, "target", target_sarray, &error);
    CAPI_CHECK_ERROR(error);

    tc_parameters* args = tc_parameters_create_empty(&error);
    CAPI_CHECK_ERROR(error);

    { // Populate args
      tc_parameters_add_sframe(args, "data", data, &error);
      CAPI_CHECK_ERROR(error);

      // Set the target column
      tc_flexible_type* ft_name = tc_ft_create_from_cstring("target", &error);
      CAPI_CHECK_ERROR(error);
      tc_parameters_add_flexible_type(args, "target", ft_name, &error);
      CAPI_CHECK_ERROR(error);
      tc_release(ft_name);

      // Set the validation data to something empty
      tc_sframe* ft_empty_sframe = tc_sframe_create_empty(&error);
      CAPI_CHECK_ERROR(error);

      tc_parameters_add_sframe(args, "validation_data", ft_empty_sframe, &error);
      CAPI_CHECK_ERROR(error);

      tc_release(ft_empty_sframe);

      // Set the options
      tc_flex_dict* fd = tc_flex_dict_create(&error);
      CAPI_CHECK_ERROR(error);

      tc_parameters_add_flex_dict(args, "options", fd, &error);
      CAPI_CHECK_ERROR(error);

      tc_release(fd);
    }

    { // Model selection without validation data
      tc_variant* var_m =
        tc_function_call("_supervised_learning.create_automatic_classifier_model", args, &error);
      CAPI_CHECK_ERROR(error);

      tc_model* model = tc_variant_model(var_m, &error);
      CAPI_CHECK_ERROR(error);
      TS_ASSERT(model != NULL);

      tc_release(model);
      tc_release(var_m);
    }

    { // Model selection with validation data
      tc_parameters_add_sframe(args, "validation_data", data, &error);
      CAPI_CHECK_ERROR(error);

      tc_variant* var_m =
        tc_function_call("_supervised_learning.create_automatic_classifier_model", args, &error);
      CAPI_CHECK_ERROR(error);

      tc_model* model = tc_variant_model(var_m, &error);
      CAPI_CHECK_ERROR(error);
      TS_ASSERT(model != NULL);

      tc_release(model);
      tc_release(var_m);
    }

    tc_release(data);
    tc_release(args);
}

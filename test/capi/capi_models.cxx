/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE capi_models
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#include <capi/TuriCreate.h>
#include <algorithm>
#include <core/data/image/image_type.hpp>
#include <vector>
#include <core/storage/fileio/fileio_constants.hpp>
#include <core/util/fs_util.hpp>

#ifdef __linux
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <boost/filesystem.hpp>

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
      std::string model_path;
      std::string error_message;
      std::string expected_substr;

#ifdef MECHANISM_FOR_TRIGGERING_AN_ERROR_IN_DOCKER
      // sad path 1 - attempting to save without permission to location
      // ensure the error message contains useful info

      // first, make a directory we can't write to
      std::string bad_directory =
        turi::fs_util::system_temp_directory_unique_path("capi_model_permission_denied", "");
      turi::fileio::create_directory(bad_directory);
      model_path = turi::fs_util::join({bad_directory, "model.mlmodel"});

#ifdef __linux
      // set the immutable bit on it
      int new_attrs = FS_IMMUTABLE_FL;
      size_t fd = open(bad_directory.c_str(), 0);
      // If ioctl returns -1, it means the user probably isn't root -
      // in that case, try setting owner_read permission instead.
      // As root, we must rely on the immutable flag.
      if (ioctl(fd, FS_IOC_SETFLAGS, &new_attrs) == -1) {
        boost::filesystem::permissions(bad_directory, boost::filesystem::owner_read);
      }
      close(fd);
#else
      boost::filesystem::permissions(bad_directory, boost::filesystem::owner_read);
#endif

      tc_model_save(model, model_path.c_str(), &error);
      TS_ASSERT_DIFFERS(error, nullptr);
      error_message = tc_error_message(error);
      expected_substr = "Ensure that you have write permission to this location, or try again with a different path";
      TS_ASSERT_DIFFERS(error_message.find(expected_substr), error_message.npos);
      error = nullptr;
#endif  // MECHANISM_FOR_TRIGGERING_AN_ERROR_IN_DOCKER

      // sad path 2 - attempting to save into an existing non-directory path
      // ensure the error message contains useful info
      model_path =
        turi::fs_util::system_temp_directory_unique_path("", "_save_test_1_tmp_file");
      {
        std::ofstream tmp_file(model_path);
        tmp_file << "Hello world";
      }
      tc_model_save(model, model_path.c_str(), &error);
      TS_ASSERT_DIFFERS(error, nullptr);
      error_message = tc_error_message(error);
      expected_substr = "It already exists as a file";
      TS_ASSERT_DIFFERS(error_message.find(expected_substr), error_message.npos);
      error = nullptr;

      // happy path - save should succeed
      model_path =
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

BOOST_AUTO_TEST_CASE(test_recommender) {
  for (const char* model_name :
       {"popularity", "item_similarity"}) {

    tc_error* error = NULL;

    CAPI_CHECK_ERROR(error);

    std::vector<std::pair<std::string, std::vector<int64_t> > > data = {
        {"user_id", {1, 1, 2, 2, 2, 3}},
        {"item_id", {1, 2, 2, 3, 4, 3}},
        {"target", {0, 1, 1, 1, 1, 0}}};

    tc_sframe* sf = make_sframe_integer(data);

    tc_parameters* args = tc_parameters_create_empty(&error);

    CAPI_CHECK_ERROR(error);

    // Add in the sframe; then destroy it when we're done with it.
    
    tc_parameters_add_sframe(args, "dataset", sf, &error);
    CAPI_CHECK_ERROR(error);
    
    tc_parameters_add_sframe(args, "user_data", sf, &error);
    CAPI_CHECK_ERROR(error);
    
    tc_parameters_add_sframe(args, "item_data", sf, &error);
    CAPI_CHECK_ERROR(error);

    // Set the options
    {
      tc_flex_dict* fd = tc_flex_dict_create(&error); 
      CAPI_CHECK_ERROR(error);

      tc_parameters_add_flex_dict(args, "opts", fd, &error);
      CAPI_CHECK_ERROR(error);


      tc_release(fd);
    }

    {
      tc_flex_dict* fd = tc_flex_dict_create(&error); 
      CAPI_CHECK_ERROR(error);

      tc_parameters_add_flex_dict(args, "extra_data", fd, &error);
      CAPI_CHECK_ERROR(error);


      tc_release(fd);
    }

    tc_model* model; 

    // We now have enough to create the model.
    model = tc_model_new(model_name, &error);
    CAPI_CHECK_ERROR(error);

    tc_variant* ret = tc_model_call_method(model, "train", args, &error);
    CAPI_CHECK_ERROR(error);
    TS_ASSERT(ret != NULL);

    tc_release(ret);
    

    tc_release(args);

    TS_ASSERT(model != NULL);

    std::string ret_name = tc_model_name(model, &error);

    TS_ASSERT(ret_name == model_name);
    

    // Test predictions on the same data.  Should be almost completely
    // accurate...
    {
      std::vector<std::pair<std::string, std::vector<int64_t> > > rec_data = {
        {"user_id", {1}}};

      tc_sframe* sf = make_sframe_integer(rec_data);

      CAPI_CHECK_ERROR(error);

      tc_parameters* p_args = tc_parameters_create_empty(&error);

      tc_parameters_add_sframe(p_args, "query", sf, &error);
      CAPI_CHECK_ERROR(error);

      tc_parameters_add_int64(p_args, "top_k", 2, &error);
      CAPI_CHECK_ERROR(error);

      {
        tc_variant* ret_2 = tc_model_call_method(model, "recommend", p_args, &error);
        CAPI_CHECK_ERROR(error);

        tc_release(p_args);

        int is_sframe = tc_variant_is_sframe(ret_2);
        TS_ASSERT(is_sframe);

        tc_sframe* sa = tc_variant_sframe(ret_2, &error);
        CAPI_CHECK_ERROR(error);
        size_t n_rows = tc_sframe_num_rows(sa, &error);

        TS_ASSERT(n_rows == 2);
        CAPI_CHECK_ERROR(error);

        tc_release(ret_2);
      }

      if(std::string(model_name) == "item_similarity") {
        tc_parameters* export_args = tc_parameters_create_empty(&error);
        CAPI_CHECK_ERROR(error);

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
      std::string model_path;
      std::string error_message;
      std::string expected_substr;
      
      model_path =
        turi::fs_util::system_temp_directory_unique_path("", "_save_test_1_tmp_model");

      tc_model_save(model, model_path.c_str(), &error);
      CAPI_CHECK_ERROR(error);

      tc_model* loaded_model = tc_model_load(model_path.c_str(), &error);
      CAPI_CHECK_ERROR(error);
      TS_ASSERT(loaded_model != nullptr);

      std::string ret_name = tc_model_name(loaded_model, &error);
      CAPI_CHECK_ERROR(error);
    
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

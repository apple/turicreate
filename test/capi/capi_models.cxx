#define BOOST_TEST_MODULE capi_models
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <capi/TuriCore.h>
#include <vector>
#include "capi_utils.hpp"

BOOST_AUTO_TEST_CASE(test_boosted_trees_double) {


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

  // Set the l2 regression
  {
    tc_flexible_type* ft_name = tc_ft_create_from_cstring("target", &error);
    TS_ASSERT(error == NULL);
    tc_parameters_add_flexible_type(args, "target", ft_name, &error);
    TS_ASSERT(error == NULL);
    tc_ft_destroy(ft_name);
  }

  // We now have enough to create the model.
  tc_model* model = tc_model_new("boosted_trees_regression", &error);
  TS_ASSERT(error == NULL);

  tc_model_call_method(model, "train", args, &error);
  TS_ASSERT(error == NULL);

  tc_parameters_destroy(args);

  TS_ASSERT(model != NULL);

  std::string ret_name = tc_model_name(model, &error);

  TS_ASSERT(ret_name == "boosted_trees_regression");

  // Test predictions on the same data.  Should be almost completely accurate...
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
        tc_flexible_type* ft_name = tc_ft_create_from_cstring("coreml_export_test_1_tmp.mlmodel", &error);
        TS_ASSERT(error == NULL);
        tc_parameters_add_flexible_type(export_args, "filename", ft_name, &error);
        TS_ASSERT(error == NULL);
        tc_ft_destroy(ft_name);
      }

      tc_variant* ret_2 = tc_model_call_method(model, "export_to_coreml", export_args, &error);
      TS_ASSERT(error == NULL);
      tc_parameters_destroy(export_args);
    }
  }
}


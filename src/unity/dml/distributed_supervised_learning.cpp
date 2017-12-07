/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Data structures
#include <unity/lib/unity_sframe.hpp>
#include <sframe/sframe.hpp>
#include <unity/lib/gl_sframe.hpp>

// Distributed
#include<distributed/distributed_context.hpp>
#include<distributed/cluster_interface.hpp>

// Toolkits
#include <unity/dml/dml_function_wrapper.hpp>
#include <unity/dml/dml_class_registry.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>
#include <toolkits/supervised_learning/linear_regression.hpp>
#include <toolkits/supervised_learning/linear_svm.hpp>
#include <toolkits/supervised_learning/logistic_regression.hpp>
#include <toolkits/supervised_learning/boosted_trees.hpp>
#include <toolkits/supervised_learning/random_forest.hpp>

namespace turi {
namespace supervised {

void register_supervised_learning_models() {
  dml_class_registry::get_instance().register_model<linear_regression>();
  dml_class_registry::get_instance().register_model<linear_svm>();
  dml_class_registry::get_instance().register_model<logistic_regression>();
  dml_class_registry::get_instance().register_model<xgboost::boosted_trees_regression>();
  dml_class_registry::get_instance().register_model<xgboost::boosted_trees_classifier>();
  dml_class_registry::get_instance().register_model<xgboost::random_forest_regression>();
  dml_class_registry::get_instance().register_model<xgboost::random_forest_classifier>();
}

/**
 * Distributed model training.
 */
variant_type distributed_supervised_train_impl(const variant_map_type& kwargs) {
  register_supervised_learning_models();
  DASSERT_TRUE(kwargs.count("model_name") > 0);
  DASSERT_TRUE(kwargs.count("target") > 0);
  DASSERT_TRUE(kwargs.count("features") > 0);

  // Get data.
  sframe  X = *(variant_get_value<std::shared_ptr<unity_sframe>>(
             kwargs.at("features"))->get_underlying_sframe());
  sframe  y = *(variant_get_value<std::shared_ptr<unity_sframe>>(
             kwargs.at("target"))->get_underlying_sframe());
  std::string model_name =
            variant_get_value<std::string>(kwargs.at("model_name"));

  // Remove option names that are not needed.
  variant_map_type opts = kwargs;
  opts.erase("model_name");
  opts.erase("target");
  opts.erase("features");

  // Remove internal double underscore variables
  // E.g. __path_of_features, __path_of_target..
  std::vector<std::string> internal_keys;
  for (auto& kv : kwargs) {
    if (boost::starts_with(kv.first, "__"))
      internal_keys.push_back(kv.first);
  }
  for (auto& key : internal_keys)
    opts.erase(key);

  // Train a model.
  std::shared_ptr<supervised_learning_model_base> model = create(
                          X, y, model_name, opts);

  return to_variant(model);
}

} // supervised
} // turicreate

REGISTER_DML_DISTRIBUTED_FUNCTION(distributed_supervised_train,
  turi::supervised::distributed_supervised_train_impl);

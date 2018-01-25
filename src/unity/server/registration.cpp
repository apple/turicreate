/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/server/registration.hpp>

#include <unity/lib/simple_model.hpp>
#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/unity_sarray_builder.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/unity_sframe_builder.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/unity_sketch.hpp>

#include <unity/lib/extensions/ml_model.hpp>

#include <unity/lib/visualization/show.hpp>

#include <unity/toolkits/activity_classification/class_registrations.hpp>

#include <unity/toolkits/evaluation/metrics.hpp>
#include <unity/toolkits/evaluation/unity_evaluation.hpp>

#include <unity/toolkits/graph_analytics/include.hpp>
#include <unity/toolkits/image/image_fn_export.hpp>

#include <unity/toolkits/ml_model/python_model.hpp>

#include <unity/toolkits/nearest_neighbors/distances.hpp>
#include <unity/toolkits/nearest_neighbors/unity_nearest_neighbors.hpp>
#include <unity/toolkits/supervised_learning/unity_supervised_learning.hpp>
#include <unity/toolkits/text/unity_text.hpp>
#include <unity/toolkits/recsys/unity_recsys.hpp>
#include <unity/toolkits/clustering/unity_kmeans.hpp>

#include <unity/toolkits/nearest_neighbors/class_registrations.hpp>
#include <unity/toolkits/recsys/class_registrations.hpp>
#include <unity/toolkits/text/class_registrations.hpp>
#include <unity/toolkits/supervised_learning/class_registrations.hpp>
#include <unity/toolkits/feature_engineering/class_registrations.hpp>
#include <unity/toolkits/pattern_mining/class_registrations.hpp>
#include <unity/toolkits/clustering/class_registrations.hpp>

namespace turi {

void register_functions(toolkit_function_registry& registry) {
  registry.register_toolkit_function(image_util::get_toolkit_function_registration());
  registry.register_toolkit_function(visualization::get_toolkit_function_registration());

  // Register proprietary toolkits
  registry.register_toolkit_function(turi::kmeans::get_toolkit_function_registration(), "_kmeans");

  registry.register_toolkit_function(turi::pagerank::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::kcore::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::connected_component::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::graph_coloring::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::triangle_counting::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::sssp::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::degree_count::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::label_propagation::get_toolkit_function_registration());

  registry.register_toolkit_function(turi::text::get_toolkit_function_registration(), "_text");
  registry.register_toolkit_function(turi::evaluation::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::recsys::get_toolkit_function_registration(), "_recsys");
  registry.register_toolkit_function(turi::supervised::get_toolkit_function_registration(), "_supervised_learning");
  registry.register_toolkit_function(turi::nearest_neighbors::get_toolkit_function_registration(), "_nearest_neighbors");
  registry.register_toolkit_function(turi::distances::get_toolkit_function_registration(), "_distances");
  registry.register_toolkit_function(turi::image_util::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::ml_model_sdk::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::pattern_mining::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::sdk_model::activity_classification::get_toolkit_function_registration());
}

/// A helper function to automatically add an entry to the model
/// lookup table with the proper information
template <typename Model>
static inline void register_model_helper(turi::toolkit_class_registry& g_toolkit_classes) {
  Model m;
  std::string name = (dynamic_cast<turi::model_base*>(&m))->name();
  g_toolkit_classes.register_toolkit_class(name, [](){return new Model;});
}

void register_models(toolkit_class_registry& registry) {

  // Register toolkit models
  // Python Model
  registry.register_toolkit_class(turi::python_model::get_toolkit_class_registration());

  register_model_helper<simple_model>(registry);

  // Recsys Models
  registry.register_toolkit_class(turi::recsys::get_toolkit_class_registration());

  // Supervised_learning models.
  registry.register_toolkit_class(turi::supervised::get_toolkit_class_registration());

  // Nearest neighbors models
  registry.register_toolkit_class(turi::nearest_neighbors::get_toolkit_class_registration());

  // Text models
  registry.register_toolkit_class(turi::text::get_toolkit_class_registration());


  // Clustering
  registry.register_toolkit_class(turi::kmeans::get_toolkit_class_registration());


 // Feature Transformations
  registry.register_toolkit_class(turi::sdk_model::feature_engineering::get_toolkit_class_registration());

  // Pattern Mining
  registry.register_toolkit_class(turi::pattern_mining::get_toolkit_class_registration());

}

}

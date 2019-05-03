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

#include <unity/lib/annotation/class_registrations.hpp>
#include <unity/lib/visualization/show.hpp>

#include <unity/toolkits/activity_classification/class_registrations.hpp>
#include <unity/toolkits/object_detection/class_registrations.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/class_registrations.hpp>
#include <unity/toolkits/drawing_classifier/class_registrations.hpp>

#include <unity/toolkits/evaluation/metrics.hpp>
#include <unity/toolkits/evaluation/unity_evaluation.hpp>

#include <unity/toolkits/graph_analytics/include.hpp>
#include <unity/toolkits/image/image_fn_export.hpp>

#include <unity/toolkits/ml_model/python_model.hpp>

#include <unity/toolkits/nearest_neighbors/distances.hpp>
#include <unity/toolkits/nearest_neighbors/unity_nearest_neighbors.hpp>
#include <unity/toolkits/text/unity_text.hpp>
#include <unity/toolkits/recsys/recsys_model_base.hpp>
#include <unity/toolkits/clustering/unity_kmeans.hpp>

#include <unity/toolkits/nearest_neighbors/class_registrations.hpp>
#include <unity/toolkits/recsys/class_registrations.hpp>
#include <unity/toolkits/text/class_registrations.hpp>
#include <unity/toolkits/supervised_learning/class_registrations.hpp>
#include <unity/toolkits/supervised_learning/supervised_learning.hpp>
#include <unity/toolkits/feature_engineering/class_registrations.hpp>
#include <unity/toolkits/pattern_mining/class_registrations.hpp>
#include <unity/toolkits/clustering/class_registrations.hpp>
#include <unity/toolkits/util/class_registrations.hpp>
#include <unity/toolkits/prototype/class_registrations.hpp>

#include <toolkits/image_deep_feature_extractor/class_registrations.hpp>


namespace turi {

void register_functions(toolkit_function_registry& registry) {

  registry.register_toolkit_function(turi::evaluation::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::supervised::get_toolkit_function_registration());

  registry.register_toolkit_function(image_util::get_toolkit_function_registration());
  registry.register_toolkit_function(visualization::get_toolkit_function_registration());

  registry.register_toolkit_function(turi::annotate::get_toolkit_function_registration());

  // Register proprietary toolkits
  registry.register_toolkit_function(turi::kmeans::get_toolkit_function_registration(), "_kmeans");

  registry.register_toolkit_function(turi::pagerank::get_toolkit_function_registration(), "_toolkits.graph.pagerank");
  registry.register_toolkit_function(turi::kcore::get_toolkit_function_registration(), "_toolkits.graph.kcore");
  registry.register_toolkit_function(turi::connected_component::get_toolkit_function_registration(), "_toolkits.graph.connected_components");
  registry.register_toolkit_function(turi::graph_coloring::get_toolkit_function_registration(), "_toolkits.graph.graph_coloring");
  registry.register_toolkit_function(turi::triangle_counting::get_toolkit_function_registration(), "_toolkits.graph.triangle_counting");
  registry.register_toolkit_function(turi::sssp::get_toolkit_function_registration(), "_toolkits.graph.sssp");
  registry.register_toolkit_function(turi::degree_count::get_toolkit_function_registration(), "_toolkits.graph.degree_count");
  registry.register_toolkit_function(turi::label_propagation::get_toolkit_function_registration(), "_toolkits.graph.label_propagation");

  registry.register_toolkit_function(turi::text::get_toolkit_function_registration(), "_text");
  registry.register_toolkit_function(turi::evaluation::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::recsys::get_toolkit_function_registration(), "_recsys");
  registry.register_toolkit_function(turi::supervised::get_toolkit_function_registration(), "_supervised_learning");
  registry.register_toolkit_function(turi::nearest_neighbors::get_toolkit_function_registration(), "_nearest_neighbors");
  registry.register_toolkit_function(turi::distances::get_toolkit_function_registration(), "_distances");
  registry.register_toolkit_function(turi::image_util::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::ml_model_sdk::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::pattern_mining::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::activity_classification::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::drawing_classifier::get_toolkit_function_registration());
  registry.register_toolkit_function(turi::util::get_toolkit_function_registration());
}

namespace registration_internal {

// Define get_toolkit_class_registration for simple_model so that some toolkits
// can just wrap their outputs in a simple_model instance (without subclassing).
BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(simple_model)
END_CLASS_REGISTRATION

}  // registration_internal namespace

void register_models(toolkit_class_registry& registry) {

  // Register toolkit models
  // Python Model
  registry.register_toolkit_class(turi::python_model::get_toolkit_class_registration());

  // Toolkits using simple_model
  registry.register_toolkit_class(
      registration_internal::get_toolkit_class_registration());

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

#if HAS_CORE_ML
  // Image Deep Feature Extractor
  registry.register_toolkit_class(turi::image_deep_feature_extractor::get_toolkit_class_registration());
#endif

  // Object Detection
  registry.register_toolkit_class(turi::object_detection::get_toolkit_class_registration());

  // One Shot Object Detection
  registry.register_toolkit_class(turi::one_shot_object_detection::get_toolkit_class_registration());
  
  // Activity Classification
  registry.register_toolkit_class(turi::activity_classification::get_toolkit_class_registration());

  // Various prototypes
  registry.register_toolkit_class(turi::prototype::get_toolkit_class_registration());

  // Annotate Registration
  registry.register_toolkit_class(turi::annotate::get_toolkit_class_registration());

}

}

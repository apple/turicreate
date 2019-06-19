/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_neural_nets_model_exporter

#include <toolkits/coreml_export/neural_net_models_exporter.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <core/util/test_macros.hpp>

namespace turi {
namespace object_detection {
namespace {

using turi::neural_net::model_spec;

BOOST_AUTO_TEST_CASE(test_object_detector_export_coreml_with_nms) {
    
    const std::string test_annotations_name = "test_annotations";
    const std::string test_image_name = "test_image";
    const std::vector<std::string> test_class_labels = { "label1", "label2" };
    static constexpr size_t test_max_iterations = 4;
    double test_iou_threshold = 0.55;
    double test_confidence_threshold = 0.15;
    
    std::map<std::string, flexible_type> options;
    options["include_non_maximum_suppression"] = 1;
    options["iou_threshold"] = test_iou_threshold;
    options["confidence_threshold"] = test_confidence_threshold;
    
    flex_dict user_defined_metadata;
    user_defined_metadata.emplace_back("model", "model");
    user_defined_metadata.emplace_back("max_iterations", test_max_iterations);
    user_defined_metadata.emplace_back("training_iterations", test_max_iterations);
    user_defined_metadata.emplace_back("include_non_maximum_suppression", "True");
    user_defined_metadata.emplace_back("feature", test_image_name);
    user_defined_metadata.emplace_back("annotations", test_annotations_name);
    user_defined_metadata.emplace_back("classes", "label1, label2");
    user_defined_metadata.emplace_back("type", "object_detector");
    user_defined_metadata.emplace_back("confidence_threshold", options["confidence_threshold"]);
    user_defined_metadata.emplace_back("iou_threshold", options["iou_threshold"]);
    
    flex_list t_class_labels = flex_list(test_class_labels.begin(), test_class_labels.end());
    
    model_spec yolo_nn_spec;
    yolo_nn_spec.add_convolution("test_layer", "image", 16, 16, 3, 3, 1, 1,
                                 model_spec::padding_type::SAME,
                                 /* weight_init_fn */ [](float*w , float* w_end) {
                                     for (int i = 0; i < w_end - w; ++i) {
                                         w[i] = static_cast<float>(i);
                                     }
                                 });
    
    std::shared_ptr<coreml::MLModelWrapper> model_wrapper =
    export_object_detector_model(yolo_nn_spec,
                                 13 * 32,
                                 13 * 32,
                                 test_class_labels.size(),
                                 13 * 13 * 15,
                                 std::move(user_defined_metadata), std::move(t_class_labels),
                                 std::move(options));
    
    std::shared_ptr<CoreML::Model> c_model = model_wrapper->coreml_model();
    auto p_model = c_model->getProto();
    
    const ::CoreML::Specification::Pipeline &pipeline = p_model.pipeline();
    
    const auto &model_nms = pipeline.models(1).nonmaximumsuppression();
    
    auto labels = model_nms.stringclasslabels();
    for (int i = 0; i < labels.vector_size(); i++) {
        TS_ASSERT_EQUALS(labels.vector(i), test_class_labels[i]);
    }
    TS_ASSERT_EQUALS(model_nms.iouthreshold(), test_iou_threshold);
    TS_ASSERT_EQUALS(model_nms.confidencethreshold(), test_confidence_threshold);
    TS_ASSERT_EQUALS(model_nms.confidenceinputfeaturename(), "raw_confidence");
    TS_ASSERT_EQUALS(model_nms.coordinatesinputfeaturename(), "raw_coordinates");
    TS_ASSERT_EQUALS(model_nms.iouthresholdinputfeaturename(), "iouThreshold");
    TS_ASSERT_EQUALS(model_nms.confidencethresholdinputfeaturename(), "confidenceThreshold");
    TS_ASSERT_EQUALS(model_nms.confidenceoutputfeaturename(), "confidence");
    TS_ASSERT_EQUALS(model_nms.coordinatesoutputfeaturename(), "coordinates");
    
}
    
}  // namespace
}  // namespace object_detection
}  // namespace turi

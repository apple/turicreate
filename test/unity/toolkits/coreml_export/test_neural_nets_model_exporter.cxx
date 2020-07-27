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
    float test_iou_threshold = 0.55f;
    float test_confidence_threshold = 0.15f;

    flex_dict user_defined_metadata;
    user_defined_metadata.emplace_back("model", "model");
    user_defined_metadata.emplace_back("max_iterations", test_max_iterations);
    user_defined_metadata.emplace_back("training_iterations", test_max_iterations);
    user_defined_metadata.emplace_back("include_non_maximum_suppression", "True");
    user_defined_metadata.emplace_back("feature", test_image_name);
    user_defined_metadata.emplace_back("annotations", test_annotations_name);
    user_defined_metadata.emplace_back("classes", "label1, label2");
    user_defined_metadata.emplace_back("type", "object_detector");
    user_defined_metadata.emplace_back("confidence_threshold",
                                       test_confidence_threshold);
    user_defined_metadata.emplace_back("iou_threshold", test_iou_threshold);

    // Create an arbitrary pipeline with one model with one input description.
    std::unique_ptr<CoreML::Specification::Pipeline> model_to_export;
    model_to_export.reset(new CoreML::Specification::Pipeline);
    model_to_export->add_models()->mutable_description()->add_input()->set_name(
        "test_input");

    flex_list t_class_labels =
        flex_list(test_class_labels.begin(), test_class_labels.end());
    std::shared_ptr<coreml::MLModelWrapper> model_wrapper = export_object_detector_model(
        neural_net::pipeline_spec(std::move(model_to_export)), test_class_labels.size(),
        13 * 13 * 15, std::move(t_class_labels), test_confidence_threshold, test_iou_threshold,
        /* include_non_maximum_suppression */ true,
        /* use_nms_layer */ false,
        /* use_most_confident_class*/ false);
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

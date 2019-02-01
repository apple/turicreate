/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#define BOOST_TEST_MODULE test_od_yolo

#include <unity/toolkits/object_detection/od_yolo.hpp>

#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <unity/toolkits/coreml_export/mlmodel_include.hpp>
#include <unity/toolkits/neural_net/model_spec.hpp>

namespace turi {
namespace object_detection {
namespace {

using turi::neural_net::model_spec;

BOOST_AUTO_TEST_CASE(test_add_tiny_darknet_yolo) {

  static constexpr char COORDINATES_NAME[] = "test_coordinates";
  static constexpr char CONFIDENCE_NAME[] = "test_confidence";
  static constexpr char INPUT_NAME[] = "test_input";
  static constexpr size_t OUTPUT_GRID_SIZE = 13;
  static constexpr size_t OUTPUT_GRID_AREA = OUTPUT_GRID_SIZE*OUTPUT_GRID_SIZE;
  static constexpr size_t NUM_CLASSES = 6;
  static constexpr size_t NUM_PREDS = NUM_CLASSES + 5;  // 4 for bbox, 1 conf

  const std::string prefix = "__test__";
  const std::vector<std::pair<float, float>> anchor_boxes = {
      {1.f, 2.f}, {1.f, 1.f}, {2.f, 1.f},
      {2.f, 4.f}, {2.f, 2.f}, {4.f, 2.f},
      {4.f, 8.f}, {4.f, 4.f}, {8.f, 4.f},
      {8.f, 16.f}, {8.f, 8.f}, {16.f, 8.f},
      {16.f, 32.f}, {16.f, 16.f}, {32.f, 16.f},
  };

  model_spec nn_spec;
  add_yolo(&nn_spec, COORDINATES_NAME, CONFIDENCE_NAME, INPUT_NAME,
           anchor_boxes, NUM_CLASSES, OUTPUT_GRID_SIZE, OUTPUT_GRID_SIZE,
           prefix);

  // The add_yolo function simply appends a mostly fixed sequence of 24 layers
  // to an existing model_spec. Assert that the resulting proto is what we want.
  // In theory, some of the layers could be reordered or have different names,
  // but it's much easier to test for exact equality.

  const CoreML::Specification::NeuralNetwork& nn = nn_spec.get_coreml_spec();
  TS_ASSERT_EQUALS(nn.layers_size(), 24);

  const auto& ymap_sp_pre = nn.layers(0);
  TS_ASSERT_EQUALS(ymap_sp_pre.input_size(), 1);
  TS_ASSERT_EQUALS(ymap_sp_pre.input(0), INPUT_NAME);
  TS_ASSERT_EQUALS(ymap_sp_pre.output_size(), 1);
  TS_ASSERT_EQUALS(ymap_sp_pre.output(0), prefix + "ymap_sp_pre");
  TS_ASSERT_EQUALS(ymap_sp_pre.reshape().targetshape_size(), 4);
  TS_ASSERT_EQUALS(ymap_sp_pre.reshape().targetshape(0), 1);
  TS_ASSERT_EQUALS(ymap_sp_pre.reshape().targetshape(1), anchor_boxes.size());
  TS_ASSERT_EQUALS(ymap_sp_pre.reshape().targetshape(2), NUM_PREDS);
  TS_ASSERT_EQUALS(ymap_sp_pre.reshape().targetshape(3), OUTPUT_GRID_AREA);

  const auto& ymap_sp = nn.layers(1);
  TS_ASSERT_EQUALS(ymap_sp.input_size(), 1);
  TS_ASSERT_EQUALS(ymap_sp.input(0), prefix + "ymap_sp_pre");
  TS_ASSERT_EQUALS(ymap_sp.output_size(), 1);
  TS_ASSERT_EQUALS(ymap_sp.output(0), prefix + "ymap_sp");
  TS_ASSERT_EQUALS(ymap_sp.permute().axis_size(), 4);
  TS_ASSERT_EQUALS(ymap_sp.permute().axis(0), 0);
  TS_ASSERT_EQUALS(ymap_sp.permute().axis(1), 2);
  TS_ASSERT_EQUALS(ymap_sp.permute().axis(2), 1);
  TS_ASSERT_EQUALS(ymap_sp.permute().axis(3), 3);

  const auto& raw_rel_xy_sp = nn.layers(2);
  TS_ASSERT_EQUALS(raw_rel_xy_sp.input_size(), 1);
  TS_ASSERT_EQUALS(raw_rel_xy_sp.input(0), prefix + "ymap_sp");
  TS_ASSERT_EQUALS(raw_rel_xy_sp.output_size(), 1);
  TS_ASSERT_EQUALS(raw_rel_xy_sp.output(0), prefix + "raw_rel_xy_sp");
  TS_ASSERT_EQUALS(raw_rel_xy_sp.slice().startindex(), 0);
  TS_ASSERT_EQUALS(raw_rel_xy_sp.slice().endindex(), 2);
  TS_ASSERT_EQUALS(raw_rel_xy_sp.slice().stride(), 1);
  TS_ASSERT_EQUALS(raw_rel_xy_sp.slice().axis(),
                   CoreML::Specification::SliceLayerParams::CHANNEL_AXIS);

  const auto& rel_xy_sp = nn.layers(3);
  TS_ASSERT_EQUALS(rel_xy_sp.input_size(), 1);
  TS_ASSERT_EQUALS(rel_xy_sp.input(0), prefix + "raw_rel_xy_sp");
  TS_ASSERT_EQUALS(rel_xy_sp.output_size(), 1);
  TS_ASSERT_EQUALS(rel_xy_sp.output(0), prefix + "rel_xy_sp");
  TS_ASSERT(rel_xy_sp.activation().has_sigmoid());

  const auto& rel_xy = nn.layers(4);
  TS_ASSERT_EQUALS(rel_xy.input_size(), 1);
  TS_ASSERT_EQUALS(rel_xy.input(0), prefix + "rel_xy_sp");
  TS_ASSERT_EQUALS(rel_xy.output_size(), 1);
  TS_ASSERT_EQUALS(rel_xy.output(0), prefix + "rel_xy");
  TS_ASSERT_EQUALS(rel_xy.reshape().targetshape_size(), 4);
  TS_ASSERT_EQUALS(rel_xy.reshape().targetshape(0), 1);
  TS_ASSERT_EQUALS(rel_xy.reshape().targetshape(1), 2);
  TS_ASSERT_EQUALS(rel_xy.reshape().targetshape(2),
                   OUTPUT_GRID_AREA * anchor_boxes.size());
  TS_ASSERT_EQUALS(rel_xy.reshape().targetshape(3), 1);

  const auto& constant_xy = nn.layers(5);
  TS_ASSERT_EQUALS(constant_xy.input_size(), 0);
  TS_ASSERT_EQUALS(constant_xy.output_size(), 1);
  TS_ASSERT_EQUALS(constant_xy.output(0), prefix + "constant_xy");
  TS_ASSERT_EQUALS(constant_xy.loadconstant().shape_size(), 3);
  TS_ASSERT_EQUALS(constant_xy.loadconstant().shape(0), 2);
  TS_ASSERT_EQUALS(constant_xy.loadconstant().shape(1),
                   OUTPUT_GRID_AREA * anchor_boxes.size());
  TS_ASSERT_EQUALS(constant_xy.loadconstant().shape(2), 1);
  TS_ASSERT_EQUALS(constant_xy.loadconstant().data().floatvalue_size(),
                   2 * 2535 * 1);
  for (size_t b = 0; b < anchor_boxes.size(); ++b) {
    for (size_t y = 0; y < OUTPUT_GRID_SIZE; ++y) {
      for (size_t x = 0; x < OUTPUT_GRID_SIZE; ++x) {
        int idx = static_cast<int>(  b * OUTPUT_GRID_AREA
                                   + y * OUTPUT_GRID_SIZE
                                   + x);
        TS_ASSERT_EQUALS(constant_xy.loadconstant().data().floatvalue(idx), x);
        idx += anchor_boxes.size() * OUTPUT_GRID_AREA;
        TS_ASSERT_EQUALS(constant_xy.loadconstant().data().floatvalue(idx), y);
      }
    }
  }

  const auto& xy = nn.layers(6);
  TS_ASSERT_EQUALS(xy.input_size(), 2);
  TS_ASSERT_EQUALS(xy.input(0), prefix + "constant_xy");
  TS_ASSERT_EQUALS(xy.input(1), prefix + "rel_xy");
  TS_ASSERT_EQUALS(xy.output_size(), 1);
  TS_ASSERT_EQUALS(xy.output(0), prefix + "xy");
  TS_ASSERT(xy.has_add());

  const auto& raw_rel_wh_sp = nn.layers(7);
  TS_ASSERT_EQUALS(raw_rel_wh_sp.input_size(), 1);
  TS_ASSERT_EQUALS(raw_rel_wh_sp.input(0), prefix + "ymap_sp");
  TS_ASSERT_EQUALS(raw_rel_wh_sp.output_size(), 1);
  TS_ASSERT_EQUALS(raw_rel_wh_sp.output(0), prefix + "raw_rel_wh_sp");
  TS_ASSERT_EQUALS(raw_rel_wh_sp.slice().startindex(), 2);
  TS_ASSERT_EQUALS(raw_rel_wh_sp.slice().endindex(), 4);
  TS_ASSERT_EQUALS(raw_rel_wh_sp.slice().stride(), 1);
  TS_ASSERT_EQUALS(raw_rel_wh_sp.slice().axis(),
                   CoreML::Specification::SliceLayerParams::CHANNEL_AXIS);

  const auto& rel_wh_sp = nn.layers(8);
  TS_ASSERT_EQUALS(rel_wh_sp.input_size(), 1);
  TS_ASSERT_EQUALS(rel_wh_sp.input(0), prefix + "raw_rel_wh_sp");
  TS_ASSERT_EQUALS(rel_wh_sp.output_size(), 1);
  TS_ASSERT_EQUALS(rel_wh_sp.output(0), prefix + "rel_wh_sp");
  TS_ASSERT_EQUALS(rel_wh_sp.unary().type(),
                   CoreML::Specification::UnaryFunctionLayerParams::EXP);

  const auto& rel_wh = nn.layers(9);
  TS_ASSERT_EQUALS(rel_wh.input_size(), 1);
  TS_ASSERT_EQUALS(rel_wh.input(0), prefix + "rel_wh_sp");
  TS_ASSERT_EQUALS(rel_wh.output_size(), 1);
  TS_ASSERT_EQUALS(rel_wh.output(0), prefix + "rel_wh");
  TS_ASSERT_EQUALS(rel_wh.reshape().targetshape_size(), 4);
  TS_ASSERT_EQUALS(rel_wh.reshape().targetshape(0), 1);
  TS_ASSERT_EQUALS(rel_wh.reshape().targetshape(1), 2 * anchor_boxes.size());
  TS_ASSERT_EQUALS(rel_wh.reshape().targetshape(2), OUTPUT_GRID_SIZE);
  TS_ASSERT_EQUALS(rel_wh.reshape().targetshape(3), OUTPUT_GRID_SIZE);

  const auto& c_anchors = nn.layers(10);
  TS_ASSERT_EQUALS(c_anchors.input_size(), 0);
  TS_ASSERT_EQUALS(c_anchors.output_size(), 1);
  TS_ASSERT_EQUALS(c_anchors.output(0), prefix + "c_anchors");
  TS_ASSERT_EQUALS(c_anchors.loadconstant().shape_size(), 3);
  TS_ASSERT_EQUALS(c_anchors.loadconstant().shape(0), 2 * anchor_boxes.size());
  TS_ASSERT_EQUALS(c_anchors.loadconstant().shape(1), OUTPUT_GRID_SIZE);
  TS_ASSERT_EQUALS(c_anchors.loadconstant().shape(2), OUTPUT_GRID_SIZE);
  TS_ASSERT_EQUALS(c_anchors.loadconstant().data().floatvalue_size(),
                   2 * anchor_boxes.size() * OUTPUT_GRID_AREA);
  for (size_t i = 0; i < anchor_boxes.size(); ++i) {
    for (size_t j = 0; j < OUTPUT_GRID_AREA; ++j) {
      int idx = static_cast<int>(i * OUTPUT_GRID_AREA + j);
      TS_ASSERT_EQUALS(c_anchors.loadconstant().data().floatvalue(idx),
                       anchor_boxes[i].first);
      idx += static_cast<int>(anchor_boxes.size() * OUTPUT_GRID_AREA);
      TS_ASSERT_EQUALS(c_anchors.loadconstant().data().floatvalue(idx),
                       anchor_boxes[i].second);
    }
  }

  const auto& wh_pre = nn.layers(11);
  TS_ASSERT_EQUALS(wh_pre.input_size(), 2);
  TS_ASSERT_EQUALS(wh_pre.input(0), prefix + "c_anchors");
  TS_ASSERT_EQUALS(wh_pre.input(1), prefix + "rel_wh");
  TS_ASSERT_EQUALS(wh_pre.output_size(), 1);
  TS_ASSERT_EQUALS(wh_pre.output(0), prefix + "wh_pre");
  TS_ASSERT(wh_pre.has_multiply());

  const auto& wh = nn.layers(12);
  TS_ASSERT_EQUALS(wh.input_size(), 1);
  TS_ASSERT_EQUALS(wh.input(0), prefix + "wh_pre");
  TS_ASSERT_EQUALS(wh.output_size(), 1);
  TS_ASSERT_EQUALS(wh.output(0), prefix + "wh");
  TS_ASSERT_EQUALS(wh.reshape().targetshape_size(), 4);
  TS_ASSERT_EQUALS(wh.reshape().targetshape(0), 1);
  TS_ASSERT_EQUALS(wh.reshape().targetshape(1), 2);
  TS_ASSERT_EQUALS(wh.reshape().targetshape(2),
                   OUTPUT_GRID_AREA * anchor_boxes.size());
  TS_ASSERT_EQUALS(wh.reshape().targetshape(3), 1);

  const auto& boxes_out_transposed = nn.layers(13);
  TS_ASSERT_EQUALS(boxes_out_transposed.input_size(), 2);
  TS_ASSERT_EQUALS(boxes_out_transposed.input(0), prefix + "xy");
  TS_ASSERT_EQUALS(boxes_out_transposed.input(1), prefix + "wh");
  TS_ASSERT_EQUALS(boxes_out_transposed.output_size(), 1);
  TS_ASSERT_EQUALS(boxes_out_transposed.output(0),
                   prefix + "boxes_out_transposed");
  TS_ASSERT(boxes_out_transposed.has_concat());
  TS_ASSERT(!boxes_out_transposed.concat().sequenceconcat());

  const auto& boxes_out = nn.layers(14);
  TS_ASSERT_EQUALS(boxes_out.input_size(), 1);
  TS_ASSERT_EQUALS(boxes_out.input(0), prefix + "boxes_out_transposed");
  TS_ASSERT_EQUALS(boxes_out.output_size(), 1);
  TS_ASSERT_EQUALS(boxes_out.output(0), prefix + "boxes_out");
  TS_ASSERT_EQUALS(boxes_out.permute().axis_size(), 4);
  TS_ASSERT_EQUALS(boxes_out.permute().axis(0), 0);
  TS_ASSERT_EQUALS(boxes_out.permute().axis(1), 2);
  TS_ASSERT_EQUALS(boxes_out.permute().axis(2), 1);
  TS_ASSERT_EQUALS(boxes_out.permute().axis(3), 3);

  const auto& coordinates = nn.layers(15);
  TS_ASSERT_EQUALS(coordinates.input_size(), 1);
  TS_ASSERT_EQUALS(coordinates.input(0), prefix + "boxes_out");
  TS_ASSERT_EQUALS(coordinates.output_size(), 1);
  TS_ASSERT_EQUALS(coordinates.output(0), COORDINATES_NAME);
  TS_ASSERT_EQUALS(coordinates.scale().shapescale_size(), 3);
  TS_ASSERT_EQUALS(coordinates.scale().shapescale(0),
                   OUTPUT_GRID_AREA * anchor_boxes.size());
  TS_ASSERT_EQUALS(coordinates.scale().shapescale(1), 4);
  TS_ASSERT_EQUALS(coordinates.scale().shapescale(2), 1);
  TS_ASSERT_EQUALS(coordinates.scale().scale().floatvalue_size(),
                   OUTPUT_GRID_AREA * anchor_boxes.size() * 4 * 1);
  for (int i = 0; i < coordinates.scale().scale().floatvalue_size(); ++i) {
    TS_ASSERT_EQUALS(coordinates.scale().scale().floatvalue(i),
                     1.f / OUTPUT_GRID_SIZE);
  }

  const auto& scores_sp = nn.layers(16);
  TS_ASSERT_EQUALS(scores_sp.input_size(), 1);
  TS_ASSERT_EQUALS(scores_sp.input(0), prefix + "ymap_sp");
  TS_ASSERT_EQUALS(scores_sp.output_size(), 1);
  TS_ASSERT_EQUALS(scores_sp.output(0), prefix + "scores_sp");
  TS_ASSERT_EQUALS(scores_sp.slice().startindex(), 5);
  TS_ASSERT_EQUALS(scores_sp.slice().endindex(), NUM_PREDS);
  TS_ASSERT_EQUALS(scores_sp.slice().stride(), 1);
  TS_ASSERT_EQUALS(scores_sp.slice().axis(),
                   CoreML::Specification::SliceLayerParams::CHANNEL_AXIS);

  const auto& probs_sp = nn.layers(17);
  TS_ASSERT_EQUALS(probs_sp.input_size(), 1);
  TS_ASSERT_EQUALS(probs_sp.input(0), prefix + "scores_sp");
  TS_ASSERT_EQUALS(probs_sp.output_size(), 1);
  TS_ASSERT_EQUALS(probs_sp.output(0), prefix + "probs_sp");
  TS_ASSERT(probs_sp.has_softmax());

  const auto& logit_conf_sp = nn.layers(18);
  TS_ASSERT_EQUALS(logit_conf_sp.input_size(), 1);
  TS_ASSERT_EQUALS(logit_conf_sp.input(0), prefix + "ymap_sp");
  TS_ASSERT_EQUALS(logit_conf_sp.output_size(), 1);
  TS_ASSERT_EQUALS(logit_conf_sp.output(0), prefix + "logit_conf_sp");
  TS_ASSERT_EQUALS(logit_conf_sp.slice().startindex(), 4);
  TS_ASSERT_EQUALS(logit_conf_sp.slice().endindex(), 5);
  TS_ASSERT_EQUALS(logit_conf_sp.slice().stride(), 1);
  TS_ASSERT_EQUALS(logit_conf_sp.slice().axis(),
                   CoreML::Specification::SliceLayerParams::CHANNEL_AXIS);

  const auto& conf_sp = nn.layers(19);
  TS_ASSERT_EQUALS(conf_sp.input_size(), 1);
  TS_ASSERT_EQUALS(conf_sp.input(0), prefix + "logit_conf_sp");
  TS_ASSERT_EQUALS(conf_sp.output_size(), 1);
  TS_ASSERT_EQUALS(conf_sp.output(0), prefix + "conf_sp");
  TS_ASSERT(conf_sp.activation().has_sigmoid());

  const auto& conf_tiled_sp = nn.layers(20);
  TS_ASSERT_EQUALS(conf_tiled_sp.input_size(), NUM_CLASSES);
  for (int i = 0; i < static_cast<int>(NUM_CLASSES); ++i) {
    TS_ASSERT_EQUALS(conf_tiled_sp.input(i), prefix + "conf_sp");
  }
  TS_ASSERT_EQUALS(conf_tiled_sp.output_size(), 1);
  TS_ASSERT_EQUALS(conf_tiled_sp.output(0), prefix + "conf_tiled_sp");
  TS_ASSERT(conf_tiled_sp.has_concat());
  TS_ASSERT(!conf_tiled_sp.concat().sequenceconcat());

  const auto& confprobs_sp = nn.layers(21);
  TS_ASSERT_EQUALS(confprobs_sp.input_size(), 2);
  TS_ASSERT_EQUALS(confprobs_sp.input(0), prefix + "conf_tiled_sp");
  TS_ASSERT_EQUALS(confprobs_sp.input(1), prefix + "probs_sp");
  TS_ASSERT_EQUALS(confprobs_sp.output_size(), 1);
  TS_ASSERT_EQUALS(confprobs_sp.output(0), prefix + "confprobs_sp");
  TS_ASSERT(confprobs_sp.has_multiply());

  const auto& confprobs_transposed = nn.layers(22);
  TS_ASSERT_EQUALS(confprobs_transposed.input_size(), 1);
  TS_ASSERT_EQUALS(confprobs_transposed.input(0), prefix + "confprobs_sp");
  TS_ASSERT_EQUALS(confprobs_transposed.output_size(), 1);
  TS_ASSERT_EQUALS(confprobs_transposed.output(0),
                   prefix + "confprobs_transposed");
  TS_ASSERT_EQUALS(confprobs_transposed.reshape().targetshape_size(), 4);
  TS_ASSERT_EQUALS(confprobs_transposed.reshape().targetshape(0), 1);
  TS_ASSERT_EQUALS(confprobs_transposed.reshape().targetshape(1), NUM_CLASSES);
  TS_ASSERT_EQUALS(confprobs_transposed.reshape().targetshape(2),
                   OUTPUT_GRID_AREA * anchor_boxes.size());
  TS_ASSERT_EQUALS(confprobs_transposed.reshape().targetshape(3), 1);

  const auto& confidence = nn.layers(23);
  TS_ASSERT_EQUALS(confidence.input_size(), 1);
  TS_ASSERT_EQUALS(confidence.input(0), prefix + "confprobs_transposed");
  TS_ASSERT_EQUALS(confidence.output_size(), 1);
  TS_ASSERT_EQUALS(confidence.output(0), CONFIDENCE_NAME);
  TS_ASSERT_EQUALS(confidence.permute().axis_size(), 4);
  TS_ASSERT_EQUALS(confidence.permute().axis(0), 0);
  TS_ASSERT_EQUALS(confidence.permute().axis(1), 2);
  TS_ASSERT_EQUALS(confidence.permute().axis(2), 1);
  TS_ASSERT_EQUALS(confidence.permute().axis(3), 3);
}

}  // namespace
}  // namespace object_detection
}  // namespace turi

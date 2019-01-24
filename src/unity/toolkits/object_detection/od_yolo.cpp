/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/neural_net/model_spec.hpp>

#include <logger/assertions.hpp>

using turi::neural_net::model_spec;

namespace turi {
namespace object_detection {

void add_yolo(model_spec* nn_spec, const std::string& coordinates_name,
              const std::string& confidence_name, const std::string& input,
              const std::vector<std::pair<float, float>>& anchor_boxes,
              size_t num_classes, size_t output_grid_height,
              size_t output_grid_width, std::string prefix) {

  // For darknet-yolo, input should be the (B*(5+C), H, W) conv8_fwd output,
  // where B is the number of anchor boxes, C is the number of classes, and H
  // is the output grid height, and W is the output grid width.

  // Note that the shapes below conform to the CoreML layout
  // (Seq_length, C, H, W), although sequence length is always 1 here.

  const size_t num_spatial = output_grid_height * output_grid_width;
  const size_t num_bounding_boxes = num_spatial * anchor_boxes.size();

  // First, organize the output of the trained model into predictions (bounding
  // box and one-hot class probabilities), by anchor box, by output-grid cell.

  // (1, B, 5+C, H*W)
  nn_spec->add_reshape(
      prefix + "ymap_sp_pre", input,
      {{ 1, anchor_boxes.size(), (5 + num_classes), num_spatial }});

  // (1, 5+C, B, H*W)
  nn_spec->add_permute(prefix + "ymap_sp", prefix + "ymap_sp_pre",
                       {{ 0, 2, 1, 3 }});

  // POSITION: X/Y
  // Slice out the predicted X/Y offsets and add in the corresponding output
  // grid cell's location.

  // (1, 2, B, H*W)
  nn_spec->add_channel_slice(prefix + "raw_rel_xy_sp", prefix + "ymap_sp",
                             /* start_index */ 0, /* end_index */ 2,
                             /* stride */ 1);

  // (1, 1, 2, B, H*W)
  nn_spec->add_sigmoid(prefix + "rel_xy_sp", prefix + "raw_rel_xy_sp");

  // (1, 2, B*H*W, 1)
  nn_spec->add_reshape(prefix + "rel_xy", prefix + "rel_xy_sp",
                       {{ 1, 2, num_bounding_boxes, 1 }});

  // (1, 2, B*H*W, 1)
  auto constant_xy_init = [=](float* out, float* last) {
    // Write the X coordinate of each output grid cell.
    for (size_t b = 0; b < anchor_boxes.size(); ++b) {
      for (size_t y = 0; y < output_grid_height; ++y) {
        for (size_t x = 0; x < output_grid_width; ++x) {
          *out++ = static_cast<float>(x);
        }
      }
    }
    // Write the Y coordinate of each output grid cell.
    for (size_t b = 0; b < anchor_boxes.size(); ++b) {
      for (size_t y = 0; y < output_grid_height; ++y) {
        for (size_t x = 0; x < output_grid_width; ++x) {
          *out++ = static_cast<float>(y);
        }
      }
    }
    ASSERT_EQ(out, last);
  };
  nn_spec->add_constant(prefix + "constant_xy", {{ 2, num_bounding_boxes, 1 }},
                        constant_xy_init);

  // (1, 2, B*H*W, 1)
  nn_spec->add_addition(prefix + "xy",
                        {prefix + "constant_xy", prefix + "rel_xy"});

  // SHAPE: WIDTH/HEIGHT
  // Slice out the predicted W/H size adjustment factors and apply them to each
  // corresponding anchor box size.

  // (1, 2, B, H*W)
  nn_spec->add_channel_slice(prefix + "raw_rel_wh_sp", prefix + "ymap_sp",
                             /* start_index */ 2, /* end_index */ 4,
                             /* stride */ 1);

  // (1, 2, B, H*W)
  nn_spec->add_exp(prefix + "rel_wh_sp", prefix + "raw_rel_wh_sp");

  // (1, 2*B, H, W)
  nn_spec->add_reshape(
      prefix + "rel_wh", prefix + "rel_wh_sp",
      {{1, 2 * anchor_boxes.size(), output_grid_height, output_grid_width}});

  // (1, 2*B, H, W)
  auto c_anchors_init = [&](float* out, float* last) {
    // Write the widths of each anchor box.
    for (const std::pair<float,float>& b : anchor_boxes) {
      for (size_t h = 0; h < output_grid_height; ++h) {
        for (size_t w = 0; w < output_grid_width; ++w) {
          *out++ = b.first;
        }
      }
    }
    // Write the heights of each anchor box.
    for (const std::pair<float,float>& b : anchor_boxes) {
      for (size_t h = 0; h < output_grid_height; ++h) {
        for (size_t w = 0; w < output_grid_width; ++w) {
          *out++ = b.second;
        }
      }
    }
    ASSERT_EQ(out, last);
  };
  nn_spec->add_constant(
      prefix + "c_anchors",
      {{ 2*anchor_boxes.size(), output_grid_height, output_grid_width }},
      c_anchors_init);

  // (1, 2*B, H, W)
  nn_spec->add_multiplication(prefix + "wh_pre",
                              { prefix + "c_anchors", prefix + "rel_wh" });

  // (1, 2, B*H*W, 1)
  nn_spec->add_reshape(prefix + "wh", prefix + "wh_pre",
                       {{ 1, 2, num_bounding_boxes, 1 }});

  // BOXES: X/Y/WIDTH/HEIGHT
  // Concatenate the POSITION and SHAPE results and normalize to [0,1].

  // (1, 4, B*H*W, 1)
  nn_spec->add_channel_concat(prefix + "boxes_out_transposed",
                              { prefix + "xy", prefix + "wh" });

  // (1, B*H*W, 4, 1)
  nn_spec->add_permute(prefix + "boxes_out", prefix + "boxes_out_transposed",
                       {{ 0, 2, 1, 3 }} );

  // (1, B*H*W, 4, 1)
  auto boxes_out_init = [&](float* out, float* last) {
    for (size_t i = 0; i < num_bounding_boxes; ++i) {
      *out++ = 1.f / output_grid_width;   // x
      *out++ = 1.f / output_grid_height;  // y
      *out++ = 1.f / output_grid_width;   // width
      *out++ = 1.f / output_grid_height;  // height
    }
    ASSERT_EQ(out, last);
  };
  nn_spec->add_scale(coordinates_name, prefix + "boxes_out",
                     {{ num_bounding_boxes, 4, 1 }}, boxes_out_init);

  // CLASS PROBABILITIES AND OBJECT CONFIDENCE

  // First, slice out the class-label scores (conditional on the predicted
  // bounding box) and the object confidence (for the bounding box).

  // (1, C, B, H*W)
  nn_spec->add_channel_slice(prefix + "scores_sp", prefix + "ymap_sp",
                             /* start_index */ 5,
                             /* end_index */ 5 + num_classes, /* stride */ 1);

  // (1, C, B, H*W)
  nn_spec->add_softmax(prefix + "probs_sp", prefix + "scores_sp");

  // (1, 1, B, H*W)
  nn_spec->add_channel_slice(prefix + "logit_conf_sp", prefix + "ymap_sp",
                             /* start_index */ 4, /* end_index */ 5,
                             /* stride */ 1);

  // (1, 1, B, H*W)
  nn_spec->add_sigmoid(prefix + "conf_sp", prefix + "logit_conf_sp");

  // Multiply the class scores and the object confidence to obtain the
  // overall confidence for each class/box pair.

  // (1, C, B, H*W)
  std::string conf = prefix + "conf_sp";
  if (num_classes > 1) {
    nn_spec->add_channel_concat(prefix + "conf_tiled_sp",
                                std::vector<std::string>(num_classes, conf));
    conf = prefix + "conf_tiled_sp";
  }

  // (1, C, B, H*W)
  nn_spec->add_multiplication(prefix + "confprobs_sp",
                              { conf, prefix + "probs_sp" });

  // (1, C, B*H*W, 1)
  nn_spec->add_reshape(prefix + "confprobs_transposed", prefix + "confprobs_sp",
                       {{ 1, num_classes, num_bounding_boxes, 1 }});

  // (1, B*H*W, C, 1)
  nn_spec->add_permute(confidence_name, prefix + "confprobs_transposed",
                       {{ 0, 2, 1, 3 }});
}

}  // object_detection
}  // turi

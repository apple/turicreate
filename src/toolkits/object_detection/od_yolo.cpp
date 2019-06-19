/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/object_detection/od_yolo.hpp>

#include <algorithm>
#include <cmath>

#include <core/logging/assertions.hpp>

namespace turi {
namespace object_detection {

using neural_net::float_array;
using neural_net::image_annotation;
using neural_net::image_box;
using neural_net::model_spec;

namespace {

float sigmoid(float x) {
  return 1.f / (1.f + std::exp(-x));
}

void apply_softmax(std::vector<float>* values) {
  const float max_value = *std::max_element(values->begin(), values->end());
  float norm = 0.f;
  for (float& value : *values) {
    value = std::exp(value - max_value);
    norm += value;
  }
  for (float& value : *values) {
    value /= norm;
  }
}

}  // namespace

void convert_annotations_to_yolo(
    const std::vector<image_annotation>& annotations, size_t output_height,
    size_t output_width, size_t num_anchors, size_t num_classes, float* out) {

  // Number of floats to represent bbox (4), confidence (1), and a one-hot
  // encoding of the class (num_classes).
  size_t label_size = 5 + num_classes;

  // Initialize the output buffer. We can iterate by "label", which is
  // conceptually the lowest-order dimension of the (H,W,num_anchors,label_size)
  // array.
  // TODO: Add a mutable float_array interface so we can validate size.
  float* out_end =
      out + output_height * output_width * num_anchors * label_size;
  for (float* ptr = out; ptr < out_end; ptr += label_size) {

    // Initialize the bounding boxes and confidences to 0.
    std::fill(ptr, ptr + 5, 0.f);

    // Initialize the class probabilities for each output-grid cell and anchor
    // box to 1/num_classes.
    std::fill(ptr + 5, ptr + label_size, 1.0f / num_classes);
  }

  // Iterate through all the annotations for one image.
  for (const image_annotation& annotation : annotations) {

    // Scale the bounding box to the output grid, converting to the YOLO
    // representation, defining each box by its center.
    const image_box& bbox = annotation.bounding_box;
    float center_x = output_width * (bbox.x + (bbox.width / 2.f));
    float center_y = output_height * (bbox.y + (bbox.height / 2.f));
    float width = output_width * bbox.width;
    float height = output_height * bbox.height;

    // Skip bounding boxes with trivial area, to guard against issues in
    // augmentation.
    if (width * height < 0.001f) continue;

    // Write the label into the output grid cell containing the bounding box
    // center.
    float icenter_x = std::floor(center_x);
    float icenter_y = std::floor(center_y);
    if (0.f <= icenter_x && icenter_x < output_width &&
        0.f <= icenter_y && icenter_y < output_height) {

      size_t output_grid_stride = num_anchors * label_size;
      size_t output_grid_offset = static_cast<size_t>(icenter_x) +
          static_cast<size_t>(icenter_y) * output_width;

      // Write the label once for each anchor box.
      float* anchor_out = out + output_grid_offset * output_grid_stride;
      for (size_t anchor_idx = 0; anchor_idx < num_anchors; ++anchor_idx) {

        // Write YOLO-formatted bounding box. YOLO uses (x, y)/(w, h) order.
        anchor_out[0] = center_x - icenter_x;
        anchor_out[1] = center_y - icenter_y;
        anchor_out[2] = width;
        anchor_out[3] = height;

        // Set confidence to 1.
        anchor_out[4] = 1.f;

        // One-hot encoding of the class label.
        std::fill(anchor_out + 5, anchor_out + label_size, 0.f);
        anchor_out[5 + annotation.identifier] = 1.f;

        // Advance the output iterator to the next anchor.
        anchor_out += label_size;
      }
    }
  }
}

std::vector<image_annotation> convert_yolo_to_annotations(
    const float_array& yolo_map,
    const std::vector<std::pair<float, float>>& anchor_boxes,
    float min_confidence) {

  ASSERT_EQ(yolo_map.dim(), 3);
  const size_t* const shape = yolo_map.shape();
  const size_t output_height = shape[0];
  const size_t output_width = shape[1];
  const size_t num_channels = shape[2];

  ASSERT_GT(anchor_boxes.size(), 0);
  ASSERT_EQ(num_channels % anchor_boxes.size(), 0);
  const size_t num_predictions = num_channels / anchor_boxes.size();

  ASSERT_GT(num_predictions, 5);
  const size_t num_classes = num_predictions - 5;

  std::vector<image_annotation> result;
  std::vector<float> class_scores(num_classes);  // Scratch buffer

  // Iterate over each prediction (x/y/w/h/conf + one-hot encoding of class),
  // for each anchor box, for each cell of the output grid.
  const size_t row_stride = output_width * num_channels;
  for (size_t output_y = 0; output_y < output_height; ++output_y) {

    // For this output grid row, iterate over each cell.
    const float* output_row_base = yolo_map.data() + output_y * row_stride;
    for (size_t output_x = 0; output_x < output_width; ++output_x) {

      // For this output grid cell, iterate over each anchor box.
      const float* output_cell_base = output_row_base + output_x * num_channels;
      for (size_t b = 0; b < anchor_boxes.size(); ++b) {

        // Extract the prediction for this anchor and output grid cell.
        const float* prediction_base = output_cell_base + b * num_predictions;
        const float raw_x = prediction_base[0];
        const float raw_y = prediction_base[1];
        const float raw_w = prediction_base[2];
        const float raw_h = prediction_base[3];
        const float raw_conf = prediction_base[4];
        const float* one_hot_base = prediction_base + 5;

        // Convert from raw output to bounding box (in normalized coordinates)
        const float anchor_w = anchor_boxes[b].first;
        const float anchor_h = anchor_boxes[b].second;
        const float x = (output_x + sigmoid(raw_x)) / output_width;
        const float y = (output_y + sigmoid(raw_y)) / output_height;
        const float w = std::exp(raw_w) * anchor_w / output_width;
        const float h = std::exp(raw_h) * anchor_h / output_height;

        // Convert overall object confidence and conditional class confidences.
        const float conf = sigmoid(raw_conf);
        std::copy(one_hot_base, one_hot_base + num_classes,
                  class_scores.begin());
        apply_softmax(&class_scores);

        // Add to our results any predictions meeting our confidence threshold.
        for (size_t c = 0; c < num_classes; ++c) {

          const float class_conf = conf * class_scores[c];
          if (class_conf >= min_confidence) {

            image_annotation ann;
            ann.identifier = static_cast<int>(c);
            ann.confidence = class_conf;
            ann.bounding_box.x = x - w/2;
            ann.bounding_box.y = y - h/2;
            ann.bounding_box.width = w;
            ann.bounding_box.height = h;
            result.push_back(std::move(ann));
          }
        }
      }
    }
  }

  return result;
}

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

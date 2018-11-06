#include <unity/toolkits/object_detection/od_data_iterator.hpp>

#include <algorithm>
#include <cmath>

#include <image/io.hpp>

namespace turi {
namespace object_detection {

namespace {

using neural_net::image_annotation;
using neural_net::image_box;
using neural_net::labeled_image;
using neural_net::shared_float_array;

flex_image get_image(const flexible_type& image_feature) {
  if (image_feature.get_type() == flex_type_enum::STRING) {
    return read_image(image_feature, /* format_hint */ "");
  } else {
    return image_feature;
  }
}

std::vector<image_annotation> parse_annotations(
    const flex_list& flex_annotations,
    size_t image_width, size_t image_height,
    const std::unordered_map<std::string, int>& class_to_index_map) {

  std::vector<image_annotation> result;
  result.reserve(flex_annotations.size());

  for (const flexible_type& flex_annotation : flex_annotations) {
    image_annotation annotation;

    // Scan through the flexible_type representation, populating each field.
    bool has_label = false;
    bool has_x = false;
    bool has_y = false;
    for (const auto& kv : flex_annotation.get<flex_dict>()) {

      const flex_string& key = kv.first.get<flex_string>();

      if (key == "label") {

        annotation.identifier =
            class_to_index_map.at(kv.second.get<flex_string>());
        has_label = true;

      } else if (key == "coordinates") {

        // Scan through the nested "coordinates" keys, populating the bounding
        // box.
        const flex_dict& coordinates = kv.second.get<flex_dict>();
        for (const auto& coord_kv : coordinates) {

          const flex_string& coord_key = coord_kv.first.get<flex_string>();
          float coord_val = coord_kv.second;

          if (coord_key == "x") {

            annotation.bounding_box.x = coord_val;
            has_x = true;

          } else if (coord_key == "y") {

            annotation.bounding_box.y = coord_val;
            has_y = true;

          } if (coord_key == "width") {

            annotation.bounding_box.width = coord_val;

          } if (coord_key == "height") {

            annotation.bounding_box.height = coord_val;
          }
        }
      }
    }

    // Verify that all the fields were populated.
    // TODO: Validate the dictionary keys in compute_properties. Let downstream
    // code worry about semantics such as non-empty boxes. Then the number of
    // instances we report will actually equal the number of image_annotation
    // values.
    if (has_label && has_x && has_y && annotation.bounding_box.area() > 0.f) {

      // Use x and y fields to store upper-left corner, not center.
      annotation.bounding_box.x -= annotation.bounding_box.width / 2.f;
      annotation.bounding_box.y -= annotation.bounding_box.height / 2.f;

      // Translate to normalized coordinates.
      annotation.bounding_box.normalize(image_width, image_height);

      // Add this annotation if we still have a valid bounding box.
      if (annotation.bounding_box.area() > 0.f) {

        annotation.confidence = 1.0f;
        result.push_back(std::move(annotation));
      }
    }
  }

  return result;
}

}  // namespace

// static
void data_iterator::convert_annotations_to_yolo(
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

simple_data_iterator::annotation_properties
simple_data_iterator::compute_properties(const gl_sarray& annotations) {

  annotation_properties result;

  // Construct an SFrame with one row per bounding box.
  gl_sframe instances;
  if (annotations.dtype() == flex_type_enum::LIST) {
    gl_sframe unstacked_instances({{"annotations", annotations}});
    instances = unstacked_instances.stack("annotations", "bbox",
                                          /* drop_na */ true);
  } else {
    instances["bbox"] = annotations;
  }

  // Extract the label for each bounding box.
  instances = instances.unpack("bbox", /* column_name_prefix */ "",
                               { flex_type_enum::STRING },
                               /* na_value */ FLEX_UNDEFINED, { "label" });

  // Determine the list of unique class labels and construct the class-to-index
  // map.
  gl_sarray classes = instances["label"].unique().sort();
  result.classes.reserve(classes.size());
  int i = 0;
  for (const flexible_type& label : classes.range_iterator()) {
    result.classes.push_back(label);
    result.class_to_index_map[label] = i++;
  }

  // Record the number of labeled bounding boxes.
  result.num_instances = instances.size();

  return result;
}

simple_data_iterator::simple_data_iterator(const parameters& params)

    // Reduce SFrame to the two columns we care about.
  : data_(params.data[ { params.annotations_column_name,
                         params.image_column_name        } ] ),

    // Determine which column is which within each (ordered) row.
    annotations_index_(data_.column_index(params.annotations_column_name)),
    image_index_(data_.column_index(params.image_column_name)),

    // Identify the class labels and other annotation properties.
    annotation_properties_(
        compute_properties(data_[params.annotations_column_name])),

    // Start an iteration through the entire SFrame.
    range_iterator_(data_.range_iterator()),
    next_row_(range_iterator_.begin())
{}

std::vector<labeled_image> simple_data_iterator::next_batch(size_t batch_size) {

  // For now, only return empty if we literally have no data at all.
  if (data_.empty()) return {};

  std::vector<std::pair<flexible_type, flexible_type>> raw_batch;
  raw_batch.reserve(batch_size);
  while (raw_batch.size() < batch_size) {

    const sframe_rows::row& row = *next_row_;
    raw_batch.emplace_back(row[image_index_], row[annotations_index_]);

    if (++next_row_ == range_iterator_.end()) {

      // TODO: Shuffle if desired.
      range_iterator_ = data_.range_iterator();
      next_row_ = range_iterator_.begin();
    }
  }

  std::vector<labeled_image> result(batch_size);
  for (size_t i = 0; i < batch_size; ++i) {

    // Reads the undecoded image data from disk, if necessary.
    // TODO: Investigate parallelizing this file I/O.
    result[i].image = get_image(raw_batch[i].first);

    result[i].annotations = parse_annotations(
        raw_batch[i].second, result[i].image.m_width, result[i].image.m_height,
        annotation_properties_.class_to_index_map);
  }

  return result;
}

}  // object_detection
}  // turi

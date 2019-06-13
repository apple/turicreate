#include <unity/toolkits/object_detection/od_data_iterator.hpp>

#include <algorithm>
#include <cmath>

#include <image/io.hpp>
#include <logger/logger.hpp>
#include <unity/lib/image_util.hpp>

namespace turi {
namespace object_detection {

namespace {

using neural_net::image_annotation;
using neural_net::labeled_image;
using neural_net::shared_float_array;

flex_image get_image(const flexible_type& image_feature) {
  if (image_feature.get_type() == flex_type_enum::STRING) {
    return read_image(image_feature, /* format_hint */ "");
  } else {
    return image_feature;
  }
}

gl_sframe get_data(const data_iterator::parameters& params) {

  gl_sarray annotations = params.data[params.annotations_column_name];
  gl_sarray images = params.data[params.image_column_name];

  if (images.dtype() == flex_type_enum::IMAGE) {

    // Ensure that all images are (losslessly) compressed to minimize the I/O
    // pain, especially when shuffling.
    images = images.apply(image_util::encode_image, flex_type_enum::IMAGE);
  }

  gl_sframe result({ { params.annotations_column_name, annotations },
                     { params.image_column_name,       images      }  });

  if (!params.predictions_column_name.empty()) {
    result[params.predictions_column_name] =
        params.data[params.predictions_column_name];
  }

  return result;
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

simple_data_iterator::annotation_properties
simple_data_iterator::compute_properties(
    const gl_sarray& annotations,
    std::vector<std::string> expected_class_labels) {

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

  // Determine the list of unique class labels,
  gl_sarray classes = instances["label"].unique().sort();

  if (expected_class_labels.empty()) {

    // Infer the class-to-index map from the observed labels.
    result.classes.reserve(classes.size());
    int i = 0;
    for (const flexible_type& label : classes.range_iterator()) {
      result.classes.push_back(label);
      result.class_to_index_map[label] = i++;
    }
  } else {

    // Construct the class-to-index map from the expected labels.
    result.classes = std::move(expected_class_labels);
    int i = 0;
    for (const std::string& label : result.classes) {
      result.class_to_index_map[label] = i++;
    }

    // Use the map to verify that we only encountered expected labels.
    for (const flexible_type& ft : classes.range_iterator()) {
      std::string label(ft);  // Ensures correct overload resolution below.
      if (result.class_to_index_map.count(label) == 0) {
        log_and_throw("Annotations contained unexpected class label " + label);
      }
    }
  }

  // Record the number of labeled bounding boxes.
  result.num_instances = instances.size();

  return result;
}

simple_data_iterator::simple_data_iterator(const parameters& params)

    // Reduce SFrame to the two columns we care about.
  : data_(get_data(params)),

    // Determine which column is which within each (ordered) row.
    annotations_index_(data_.column_index(params.annotations_column_name)),
    predictions_index_(params.predictions_column_name.empty()
                       ? -1
                       : data_.column_index(params.predictions_column_name)),
    image_index_(data_.column_index(params.image_column_name)),

    // Whether to traverse the SFrame more than once, and whether to shuffle.
    repeat_(params.repeat),
    shuffle_(params.shuffle),

    // Identify/verify the class labels and other annotation properties.
    annotation_properties_(
        compute_properties(data_[params.annotations_column_name],
                           params.class_labels)),

    // Start an iteration through the entire SFrame.
    range_iterator_(data_.range_iterator()),
    next_row_(range_iterator_.begin())
{}

std::vector<labeled_image> simple_data_iterator::next_batch(size_t batch_size) {

  // Accumulate batch_size tuples: (image, annotations, predictions).
  std::vector<std::tuple<flexible_type,flexible_type,flexible_type>> raw_batch;
  raw_batch.reserve(batch_size);
  while (raw_batch.size() < batch_size && next_row_ != range_iterator_.end()) {

    const sframe_rows::row& row = *next_row_;
    flexible_type preds = FLEX_UNDEFINED;
    if (predictions_index_ >= 0) {
      preds = row[predictions_index_];
    }
    raw_batch.emplace_back(row[image_index_], row[annotations_index_], preds);

    if (++next_row_ == range_iterator_.end() && repeat_) {

      if (shuffle_) {
        // Shuffle the data.
        // TODO: This heavyweight shuffle operation introduces spikes into the
        // wall-clock time of this function. SFrame should either provide an
        // optimized implementation, or we should implement an approach that
        // amortizes the cost across calls.
        // TODO: Avoid traversing the data in gl_sframe::apply by instead
        // generating a gl_sarray from a sequence of the desired length and
        // hashing each element.
        auto rng = [](const sframe_rows::row&) {
          return random::rand();
        };
        data_.add_column(data_.apply(rng, flex_type_enum::INTEGER),
                         "_random_order");
        data_ = data_.sort("_random_order");
        data_.remove_column("_random_order");
      }

      // Reset iteration.
      range_iterator_ = data_.range_iterator();
      next_row_ = range_iterator_.begin();
    }
  }

  std::vector<labeled_image> result(raw_batch.size());
  for (size_t i = 0; i < raw_batch.size(); ++i) {
    flexible_type raw_image, raw_annotations, raw_predictions;
    std::tie(raw_image, raw_annotations, raw_predictions) = raw_batch[i];

    // Reads the undecoded image data from disk, if necessary.
    // TODO: Investigate parallelizing this file I/O.
    result[i].image = get_image(raw_image);

    result[i].annotations = parse_annotations(
        raw_annotations, result[i].image.m_width, result[i].image.m_height,
        annotation_properties_.class_to_index_map);

    if (raw_predictions != FLEX_UNDEFINED) {
      result[i].predictions = parse_annotations(
          raw_predictions, result[i].image.m_width, result[i].image.m_height,
          annotation_properties_.class_to_index_map);
    }
  }

  return result;
}

}  // object_detection
}  // turi

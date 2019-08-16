/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

#include <boost/gil/gil_all.hpp>

#include <Eigen/Core>
#include <Eigen/Dense>

#include <core/data/image/image_util_impl.hpp>

#include <model_server/lib/image_util.hpp>
#include <toolkits/object_detection/object_detector.hpp>
#include <toolkits/object_detection/one_shot_object_detection/one_shot_object_detector.hpp>
#include <toolkits/object_detection/one_shot_object_detection/util/color_convert.hpp>
#include <toolkits/object_detection/one_shot_object_detection/util/mapping_function.hpp>
#include <toolkits/object_detection/one_shot_object_detection/util/parameter_sampler.hpp>
#include <toolkits/object_detection/one_shot_object_detection/util/superposition.hpp>

namespace turi {
namespace one_shot_object_detection {
namespace data_augmentation {

flex_dict build_annotation(ParameterSampler &parameter_sampler,
                           std::string label, size_t object_width,
                           size_t object_height, long seed) {
  parameter_sampler.sample(seed);

  size_t original_top_left_x = 0;
  size_t original_top_left_y = 0;
  size_t original_top_right_x = object_width;
  size_t original_top_right_y = 0;
  size_t original_bottom_left_x = 0;
  size_t original_bottom_left_y = object_height;
  size_t original_bottom_right_x = object_width;
  size_t original_bottom_right_y = object_height;

  Eigen::Vector3f top_left_corner(3), top_right_corner(3);
  Eigen::Vector3f bottom_left_corner(3), bottom_right_corner(3);
  top_left_corner << original_top_left_x, original_top_left_y, 1;
  top_right_corner << original_top_right_x, original_top_right_y, 1;
  bottom_left_corner << original_bottom_left_x, original_bottom_left_y, 1;
  bottom_right_corner << original_bottom_right_x, original_bottom_right_y, 1;

  auto normalize = [](Eigen::Vector3f corner) {
    corner[0] /= corner[2];
    corner[1] /= corner[2];
    corner[2] = 1.0;
    return corner;
  };

  Eigen::Matrix<float, 3, 3> mat = parameter_sampler.get_transform();

  const std::vector<Eigen::Vector3f> warped_corners = {
      normalize(mat * top_left_corner), normalize(mat * top_right_corner),
      normalize(mat * bottom_left_corner),
      normalize(mat * bottom_right_corner)};
  parameter_sampler.set_warped_corners(warped_corners);

  float min_x = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::min();
  float min_y = std::numeric_limits<float>::max();
  float max_y = std::numeric_limits<float>::min();
  for (const auto &corner : warped_corners) {
    min_x = std::min(min_x, corner[0]);
    max_x = std::max(max_x, corner[0]);
    min_y = std::min(min_y, corner[1]);
    max_y = std::max(max_y, corner[1]);
  }
  float center_x = (min_x + max_x) / 2;
  float center_y = (min_y + max_y) / 2;
  float bounding_box_width = max_x - min_x;
  float bounding_box_height = max_y - min_y;

  flex_dict coordinates = {std::make_pair("x", center_x),
                           std::make_pair("y", center_y),
                           std::make_pair("width", bounding_box_width),
                           std::make_pair("height", bounding_box_height)};
  flex_dict annotation = {std::make_pair("coordinates", coordinates),
                          std::make_pair("label", label)};
  return annotation;
}

static std::map<std::string, size_t> generate_column_index_map(
    const std::vector<std::string> &column_names) {
  std::map<std::string, size_t> index_map;
  for (size_t k = 0; k < column_names.size(); ++k) {
    index_map[column_names[k]] = k;
  }
  return index_map;
}

flex_image create_rgba_flex_image(const flex_image &object_input) {
  if (!(object_input.is_decoded())) {
    log_and_throw("Input object starter image is not decoded.");
  }
  flex_image rgba_flex_image =
      image_util::resize_image(object_input, object_input.m_width,
                               object_input.m_height, 4, true)
          .to<flex_image>();
  if (!(rgba_flex_image.is_decoded())) {
    log_and_throw("Resized object starter image is not decoded.");
  }
  if (rgba_flex_image.m_channels != 4) {
    log_and_throw("Object image is not resized to be 4.");
  }
  return rgba_flex_image;
}

std::pair<flex_image, flex_dict>
create_synthetic_image_from_background_and_starter(const flex_image &starter,
                                                   const flex_image &background,
                                                   std::string &label,
                                                   size_t seed,
                                                   size_t row_number) {
  ParameterSampler parameter_sampler =
      ParameterSampler(background.m_width, background.m_height,
                       (background.m_width - starter.m_width) / 2,
                       (background.m_height - starter.m_height) / 2);

  // construct annotation dictionary from parameters
  flex_dict annotation =
      build_annotation(parameter_sampler, label, starter.m_width,
                       starter.m_height, seed + row_number);

  if (background.get_image_data() == nullptr) {
    log_and_throw("Background image has null image data.");
  }
  if (!(starter.is_decoded())) {
    log_and_throw("Starter image is not decoded into raw format.");
  }
  if (!(background.is_decoded())) {
    log_and_throw("Background image is not decoded into raw format.");
  }

  flex_image rgba_flex_image = create_rgba_flex_image(starter);
  boost::gil::rgba8_image_t::const_view_t starter_image_view =
      interleaved_view(rgba_flex_image.m_width, rgba_flex_image.m_height,
                       reinterpret_cast<const boost::gil::rgba8_pixel_t *>(
                           rgba_flex_image.get_image_data()),
                       rgba_flex_image.m_channels *
                           rgba_flex_image.m_width  // row length in bytes
      );

  boost::gil::rgb8_image_t::const_view_t background_view = interleaved_view(
      background.m_width, background.m_height,
      reinterpret_cast<const boost::gil::rgb8_pixel_t *>(
          background.get_image_data()),
      background.m_channels * background.m_width  // row length in bytes
  );
  flex_image synthetic_image = create_synthetic_image(
      starter_image_view, background_view, parameter_sampler);
  encode_image_inplace(synthetic_image);
  return std::make_pair(synthetic_image, annotation);
}

gl_sframe augment_data(const gl_sframe &data,
                       const std::string &image_column_name,
                       const std::string &target_column_name,
                       const gl_sarray &backgrounds, long long seed,
                       bool verbose) {
  size_t backgrounds_size = backgrounds.size();
  size_t total_augmented_rows = data.size() * backgrounds_size;
  table_printer table(
      {{"Images Augmented", 0}, {"Elapsed Time", 0}, {"Percent Complete", 0}});
  if (verbose) {
    logprogress_stream << "Augmenting input images using " << backgrounds_size
                       << " background images." << std::endl;
    table.print_header();
  }
  std::vector<std::string> output_column_names = {image_column_name,
                                                  "annotation"};
  std::vector<flex_type_enum> output_column_types = {flex_type_enum::IMAGE,
                                                     flex_type_enum::DICT};
  gl_sframe_writer output_writer(output_column_names, output_column_types);
  auto column_index_map = generate_column_index_map(data.column_names());
  size_t image_column_index = column_index_map[image_column_name];
  size_t target_column_index = column_index_map[target_column_name];
  if (data[image_column_name].dtype() != flex_type_enum::IMAGE) {
    log_and_throw("Image column name is not of type Image.");
  }
  if (data[target_column_name].dtype() != flex_type_enum::STRING) {
    log_and_throw("Target column name is not of type String.");
  }
  size_t nsegments = output_writer.num_segments();
  std::atomic<size_t> augmented_counter(0);
  gl_sframe decompressed_data = gl_sframe(data);
  decompressed_data[image_column_name] =
      decompressed_data[image_column_name].apply(
          [&](flexible_type starter_ft) {
            flex_image starter =
                image_util::decode_image(starter_ft.to<flex_image>());
            return starter;
          },
          flex_type_enum::IMAGE);

  /* @TODO: Split all backgrounds into as many chunks as there are cores
   * available (= nsegments), and create augmented images in parallel.
   * Replacing the `for` with a `parallel_for` fails the export_coreml unit test
   * with an EXC_BAD_ACCESS in the function call to boost::gil::resample_pixels
   */
  for (size_t segment_id = 0; segment_id < nsegments; segment_id++) {
    size_t segment_start = (segment_id * backgrounds.size()) / nsegments;
    size_t segment_end = ((segment_id + 1) * backgrounds.size()) / nsegments;
    size_t row_number = segment_start;
    for (const auto &background_ft :
         backgrounds.range_iterator(segment_start, segment_end)) {
      row_number++;
      flex_image flex_background =
          image_util::decode_image(background_ft.to<flex_image>());
      for (const auto &row : decompressed_data.range_iterator()) {
        // go through all the starter images and create augmented images for
        // all starter images and the respective chunk of background images
        const flex_image &object = row[image_column_index].get<flex_image>();
        std::string label = row[target_column_index].to<flex_string>();
        std::pair<flex_image, flex_dict> synthetic_row =
            create_synthetic_image_from_background_and_starter(
                object, flex_background, label, seed, row_number);
        flex_image synthetic_image = synthetic_row.first;
        flex_dict annotation = synthetic_row.second;
        // write the synthetically generated image and the constructed
        // annotation to output SFrame.
        output_writer.write({std::move(synthetic_image), std::move(annotation)},
                            segment_id);
        size_t augmented_rows_completed = 1 + augmented_counter.fetch_add(1);
        if (verbose) {
          std::ostringstream d;
          // For pretty printing, floor percent done
          // resolution to the nearest .25% interval.  Do this by multiplying by
          // 400, then do integer division by the total size, then float divide
          // by 4.0
          if (augmented_rows_completed % 100 == 0) {
            d << augmented_rows_completed * 400 / total_augmented_rows / 4.0
              << '%';
            table.print_progress_row(augmented_rows_completed,
                                     augmented_rows_completed, progress_time(),
                                     d.str());
          }
        }
      }
    }
  }
  if (verbose) {
    table.print_footer();
  }

  gl_sframe synthetic_sframe = output_writer.close();
  return synthetic_sframe;
}

}  // namespace data_augmentation

one_shot_object_detector::one_shot_object_detector() {
  model_.reset(new turi::object_detection::object_detector());
}

gl_sframe one_shot_object_detector::augment(
    const gl_sframe &data, const std::string &image_column_name,
    const std::string &target_column_name, const gl_sarray &backgrounds,
    std::map<std::string, flexible_type> &options) {
  // TODO: Automatically infer the image column name, or throw error if you
  // can't This should just happen on the Python side.
  gl_sframe augmented_data = data_augmentation::augment_data(
      data, image_column_name, target_column_name, backgrounds, options["seed"],
      options["verbose"]);
  // TODO: Call object_detector::train from here once we incorporate mxnet into
  // the C++ Object Detector.
  return augmented_data;
}

}  // namespace one_shot_object_detection
}  // namespace turi

/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_OD_DATA_ITERATOR_HPP_
#define TURI_OBJECT_DETECTION_OD_DATA_ITERATOR_HPP_

#include <string>
#include <unordered_map>
#include <vector>

#include <unity/lib/gl_sframe.hpp>
#include <unity/toolkits/neural_net/float_array.hpp>
#include <unity/toolkits/neural_net/image_augmentation.hpp>

namespace turi {
namespace object_detection {

/**
 * Pure virtual interface for classes that produce batches of data
 * (pre-augmentation) from a raw SFrame.
 */
class data_iterator {
public:

  /**
   * Writes a list of image_annotation values into an output float buffer.
   *
   * \param annotations The list of annotations (for one image) to write
   * \param output_height The height of the YOLO output grid
   * \param output_width The width of the YOLO output grid
   * \param num_anchors The number of YOLO anchors
   * \param num_classes The number of classes in the output one-hot encoding
   * \param out Address to a float buffer of size output_height * output_width *
   *            num_anchors * (5 + num_classes)
   * \todo Add a mutable_float_array or shared_float_buffer type for functions
   *       like this one to write into.
   * \todo This strictly speaking doesn't belong in this data iterator type but
   *       probably doesn't warrant its own file yet (and would be nice not to
   *       bury in object_detector.cpp).
   */
  static void convert_annotations_to_yolo(
      const std::vector<neural_net::image_annotation>& annotations,
      size_t output_height, size_t output_width, size_t num_anchors,
      size_t num_classes, float* out);

  /**
   * Defines the inputs to a data_iterator factory function.
   */
  struct parameters {

    /** The SFrame to traverse */
    gl_sframe data;

    /**
     * The name of the column containing the annotations.
     *
     * The values must either be dictionaries containing an annotation, or a
     * list of such dictionaries. An annotation dictionary has a "label" key
     * whose value is a string, and a "coordinates" key whose value is another
     * dictionary containing "x", "y", "width", and "height", describing the
     * position of the center and the size of the bounding box (in the image's
     * coordinates, with the origin at the top left).
     */
    std::string annotations_column_name;

    /**
     * The name of the column containing the images.
     *
     * Each value is either an image or a path to an image file on disk.
     */
    std::string image_column_name;
  };

  virtual ~data_iterator() = default;

  /**
   * Returns a vector whose size is equal to `batch_size`.
   *
   * Note that the iterator will cycle indefinitely through the SFrame over and
   * over. The x,y coordinates in the returned annotations indicate the
   * upper-left corner of the bounding box.
   */
  virtual std::vector<neural_net::labeled_image>
      next_batch(size_t batch_size) = 0;

  /**
   * Returns a sorted list of the unique "label" values found in the
   * annotations.
   */
  virtual const std::vector<std::string>& class_labels() const = 0;

  /**
   * Returns the number of annotations (bounding boxes) found across all rows.
   */
  virtual size_t num_instances() const = 0;
};

/**
 * Concrete data_iterator implementation that doesn't attempt any
 * parallelization or background I/O.
 *
 * \todo This classs should become an abstract_data_iterator base class with
 *       override points for dispatching work to other threads.
 */
class simple_data_iterator: public data_iterator {
public:

  simple_data_iterator(const parameters& params);

  // Not copyable or movable.
  simple_data_iterator(const simple_data_iterator&) = delete;
  simple_data_iterator& operator=(const simple_data_iterator&) = delete;

  std::vector<neural_net::labeled_image> next_batch(size_t batch_size) override;

  const std::vector<std::string>& class_labels() const override {
    return annotation_properties_.classes;
  }

  size_t num_instances() const override {
    return annotation_properties_.num_instances;
  }

private:
  struct annotation_properties {
    std::vector<std::string> classes;
    std::unordered_map<std::string, int> class_to_index_map;
    size_t num_instances;
  };

  annotation_properties compute_properties(const gl_sarray& annotations);

  const gl_sframe data_;
  const size_t annotations_index_;
  const size_t image_index_;

  const annotation_properties annotation_properties_;

  gl_sframe_range range_iterator_;
  gl_sframe_range::iterator next_row_;
};

}  // object_detection
}  // turi

#endif  // TURI_OBJECT_DETECTION_OD_DATA_ITERATOR_HPP_

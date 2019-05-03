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
     * Optional name of a column containing predictions.
     *
     * If not empty, then the iterator will parse and yield a secondary stream
     * of bounding boxes, intended for use in evaluating existing predictions.
     */
    std::string predictions_column_name;

    /**
     * The name of the column containing the images.
     *
     * Each value is either an image or a path to an image file on disk.
     */
    std::string image_column_name;

    /**
     * The expected class labels, indexed by identifier.
     *
     * If empty, then the labels will be inferred from the data. If non-empty,
     * an exception will be thrown upon encountering an unexpected label.
     *
     * \TODO: This should be a flex_list to accomodate integer labels!
     */
    std::vector<std::string> class_labels;

    /**
     * Whether to traverse the data more than once.
     */
    bool repeat = true;

    /** Whether to shuffle the data on subsequent traversals. */
    bool shuffle = true;
  };

  virtual ~data_iterator() = default;

  /**
   * Returns a vector whose size is equal to `batch_size`.
   *
   * If `repeat` was set in the parameters, then the iterator will cycle
   * indefinitely through the SFrame over and over. Otherwise, the last
   * non-empty batch may contain fewer than `batch_size` elements, and every
   * batch after that will be empty.
   *
   * The x,y coordinates in the returned annotations indicate the upper-left
   * corner of the bounding box.
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

  annotation_properties compute_properties(
      const gl_sarray& annotations,
      std::vector<std::string> expected_class_labels);

  gl_sframe data_;
  const size_t annotations_index_;
  const ssize_t predictions_index_;
  const size_t image_index_;
  const bool repeat_;
  const bool shuffle_;

  const annotation_properties annotation_properties_;

  gl_sframe_range range_iterator_;
  gl_sframe_range::iterator next_row_;
};

}  // object_detection
}  // turi

#endif  // TURI_OBJECT_DETECTION_OD_DATA_ITERATOR_HPP_

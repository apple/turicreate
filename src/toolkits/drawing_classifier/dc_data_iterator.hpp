/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_DRAWING_CLASSIFICATION_DC_DATA_ITERATOR_HPP_
#define TURI_DRAWING_CLASSIFICATION_DC_DATA_ITERATOR_HPP_

#include <random>
#include <string>
#include <vector>

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <ml/neural_net/float_array.hpp>

namespace turi {
namespace drawing_classifier {

/**
 * Pure virtual interface for classes that produce batches of data
 * (pre-augmentation) from a raw SFrame.
 * \TODO Factor out the shared structure for data iterators
 *        with the other iterators!
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
     * The name of the column containing the target variable.
     *
     * If empty, then the output will not contain labels.
     */
    std::string target_column_name;

    /** The name of the feature column. */
    std::string feature_column_name{"feature"};

    /** The name of the predictions column. */
    std::string predictions_column_name;

    /**
     * The expected class labels, indexed by identifier.
     *
     * If empty, then the labels will be inferred from the data. If non-empty,
     * an exception will be thrown upon encountering an unexpected label.
     */
    flex_list class_labels;

    /** Whether this is training data or not. */
    bool is_train = true;

    /** Whether to traverse the data more than once. */
    bool repeat = true;

    /** Whether to shuffle the data on subsequent traversals. */
    bool shuffle = true;

    /** Determines results of shuffle operations if enabled. */
    int random_seed = 0;

    // normalization factor for input data
    float scale_factor = 1 / 255.f;
  };

  /** Defines the output of a data_iterator. */
  struct batch {
    /* Number of examples in batch */
    size_t num_samples = 256;

    /**
     * An array with shape: (requested_batch_size, 28, 28 1)
     *
     * Each row is an image.
     */
    neural_net::shared_float_array drawings;

    /**
     * An array with shape: (requested_batch_size, 1)
     *
     * Each row is the target.
     */
    neural_net::shared_float_array targets;

    /**
     * An array with shape: (requested_batch_size, 1)
     *
     * Each row is the weight associated with the target.
     */
    neural_net::shared_float_array weights;

    /**
     * An array with shape: (requested_batch_size, 1)
     *
     * Each row is the prediction.
     */
    neural_net::shared_float_array predictions;
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
   */
  virtual batch next_batch(size_t batch_size) = 0;

  /**
   * Returns true if and only if the next call to `next_batch` will return a
   * batch with size greater than 0.
   */
  virtual bool has_next_batch() = 0;

  /** Begins a fresh traversal of the dataset. */
  virtual void reset() = 0;

  /**
   * Returns a sorted list of the unique "label" values found in the
   * target.
   */
  virtual const flex_list& class_labels() const = 0;

};

/**
 * Concrete data_iterator implementation that doesn't attempt any
 * parallelization or background I/O.
 *
 * \todo This classs should become an abstract_data_iterator base class with
 *       override points for dispatching work to other threads.
 */
class simple_data_iterator : public data_iterator {
 public:
  simple_data_iterator(const parameters& params);

  // Not copyable or movable.
  simple_data_iterator(const simple_data_iterator&) = delete;
  simple_data_iterator& operator=(const simple_data_iterator&) = delete;

  batch next_batch(size_t batch_size) override;

  bool has_next_batch() override;

  void reset() override;

  const flex_list& class_labels() const override {
    return target_properties_.classes;
  }

 private:
  struct target_properties {
    flex_list classes;
  };

  target_properties compute_properties(
      const gl_sframe& data, const std::string& target_column_name,
      const flex_list& expected_class_labels);

  gl_sframe data_;
  const int64_t target_index_;
  const int64_t predictions_index_; // -1 if not present
  const int64_t feature_index_;
  const bool repeat_;
  const bool shuffle_;
  const float scale_factor_ = 1 / 255.0f;

  const target_properties target_properties_;

  gl_sframe_range range_iterator_;
  gl_sframe_range::iterator next_row_;
  gl_sframe_range::iterator end_of_rows_;

  std::default_random_engine random_engine_;
};

}  // namespace drawing_classifier
}  // namespace turi

#endif  // TURI_DRAWING_CLASSIFICATION_DC_DATA_ITERATOR_HPP_

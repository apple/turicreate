/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_DRAWING_CLASSIFICATION_DC_DATA_ITERATOR_HPP_
#define TURI_DRAWING_CLASSIFICATION_DC_DATA_ITERATOR_HPP_

#include <random>
#include <string>
#include <vector>

#include <core/data/sframe/gl_sframe.hpp>
#include <ml/neural_net/float_array.hpp>

namespace turi {
namespace drawing_classifier {

/**
 * Pure virtual interface for classes that produce batches of activity
 * classification data from a raw SFrame.
 */
class data_iterator {
public:

  /** Defines the inputs to a data_iterator factory function. */
  struct parameters {

    /** The SFrame to traverse */
    gl_sframe data;

    /**
     * The name of the column containing the target variable.
     *
     * If empty, then the output will not contain labels or weights.
     */
    std::string target_column_name;

    /** The name of the column containing the feature column. */
    std::string feature_column_name;

    /**
     * The expected class labels, indexed by identifier.
     *
     * If empty, then the labels will be inferred from the data. If non-empty,
     * an exception will be thrown upon encountering an unexpected label.
     */
    flex_list class_labels;

    /**  Set to true, when the data is used for training. */
    bool is_train = false;

    /** Determines results of data augmentation if enabled. */
    int random_seed = 0;
  };

  /** Defines the output of a data_iterator. */
  struct batch {

    /** Defines the metadata associated with each chunk. */
    struct chunk_info {

      /** Number of samples (rows from the raw SFrame) comprising the chunk. */
      size_t num_samples;
    };

    /**
     * TODO: Add explanatory comment 
     */
    neural_net::shared_float_array features;

    /**
     * TODO: Add explanatory comment 
     */
    neural_net::shared_float_array labels;

    /**
     * TODO: Add explanatory comment 
     */
    neural_net::shared_float_array weights;


    /**
     * TODO: Add explanatory comment 
     */
    neural_net::shared_float_array labels_per_row;

    /**
     * TODO: Add explanatory comment 
     */
    std::vector<chunk_info> batch_info;

  };

  virtual ~data_iterator();

  virtual const flex_list& feature_names() const = 0;
  virtual const flex_list& class_labels() const = 0;
  virtual flex_type_enum session_id_type() const = 0;

  /**
   * Returns true if and only if the next call to `next_batch` will return a
   * batch with size greater than 0.
   */
  virtual bool has_next_batch() const = 0;

  /**
   * Returns a batch containing float arrays with the indicated batch size.
   *
   * Eventually returns a batch with size smaller than the requested size,
   * indicating that the entire dataset has been traversed. All subsequent
   * calls will return an empty (all padding) batch, until reset.
   */
  virtual batch next_batch(size_t batch_size) = 0;

  /** Begins a fresh traversal of the dataset. */
  virtual void reset() = 0;
};

/**
 * Concrete data_iterator implementation that doesn't attempt any
 * parallelization or background I/O.
 */
class simple_data_iterator: public data_iterator {
public:

  simple_data_iterator(const parameters& params);

  // Not copyable or movable.
  simple_data_iterator(const simple_data_iterator&) = delete;
  simple_data_iterator& operator=(const simple_data_iterator&) = delete;

  const flex_list& feature_names() const override;
  const flex_list& class_labels() const override;
  flex_type_enum session_id_type() const override;
  bool has_next_batch() const override;
  batch next_batch(size_t batch_size) override;
  void reset() override;

private:

  struct preprocessed_data {
    gl_sframe chunks;
    bool has_target = false;
    flex_list class_labels;
  };

  static preprocessed_data preprocess_data(const parameters& params);

  const preprocessed_data data_;
  
  gl_sframe_range range_iterator_;
  gl_sframe_range::iterator next_row_;
  gl_sframe_range::iterator end_of_rows_;
  size_t sample_in_row_ = 0;
  bool is_train_ = false;
  std::default_random_engine random_engine_;
};

}  // namespace drawing_classifier
}  // namespace turi

#endif  // TURI_DRAWING_CLASSIFICATION_DC_DATA_ITERATOR_HPP_

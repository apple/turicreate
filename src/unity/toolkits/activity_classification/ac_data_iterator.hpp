/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_ACTIVITY_CLASSIFICATION_AC_DATA_ITERATOR_HPP_
#define TURI_ACTIVITY_CLASSIFICATION_AC_DATA_ITERATOR_HPP_

#include <string>
#include <vector>

#include <unity/lib/gl_sframe.hpp>
#include <unity/toolkits/neural_net/float_array.hpp>

namespace turi {
namespace activity_classification {

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

    /** The name of the column containing the session ID. */
    std::string session_id_column_name;

    /** The names of the feature columns. */
    std::vector<std::string> feature_column_names;

    /**
     * Each group of this many consecutive samples from the same session are
     * assumed to have the same class label. 
     */
    size_t prediction_window = 100;

    /**
     * Each session is segmented into chunks of this many prediction windows.
     */
    size_t predictions_in_chunk = 20;

    /**
     * The expected class labels, indexed by identifier.
     *
     * If empty, then the labels will be inferred from the data. If non-empty,
     * an exception will be thrown upon encountering an unexpected label.
     */
    flex_list class_labels;

    /**  Generates verbose output when set to true. */
    bool verbose;
  };

  /** Defines the output of a data_iterator. */
  struct batch {

    /** Defines the metadata associated with each chunk. */
    struct chunk_info {

      /** The session ID from which the chunk was segmented. */
      flexible_type session_id;

      /** Number of samples (rows from the raw SFrame) comprising the chunk. */
      size_t num_samples;
    };

    /**
     * An array with shape: (requested_batch_size, 
     * 1, prediction_window * predictions_in_chunk, num_feature_columns)
     *
     * Each row is a chunk of feature values from one session.
     */
    neural_net::shared_float_array features;

    /**
     * An array with shape: (requested_batch_size, 1, predictions_in_chunk, 1)
     *
     * Each row is the sequence of class label (indices) from one chunk.
     *
     * If no target was specified, then this value is default constructed.
     */
    neural_net::shared_float_array labels;

    /**
     * An array with shape: (requested_batch_size, 1, predictions_in_chunk, 1)
     *
     * Each row is a sequence of 0 or 1 values indicating whether the
     * corresponding label is padding (0) or refers to actual data (1).
     *
     * If no target was specified, then this value is default constructed.
     */
    neural_net::shared_float_array weights;

    /**
     * Metadata for each valid (non-padded) row in the batch.
     *
     * The size of this vector is at most requested_batch_size. The info at
     * index `i` describes the array at `features[i]`.
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
    size_t num_sessions = 0;
    flex_type_enum session_id_type = flex_type_enum::UNDEFINED;
    bool has_target = false;
    flex_list feature_names;
    flex_list class_labels;
  };

  static preprocessed_data preprocess_data(const parameters& params);

  const preprocessed_data data_;
  const size_t num_samples_per_prediction_;
  const size_t num_predictions_per_chunk_;

  gl_sframe_range range_iterator_;
  gl_sframe_range::iterator next_row_;
  gl_sframe_range::iterator end_of_rows_;
};

/**
 *  Convert SFrame to batch form, where each row contains a sequence of length
 *  predictions_in_chunk * prediction_window, and there is a single label per
 *  prediction window.
 *
 * \param[in] data                  Original data. Sframe containing one line per time sample.
 * \param[in] features              List of names of the columns containing the input features.
 * \param[in] session_id            Name of the column containing ids for each session in the dataset.
 *                                  A session is a single user time-series sequence.
 * \param[in] prediction_window     Number of time samples in every prediction window. A label is expected
 *                                  (for training), or predicted (in inference) every time a sequence of
 *                                  prediction_window samples have been collected.
 * \param[in] predictions_in_chunk  Each session is chunked into shorter sequences. This is the number of
 *                                  prediction windows desired in each chunk.
 * \param[in] target                Name of the coloumn containing the output labels. Empty string if None.
 *
 * \return                          SFrame with the data converted to batch form.
 */
EXPORT variant_map_type _activity_classifier_prepare_data(const gl_sframe &data,
                                                   const std::vector<std::string> &features,
                                                   const std::string &session_id,
                                                   const int &prediction_window,
                                                   const int &predictions_in_chunk,
                                                   const std::string &target);


// Same as above, with verbose=True
EXPORT variant_map_type _activity_classifier_prepare_data_verbose(const gl_sframe &data,
                                                   const std::vector<std::string> &features,
                                                   const std::string &session_id,
                                                   const int &prediction_window,
                                                   const int &predictions_in_chunk,
                                                   const std::string &target);

}  // namespace activity_classification
}  // namespace turi

#endif  // TURI_ACTIVITY_CLASSIFICATION_AC_DATA_ITERATOR_HPP_

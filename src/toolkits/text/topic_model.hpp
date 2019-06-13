/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TEXT_TOPICMODEL_H_
#define TURI_TEXT_TOPICMODEL_H_

// SFrame
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/sframe.hpp>

// Other
#include <core/storage/fileio/temp_files.hpp>
#include <iostream>

// Types
#include <model_server/lib/unity_base_types.hpp>
#include <core/util/hash_value.hpp>
#include <model_server/lib/flex_dict_view.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/metadata.hpp>

// Interfaces
#include <model_server/lib/extensions/ml_model.hpp>

// External
#include <Eigen/Core>
#include <core/export.hpp>
namespace turi {

namespace text {

/**
 * Class for learning topic models of text corpora.
 *
 * Typical use (as seen in cgs.cpp):
 *
 * 1) Create a topic model with a map of options:
 *
 * topic_model m = new topic_model(options);
 *
 * 2) Create an ml_data object where words have been assigned integers
 *    to faciliate indexing.
 *
 * ml_data d = m->create_ml_data_using_metadata(dataset);
 *
 * 3) Initialize the model so that we have the internal parameters needed
 *    for each of the words observed in the dataset.
 *
 * m->init();
 *
 * Note: Two other actions can be useful after initialization:
 *
 * set_topics: Loads a set of topics and vocabulary.
 * set_associations: Loads a set of word-topic assignments.
 *
 */
class EXPORT topic_model : public ml_model_base{

 public:

  typedef Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> count_matrix_type;
  typedef Eigen::Matrix<int, 1, Eigen::Dynamic, Eigen::RowMajor> count_vector_type;

  static constexpr size_t TOPIC_MODEL_VERSION = 1;

 protected:
  // Model options
  size_t num_topics;          /* < Number of topics to learn. */
  size_t vocab_size;          /* < Number of words in the vocabulary. */
  size_t num_words;           /* < Number of words in the corpus. */
  std::map<size_t, size_t> associations; /* < Fixed word-topic associations. */

  // Hyperparameters
  double alpha;               /* < Controls smoothing over topics. */
  double beta;                /* < Controls smoothing over words. */

  // Vocabulary lookup
  std::shared_ptr<v2::ml_metadata> metadata;

  // Statistics
  count_matrix_type word_topic_counts;      /* < Total count for each word. */

  // State
  bool is_initialized;        /* < Flag for whether model is ready. */
  bool option_info_set;

  // Validation data
  std::shared_ptr<sarray<flexible_type>> validation_train;
  std::shared_ptr<sarray<flexible_type>> validation_test;

  /**
   * Methods that must be implemented in a new nearest neighbors model
   * ---------------------------------------------------------------------------
   */
  public:

  /**
   * Clone objects to a topic_model class
   *
   * \returns A new model with the same things in it.
   *
   * \ref model_base for details.
   */
  virtual topic_model* topic_model_clone() = 0;

  /**
   * Set the model options. Use the option manager to set these options. The
   * option manager should throw errors if the options do not satisfy the option
   * manager's conditions.
   *
   * \param[in] opts Options to set
   */
  virtual void init_options(const std::map<std::string,flexible_type>& _opts) override = 0;


  /**
   * Gets the model version number
   */
  virtual size_t get_version() const override = 0;

  /**
   * Serialize the model object.
   */
  virtual void save_impl(turi::oarchive& oarc) const override = 0;

  /**
   * Load the model object.
   */
  virtual void load_version(turi::iarchive& iarc, size_t version) override = 0;

  /**
   * Create a topic model.
   */
  virtual void train(std::shared_ptr<sarray<flexible_type>> dataset, bool verbose) = 0;


  /**
   * Lists all the keys accessible in the "model" map.
   *
   * \returns List of keys in the model map.
   * \ref model_base for details.
   *
   */
  std::vector<std::string> list_fields();

  /**
   * Methods with meaningful default implementations.
   * -------------------------------------------------------------------------
   */
  public:

  /**
   * Helper function for creating the appropriate ml_data from an sarray of
   * documents.
   *
   * \param dataset An SArray (of dictionary type) containing document
   * data in bag of words format, where each element has words as keys
   * and the corresponding counts as values.
   */
  v2::ml_data create_ml_data_using_metadata(
      std::shared_ptr<sarray<flexible_type>> dataset);

  /**
   * Methods available to all topic_models.
   * ----------------------------------------------------------------------
   */

  /**
   * Load a set of associations comprising a (word, topic) pair that should
   * be considered fixed.
   *
   * \param associations An SFrame with two columns named 'word' and 'topic'.
   */
  void set_associations(const sframe& associations);

  /**
   * Remove current vocabulary and topics and load these instead.
   *
   * \param word_topic_prob An SArray of vector type, where each element
   * has size num_topics. The k'th element represents the probability of
   * the corresponding word in vocabulary under topic k.
   * \param vocabulary An SArray of string type containing the unique
   * words that should be loaded into the model. This must have the same
   * length as word_topic_prob.
   * \param weight The weight the model should give these probabilites
   * when learning. In other words, the provided word-topic probabilities
   * are multiplied by this weight before used as count
   * matrices within the model.
   *
   */
  void set_topics(const std::shared_ptr<sarray<flexible_type>> word_topic_prob,
      const std::shared_ptr<sarray<flexible_type>> vocabulary,
      size_t weight);

  /**
   * Get the most probable words for a given topic.
   *
   * \param topic_id The integer id of the topic. Must be in [0, num_topics)
   * length vocab_size used to construct the topic_model object.
   * \param num_words The number of words to return for the given topic.
   * \param cdf_cutoff After ordering words by probability, this will only
   * return words while the cumulative probability of the words is below
   * this cutoff value.
   *
   * \returns Returns an SFrame with the word and its corresponding score.
   * The SFrame is sorted by score.
   */
  std::pair<std::vector<flexible_type>, std::vector<double>>
    get_topic(size_t topic_id, size_t num_words=5, double cdf_cutoff=1.0);

  public:

  /**
   * Make predictions on the given data set.
   *
   * This method closely resembles the sampler in the collapsed Gibbs
   * sampler solver found in cgs.hpp. Here, however, the word_topic_counts
   * matrix is held fixed. For each document, num_burnin iterations are
   * performed where in each iteration we sample the topic_assignments.
   * The returned predictions are probabilities, and are computed by
   * smoothing the doc_topic_counts matrix that arising from sampling.
   *
   */
  std::shared_ptr<sarray<flexible_type>>
    predict_gibbs(std::shared_ptr<sarray<flexible_type>> data,
                  size_t num_burnin);

  /**
   * Make predictions for a given data set. Return the number of assignments
   * of each topic for each document in the dataset.
   */
  count_matrix_type predict_counts(std::shared_ptr<sarray<flexible_type> > dataset, size_t num_burnin);


  /**
   * Returns the current topics matrix as an SFrame
   */
  std::shared_ptr<sarray<flexible_type>> get_topics_matrix();

  /**
   * Returns current vocabulary of words.
   */
  std::shared_ptr<sarray<flexible_type>> get_vocabulary();

  /**
   * Compute perplexity. For more details see the docstrings for
   * the version that is not a member of the topic_model class.
   * This version is for a model's internal usage, i.e. where the two
   * count matrices are already available. Note that the first thing
   * this method does is normalize counts to be proper probabilities.
   * This is done via:
   *  doc_topic_prob[d, k] = p(topic k | document d)
   *                       = (doc_topic_count[d, k] + alpha) /
   *                        \sum_k' (doc_topic_count[d, k'] + alpha)
   *  word_topic_prob[w, k] = p(word w | topic k)
   *                        = (word_topic_count[w, k] + eta) /
   *                         \sum_w' (word_topic_count[w', k] + eta)
   *
   */
  double perplexity(std::shared_ptr<sarray<flexible_type>> documents,
                    const count_matrix_type& doc_topic_counts,
                    const count_matrix_type& word_topic_counts);

  void init_validation(std::shared_ptr<sarray<flexible_type> > validation_train,
std::shared_ptr<sarray<flexible_type> > validation_test);


};


} // text
} // turicreate

#endif

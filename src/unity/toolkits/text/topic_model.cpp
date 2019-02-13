/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <fstream>
#include <algorithm>
#include <iostream>
#include <parallel/pthread_tools.hpp>
#include <unity/lib/unity_sarray.hpp>
#include <sframe/sframe_iterators.hpp>
#include <unity/toolkits/util/sframe_utils.hpp>
#include <unity/lib/flex_dict_view.hpp>
#include <unity/toolkits/text/topic_model.hpp>
#include <unity/toolkits/text/cgs.hpp>
#include <logger/assertions.hpp>
#include <timer/timer.hpp>
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <unity/toolkits/util/indexed_sframe_tools.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/lib/variant_deep_serialize.hpp>
#include <Eigen/Core>

/**
 * TODO:
 * [ ] Better handling of solver options
 */

namespace turi {
namespace text {

/**
 * List all the keys that are present in the state.
 */
std::vector<std::string> topic_model::list_fields() {
  std::vector<std::string> ret;
  for (const auto& kvp: state){
    ret.push_back(kvp.first);
  }
  ret.push_back("topics");
  ret.push_back("vocabulary");
  return ret;
}


/**
 * Helper function for creating the appropriate ml_data from an sarray of
 * documents.
 */
v2::ml_data topic_model::create_ml_data_using_metadata(std::shared_ptr<sarray<flexible_type>> dataset) {

  // Construct an SFrame with one column (so that we may use ml_data)
  std::vector<std::shared_ptr<sarray<flexible_type>>> columns = {dataset};
  std::vector<std::string> column_names = {"data"};
  sframe dataset_sf = sframe(columns, column_names);

  // Create an ml_data object using the model's current metadata.
  v2::ml_data d(metadata);
  d.fill(dataset_sf);

  return d;
}

/**
 * Load a set of associations comprising a (word, topic) pair that should
 * be considered fixed.
 */
void
topic_model::set_associations(const sframe& data) {

  std::vector<std::string> colnames = {"word", "topic"};
  sframe d = data.select_columns(colnames);
  d.set_column_name(0, "data");  // Allows us to use model's internal metadata.

  sframe indexed_sf = sframe({v2::map_to_indexed_sarray(metadata->indexer(0),
                                                    d.select_column("data")),
                             data.select_column("topic")},
                             colnames);

  for (parallel_sframe_iterator it(indexed_sf); !it.done(); ++it) {
    size_t word_id = it.value(0);
    size_t topic_id = it.value(1);
    associations[word_id] = topic_id;
  }

  // Save this in the model status
 // status["associations"] = associations;
}

/**
 * Get the most probable words for a given topic.
 */
std::pair<std::vector<flexible_type>, std::vector<double>>
topic_model::get_topic(size_t topic_id, size_t num_words, double cdf_cutoff) {

  DASSERT_LT(topic_id, get_option_value("num_topics"));

  // Compute probabilities of words for this topic by normalizing smoothed counts.
  Eigen::MatrixXd word_topic_prob =
      (word_topic_counts.col(topic_id).cast<double>().array() + beta).matrix();
  word_topic_prob = word_topic_prob.array() / word_topic_prob.sum();

  // Get a list of (word_id, score) pairs for this topic.
  std::vector<std::pair<int, double>> data(vocab_size);
  for(size_t i=0; i < data.size(); ++i) {
    data[i].first = i;
    data[i].second = word_topic_prob(i, 0);
  }

  // Sort column k of the topics matrix, phi.
  // TODO: Only sort the largest num_words
  std::sort(data.begin(), data.end(),
            [](const std::pair<int, double>& a,
               const std::pair<int, double>& b) -> bool {
                return a.second > b.second;
            });

  // Copy the top words into a vector. Skip the word if we are over the cdf cutoff.
  double current_cdf = 0.0;
  data.resize(num_words);
  std::vector<flexible_type> top_words;
  std::vector<double> scores;
  for (size_t i=0; i < data.size(); ++i) {
    current_cdf += data[i].second;
    if (current_cdf <= cdf_cutoff) {
      top_words.push_back(metadata->indexer(0)->map_index_to_value(data[i].first));
      scores.push_back(data[i].second);
    }
  }

  return std::make_pair(top_words, scores);
}

/**
 * Compute the perplexity of the provided documents given the provided
 * topic model estimates.
 */
double topic_model::perplexity(std::shared_ptr<sarray<flexible_type> > dataset,
                               const count_matrix_type& doc_topic_counts,
                               const count_matrix_type& word_topic_counts) {

  // Number of documents must match the number for which we have topic estimates.
  DASSERT_EQ(dataset->size(), (size_t) doc_topic_counts.rows());

  // Construct an SFrame with one column (so that we may use ml_data)
  std::vector<std::shared_ptr<sarray<flexible_type>>> columns = {dataset};
  std::vector<std::string> column_names = {"data"};
  sframe dataset_sf = sframe(columns, column_names);

  // Construct an ml_data object using the metadata stored by the model.
  v2::ml_data d(metadata);
  d.fill(dataset_sf);


  typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> prob_matrix_type;

  // Compute probabilities by normalizing smoothed counts.
  prob_matrix_type doc_topic_prob = (doc_topic_counts.cast<double>().array() + alpha).matrix();
  prob_matrix_type word_topic_prob = (word_topic_counts.cast<double>().array() + beta).matrix();
  prob_matrix_type doc_topic_total = doc_topic_prob.rowwise().sum();
  prob_matrix_type word_topic_total = word_topic_prob.colwise().sum();

  size_t num_topics = doc_topic_counts.cols();
  size_t num_docs = doc_topic_counts.rows();
  for (size_t d = 0; d < num_docs; ++d) {
    doc_topic_prob.row(d) = doc_topic_prob.row(d).array() / doc_topic_total(d, 0);
  }
  for (size_t k = 0; k < num_topics; ++k) {
    word_topic_prob.col(k) = word_topic_prob.col(k).array() / word_topic_total(0, k);
  }

  // Initialize loglikelihood aggregator.
  size_t num_segments = thread::cpu_count();
  std::vector<double> llk_per_thread(num_segments);
  std::vector<size_t> num_words_per_thread(num_segments);

  // Start iterating through documents
  in_parallel([&](size_t thread_idx, size_t num_threads) {

    std::vector<v2::ml_data_entry> x;
    for(auto it = d.get_iterator(thread_idx, num_threads); !it.done(); ++it) {

      size_t doc_id = it.row_index();
      it.fill_observation(x);

      for (size_t j = 0; j < x.size(); ++j) {

        // Get word index and frequency
        size_t word_id = x[j].index;
        size_t freq = x[j].value;

        if (word_id < vocab_size) {
          // Compute Pr(word | theta, phi) =
          //    \sum_k doc_topic_prob[doc_id, k] * word_topic_prob[word_id, k]
          DASSERT_LT(word_id, word_topic_prob.rows());
          DASSERT_LT(doc_id, doc_topic_prob.rows());
          double prob = doc_topic_prob.row(doc_id).dot(word_topic_prob.row(word_id));

          // Increment numerator and denominator of perplexity estimate
          llk_per_thread[thread_idx] += freq * log(prob);
          num_words_per_thread[thread_idx] += freq;
        }
      }
    }
  });

  double llk = std::accumulate(llk_per_thread.begin(),
                               llk_per_thread.end(), 0.0);
  size_t num_words = std::accumulate(num_words_per_thread.begin(),
                                     num_words_per_thread.end(), 0.0);

  double perp = exp(- llk / num_words);
  if (std::isnan(perp))
    log_and_throw("NaN detected while computing perplexity.");

  return perp;
}

/**
 * TODO: Refactor this and perplexity to share code.
 */
void topic_model::set_topics(const std::shared_ptr<sarray<flexible_type>> word_topic_prob,
                             const std::shared_ptr<sarray<flexible_type>> vocabulary,
                             size_t weight) {

  logprogress_stream << "Initializing from provided topics and vocabulary." << std::endl;
  if (word_topic_prob->size() != vocabulary->size()) {
    log_and_throw("Number of word topics does not match the number of words in the vocabulary.");
  }

  bool allow_new_categorical_values = true;
  auto indexed_vocab = v2::map_to_indexed_sarray(metadata->indexer(0),
                                                 vocabulary,
                                                 allow_new_categorical_values);

  // Load the topics SArray into a vector of vectors
  std::vector<std::vector<double>> phi(word_topic_prob->size());
  size_t num_segments = thread::cpu_count();
  auto phi_reader = word_topic_prob->get_reader(num_segments);
  auto vocab_reader = indexed_vocab->get_reader(num_segments);

  in_parallel([&](size_t thread_idx, size_t num_threads) {

    auto iter = phi_reader->begin(thread_idx);
    auto enditer = phi_reader->end(thread_idx);
    auto vocab_iter = vocab_reader->begin(thread_idx);
    while (iter != enditer) {

      size_t word_id = *vocab_iter;

      DASSERT_EQ(flex_type_enum_to_name((*iter).get_type()),
                 flex_type_enum_to_name(flex_type_enum::VECTOR));

      phi[word_id] = (*iter).get<flex_vec>();

      // Assume the number of topics is the length of the first vector.
      if (word_id == 0)
        num_topics = phi[word_id].size();

      // Throw an error of the length of some vector doesn't match num_topics.
      if (phi[word_id].size() != num_topics)
        log_and_throw("Provided topic probability vectors do not have the same length.");

      ++word_id;
      ++iter;
      ++vocab_iter;
    }
  } );

  // Initialize word_topic_counts from the provided SArray
  vocab_size = metadata->indexer(0)->indexed_column_size();
  word_topic_counts = Eigen::MatrixXi::Zero(vocab_size, num_topics);
  for (size_t i = 0; i < vocab_size; ++i) {
    for (size_t k = 0; k < num_topics; ++k) {
      // Convert probabilities into (approximate) counts by
      // multiplying by the provided weight.
      word_topic_counts(i, k) = (size_t) ceil(phi[i][k] * weight);
    }
  }

  // Makes sure that these topics are kept during training, rather than
  // set to zero when  init() is called on the model.
  is_initialized = true;
}

topic_model::count_matrix_type topic_model::predict_counts(std::shared_ptr<sarray<flexible_type>> dataset, size_t num_burnin) {

  // Construct an SFrame with one column (so that we may use ml_data)
  std::vector<std::shared_ptr<sarray<flexible_type>>> columns = {dataset};
  std::vector<std::string> column_names = {"data"};
  sframe dataset_sf = sframe(columns, column_names);

  timer tmp;
  tmp.start();
  v2::ml_data d(metadata);
  d.fill(dataset_sf);

  // Initialize topic count matrices
  std::vector<size_t> topic_assignments;
  count_matrix_type doc_topic_counts(dataset->size(), num_topics);
  doc_topic_counts.setZero();

  count_matrix_type topic_counts = word_topic_counts.colwise().sum();
  DASSERT_EQ(topic_counts.cols(), num_topics);

  // Start iterating through documents in parallel
  in_parallel([&](size_t thread_idx, size_t n_threads) GL_GCC_ONLY(GL_HOT_FLATTEN) {

    std::vector<v2::ml_data_entry> x;
    x.reserve(d.max_row_size());

    std::vector<size_t> topic_assignments;
    topic_assignments.reserve(d.max_row_size());

    // Initialize probability vector
    Eigen::VectorXd gamma_base_vec(num_topics);
    Eigen::VectorXd gamma_vec(num_topics);

    for(auto it = d.get_iterator(thread_idx, n_threads); !it.done(); ++it) {

      size_t doc_id = it.row_index();
      it.fill_observation(x);

      auto end_it = std::remove_if(x.begin(), x.end(),
                                   [=](const v2::ml_data_entry& e) { return e.index >= vocab_size; } );
      x.resize(end_it - x.begin());

      // Initialize topic assignments for new doc
      size_t num_words_in_doc = 0;
      topic_assignments.clear();


      for (size_t j = 0; j < x.size(); ++j) {

        // Get word index and frequency
        size_t word_id = x[j].index;
        size_t freq = x[j].value;

        {
          auto it = associations.find(word_id);

          if (it != associations.end()) {

            size_t topic = it->second;
            topic_assignments.push_back(topic);
            doc_topic_counts(doc_id, topic) += freq;
            // Ignore words outside of provided vocabulary
          } else {

            num_words_in_doc += freq;
            size_t topic = random::fast_uniform<size_t>(0, num_topics - 1);
            DASSERT_TRUE(topic < num_topics);
            topic_assignments.push_back(topic);
            doc_topic_counts(doc_id, topic) += freq;

          }
        }
      }

      DASSERT_EQ(doc_topic_counts.row(doc_id).sum(), num_words_in_doc);

      // Compute the base probability of the gamma vector.
      gamma_base_vec =
          ((doc_topic_counts.row(doc_id).cast<double>().array() + alpha) /
           (topic_counts.cast<double>().array() + vocab_size * beta));

      auto gamma_base = [&](size_t doc_id, size_t topic, double freq) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
        return ((doc_topic_counts(doc_id, topic) + freq + alpha)
                / (topic_counts(0, topic) + freq + vocab_size * beta));
      };

      // Sample topics for this document
      for (size_t burnin = 0; burnin < num_burnin; ++burnin) {
        size_t shift = random::fast_uniform<size_t>(0, x.size()-1);
        for (size_t _j = 0; _j < x.size(); ++_j) {
          size_t j = (_j + shift) % x.size();

          // Get word index and frequency
          size_t word_id = x[j].index;
          double freq = x[j].value;

          DASSERT_LT(word_id,  vocab_size);

          size_t topic = topic_assignments[j];
          gamma_base_vec[topic] = gamma_base(doc_id, topic, -freq);

          DASSERT_TRUE(doc_topic_counts(doc_id, topic) >= 0);

          gamma_vec = word_topic_counts.row(word_id).cast<double>().array() + beta;
          gamma_vec.array() *= gamma_base_vec.array();

          // Sample topic for this token
          size_t old_topic = topic;
          topic = random::multinomial(gamma_vec, gamma_vec.sum());
          topic_assignments[j] = topic;

          gamma_base_vec[topic] = gamma_base(doc_id, topic, freq);

          if(topic != old_topic) {
            // Increment counts
            doc_topic_counts(doc_id, old_topic) -= freq;
            doc_topic_counts(doc_id, topic) += freq;
            DASSERT_EQ(doc_topic_counts.row(doc_id).sum(), num_words_in_doc);
          }
        }

        if (cppipc::must_cancel()) {
          log_and_throw("Toolkit canceled by user.");
        }
      } // end burnin
    }
  });

  return doc_topic_counts;
}

/**
 * Make predictions on the given data set.
 */
std::shared_ptr<sarray<flexible_type> > topic_model::predict_gibbs(
    std::shared_ptr<sarray<flexible_type> > dataset, size_t num_burnin) {

  count_matrix_type doc_topic_counts = predict_counts(dataset, num_burnin);
  size_t n_docs = doc_topic_counts.rows();
  DASSERT_EQ(n_docs, dataset->size());

  size_t num_segments = thread::cpu_count();

  // Initialize predictions
  std::shared_ptr<sarray<flexible_type> > predictions(new sarray<flexible_type>);
  predictions->open_for_write(num_segments);
  predictions->set_type(flex_type_enum::VECTOR);

  in_parallel([&](size_t thread_idx, size_t num_threads) {

      // Write predictions for document
      flex_vec doc_preds(num_topics);

      size_t start_idx = (thread_idx * n_docs) / num_threads;
      size_t end_idx = ((thread_idx+1) * n_docs) / num_threads;
      auto out_it = predictions->get_output_iterator(thread_idx);

      for (size_t doc_id = start_idx; doc_id < end_idx; ++doc_id, ++out_it) {

        // Normalize to be proper probabilities.
        // Include hyperparameter alpha for smoothing.
        double norm = doc_topic_counts.row(doc_id).sum() + num_topics * alpha;
        for(size_t topic_id = 0; topic_id < num_topics; ++topic_id) {
          doc_preds[topic_id] = (doc_topic_counts(doc_id, topic_id) + alpha) / norm;
        }

        *out_it = doc_preds;
      }
    });

  predictions->close();
  return predictions;
}


/// Returns the current normalized topics matrix as an SFrame
std::shared_ptr<sarray<flexible_type>> topic_model::get_topics_matrix() {

  // Normalize smoothed counts.
  Eigen::MatrixXd word_topic_prob = (word_topic_counts.cast<double>().array() + beta).matrix();
  Eigen::MatrixXd word_topic_total = word_topic_prob.colwise().sum();
  size_t num_topics = word_topic_counts.cols();
  for (size_t k = 0; k < num_topics; ++k) {
    word_topic_prob.col(k) = word_topic_prob.col(k).array() /
        word_topic_total(0, k);
  }

  return matrix_to_sarray(word_topic_prob);
}

/// Returns current vocabulary
std::shared_ptr<sarray<flexible_type>> topic_model::get_vocabulary() {

  std::shared_ptr<sarray<flexible_type>> column(new sarray<flexible_type>);
  column->open_for_write(1);
  column->set_type(flex_type_enum::STRING);
  auto it_out = column->get_output_iterator(0);
  //DASSERT_EQ(metadata.size(), 1);
  for (size_t word_id = 0; word_id < (size_t) word_topic_counts.rows(); ++word_id) {
    *it_out = metadata->indexer(0)->map_index_to_value(word_id);
    ++it_out;
  }
  column->close();
  return column;
}

void topic_model::init_validation(std::shared_ptr<sarray<flexible_type>> _validation_train,
std::shared_ptr<sarray<flexible_type>> _validation_test) {
  validation_train = _validation_train,
  validation_test = _validation_test;
}


} // text
} // turicreate

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <core/parallel/pthread_tools.hpp>
#include <core/storage/sframe_interface/unity_sarray.hpp>
#include <model_server/lib/flex_dict_view.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/text/topic_model.hpp>
#include <toolkits/text/cgs.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <toolkits/util/indexed_sframe_tools.hpp>
#include <toolkits/ml_data_2/metadata.hpp>

#include <core/logging/assertions.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <timer/timer.hpp>
#include <Eigen/Core>
#include <core/parallel/atomic.hpp>

namespace turi {
namespace text {


/**
 * Destructor. Make sure bad things don't happen
 */
cgs_topic_model::~cgs_topic_model() {
}

/** Cast the object to a topic_model.
 */
topic_model* cgs_topic_model::topic_model_clone() {
  cgs_topic_model* m = new cgs_topic_model(*this);
  return (topic_model*) m;
}

void cgs_topic_model::init_options(const std::map<std::string,
    flexible_type>&_options){

  options.create_boolean_option(
      "verbose",
      "Verbose printing",
      true,
      false);

  options.create_integer_option(
      "num_topics",
      "Number of topics to learn",
      10,
      0,
      std::numeric_limits<flex_int>::max(),
      false);

  options.create_integer_option(
      "num_iterations",
      "Number of iterations to take through the data",
      10,
      0,
      std::numeric_limits<flex_int>::max(),
      false);

  options.create_integer_option(
      "num_burnin",
      "Number of passes to take through a document before using its data to update the topics.",
      5,
      0,
      std::numeric_limits<flex_int>::max(),
      false);

  options.create_integer_option(
      "print_interval",
      "Number of iterations to wait before printing status.",
      10,
      0,
      std::numeric_limits<flex_int>::max(),
      false);


  options.create_real_option(
      "alpha",
      "Hyperparameter for smoothing the number of topics per document. Must be positive.",
      0.1,
      std::numeric_limits<double>::min(),
      std::numeric_limits<double>::max(),
      false);

  options.create_real_option(
      "beta",
      "Hyperparameter for smoothing the number of topics per word. Must be positive.",
      0.1,
      std::numeric_limits<double>::min(),
      std::numeric_limits<double>::max(),
      false);

  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));

  option_info_set = true;

  // Set internal values
  num_topics = get_option_value("num_topics");

  // Hyperparameters
  alpha = get_option_value("alpha");
  beta = get_option_value("beta");

  // Current settings
  is_initialized = false;

  // Initialize metadata.
  v2::ml_data d({{"missing_value_action_on_predict", "error"}});
  sframe sf = sframe();
  sf.open_for_write({"data"}, {flex_type_enum::DICT});
  sf.close();
  d.fill(sf);
  metadata = d.metadata();

  // Initialize assocations
  associations = std::map<size_t, size_t>();
}


/**
 *
 */
std::shared_ptr<sarray<std::vector<size_t> > >
cgs_topic_model::forward_sample(const v2::ml_data& d,
                                count_vector_type& topic_counts,
                                count_matrix_type& doc_topic_counts) {

  // Initialize latent variable assignments
  std::shared_ptr<sarray<std::vector<size_t> > > assignments(new sarray<std::vector<size_t>>);
  size_t num_segments = thread::cpu_count();
  assignments->open_for_write(num_segments);

  // Start iterating through documents in parallel
  in_parallel([&](size_t thread_idx, size_t num_threads) {

      // Initialize topic assignments for new doc
      std::vector<size_t> doc_assignments;
      doc_assignments.reserve(d.max_row_size());

    auto gamma_vec = std::vector<double>(num_topics);
    std::vector<v2::ml_data_entry> x;
    auto assignments_out = assignments->get_output_iterator(thread_idx);

    // Start iterating through documents
    for(auto it = d.get_iterator(thread_idx, num_threads); !it.done(); ++it) {

      size_t doc_id = it.row_index();
      it.fill_observation(x);

      doc_assignments.clear();

      for (size_t j = 0; j < x.size(); ++j) {
        size_t word_id = x[j].index;
        size_t freq = x[j].value;
        size_t topic = 0;
        bool topic_sampled = false;
        // If this word is in the set of fixed associations, get its topic.
        if (associations.count(word_id) != 0) {

          topic = associations[word_id];
          topic_sampled = true;

          // Ignore words outside of provided vocabulary
        } else if (word_id < vocab_size) {

          // Compute unnormalized topic probabilities for this word
          for (size_t k = 0; k < num_topics; ++k) {
            gamma_vec[k] = (doc_topic_counts(doc_id, k) + alpha) *
              (word_topic_counts(word_id, k) + beta) /
              (topic_counts(0, k) + vocab_size * beta);
          }

          // Sample topic for this token
          topic = random::multinomial(gamma_vec);
          topic_sampled = true;
        }

        if (topic_sampled) {
          DASSERT_TRUE(word_id < vocab_size);
          doc_assignments.push_back(topic);

          // Increment counts
          __sync_add_and_fetch(&word_topic_counts(word_id, topic), freq);
          __sync_add_and_fetch(&topic_counts(0, topic), freq);

          doc_topic_counts(doc_id, topic) += freq;
        }
      } // end words in document
      *assignments_out = doc_assignments;
      ++assignments_out;
    }
  });
  assignments->close();

  return assignments;
}

std::map<std::string, size_t> cgs_topic_model::sample_counts(
    const v2::ml_data& d,
    count_vector_type& topic_counts,
    count_matrix_type& doc_topic_counts,
    std::shared_ptr<sarray<std::vector<size_t> > >& assignments) {

  atomic<size_t> token_count = 0;
  atomic<size_t> num_different = 0;

  // Initialize iterators
  auto assignments_reader = assignments->get_reader();

  // Create an SArray for new assignments
  std::shared_ptr<sarray<std::vector<size_t> > > new_assignments(new sarray<std::vector<size_t>>);
  size_t num_segments = thread::cpu_count();
  new_assignments->open_for_write(num_segments);

  in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_FLATTEN) {

    Eigen::VectorXd gamma_base_vec(num_topics);
    Eigen::VectorXd gamma_vec(num_topics);
    std::vector<size_t> doc_assignments;

    std::vector<v2::ml_data_entry> x;

    // Set up SArrays for reading current assignments and writing new ones
    auto iter = assignments_reader->begin(thread_idx);
    auto enditer = assignments_reader->end(thread_idx);
    auto new_assignments_out = new_assignments->get_output_iterator(thread_idx);

    for(auto it = d.get_iterator(thread_idx, num_threads); !it.done(); ++it) {

      size_t doc_id = it.row_index();

      it.fill_observation(x);

      doc_assignments = *iter;
      DASSERT_EQ(x.size(), doc_assignments.size());

      // Compute the base probability of the gamma vector.
      gamma_base_vec =
          ((doc_topic_counts.row(doc_id).cast<double>().array() + alpha) /
           (topic_counts.cast<double>().array() + vocab_size * beta));

      auto gamma_base = [&](size_t doc_id, size_t topic, double freq) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
        return ((doc_topic_counts(doc_id, topic) + freq + alpha)
                / (topic_counts(topic) + freq + vocab_size * beta));
      };

      // Iterate through each token

      // Choose a random spot in the document to try first.   This way we reduce biases.
      size_t shift = random::fast_uniform<size_t>(0, x.size()-1);
      for (size_t _j = 0; _j < x.size(); ++_j) {
        size_t j = (_j + shift) % x.size();

        size_t word_id = x[j].index;
        double freq = x[j].value;
        DASSERT_GE(freq, 0);

        // If this word is in the set of fixed associations, do nothing.
        if (associations.count(word_id) != 0) {

          // Ignore any words that are outside of the vocabulary.
        } else if (word_id < vocab_size) {

          // Get current topic assignment for this word
          size_t topic = doc_assignments[j];
          __sync_add_and_fetch(&word_topic_counts(word_id, topic), -freq);

          gamma_base_vec[topic] = gamma_base(doc_id, topic, -freq);

          gamma_vec = word_topic_counts.row(word_id).cast<double>().array() + beta;
          gamma_vec.array() *= gamma_base_vec.array();

          // Sample topic for this token
          size_t old_topic = topic;
          topic = random::multinomial(gamma_vec, gamma_vec.sum());

          if (topic != old_topic) {
          // Remove counts due to current tokens
            __sync_add_and_fetch(&topic_counts(0, old_topic), -freq);
            doc_topic_counts(doc_id, old_topic) -= freq;

            ++num_different;
            doc_assignments[j] = topic;

            // Increment counts
            __sync_add_and_fetch(&topic_counts(0, topic), freq);
            doc_topic_counts(doc_id, topic) += freq;
          }

          __sync_add_and_fetch(&word_topic_counts(word_id, topic), freq);

          gamma_base_vec[topic] = gamma_base(doc_id, topic, freq);

#ifndef NDEBUG
          // Check that we have not over decremented the other counts
          for (size_t k = 0; k < num_topics; ++k) {
            DASSERT_TRUE(topic_counts(0, k) >= 0);
            DASSERT_TRUE(word_topic_counts(word_id, k) >= 0);
            DASSERT_TRUE(doc_topic_counts(doc_id, k) >= 0);
          }
#endif

          DASSERT_TRUE(topic >= 0 && topic < num_topics);

          // Increment counter
          ++token_count;
        } // end of sampling topics for each word
      } // end of words for this document

      // Write assignments for this document
      *new_assignments_out = doc_assignments;
      ++new_assignments_out;
      ++iter;
    } // end of documents for this thread
  });
  new_assignments->close();
  assignments_reader->reset_iterators();

  // Use these new assignments from now on.
  assignments = new_assignments;

  std::map<std::string, size_t> ret;
  ret["token_count"] = (size_t) token_count;
  ret["num_different"] = (size_t) num_different;
  return ret;
}

/**
 * Train a model using collapsed Gibbs sampling.
 */
void cgs_topic_model::train(std::shared_ptr<sarray<flexible_type>> dataset, bool verbose) {

  size_t num_iterations = get_option_value("num_iterations");
  size_t print_interval = get_option_value("print_interval");
  size_t num_burnin = get_option_value("num_burnin");

  // Do nothing if the number of iterations is zero.
  if (num_iterations == 0) {
    return;
  }

  // Convert documents to use internal indexing
  v2::ml_data d = create_ml_data_using_metadata(dataset);

  /**
   * Initializes the model using the current metadata.
   * This sets the internal vocab_size and ensures word_topic_counts
   * has at vocab_size number of rows. If word_topic_counts already exists
   * this function pads it with more rows until it has reached the right
   * size.
   */

  // Initalize other items
  vocab_size = metadata->column_size(0);

  if (!is_initialized) {

    // Create a word_topic_counts matrix of all zeros.
    word_topic_counts = Eigen::MatrixXi::Zero(vocab_size, num_topics);

  } else {

    // Copy over old word_topic_counts matrix to the top of a new one.
    Eigen::MatrixXi tmp = Eigen::MatrixXi::Zero(vocab_size, num_topics);
    tmp.topRows(word_topic_counts.rows()) = word_topic_counts;
    word_topic_counts = tmp;

  }

  is_initialized = true;

  logprogress_stream << "Learning a topic model" << std::endl;
  logprogress_stream << std::setw(26) << "   Number of documents"
                     << std::setw(10) << d.num_rows()
                     << std::endl;
  logprogress_stream << std::setw(26) << "   Vocabulary size"
                     << std::setw(10) << vocab_size
                     << std::endl;

  logprogress_stream << "   Running collapsed Gibbs sampling" << std::endl;

  // Step 1.
  // Initialize count matrices by "forward sampling". We simply sample each latent
  // topic assignment, z, using the current counts. Note that the elements of
  // word_topic_counts may be nonzero if the user has initialized the model
  // from another set of topics.

  // Initialize temporary variables
  count_vector_type topic_counts(num_topics);
  topic_counts.setZero();

  count_matrix_type doc_topic_counts(d.num_rows(), num_topics);
  doc_topic_counts.setZero();

  auto assignments = forward_sample(d, topic_counts, doc_topic_counts);

  // Step 2. Gibbs sampling
  // -------------------------------------------------------------------------
  // For each (doc, word) pair, we compute the following conditional
  // distribution:
  //
  //   p(z_ij = k | word_ij, all other z, Theta, Phi) \propto
  //      p(word_ij | z_ij = k)p(z_ij = k | Theta, Phi)
  //
  // where z_ij is the latent "topic" assignment for the j'th word in
  // document i. We use the version called "collapsed" conditional that only
  // uses the counts of assignments.

  timer ti;
  timer training_timer;
  training_timer.start();
  timer validation_timer;
  validation_timer.start();
  double validation_time = 0.0;

  // Print timing information
  table_printer table( { {"Iteration", 0},
                         {"Elapsed Time", 13},
                         {"Tokens/Second", 14},
                         {"Est. Perplexity", 11} } );
  table.print_header();

  for (size_t iteration = 0; iteration <= num_iterations; ++iteration) {

    // Reset old assignments before the next iteration
    ti.start();
    auto info = sample_counts(d, topic_counts, doc_topic_counts, assignments);
    double tokens_per_second = info["token_count"] / ti.current_time();

    if ((print_interval > 0) &&
        ((iteration % print_interval == 0 && iteration > 0) ||
         (iteration == num_iterations))) {

      // Get current estimate of model quality
      double perp = 0.0;
      if (validation_train != nullptr && validation_test != nullptr) {
        validation_timer.start();
        count_matrix_type pred_doc_topic_counts = predict_counts(validation_train, num_burnin);
        perp = perplexity(validation_test,
                          pred_doc_topic_counts,
                          word_topic_counts);
        validation_time += validation_timer.current_time();
        add_or_update_state({{"validation_perplexity", perp}});
      }

      // Print timing information
      table.print_row(iteration,
                      progress_time(),
                      tokens_per_second,
                      perp);

      ti.start();

      if (verbose) {

        // For each topic, print the top few words and scores
        std::stringstream ss;
        size_t num_words_to_show = std::min(size_t(15), vocab_size);

        for (size_t topic_id = 0; topic_id < num_topics; ++topic_id) {

          ss << "topic " << topic_id << ": ";

          auto top_words = get_topic(topic_id, num_words_to_show);
          for (size_t j = 0; j < top_words.first.size(); ++j) {
            ss << top_words.first[j] << " ";
          }
          // ss << " | ";
          // for (size_t j = 0; j < top_words.first.size(); ++j) {
          //   ss << top_words.second[j] << " ";
          // }
          ss << "\n";
          logprogress_stream << ss.str() << std::endl;
          ss.str("");
        }
      }
    }
  }
  add_or_update_state({{"training_time", training_timer.current_time()},
                       {"training_iterations", num_iterations},
                       {"validation_time", validation_time}});


  table.print_footer();

  return;
}


/**
 * Turi Serialization Save.
 *
 */
void cgs_topic_model::save_impl(turi::oarchive& oarc) const {

  // Python facing objects
  variant_deep_save(state, oarc);

  std::map<std::string, variant_type> data;
  data["alpha"]      = to_variant(alpha);
  data["beta"]      = to_variant(beta);
  data["num_topics"]      = to_variant(num_topics);
  data["vocab_size"]      = to_variant(vocab_size);
  data["option_info_set"]      = to_variant(vocab_size);
  data["is_initialized"] = to_variant(is_initialized);
  data["associations"] = to_variant(associations);
  variant_deep_save(data, oarc);

  // Member variables

  oarc << metadata
       << options
       << word_topic_counts;
}

/**
 * Turi Serialization Load
 */
void cgs_topic_model::load_version(turi::iarchive& iarc, size_t version) {
  ASSERT_MSG(version == 0 || version == 1, "This model version cannot be loaded. Please re-save your model.");
  variant_deep_load(state, iarc);

  std::map<std::string, variant_type> data;
  variant_deep_load(data, iarc);

#define __EXTRACT(var) var = variant_get_value<decltype(var)>(data.at(#var));

  __EXTRACT(alpha);
  __EXTRACT(beta);
  __EXTRACT(num_topics);
  __EXTRACT(vocab_size);
  __EXTRACT(option_info_set);
  __EXTRACT(is_initialized);
  __EXTRACT(associations);

#undef __EXTRACT

  iarc >> metadata
       >> options;

  if(version == 0) {
    Eigen::MatrixXi _word_topic_counts;
    iarc >> _word_topic_counts;
    word_topic_counts = _word_topic_counts;
  } else {
    iarc >> word_topic_counts;
  }


}
}
}

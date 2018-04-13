/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <fstream>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <parallel/pthread_tools.hpp>
#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/flex_dict_view.hpp>
#include <unity/lib/variant_deep_serialize.hpp>
#include <unity/toolkits/text/topic_model.hpp>
#include <unity/toolkits/text/alias.hpp>
#include <unity/toolkits/util/spmat.hpp>
#include <random/alias.hpp>
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <unity/toolkits/util/indexed_sframe_tools.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <logger/assertions.hpp>
#include <table_printer/table_printer.hpp>
#include <timer/timer.hpp>
#include <numerics/armadillo.hpp>
#include <parallel/atomic.hpp>

namespace turi {
namespace text {

/**
 * Destructor. Make sure bad things don't happen
 */
alias_topic_model::~alias_topic_model() {
}

/** Cast the object to a topic_model.
 */
topic_model* alias_topic_model::topic_model_clone() {
  alias_topic_model* m = new alias_topic_model(*this);
  return (topic_model*) m;
}

void alias_topic_model::init_options(const std::map<std::string,
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
      "Number of passes to take through a document before using its data to update the topics at predict time",
      3,
      0,
      std::numeric_limits<flex_int>::max(),
      false);

  options.create_integer_option(
      "num_burnin_per_block",
      "Number of passes to take through a block document before using its data to update the topics.",
      1,
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
  doc_topic_counts = spmat(0);

  // Initialize a counter
  token_count = 0;
}


/**
 *
 */
std::shared_ptr<sarray<std::vector<size_t>>>
alias_topic_model::forward_sample(v2::ml_data d) {

  doc_topic_counts = spmat(d.num_rows());

  // Initialize latent variable assignments
  std::shared_ptr<sarray<std::vector<size_t>>> assignments(new sarray<std::vector<size_t>>);
  size_t num_segments = thread::cpu_count();
  assignments->open_for_write(num_segments);

  // Start iterating through documents in parallel
  in_parallel([&](size_t thread_idx, size_t num_threads) {

    auto gamma_vec = std::vector<double>(num_topics);
    std::vector<v2::ml_data_entry> x;
    auto assignments_out = assignments->get_output_iterator(thread_idx);

    // Start iterating through documents
    for(auto it = d.get_iterator(thread_idx, num_threads); !it.done(); ++it) {

      size_t doc_id = it.row_index();
      it.fill_observation(x);

      // Initialize topic assignments for new doc
      std::vector<size_t> doc_assignments;
      doc_assignments.reserve(x.size());

      for (size_t j = 0; j < x.size(); ++j) {
        size_t word_id = x[j].index;
        size_t freq = x[j].value;

        for (size_t z = 0; z < freq; ++z) {
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
              gamma_vec[k] = (doc_topic_counts.get(doc_id, k) + alpha) *
                  (topic_word_counts(k, word_id) + beta) /
                  (topic_counts[k] + vocab_size * beta);
            }

            // Sample topic for this token
            topic = random::multinomial(gamma_vec);
            topic_sampled = true;
          }

          if (topic_sampled) {
            DASSERT_TRUE(word_id < vocab_size);
            doc_assignments.push_back(topic);

            // Increment counts
            __sync_add_and_fetch(&topic_word_counts(topic, word_id), 1);
            __sync_add_and_fetch(&topic_counts[topic], 1);

            doc_topic_counts.increment(doc_id, topic, 1);
          }
        }
      } // end words in document
      *assignments_out = doc_assignments;
      ++assignments_out;
    }
  });
  assignments->close();

  return assignments;
}

void alias_topic_model::sample_block(const v2::ml_data& d,
                  std::vector<std::vector<size_t>>& doc_assignments) {
  /**
   * The number of suggested MH steps. See Li 2014.
   */
  size_t num_mh_steps = 2;

  if (d.num_rows() != doc_assignments.size()) {
    log_and_throw("Mismatch in block creation.");
  }

  // Populate doc topic matrix from assignments
  doc_topic_counts = spmat(d.num_rows());
  for (size_t doc_id = 0; doc_id < doc_assignments.size(); ++doc_id) {
    for (size_t z : doc_assignments[doc_id]) {
      doc_topic_counts.increment(doc_id, z, 1);
    }
  }

  in_parallel([&](size_t thread_idx, size_t num_threads) {

    /**
     * Initialize temporary variables to avoid reallocating them.
     */
    std::vector<v2::ml_data_entry> x;
    std::vector<double> pd(num_topics);

    for(auto it = d.get_iterator(thread_idx, num_threads); !it.done(); ++it) {

      // Get document words
      size_t doc_id = it.row_index();
      it.fill_observation(x);

      size_t total_words_seen = 0;
      size_t total_words_in_doc = 0;
      for (size_t j = 0; j < x.size(); ++j) {
        size_t freq = x[j].value;
        total_words_in_doc += freq;
      }
      DASSERT_TRUE(total_words_in_doc == doc_assignments[doc_id].size());

      // Iterate through each token
      for (size_t j = 0; j < x.size(); ++j) {
        size_t word_id = x[j].index;
        size_t freq = x[j].value;

        // If this word is in the set of fixed associations, do nothing.
        if (associations.count(word_id) != 0) {
          continue;
        }
        if (word_id >= vocab_size) {
          continue;
        }

        // Get current topic assignment for this word
        for (size_t ix = 0; ix < freq; ++ix) {
          size_t topic = doc_assignments[doc_id][total_words_seen + ix];

          // Remove counts due to current tokens
          topic_word_counts(topic, word_id) -= 1;
          topic_counts[topic] -= 1;
          doc_topic_counts.increment(doc_id, topic, -1);

          // TODO: This hack should not be required.
          if (topic_word_counts(topic, word_id) < 0)
            topic_word_counts(topic, word_id) = 0;

          // Check that we have not over decremented the other counts
          for (size_t k = 0; k < num_topics; ++k) {
            DASSERT_TRUE(topic_counts[k] >= 0);
            DASSERT_TRUE(topic_word_counts(k, word_id) >= 0);
            DASSERT_TRUE(doc_topic_counts.get(doc_id, k) >= 0);
          }

          size_t new_topic = topic;
          for (size_t mh = 0; mh < num_mh_steps; ++mh) {
            new_topic = sample_topic(doc_id, word_id, new_topic, pd);
          }

          doc_assignments[doc_id][total_words_seen + ix] = new_topic;
          DASSERT_TRUE(new_topic >= 0 && new_topic < num_topics);

          // Increment counts
          topic_word_counts(new_topic, word_id) += 1;
          topic_counts[new_topic] += 1;
          doc_topic_counts.increment(doc_id, new_topic, 1);

          // Increment counter
          ++token_count;
        } // end of sampling topics for this word
        total_words_seen += freq;
      } // end of words for this document
    }
  });
}


// Wrapper for loading a blocks of an ml_data into memory,
// performing computations, and writing an SArray with th esame
// number of rows as the ml_data object.
//
// for qblock in (ml_data data, SArray params) {
//   load params in memory
//   for iter in 1:num_iter {
//     par_for row in qblock {
//       read row.data
//       make updates to row.params
//     }
//   }
//   par_for row in qblock {
//      write row.params to out_sarray
//   }
// }
//


std::map<std::string, size_t> alias_topic_model::sample_counts( v2::ml_data d, size_t num_blocks) {

  // Initialize iterators to read topic assignments for each word
  auto assignments_reader = assignments->get_reader(1);

  // Create an SArray for new assignments
  std::shared_ptr<sarray<std::vector<size_t>>> new_assignments(new sarray<std::vector<size_t>>);
  size_t num_segments = 1;
  new_assignments->open_for_write(num_segments);
  auto new_assignments_out = new_assignments->get_output_iterator(0);

  // For each block
  for(size_t block_index = 0; block_index < num_blocks; ++block_index) {

    size_t block_start = (block_index * d.size()) / num_blocks;
    size_t block_end = ((block_index + 1) * d.size()) / num_blocks;
    size_t block_size = block_end - block_start;

    // Load topic assignments for words in this block's documents
    auto doc_assignments = std::vector<std::vector<size_t>>(block_size);
    assignments_reader->read_rows(block_start, block_end, doc_assignments);

    // Load documents in this block
    auto d_block = d.slice(block_start, block_end);

    // Update document assignments for this block
    for (size_t iter = 0; iter < get_option_value("num_burnin_per_block"); ++iter) {
      sample_block(d_block, doc_assignments);
    }

    // Write assignments out to SArray
    for (const auto& v : doc_assignments) {
      *new_assignments_out = std::move(v);
    }
  }
  new_assignments->close();

  // Use these new assignments from now on.
  assignments = new_assignments;

  std::map<std::string, size_t> ret;
  return ret;
}

/**
 * Train a model using collapsed Gibbs sampling.
 */
void alias_topic_model::train(std::shared_ptr<sarray<flexible_type>> dataset, bool verbose) {

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
   * This sets the internal vocab_size and ensures topic_word_counts
   * has at vocab_size number of rows. If topic_word_counts already exists
   * this function pads it with more rows until it has reached the right
   * size.
   */

  // Initalize other items
  vocab_size = metadata->column_size(0);

  if (!is_initialized) {

    // Create a topic_word_counts matrix of all zeros.
    topic_word_counts.resize(num_topics, vocab_size);
    topic_word_counts.zeros();

  } else {

    // Copy over old topic_wordcounts matrix to the top of a new one.
    count_matrix_type tmp;
    tmp.zeros(num_topics, vocab_size);
    tmp.head_cols(topic_word_counts.n_cols) = topic_word_counts;
    topic_word_counts = tmp;
  }

  is_initialized = true;

  logprogress_stream << "Learning a topic model" << std::endl;
  logprogress_stream << std::setw(26) << "   Number of documents"
                     << std::setw(10) << d.num_rows()
                     << std::endl;
  logprogress_stream << std::setw(26) << "   Vocabulary size"
                     << std::setw(10) << vocab_size
                     << std::endl;

  timer ti;
  timer perp_timer;

  // Initialize variables
  topic_counts.resize(num_topics);
  topic_counts.zeros();
  q = arma::zeros(vocab_size, num_topics);
  Q = arma::zeros(1, vocab_size);
  word_samplers = std::vector<random::alias_sampler>(vocab_size);
  word_samples = std::vector<std::vector<size_t>>(vocab_size);

  logprogress_stream << "   Initializing topic assignments" << std::endl;
  assignments = forward_sample(d);

  for (size_t w = 0; w < vocab_size; ++w) {
    cache_word_pmf_and_samples(w);
  }
  logprogress_stream << "   Constructed alias samplers:"
                     << ti.current_time() << "s"
                     << std::endl;

  // Print timing information
  timer training_timer;
  training_timer.start();
  timer validation_timer;
  double validation_time = 0.0;

  // Determine how many blocks to use.
  size_t num_blocks = 1;
  size_t target_num_rows_per_block = TARGET_BLOCK_NUM_ELEMENTS / (d.max_row_size() + 1);
  while(d.size() / num_blocks > target_num_rows_per_block) {
    num_blocks *= 2;
  }

  logprogress_stream << "   Using " << num_blocks << " blocks." << std::endl;

  // Print timing information
  table_printer table( { {"Iteration",         0},
                         {"Elapsed Time",      13},
                         {"Tokens/Second",     14},
                         {"Est. Perplexity",   11},
                         {"Elapsed for perp.", 14} });
  table.print_header();


  for (size_t iteration = 0; iteration <= num_iterations; ++iteration) {

    // Reset old assignments before the next iteration
    ti.start();
    auto info = sample_counts(d, num_blocks);
    double tokens_per_second = token_count / ti.current_time();
    token_count = 0;

    if ((print_interval > 0) &&
        (iteration % print_interval == 0) &&
        (iteration > 0)) {

      // Get current estimate of model quality
      double perp = 0.0;

      if (validation_train != nullptr && validation_test != nullptr) {
        validation_timer.start();
        count_matrix_type pred_doc_topic_counts = predict_counts(validation_train, num_burnin);
        perp = perplexity(validation_test,
                          pred_doc_topic_counts,
                          topic_word_counts);
        validation_time = validation_timer.current_time();
        add_or_update_state({{"validation_perplexity", perp}});
      }

      // Print timing information
      table.print_row(iteration,
                      progress_time(),
                      tokens_per_second,
                      perp,
                      validation_time);

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

size_t alias_topic_model::sample_topic(size_t d, size_t w, size_t s,
                                       std::vector<double>& pd) {

  // Compute sparse component of pmf for probability of each topic
  for (const auto& kv : doc_topic_counts.get_row(d)) {
    size_t topic = kv.first;
    size_t count = kv.second;
    double p = count *
        (topic_word_counts(topic, w) + beta) /
        (topic_counts(topic) + num_topics * beta);
    pd[topic] = p;
  }

  // Compute normalizing constant
  double Pdw = std::accumulate(pd.begin(), pd.end(), 0.0);

  // Choose whether to sample from sparse or dense portion
  double prob_sparse_sample = Q(0, w) / (Pdw + Q(0, w));

  size_t t;
  if (random::fast_uniform<double>(0, 1) < prob_sparse_sample) {

    // Use samples precomputed via Alias sampler
    t = word_samples[w].back();
    word_samples[w].pop_back();

    // Rejuvenate samples if empty
    if (word_samples[w].size() == 0) {
      cache_word_pmf_and_samples(w);
    }
  } else {

    // Inverse CDF method on the sparse part
    double cutoff = random::fast_uniform<double>(0, Pdw);
    double current = 0.0;
    for (const auto& kv : doc_topic_counts.get_row(d)) {
      t = kv.first;
      current += pd[t];
      if (current > cutoff) {
        break;
      }
    }
  }

  // Compute MH probability
  double pdws = pd[s];
  double pdwt = pd[t];
  double pi = (doc_topic_counts.get(d, t) + alpha) /
              (doc_topic_counts.get(d, s) + alpha) *
              (topic_word_counts(t, w) + beta) /
              (topic_word_counts(s, w) + beta) *
              (topic_counts(0, s) + beta * num_topics) /
              (topic_counts(0, t) + beta * num_topics) *
              (Pdw * pdws + Q(0, w) * q(w, s)) /
              (Pdw * pdwt + Q(0, w) * q(w, t));

  // Perform MH step
  size_t chosen_topic = (random::fast_uniform<double>(0, 1) <
                         std::min(1.0, pi)) ? t : s;
  // Reset probs to 0
  for (const auto& kv : doc_topic_counts.get_row(d)) {
    pd[kv.first] = 0.0;
  }

  return chosen_topic;
}


/**
 * Turi Serialization Save.
 */
void alias_topic_model::save_impl(turi::oarchive& oarc) const {

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
       << options;

  // we have to save topic_word_counts manually here unfortunately
  // if we want to preserve compatibility
  oarc << (size_t)topic_word_counts.n_cols << (size_t)topic_word_counts.n_rows;
  turi::serialize(oarc, topic_word_counts.memptr(), topic_word_counts.n_elem*sizeof(int));

}

/**
 * Turi Serialization Load
 */
void alias_topic_model::load_version(turi::iarchive& iarc, size_t version) {
  ASSERT_MSG(version <= TOPIC_MODEL_VERSION, "This model version cannot be loaded. Please re-save your model.");
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

  // we have to save topic_word_counts manually here unfortunately
  // if we want to preserve compatibility
  size_t ncols, nrows;
  iarc >> ncols >> nrows;
  topic_word_counts.resize(nrows, ncols);
  turi::deserialize(iarc, topic_word_counts.memptr(), topic_word_counts.n_elem*sizeof(int));
}

/**
 * Helper utility for word pmfs.
 */
void alias_topic_model::cache_word_pmf_and_samples(size_t w) {
  // Compute unnormalized pmf and normalizing constant.
  double Q_w = 0.0;
  for (size_t t = 0; t < num_topics; ++t) {
    q(w, t) =  alpha *
               (topic_word_counts(t, w) + beta) /
               (topic_counts(t) + num_topics * beta);
    Q_w += q(w, t);
  }

  // Normalize pmf
  for (size_t t = 0; t < num_topics; ++t) {
    q(w, t) = q(w, t) / Q_w;
  }

  // Compute alias data structure
  auto p = std::vector<double>(num_topics);
  for (size_t t = 0; t < num_topics; ++t) {
    p[t] = q(w, t);
  }
  word_samplers[w] = random::alias_sampler(p);

  // Sample using the alias method and store samples.
  while (word_samples[w].size() < num_topics) {
    word_samples[w].push_back(word_samplers[w].sample());
  }
}

}
}

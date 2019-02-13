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
#include <unity/toolkits/text/topic_model.hpp>
#include <logger/assertions.hpp>
#include <unity/toolkits/util/indexed_sframe_tools.hpp>
#include <ml_data/ml_data.hpp>

namespace turi {
namespace text {

/**
 * Compute the perplexity of the provided documents given the provided
 * topic model estimates. This implementation allows one to compute
 * perplexity in the absence of any model object. One drawback is ome
 * code duplication; the benefit is that it is standalone, not depending on the
 * current implementation of the topic_model class.
 */
double perplexity(std::shared_ptr<sarray<flexible_type>> dataset,
                  const std::shared_ptr<sarray<flexible_type>> doc_topic_prob, 
                  const std::shared_ptr<sarray<flexible_type>> word_topic_prob,
                  const std::shared_ptr<sarray<flexible_type>> vocabulary) {
  
  DASSERT_EQ(dataset->size(), doc_topic_prob->size());
  
  // Convert the SArray of vocabulary into std::vector 
  std::map<flexible_type, size_t> vocab;
  auto vocab_reader = vocabulary->get_reader();
  for (size_t seg = 0; seg < vocabulary->num_segments(); ++seg) {
    auto iter = vocab_reader->begin(seg);
    auto enditer = vocab_reader->end(seg);   
    while (iter != enditer) {
      DASSERT_EQ(flex_type_enum_to_name((*iter).get_type()), 
                 flex_type_enum_to_name(flex_type_enum::STRING));
      size_t idx = vocab.size();
      vocab[(*iter)] = idx;
      ++iter;
    }
  }


  // Load the topics SArray into a vector of vectors 
  std::vector<std::vector<double>> phi(word_topic_prob->size());
  auto phi_reader = word_topic_prob->get_reader();
  size_t word_id = 0;
  size_t num_topics = 0;
  for (size_t seg = 0; seg < word_topic_prob->num_segments(); ++seg) {
    auto iter = phi_reader->begin(seg);
    auto enditer = phi_reader->end(seg);   
    while (iter != enditer) {

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
    }
  } 

  // Prepare to iterate through documents 
  size_t num_segments = thread::cpu_count();
  std::vector<double> llk_per_thread(num_segments);
  std::vector<size_t> num_words_per_thread(num_segments);
  auto theta_reader = doc_topic_prob->get_reader(num_segments);
  auto doc_reader = dataset->get_reader(num_segments);

  // Start iterating through documents
  in_parallel([&](size_t thread_idx, size_t num_threads) {
      
    // Prepare to iterate through document topics
    auto theta_iter = theta_reader->begin(thread_idx);
    auto theta_enditer = theta_reader->end(thread_idx);   

    auto doc_iter = doc_reader->begin(thread_idx);
    auto doc_enditer = doc_reader->end(thread_idx);
    // Start iterating through documents
    while(doc_iter != doc_enditer && theta_iter != theta_enditer) {
      if ((*theta_iter).get_type() == flex_type_enum::VECTOR &&
          (*doc_iter).get_type() == flex_type_enum::DICT) {
        // Get topic proportions
        const flex_vec& theta_doc = *theta_iter;
        const flex_dict& doc_dict = *doc_iter;
        DASSERT_EQ(theta_doc.size(), num_topics);

        for (size_t j = 0; j < doc_dict.size(); ++j) {
          // Get word index and frequency
          flexible_type word = doc_dict[j].first;
          double freq = doc_dict[j].second.to<double>();
          auto word_find_iter = vocab.find(word);
    
          if (word_find_iter != vocab.end()) {
            size_t word_id = word_find_iter->second;
            // Compute Pr(word | theta, phi)
            double prob = 0;
            for (size_t k = 0; k < num_topics; ++k) {
              DASSERT_LT(0.0, theta_doc[k]);
              DASSERT_LT(0.0, phi[word_id][k]);
              prob += theta_doc[k] * phi[word_id][k];
            }

            // Increment numerator and denominator of perplexity estimate
            llk_per_thread[thread_idx] += freq * log(prob);
            num_words_per_thread[thread_idx] += freq;

          }
        }
      }
      ++theta_iter;
      ++doc_iter;
    }
  });

  double llk = std::accumulate(llk_per_thread.begin(), 
                               llk_per_thread.end(), 0.0);
  size_t num_words = std::accumulate(num_words_per_thread.begin(), 
                                     num_words_per_thread.end(), 0.0);

  return std::exp(- llk / num_words);
}

} // text 
} // turicreate

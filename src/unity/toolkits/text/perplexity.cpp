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
#include <numerics/armadillo.hpp>



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
  std::vector<flexible_type> vocab = std::vector<flexible_type>();
  vocab.reserve(vocabulary->size());
  auto vocab_reader = vocabulary->get_reader();
  for (size_t seg = 0; seg < vocabulary->num_segments(); ++seg) {
    auto iter = vocab_reader->begin(seg);
    auto enditer = vocab_reader->end(seg);   
    while (iter != enditer) {
      DASSERT_EQ(flex_type_enum_to_name((*iter).get_type()), 
                 flex_type_enum_to_name(flex_type_enum::STRING));
      
      // Create a flex_dict element
      std::pair<flexible_type, flexible_type> item = 
          std::make_pair(*iter, 1.0);

      vocab.push_back(flex_dict{item});
      ++iter;
    }
  }


  // Construct an SFrame with one column (so that we may use ml_data)
  std::vector<std::shared_ptr<sarray<flexible_type>>> columns = {dataset};
  std::vector<std::string> column_names = {"data"};
  sframe dataset_sf = sframe(columns, column_names);

  // Create an ml_data object using the model's current metadata.
  ml_data d;
  d.fill(dataset_sf);

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

  // Start iterating through documents
  in_parallel([&](size_t thread_idx, size_t num_threads) {
      
    // Prepare to iterate through document topics
    auto theta_iter = theta_reader->begin(thread_idx);
    auto theta_enditer = theta_reader->end(thread_idx);   

    // Start iterating through documents
    std::vector<ml_data_entry> x;
    for(auto it = d.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
    
      // Get topic proportions
      const flex_vec& theta_doc = *theta_iter;
      DASSERT_EQ(theta_doc.size(), num_topics);

      // Get document data
      it->fill(x); 
      for (size_t j = 0; j < x.size(); ++j) {
     
        // Get word index and frequency
        size_t word_id = x[j].index;
        size_t freq = x[j].value;
  
        if (word_id < vocab.size()) {
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
      ++theta_iter;
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

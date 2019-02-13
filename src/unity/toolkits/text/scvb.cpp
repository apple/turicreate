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
#include <unity/lib/flex_dict_view.hpp>
#include <unity/toolkits/text/topic_model.hpp>
#include <unity/toolkits/text/scvb.hpp>
#include <logger/assertions.hpp>
#include <timer/timer.hpp>
#include <Eigen/Core>

namespace turi {
namespace text {


/**
 * Train a model using SCVB0 algorithm.
 */
void scvb0_solver::train(std::shared_ptr<sarray<flexible_type>> dataset, bool verbose) {
  
  // Do nothing if the number of iterations is zero. 
  if (model->num_iterations == 0)
    return;

  // Convert documents to use internal indexing
  ml_data d = model->create_ml_data_using_metadata(dataset);

  // Prepare the model for training

  logprogress_stream << "Running SCVB0" << std::endl;

  // Initialize a set of estimates from current word_topic_counts 
  N_phi = Eigen::MatrixXd::Zero(model->vocab_size, model->num_topics);
  for (size_t i = 0; i < (size_t) N_phi.rows(); ++i) {
    for (size_t j = 0; j < (size_t) N_phi.cols(); ++j) {
      N_phi(i, j) = (double) model->word_topic_counts(i, j);
    }
  }

  // Initialize other statistics matrices
  N_phi_hat = Eigen::MatrixXd::Zero(model->vocab_size, model->num_topics);
  N_Z = Eigen::MatrixXd::Zero(model->num_topics, 1);
  N_Z_hat = Eigen::MatrixXd::Zero(model->num_topics, 1);
  N_Z = N_phi.colwise().sum();

  // Initialize timer
  timer ti;
  ti.start();
  size_t token_count = 0;

  for (size_t iteration = 0; iteration < model->num_iterations; ++iteration) {
    
    // Get the learning rate for this iteration.
    double rho = compute_rho(iteration, s, tau, kappa);
    model->current_iteration = iteration;

    auto reader = dataset->get_reader();
    size_t doc_id = 0;
    for (size_t seg = 0; seg < dataset->num_segments(); ++seg) {

      auto iter = reader->begin(seg);
      auto enditer = reader->end(seg);   

      // For each document
      while (iter != enditer) {

        model->current_document = doc_id;

        flex_dict_view fdv = flex_dict_view(*iter);

        // Count total number of words and initialize document topics.
        size_t C_j = 0;
        size_t Z_j = 0;
        for (auto z = fdv.begin(); z != fdv.end(); z++) {
          C_j += (size_t)(z->second); 
          Z_j += 1;
        }

        // TODO: This hack avoids small documents that screw up scvb0.
        if (fdv.size() > 10) {

          initialize_N_theta_j(C_j);
          size_t M = model->minibatch_size * Z_j * 3;

          // "Burnin" the topics for the current document.
          // Analogous to an E-step for the document's topics.
          for (size_t burnin = 0; burnin < model->num_burnin; ++burnin) {
            // Iterate through tokens
            for (auto z = fdv.begin(); z != fdv.end(); z++) {
              size_t word_id = model->metadata[0]->map_without_insertion_value_to_index(z->first); 
              DASSERT_LT(word_id, model->vocab_size);
              auto gamma_ij = compute_gamma(word_id); 
              update_N_theta_j(gamma_ij, z->second, C_j, rho);
            }
          }

          // Shuffle the order of the tokens.
          auto tokens = std::vector<std::pair<flexible_type, size_t> >(fdv.begin(), fdv.end());
          std::random_shuffle(tokens.begin(), tokens.end());

          // Iterate through each token
          for (auto z = tokens.begin(); z != tokens.end(); z++) {

            // Get the integer id of the word
            size_t word_id = model->metadata[0]->map_without_insertion_value_to_index(z->first);  

            // Get the number of times it occurred in the document
            size_t freq = z->second;

            // Compute topic probabilities for this word
            auto gamma_ij = compute_gamma(word_id);

            // Update local statistics
            update_N_theta_j(gamma_ij, freq, C_j, rho);
            update_N_phi_hat(gamma_ij, word_id, M, model->num_words);
            update_N_Z_hat(gamma_ij, M, model->num_words);
          }

          // If the minibatch is complete, make updates.
          if (doc_id % model->minibatch_size == 0 && doc_id != 0) {

            // Update global statistics. Unlock when done.
            {
              std::unique_lock<turi::mutex> global_lock(model->lock);
              update_N_phi(rho);
              update_N_Z(rho);        
            }

            if (verbose) {
              logprogress_stream << "Iteration " << iteration 
                << ". Tokens/second: " 
                << token_count / ti.current_time() 
                << std::endl;
              token_count = 0;
              ti.start();


              if (verbose) {
                logprogress_stream << "M: " << M << std::endl;
                logprogress_stream << "num_words: " << model->num_words << std::endl; 
                logprogress_stream << std::setw(16) << "sum(N_theta_j) " 
                                   << N_theta_j.sum() << std::endl;
                logprogress_stream << std::setw(16) << "sum(N_phi) " 
                                   << model->word_topic_counts.sum() << std::endl;
                logprogress_stream << std::setw(16) << "sum(N_phi_hat) " 
                                   << N_phi_hat.sum() << std::endl;
                logprogress_stream << std::setw(16) << "sum(N_Z) " 
                                   << N_Z.sum() << std::endl;
                logprogress_stream << std::setw(16) << "sum(N_Z_hat) " 
                                   << N_Z_hat.sum() << std::endl;
              }

              std::stringstream ss;

              size_t num_words_to_show = std::min(size_t(10), model->vocab_size);
              for (size_t topic_id = 0; topic_id < model->num_topics; ++topic_id) {

                ss << "topic " << topic_id << ": ";

                auto top_words = model->get_topic(topic_id, num_words_to_show);
                for (size_t j = 0; j < num_words_to_show; ++j) {
                  ss << top_words.first[j] << " ";
                }
                // // Print probabilities of each word
                // ss << " | ";
                // for (size_t j = 0; j < num_words_to_show; ++j) {
                //   ss << top_words.second[j] << " ";
                // }
                logprogress_stream << ss.str() << std::endl; 
                ss.str("");
              }

            }

            // Clear local statistics about the minibatch
            N_Z_hat.setZero();
            N_phi_hat.setZero();

          }
        }

        // Increment to the next document.
        ++doc_id;
        ++iter;
      }
    }
  }

  // Copy estimates to the model. Take floor so that we have integers.
  for (size_t i = 0; i < (size_t) N_phi.rows(); ++i) {
    for (size_t j = 0; j < (size_t) N_phi.cols(); ++j) {
      model->word_topic_counts(i, j) = floor(N_phi(i, j));
    }
  }

  model->training_complete = true;
  return;
}

}
}

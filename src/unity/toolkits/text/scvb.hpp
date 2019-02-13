/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TEXT_SCVB_H_
#define TURI_TEXT_SCVB_H_

// SFrame
#include <sframe/sarray.hpp>
#include <sframe/sframe.hpp>

// Other
#include <fileio/temp_files.hpp>
#include <thread>
#include <iostream>

// Types
#include <unity/lib/variant.hpp>
#include <unity/lib/unity_base_types.hpp>
#include <flexible_type/flexible_type.hpp>
#include <util/hash_value.hpp>
#include <unity/lib/flex_dict_view.hpp>

// External
#include <Eigen/Core>
#include <random/random.hpp>

namespace turi {
  
namespace text {

class scvb0_solver {

 public:

  scvb0_solver(topic_model* _model) {

    model = _model;
    s = 10;
    tau = 1000;
    kappa = .9;

  }

  /**
   * Train the model using the SCVB0 algorithm.
   * 
   * See Foulds, Boyles, DuBois, Smyth, Welling.
   * Stochastic Collapsed Variational Bayesian Inference
   * for Latent Dirichlet Allocation. KDD 2013.
   * See http://arxiv.org/pdf/1305.2452.pdf for a pdf. 
   *
   * The key aspect of this algorithm is to keep a set of statistics that 
   * describe the number of times each word in the vocabulary has been assigned
   * to each topic. We then iterate through minibatches of documents and
   * perform updates akin to online EM: we use our current statistics (N_Z and N_phi) 
   * to make estimated "local" versions using the minibatch.
   *
   */
  void train(std::shared_ptr<sarray<flexible_type>> data, bool verbose); 


 private:
  topic_model* model;

  // Hyperparameters for learning rate
  size_t s; 
  size_t tau;
  double kappa;

  // Variables needed for SCVB0
  Eigen::MatrixXd N_Z;        /* < Estimate of N_Z for each topic. */
  Eigen::MatrixXd N_theta_j;  /* < Estimate of N_theta for document j. */
  Eigen::MatrixXd N_phi;      /* < Estimate of N_phi. */
  Eigen::MatrixXd N_phi_hat;  /* < Estimate of N_phi based on a minibatch. */
  Eigen::MatrixXd N_Z_hat;    /* < Estimate of N_Z based on a minibatch. */

 private:

  /**
   * Initialize global estimate of N_theta_j. 
   */
  void initialize_N_theta_j(size_t C_j) {
     N_theta_j = Eigen::MatrixXd::Zero(model->num_topics, 1);
     for (size_t i = 0; i < C_j; ++i) {
       size_t ix = random::fast_uniform<size_t>(0, model->num_topics-1);
       N_theta_j(ix) += 1;
     }
  }

  /**
   * Compute the topic probabilities for a single token.
   *
   * \params w_ij The integer id for the word that is the i'th token
   *              of document j.
   *
   * \returns An Eigen vector of length num_topics containing the
   * estimated probability the word belongs to each of the topics.
   */
  Eigen::MatrixXd compute_gamma(size_t w_ij) {
    // DASSERT(j >= 0 && j < documents.size());
    // DASSERT(i >= 0 && i < vocab_size);
    Eigen::MatrixXd gamma_ij(model->num_topics, 1);
    for (size_t k = 0; k < model->num_topics; ++k) {
      gamma_ij(k, 0) = (N_phi(w_ij, k) + model->beta) * 
        (N_theta_j(k) + model->alpha) /
        (N_Z(k) + model->beta * model->vocab_size);
    }
    gamma_ij.normalize();
    return gamma_ij;
  }

  /**
   * Compute a local estimate of topic proportions for this document.
   *
   */
  void update_N_theta_j(const Eigen::MatrixXd& gamma_ij,
                        size_t count_ij,
                        size_t C_j,
                        double rho) {
    double alpha = std::pow(1 - rho, count_ij);
    N_theta_j = alpha * N_theta_j + C_j * gamma_ij * (1 - alpha); 
  }

  /**
   * Update the local estimate of N_Z using the current token probabilities.
   */
  void update_N_Z_hat(const Eigen::MatrixXd& gamma_ij,
                      size_t M, size_t C) {
    N_Z_hat += gamma_ij * C / M;
  }

  /**
   * Update the global estimate of N_Z using the current local estimates.
   */
  void update_N_Z(double rho) {
    N_Z = (1 - rho) * N_Z + rho * N_Z_hat;
  }

  /**
   * Update global estimate of N_phi using the current local estimates.
   */
  void update_N_phi(double rho) {
    N_phi = (1 - rho) * N_phi + rho * N_phi_hat;
  }

  /**
   * Update local estimate of N_phi with the current token probabilities.
   */
  void update_N_phi_hat(const Eigen::MatrixXd& gamma_ij,
                        size_t word_ij, 
                        size_t M, size_t C) {
    for (size_t k = 0; k < model->num_topics; ++k) {
      N_phi_hat(word_ij, k) += gamma_ij(k) * C / M;
    }
  }

  /**
   * Compute the learning rate for a given iteration.
   * The default values have been reported to experimentally provide
   * reasonable learning rates for real data sets.
   *
   * \returns s/(tau + t)^kappa.
   */
  double compute_rho(size_t t, size_t s = 10, size_t tau=1000, double kappa = .9) {
    return (double) s / std::pow(tau + t, kappa);
  }

};
}
}
#endif

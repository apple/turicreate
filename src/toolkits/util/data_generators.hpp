/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TESTING_DATA_GENERATORS_H_
#define TURI_TESTING_DATA_GENERATORS_H_

#include <core/storage/sframe_data/sframe.hpp>
#include <Eigen/Core>
#include <map>
#include <string>

namespace turi { namespace recsys {

/**
 * \ingroup toolkit_util
 * A simple class for generating fake linear model data for testing
 * purposes.  This uses the factorization machine model to generate
 * the data.
 *
 * The options going into this generator are as follows.  These are
 * not necessarily used by each function:
 *
 * - random_seed:    Random seed for sampling the data.
 *
 * - n_factors: Number of latent factors to use in the generation.
 *
 * - noise_sd: Standard deviation of the noise associated with each response.
 *
 * - w0_sd: The standard deviation used in generating the intercept term.
 *
 * - w_sd: The standard deviation used in generating the linear terms.
 *
 * - V_sd: The standard deviation used in generating the latent factors.
 *
 * - y_mode: The sampling model. Can be "squared_error" or "logistic".
 *
 * The defaults for these are given in data_generators.cpp.
 *
 */

class lm_data_generator {
 public:

  lm_data_generator(
      const std::vector<std::string>& column_names,
      const std::vector<size_t>& n_categorical_values,
      const std::map<std::string, flexible_type>& base_options);

  /** Fill data with the observations and responses of the linear
   * model.
   */
  sframe generate(size_t n_observations,
                  const std::string& target_column_name,
                  size_t random_seed,
                  double noise_sd) const;

  /** Fill two datasets for ranking and testing the ranking.  This
   * works by building a linear model and assuming that the
   * observations with the highest responses are those in the data
   * set.  A portion of these are split off into the test set.
   */
  std::pair<sframe, sframe> generate_for_ranking(
      size_t n_train_samples_per_user,
      size_t n_test_samples_per_user,
      size_t random_seed,
      double noise_sd) const;

 private:

  double evaluate(const std::vector<flexible_type>& x, double noise_sd) const;

  double w0;
  Eigen::VectorXd w;
  Eigen::MatrixXd V;

  std::vector<std::string> column_names;
  std::vector<size_t> n_categorical_values;

  size_t n_factors, dim;
  bool logistic_mode;

  std::map<std::string, flexible_type> _options;
};

}}

#endif /* TURI_TESTING_DATA_GENERATORS_H_ */

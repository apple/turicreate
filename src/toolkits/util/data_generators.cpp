/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/util/data_generators.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/random/random.hpp>
#include <Eigen/Core>

namespace turi { namespace recsys {

static std::map<std::string, flexible_type> default_options = {
  {"random_seed", 0 },
  {"n_factors", 0 },
  {"only_2_factor_terms", false },
  {"nmf", false },
  {"noise_sd", 1 },
  {"w0_sd", 1},
  {"w_sd", 1},
  {"V_sd", 1},
  {"y_mode", "squared_error"},
};

/** A simple function for generating fake linear model data for
 * testing purposes.
 */
lm_data_generator::lm_data_generator(
    const std::vector<std::string>& _column_names,
    const std::vector<size_t>& _n_categorical_values,
    const std::map<std::string, flexible_type>& base_options)
    : column_names(_column_names)
    , n_categorical_values(_n_categorical_values)
{

  _options = default_options;

  for(const auto& p : base_options) {
    _options[p.first] = p.second;
  }

  n_factors          = _options["n_factors"];
  size_t random_seed = _options["random_seed"];

  double w0_sd              = _options["w0_sd"];
  double w_sd               = _options["w_sd"];
  double V_sd               = _options["V_sd"];

  dim = 0;
  for(int ncv : n_categorical_values)
    dim += ((ncv == 0) ? 1 : ncv);

  w.resize(dim);
  V.resize(dim, n_factors);
  V.setZero();

  ////////////////////////////////////////////////////////////
  // Fill these up!
  random::seed(random_seed);

  bool only_2_factor_terms = _options["only_2_factor_terms"];
  bool nmf_mode = _options["nmf"];

  w0 = nmf_mode ? 0 : random::normal(0, w0_sd);

  for(size_t i = 0; i < dim; ++i) {
    w[i] = nmf_mode ? 0 : random::normal(0, w_sd);

    if(i < n_categorical_values[0] + n_categorical_values[1] || !only_2_factor_terms) {
      for(size_t j = 0; j < n_factors; ++j) {
        V(i,j) = random::normal(0, V_sd / sqrt(double(n_factors)));
        if(nmf_mode)
          V(i,j) = std::abs(V(i,j));
      }
    }
  }

  ////////////////////////////////////////////////////////////
  // Set the rest of the stuff

  std::string y_mode = _options.at("y_mode");

  if(y_mode == "squared_error" || y_mode == "ranking")
    logistic_mode = false;
  else if(y_mode == "logistic")
    logistic_mode = true;
  else
    ASSERT_TRUE(false);
}

////////////////////////////////////////////////////////////////////////////////

/** Fill data with the observations and responses of the linear
 * model.
 */
sframe lm_data_generator::generate(size_t n_observations,
                                   const std::string& target_column_name,
                                   size_t random_seed,
                                   double noise_sd) const {

  size_t n_columns = n_categorical_values.size();
  DASSERT_EQ(n_categorical_values.size(), column_names.size());

  ////////////////////////////////////////////////////////////
  // Now go through and generate things

  sframe out;

  std::vector<flex_type_enum> types(n_columns);

  for(size_t i = 0; i < n_columns; ++i)
    types[i] = (n_categorical_values[i] == 0) ? flex_type_enum::FLOAT : flex_type_enum::INTEGER;

  types.push_back(flex_type_enum::FLOAT);

  std::vector<std::string> names = column_names;
  names.push_back(target_column_name);


  //////////////////////////////

  random::seed(random_seed);

  size_t num_segments = thread::cpu_count();
  out.open_for_write(names, types, "", num_segments);

  in_parallel([&](size_t sidx, size_t num_threads) {

      auto it_out = out.get_output_iterator(sidx);

      size_t start_idx = (sidx * n_observations) / num_threads;
      size_t end_idx   = ((sidx+1) * n_observations) / num_threads;

      for(size_t i = start_idx; i < end_idx; ++i, ++it_out) {

        std::vector<flexible_type> x(n_columns + 1);

        for(size_t j = 0; j < n_columns; ++j) {
          if(n_categorical_values[j] == 0) {
            x[j] = random::normal(0, 1);
          } else {
            x[j] = random::fast_uniform<size_t>(0, n_categorical_values[j] - 1);
          }
        }

        double r = evaluate(x, noise_sd);
        if(logistic_mode)
          DASSERT_TRUE(r == 0 || r == 1);

        x.back() = r;

        *it_out = x;
      }
    });

  out.close();

  ASSERT_EQ(out.num_rows(), n_observations);

  return out;
}

////////////////////////////////////////////////////////////////////////////////

std::pair<sframe, sframe> lm_data_generator::generate_for_ranking(
    size_t n_train_samples_per_user,
    size_t n_test_samples_per_user,
    size_t random_seed,
    double noise_sd) const {

  size_t n_columns = n_categorical_values.size();

  ASSERT_MSG(n_columns == 2, "For ranking, the number of columns must be exactly 2.");

  size_t n_users = n_categorical_values[0];
  size_t n_items = n_categorical_values[1];

  ASSERT_MSG(n_train_samples_per_user + n_test_samples_per_user <= n_items,
             "number of train + test samples per user more than the number of items.");

  ////////////////////////////////////////////////////////////
  // Now go through and generate things

  std::vector<flex_type_enum> types{flex_type_enum::INTEGER, flex_type_enum::INTEGER};

  ////////////////////////////////////////////////////////////
  // Now go through and generate things

  std::vector<std::vector<flexible_type> > train_values, test_values;

  random::seed(random_seed);

  std::vector<flexible_type> x(2);

  train_values.resize((n_users - 1) * n_train_samples_per_user + n_items);
  test_values.resize((n_users - 1) * n_test_samples_per_user);

  parallel_for(0, n_users - 1, [&](size_t user_idx){

      std::vector<std::pair<double, flexible_type> > samples;
      samples.reserve(n_items);

      x[0] = user_idx;

      samples.clear();
      for(size_t item_idx = 0; item_idx < n_items; ++item_idx) {

        x[1] = item_idx;

        double fx = evaluate(x, noise_sd);

        samples.push_back({-fx, item_idx});
      }

      std::sort(samples.begin(), samples.end());
      size_t n_picked_items = n_train_samples_per_user + n_test_samples_per_user;
      samples.resize(n_picked_items);

      random::shuffle(samples.begin(), samples.begin() + n_picked_items / 2);

      // Add in the test values.  Make sure they are in the upper half
      // of the ranked items.
      for(size_t i = 0; i < n_test_samples_per_user; ++i) {
        size_t idx = user_idx * n_test_samples_per_user + i;
        test_values[idx] = {user_idx, samples[i].second};
      }

      // Add in the train values.
      for(size_t i = 0; i < n_train_samples_per_user; ++i) {
        size_t idx = user_idx * n_train_samples_per_user + i;
        train_values[idx] = {user_idx, samples[i + n_test_samples_per_user].second};
      }
    });

  // Have a dummy user who has rated all the items.
  for(size_t i = 0; i < n_items; ++i) {
    size_t idx = (n_users - 1) * n_train_samples_per_user + i;
    train_values[idx] = {n_users - 1, i};
  }

  return {make_testing_sframe(column_names, types, train_values),
        make_testing_sframe(column_names, types, test_values)};
}

////////////////////////////////////////////////////////////////////////////////

double lm_data_generator::evaluate(const std::vector<flexible_type>& x, double noise_sd) const {

  Eigen::MatrixXd Vtemp(1, n_factors);

  double y = w0;

  size_t idx_start = 0;
  size_t n_columns = n_categorical_values.size();

  Vtemp.setZero();

  double vcenter = 0;

  for(size_t j = 0; j < n_columns; ++j) {

    double v = x[j];

    if(n_categorical_values[j] == 0) {
      y += v * w[idx_start];
      Vtemp += v * V.row(idx_start);
      vcenter += (v * V.row(idx_start)).squaredNorm();
      idx_start += 1;
    } else {
      y += w[idx_start + x[j]];
      Vtemp += V.row(idx_start + x[j]);
      vcenter += (V.row(idx_start + x[j])).squaredNorm();
      idx_start += n_categorical_values[j];
    }
  }

  y += 0.5 * (Vtemp.squaredNorm() - vcenter);

  y += random::normal(0, noise_sd);

  if(logistic_mode)
    y = (y > 0) ? 1 : 0;

  return y;
}



}}

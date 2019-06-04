/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/factorization/factorization_model.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/ml_data_2/side_features.hpp>
#include <core/storage/sframe_data/sframe.hpp>

namespace turi { namespace factorization {

void factorization_model::setup(
    const std::string& _loss_model_name,
    const v2::ml_data& train_data,
    const std::map<std::string, flexible_type>& opts)
{

  ////////////////////////////////////////////////////////////
  // Step 1:  Set up the generative model.

  loss_model_name = _loss_model_name;
  loss_model = get_loss_model_profile(loss_model_name);
  options = opts;
  random_seed = options.at("random_seed");

  ////////////////////////////////////////////////////////////
  // Step 2:  Set up the indexing.

  index_sizes.resize(train_data.num_columns());

  for(size_t i = 0; i < index_sizes.size(); ++i)
    index_sizes[i] = train_data.metadata()->index_size(i);

  size_t cum_sum = 0;

  index_offsets.resize(index_sizes.size());
  for(size_t i = 0; i < index_sizes.size(); ++i) {
    index_offsets[i] = cum_sum;
    cum_sum += index_sizes[i];
  }

  n_total_dimensions = cum_sum;

  ////////////////////////////////////////////////////////////
  // Step 3:  Set up the column scaling.

  column_shift_scales.resize(n_total_dimensions);

  for(size_t c_idx = 0; c_idx < index_sizes.size(); ++c_idx) {

    const auto& m = train_data.metadata()->statistics(c_idx);

    if(train_data.metadata()->is_categorical(c_idx)) {
      for(size_t i = 0; i < index_sizes[c_idx]; ++i)
        column_shift_scales[i + index_offsets[c_idx]] = {0.0, 1.0};

    } else {
      for(size_t i = 0; i < index_sizes[c_idx]; ++i) {
        column_shift_scales[i + index_offsets[c_idx]] = {m->mean(i), 1.0 / std::max(1.0, m->stdev(i))};
      }
    }
  }

  if(train_data.has_target()) {
    DASSERT_EQ(train_data.metadata()->target_index_size(), 1);
    target_mean = train_data.metadata()->target_statistics()->mean(0);
    target_sd   = train_data.metadata()->target_statistics()->stdev(0);
  } else {
    target_mean = 0;
    target_sd = 1;
  }

  ////////////////////////////////////////////////////////////
  // Step 4: Copy remaining stuff over.

  metadata = train_data.metadata();

  ////////////////////////////////////////////////////////////
  // Step 5: Call the internal setup routine of the current model.

  internal_setup(train_data);
}

////////////////////////////////////////////////////////////////////////////////

/** Returns a map of the training statistics of the model.
 */
std::map<std::string, variant_type> factorization_model::get_training_stats() const {
  return _training_stats;
}

////////////////////////////////////////////////////////////////////////////////

/** Calculate the value of the objective function as determined by the
 *  loss function, for a full data set, minus the regularization
 *  penalty.
 */
double factorization_model::calculate_loss(const v2::ml_data& data) const {

  std::vector<double> total_loss_accumulator(thread::cpu_count(), 0);

  volatile bool numerical_error_detected = false;

  in_parallel([&](size_t thread_idx, size_t num_threads) {

      std::vector<v2::ml_data_entry> x;

      for(auto it = data.get_iterator(thread_idx, num_threads);
          !it.done() && !numerical_error_detected; ++it) {

        it.fill_observation(x);

        double y = it.target_value();

        double fx_hat = calculate_fx(x);
        double point_loss = loss_model->loss(fx_hat, y);

        if(!std::isfinite(point_loss)) {
          numerical_error_detected = true;
          break;
        }

        total_loss_accumulator[thread_idx] += point_loss;
      }
    }
    );

  if(numerical_error_detected)
    return NAN;

  double total_loss = std::accumulate(total_loss_accumulator.begin(),
                                      total_loss_accumulator.end(), double(0));

  size_t n = data.size();
  double loss_value = (n != 0) ? total_loss / n : 0;

  return loss_value;
}

////////////////////////////////////////////////////////////////////////////////

/** Make a prediction for every observation in test_data.  Returns a
 *  single-column SFrame with a prediction for every observation.
 */
sframe factorization_model::predict(const v2::ml_data& test_data) const {

  const size_t max_n_threads = thread::cpu_count();

  std::shared_ptr<sarray<flexible_type> > ret(new sarray<flexible_type>);
  size_t num_segments = max_n_threads;

  ret->open_for_write(num_segments);
  ret->set_type(flex_type_enum::FLOAT);

  in_parallel([&](size_t thread_idx, size_t n_threads) {

      std::vector<v2::ml_data_entry> x;
      auto it_out = ret->get_output_iterator(thread_idx);

      for(auto it = test_data.get_iterator(thread_idx, n_threads); !it.done(); ++it, ++it_out) {

        it.fill_observation(x);

        *it_out = loss_model->translate_fx_to_prediction(calculate_fx(x));
      }
    });

  ret->close();

  sframe ret_sf = sframe(std::vector<std::shared_ptr<sarray<flexible_type> > >{ret},
                         std::vector<std::string>{"prediction"});

  return ret_sf;
}

}}

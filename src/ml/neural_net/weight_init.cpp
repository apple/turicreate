/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/weight_init.hpp>

#include <cmath>

namespace turi {
namespace neural_net {

namespace {

std::uniform_real_distribution<float> uniform_distribution_for_xavier(
    size_t fan_in, size_t fan_out)
{
  float magnitude = std::sqrt(3.f / (0.5f * fan_in + 0.5f * fan_out));
  return std::uniform_real_distribution<float>(-magnitude, magnitude);
}

}  // namespace

xavier_weight_initializer::xavier_weight_initializer(
    size_t fan_in, size_t fan_out, std::mt19937* random_engine)
    : dist_(uniform_distribution_for_xavier(fan_in, fan_out)),
      random_engine_(*random_engine)
{}

  void xavier_weight_initializer::operator()(float* first_weight,
                                           float* last_weight)
  {
    for (float* w = first_weight; w != last_weight; ++w) {
    *w = dist_(random_engine_);
    }
  }

  // static
  lstm_weight_initializers lstm_weight_initializers::create_with_xavier_method(
    size_t input_size, size_t state_size, std::mt19937* random_engine)
  {
  xavier_weight_initializer i2h_init_fn(input_size, state_size, random_engine);
  xavier_weight_initializer h2h_init_fn(state_size, state_size, random_engine);

  lstm_weight_initializers result;

  result.input_gate_weight_fn = i2h_init_fn;
  result.forget_gate_weight_fn = i2h_init_fn;
  result.block_input_weight_fn = i2h_init_fn;
  result.output_gate_weight_fn = i2h_init_fn;

  result.input_gate_recursion_fn = h2h_init_fn;
  result.forget_gate_recursion_fn = h2h_init_fn;
  result.block_input_recursion_fn = h2h_init_fn;
  result.output_gate_recursion_fn = h2h_init_fn;

  result.input_gate_bias_fn = zero_weight_initializer();
  result.forget_gate_bias_fn = zero_weight_initializer();
  result.block_input_bias_fn = zero_weight_initializer();
  result.output_gate_bias_fn = zero_weight_initializer();

  return result;
}

}  // neural_net
}  // turi

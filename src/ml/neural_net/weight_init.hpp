/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_WEIGHT_INIT_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_WEIGHT_INIT_HPP_

#include <functional>
#include <random>

namespace turi {
namespace neural_net {

/**
 * Callback type used to initialize an underlying WeightParams instance.
 *
 * The callback should write the desired values into the provided iterator
 * range, which is initialized to 0.f.
 */
using weight_initializer = std::function<void(float* first_weight,
                                              float* last_weight)>;

class xavier_weight_initializer {
public:

  /**
   * Creates a weight initializer that performs Xavier initialization
   *
   * \param fan_in The number of inputs that affect each output from the layer
   * \param fan_out The number of outputs affected by each input to the layer
   * \param random_engine The random number generator to use, which must remain
   *     valid for the lifetime of this instance.
   */
  xavier_weight_initializer(size_t fan_in, size_t fan_out,
                            std::mt19937* random_engine);

  /**
   * Initializes each value in uniformly at random in the range [-c,c], where
   * c = sqrt(3 / (0.5 * fan_in + 0.5 * fan_out).
   */
  void operator()(float* first_weight, float* last_weight);

private:

  std::uniform_real_distribution<float> dist_;
  std::mt19937& random_engine_;
};


class uniform_weight_initializer {
  public:

  /**
   * Creates a weight initializer that performs Uniform initialization
   *
   * \param lower_bound The lower bound of the uniform distribution to be sampled
   * \param upper_bound The upper bound of the uniform distribution to be sampled
   * \param random_engine The random number generator to use, which must remain
   *     valid for the lifetime of this instance.
   */
  uniform_weight_initializer(float lower_bound, float upper_bound,
                            std::mt19937* random_engine);

  /**
   * Initializes each value in uniformly at random in the range [-lower_bound, upper_bound]
   */
  void operator()(float* first_weight, float* last_weight);

private:

  std::uniform_real_distribution<float> dist_;
  std::mt19937& random_engine_;
};

struct scalar_weight_initializer {
  /**
   * Creates a weight initializer that initializes all of the weights to a
   * constant scalar value.
   * 
   * \param scalar The scalar value to initialize the weights to.
   */
  scalar_weight_initializer(float scalar);
  void operator()(float* first_weight, float* last_weight);

 private:
  float scalar_;
};

struct zero_weight_initializer {

  // No work is required, since we assume the buffer is zero-initialized.
  void operator()(float* first_weight, float* last_weight) const {}
};

/** Convenience struct to hold all the weight initializers required by LSTM */
struct lstm_weight_initializers {

  static lstm_weight_initializers create_with_xavier_method(
      size_t input_size, size_t state_size, std::mt19937* random_engine);
  static lstm_weight_initializers create_with_zero();

  // Initializers for matrices applied to sequence input
  weight_initializer input_gate_weight_fn;
  weight_initializer forget_gate_weight_fn;
  weight_initializer block_input_weight_fn;
  weight_initializer output_gate_weight_fn;

  // Initializers for matrices applied to hidden state
  weight_initializer input_gate_recursion_fn;
  weight_initializer forget_gate_recursion_fn;
  weight_initializer block_input_recursion_fn;
  weight_initializer output_gate_recursion_fn;

  // Initializers for bias
  weight_initializer input_gate_bias_fn;
  weight_initializer forget_gate_bias_fn;
  weight_initializer block_input_bias_fn;
  weight_initializer output_gate_bias_fn;
};

}  // neural_net
}  // turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_WEIGHT_INIT_HPP_

/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#include <string>
#include <unordered_map>

#include <ml/neural_net/float_array.hpp>

#import <MLCompute/MLCompute.h>

NS_ASSUME_NONNULL_BEGIN

namespace turi {
namespace neural_net {

class API_AVAILABLE(macos(10.16)) mlc_layer_weights {
 public:
  mlc_layer_weights();

  // Movable but not copyable
  mlc_layer_weights(const mlc_layer_weights &) = delete;
  mlc_layer_weights(mlc_layer_weights &&);
  mlc_layer_weights &operator=(const mlc_layer_weights &) = delete;
  mlc_layer_weights &operator=(mlc_layer_weights &&);

  ~mlc_layer_weights();

  /**
   * Returns views into the weights managed by this instance, represented as
   * tensors formatted according to the method used to add each name.
   */
  NSDictionary<NSString *, MLCTensor *> *tensor_views() const { return tensors_; }

  /**
   * Returns strong references to the memory backing the weights managed by this
   * instance. The return value uses the same keys as the dictionary returned by
   * `tensor_views()`.
   */
  NSDictionary<NSString *, NSData *> *tensor_weights() const;

  /**
   * Returns copies of all the weights managed by this instance. The caller is
   * responsible for ensuring that MLCompute is not modifying any of these
   * weights for the duration of this function call (and that MLCompute has
   * synchronized weights from GPU to this memory, if necessary).
   */
  float_array_map export_weights() const;

  /**
   * Returns copies of all the weights managed by this instance. It also copies the
   * optimizer data to the tensors to the weights map. The caller is responsible for
   * ensuring that MLCompute is not modifying any of these weights for the duration
   * of this function call (and that MLCompute has synchronized weights from GPU
   * to this memory, if necessary).
   */
  float_array_map export_weights_and_optimizer_data() const;

  /**
   * Imports the kernel weights for a convolution layer. The input must have
   * shape OIHW.
   */
  void add_conv_weight(const std::string &name, const float_array &arr);

  /**
   * Imports the bias weights for a convolution layer. The input must be
   * one-dimensional.
   */
  void add_conv_bias(const std::string &name, const float_array &arr);

  /**
   * Imports one-dimensional weights, such as those for batch-normalization
   * layers.
   */
  void add_flat_array(const std::string &name, const float_array &arr);

  /**
   * Imports one weight matrix to be passed to an MLCompute LSTM layer. The
   * input must be two-dimensional.
   */
  void add_lstm_weight(const std::string &name, const float_array &arr);

  /**
   * Adds the optimizer data to the saved tensors.
   */
  void add_optimizer_data(const std::string &layer_name, const std::string &optimizer_data_1_key,
                          const float_array &optimizer_data_1,
                          const std::string &optimizer_data_2_key,
                          const float_array &optimizer_data_2);

 private:
  /**
   * Copies arr and stores a strong reference to it with the given name. Returns
   * a weak pointer to the copied memory to pass to MLCompute, which will write
   * updated (trained) weights back into the same memory. (Yes, MLCompute
   * ignores the fact that the tensor data is initialized with a const pointer.)
   * */
  MLCTensorData *copy_float_array(const std::string &name, const float_array &arr);

  /**
   * Strong references to the memory that MLCompute will use to read the initial
   * weights and to pass back the updated weights. Although this data structure
   * uses the shared_float_array type, the memory here should not actually be
   * shared with clients, since in general MLCompute might be modifying it
   * asynchronously.
   * */
  std::unordered_map<std::string, shared_float_array> weights_;

  /**
   * Collection of MLCompute tensors that wrap the data owned by weights_ above.
   */
  NSMutableDictionary<NSString *, MLCTensor *> *tensors_ = nil;
};

}  // namespace neural_net
}  // namespace turi

NS_ASSUME_NONNULL_END

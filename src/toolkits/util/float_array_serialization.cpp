/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/util/float_array_serialization.hpp>

namespace turi {

using neural_net::float_array_map;
using neural_net::shared_float_array;

class float_array_serialization_wrapper {
 public:
  float_array_serialization_wrapper() = default;
  explicit float_array_serialization_wrapper(shared_float_array array)
      : impl_(std::move(array)) {}

  const shared_float_array& get() const { return impl_; }

  void save(oarchive& oarc) const {
    // Write shape.
    serialize_iterator(oarc, impl_.shape(), impl_.shape() + impl_.dim(),
                       impl_.dim());

    // Write data.
    serialize_iterator(oarc, impl_.data(), impl_.data() + impl_.size(),
                       impl_.size());
  }

  void load(iarchive& iarc) {
    // Read shape.
    std::vector<size_t> shape;
    iarc >> shape;

    // Read data.
    std::vector<float> data;
    iarc >> data;

    // Overwrite self with a new float_array wrapping the deserialized data.
    impl_ = shared_float_array::wrap(std::move(data), std::move(shape));
  }

 private:
  shared_float_array impl_;
};

void save_float_array_map(const float_array_map& weights, oarchive& oarc) {
  // Wrap each shared_float_array in weights in a wrapper that knows how to
  // write itself to the oarchive.
  std::map<std::string, float_array_serialization_wrapper> wrapped_weights;
  for (const auto& key_value : weights) {
    wrapped_weights[key_value.first] =
        float_array_serialization_wrapper(key_value.second);
  }

  oarc << wrapped_weights;
}

float_array_map load_float_array_map(iarchive& iarc) {
  // Read the iarchive into wrappers around the underlying weights.
  std::map<std::string, float_array_serialization_wrapper> wrapped_weights;
  iarc >> wrapped_weights;

  // Obtain direct references to the underlying weights.
  float_array_map weights;
  for (const auto& key_value : wrapped_weights) {
    weights[key_value.first] = key_value.second.get();
  }

  return weights;
}

}  // namespace turi

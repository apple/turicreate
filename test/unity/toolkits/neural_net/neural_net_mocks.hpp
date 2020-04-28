/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TEST_UNITY_TOOLKITS_NEURAL_NET_NEURAL_NET_MOCKS_HPP_
#define TEST_UNITY_TOOLKITS_NEURAL_NET_NEURAL_NET_MOCKS_HPP_

#include <deque>
#include <functional>
#include <memory>

#include <core/util/test_macros.hpp>
#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/image_augmentation.hpp>
#include <ml/neural_net/model_backend.hpp>

namespace turi {
namespace neural_net {

using turi::neural_net::compute_context;
using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::image_annotation;
using turi::neural_net::image_augmenter;
using turi::neural_net::labeled_image;
using turi::neural_net::model_backend;
using turi::neural_net::shared_float_array;

// TODO: Adopt a real mocking library. Or at least factor out the shared
// boilerplate into some utility templates or macros. Yes, if necessary, create
// our own simplistic mocking tools.

class mock_image_augmenter : public image_augmenter {
 public:
  using prepare_images_call =
      std::function<result(std::vector<labeled_image> source_batch)>;

  ~mock_image_augmenter() { TS_ASSERT(prepare_images_calls_.empty()); }

  const options& get_options() const override { return options_; }

  result prepare_images(std::vector<labeled_image> source_batch) override {
    TS_ASSERT(!prepare_images_calls_.empty());

    prepare_images_call expected_call =
        std::move(prepare_images_calls_.front());

    prepare_images_calls_.pop_front();

    return expected_call(std::move(source_batch));
  }

  options options_;
  std::deque<prepare_images_call> prepare_images_calls_;
};

class mock_model_backend : public model_backend {
 public:
  using set_learning_rate_call = std::function<void(float lr)>;

  using train_call =
      std::function<float_array_map(const float_array_map& inputs)>;

  using predict_call =
      std::function<float_array_map(const float_array_map& inputs)>;

  ~mock_model_backend() {
    TS_ASSERT(train_calls_.empty());
    TS_ASSERT(predict_calls_.empty());
  }

  void set_learning_rate(float lr) override {
    TS_ASSERT(!set_learning_rate_calls_.empty());
    set_learning_rate_call expected_call =
        std::move(set_learning_rate_calls_.front());
    set_learning_rate_calls_.pop_front();
    expected_call(lr);
  }

  float_array_map train(const float_array_map& inputs) override {
    TS_ASSERT(!train_calls_.empty());
    train_call expected_call = std::move(train_calls_.front());
    train_calls_.pop_front();
    return expected_call(inputs);
  }

  float_array_map predict(const float_array_map& inputs) const override {
    TS_ASSERT(!predict_calls_.empty());
    predict_call expected_call = std::move(predict_calls_.front());
    predict_calls_.pop_front();
    return expected_call(inputs);
  }

  float_array_map export_weights() const override {
    return export_weights_retval_;
  }

  std::deque<set_learning_rate_call> set_learning_rate_calls_;
  std::deque<train_call> train_calls_;
  mutable std::deque<predict_call> predict_calls_;
  float_array_map export_weights_retval_;
};

class mock_compute_context : public compute_context {
 public:
  using create_augmenter_call = std::function<std::unique_ptr<image_augmenter>(
      const image_augmenter::options& opts)>;

  using create_object_detector_call =
      std::function<std::unique_ptr<model_backend>(
          int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
          const float_array_map& config, const float_array_map& weights)>;

 /**
  * TODO: const float_array_map& weights,
  *       const float_array_map& config.
  * Until the nn_spec in C++ is ready, do not pass in any weights.
  */
  using create_drawing_classifier_call =
      std::function<std::unique_ptr<model_backend>(
          /* TODO: const float_array_map& config if needed */
          const float_array_map& weights, size_t batch_size,
          size_t num_classes)>;

  ~mock_compute_context() {
    TS_ASSERT(create_augmenter_calls_.empty());
    TS_ASSERT(create_object_detector_calls_.empty());
    TS_ASSERT(create_drawing_classifier_calls_.empty());
  }

  size_t memory_budget() const override { return 0; }

  void print_training_device_info() const override { }

  std::unique_ptr<image_augmenter> create_image_augmenter(
      const image_augmenter::options& opts) override {
    TS_ASSERT(!create_augmenter_calls_.empty());

    create_augmenter_call expected_call =
        std::move(create_augmenter_calls_.front());

    create_augmenter_calls_.pop_front();

    return expected_call(opts);
  }

  std::unique_ptr<model_backend> create_object_detector(
      int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
      const float_array_map& config, const float_array_map& weights) override {
    TS_ASSERT(!create_object_detector_calls_.empty());

    create_object_detector_call expected_call =
        std::move(create_object_detector_calls_.front());

    create_object_detector_calls_.pop_front();

    return expected_call(n, c_in, h_in, w_in, c_out, h_out, w_out, config,
                         weights);
  }

  /**
   * TODO: const float_array_map& weights, const float_array_map& config.
   * Until the nn_spec in C++ isn't ready, do not pass in any weights.
   */
  std::unique_ptr<model_backend> create_drawing_classifier(
      /* TODO: const float_array_map& config if needed */
      const float_array_map& weights,
      size_t batch_size, size_t num_classes) override {
    TS_ASSERT(!create_drawing_classifier_calls_.empty());

    create_drawing_classifier_call expected_call =
        std::move(create_drawing_classifier_calls_.front());

    create_drawing_classifier_calls_.pop_front();
    return expected_call(
      /* TODO: const float_array_map& config if needed */
      weights, batch_size, num_classes);
  }

  std::unique_ptr<model_backend> create_activity_classifier(
      const ac_parameters& ac_params) override {
    throw std::runtime_error("create_activity_classifier not implemented");
  }

  std::unique_ptr<model_backend> create_style_transfer(
      const float_array_map& config, const float_array_map& weights) override {
    throw std::runtime_error("create_style_transfer not implemented");
  }

  mutable std::deque<create_augmenter_call> create_augmenter_calls_;
  mutable std::deque<create_object_detector_call> create_object_detector_calls_;
  mutable std::deque<create_drawing_classifier_call>
      create_drawing_classifier_calls_;
};

}  // namespace neural_net
}  // namespace turi

#endif  // TEST_UNITY_TOOLKITS_NEURAL_NET_NEURAL_NET_MOCKS_HPP_

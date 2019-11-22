/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <boost/test/unit_test.hpp>
#include <model_server/lib/image_util.hpp>
#include <core/util/test_macros.hpp>
#include <toolkits/drawing_classifier/dc_data_iterator.hpp>
#include <toolkits/drawing_classifier/drawing_classifier.hpp>

#include "../neural_net/neural_net_mocks.hpp"

namespace turi {
namespace drawing_classifier {

namespace {
using neural_net::compute_context;
using neural_net::model_spec;
using neural_net::model_backend;
};

/**
 * First, define mock implementations of the key drawing_classifier
 * dependencies. These implementations allow the test to define a callback for
 * each call to these classes' method, to make assertions on the inputs and to
 * provide canned outputs. The production implementations should have their own
 * separate unit tests.
 */

/**
 * TODO: Adopt a real mocking library. Or at least factor out the shared
 * boilerplate into some utility templates or macros. Yes, if necessary, create
 * our own simplistic mocking tools.
 */

class mock_data_iterator : public data_iterator {
 public:
  using has_next_batch_call = std::function<bool()>;
  using next_batch_call =
      std::function<data_iterator::batch(size_t batch_size)>;
  using reset_call = std::function<void()>;

  ~mock_data_iterator() {
    TS_ASSERT(has_next_batch_calls_.empty());
    TS_ASSERT(next_batch_calls_.empty());
    TS_ASSERT(reset_calls_.empty());
  }

  data_iterator::batch next_batch(size_t batch_size) override {
    TS_ASSERT(!next_batch_calls_.empty());
    next_batch_call expected_call = std::move(next_batch_calls_.front());
    next_batch_calls_.pop_front();
    return expected_call(batch_size);
  }

  void reset() override {
    TS_ASSERT(!reset_calls_.empty());
    reset_call expected_call = std::move(reset_calls_.front());
    reset_calls_.pop_front();
    return expected_call();
  }

  bool has_next_batch() override {
    TS_ASSERT(!has_next_batch_calls_.empty());
    has_next_batch_call expected_call =
        std::move(has_next_batch_calls_.front());
    has_next_batch_calls_.pop_front();
    return expected_call();
  }

  const flex_list& class_labels() const override {
    return class_labels_;
  }

  std::deque<has_next_batch_call> has_next_batch_calls_;
  std::deque<next_batch_call> next_batch_calls_;
  std::deque<reset_call> reset_calls_;
  flex_list class_labels_;
};

/**
 * Subclass of drawing classifier that mocks out the methods that inject the
 * drawing classifier dependencies.
 */
class test_drawing_classifier : public drawing_classifier {
 public:
  using create_iterator_call = std::function<std::unique_ptr<data_iterator>(
      data_iterator::parameters iterator_params)>;
  using create_compute_context_call =
      std::function<std::unique_ptr<compute_context>()>;

  using init_model_call = std::function<std::unique_ptr<model_spec>()>;

  // init default model spec to bypass init_train
  test_drawing_classifier()
      : drawing_classifier({}, std::unique_ptr<model_spec>(new model_spec()),
                           nullptr, nullptr, nullptr) {}

  test_drawing_classifier(
      const std::map<std::string, variant_type>& initial_state,
      std::unique_ptr<model_spec> nn_spec,
      std::unique_ptr<compute_context> training_compute_context,
      std::unique_ptr<data_iterator> training_data_iterator,
      std::unique_ptr<model_backend> training_model)
      : drawing_classifier(initial_state, std::move(nn_spec),
                           std::move(training_compute_context),
                           std::move(training_data_iterator),
                           std::move(training_model)) {}

  ~test_drawing_classifier() {
    TS_ASSERT(create_iterator_calls_.empty());
    TS_ASSERT(create_compute_context_calls_.empty());
    TS_ASSERT(init_model_calls_.empty());
  }

  std::unique_ptr<data_iterator> create_iterator(
      data_iterator::parameters iterator_params) const override {
    TS_ASSERT(!create_iterator_calls_.empty());
    create_iterator_call expected_call =
        std::move(create_iterator_calls_.front());
    create_iterator_calls_.pop_front();
    return expected_call(iterator_params);
  }

  std::unique_ptr<compute_context> create_compute_context() const override {
    TS_ASSERT(!create_compute_context_calls_.empty());
    create_compute_context_call expected_call =
        std::move(create_compute_context_calls_.front());
    create_compute_context_calls_.pop_front();
    return expected_call();
  }

  std::unique_ptr<model_spec> init_model(bool use_random_init) const override {
    TS_ASSERT(!init_model_calls_.empty());
    init_model_call expected_call = std::move(init_model_calls_.front());
    init_model_calls_.pop_front();
    return expected_call();
  }

  template <class T>
  T get_field(const std::string& name) {
    return variant_get_value<T>(get_value_from_state(name));
  }

  mutable std::deque<create_iterator_call> create_iterator_calls_;
  mutable std::deque<create_compute_context_call> create_compute_context_calls_;
  mutable std::deque<init_model_call> init_model_calls_;
};

};  // namespace drawing_classifier
};  // namespace turi

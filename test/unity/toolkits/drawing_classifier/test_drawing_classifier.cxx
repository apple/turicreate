/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_drawing_classifier

#include <toolkits/drawing_classifier/drawing_classifier.hpp>

#include <algorithm>

#include <boost/test/unit_test.hpp>
#include <model_server/lib/image_util.hpp>
#include <core/util/test_macros.hpp>

#include "../neural_net/neural_net_mocks.hpp"
#include "data_utils.hpp"

namespace turi {
namespace drawing_classifier {

namespace {

using turi::neural_net::compute_context;
using turi::neural_net::mock_compute_context;
using turi::neural_net::mock_model_backend;
using turi::neural_net::model_backend;
using turi::neural_net::model_spec;

// First, define mock implementations of the key object_detector dependencies.
// These implementations allow the test to define a callback for each call to
// these classes' method, to make assertions on the inputs and to provide
// canned outputs. The production implementations should have their own
// separate unit tests.

// TODO: Adopt a real mocking library. Or at least factor out the shared
// boilerplate into some utility templates or macros. Yes, if necessary, create
// our own simplistic mocking tools.

class mock_data_iterator: public data_iterator {
 public:
  using next_batch_call =
      std::function<data_iterator::batch(size_t batch_size)>;
  using reset_call = std::function<void()>;

  ~mock_data_iterator() {
    TS_ASSERT(next_batch_calls_.empty());
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

  bool has_next_batch() const override {
    return true;
  }

  const std::unordered_map<std::string, int>& class_to_index_map() const override {
    return class_to_index_map_;
  }

  const std::vector<std::string>& class_labels() const override {
    return class_labels_;
  }

  std::deque<next_batch_call> next_batch_calls_;
  std::deque<reset_call> reset_calls_;
  std::vector<std::string> class_labels_;
  std::unordered_map<std::string, int> class_to_index_map_;
};

// Subclass of object_detector that mocks out the methods that inject the
// object_detector dependencies.
class test_drawing_classifier: public drawing_classifier {
 public:
  using create_iterator_call =
      std::function<std::unique_ptr<data_iterator>(
        data_iterator::parameters iterator_params)>;

  using create_compute_context_call =
      std::function<std::unique_ptr<compute_context>()>;

  using init_model_call = std::function<std::unique_ptr<model_spec>(
      bool use_random_init)>;

  test_drawing_classifier() = default;
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

  std::unique_ptr<model_spec> init_model(
      bool use_random_init) const override {
    TS_ASSERT(!init_model_calls_.empty());
    init_model_call expected_call = std::move(init_model_calls_.front());
    init_model_calls_.pop_front();
    return expected_call(use_random_init);
  }

  template <class T>
  T get_field(const std::string& name) {
    return variant_get_value<T>(get_value_from_state(name));
  }

  mutable std::deque<create_iterator_call> create_iterator_calls_;
  mutable std::deque<create_compute_context_call> create_compute_context_calls_;
  mutable std::deque<init_model_call> init_model_calls_;
};

BOOST_AUTO_TEST_CASE(test_drawing_classifier_init_train) {
  // Most of this test body will be spent setting up the mock objects that we'll
  // inject into the object_detector implementation. These mock objects will
  // make assertions about their inputs along the way and provide the outputs
  // that we manually pre-program. At the end will be a single call to
  // object_detector::init_training that will trigger all the actual testing.
  test_drawing_classifier model;

  // Allocate the mock dependencies. We'll transfer ownership when the toolkit
  // code attempts to instantiate these dependencies.
  std::unique_ptr<mock_data_iterator> mock_iterator(new mock_data_iterator);
  std::unique_ptr<mock_model_backend> mock_nn_model(new mock_model_backend);
  std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

  // We'll request 4 training iterations, since the learning rate schedule
  // kicks in at the 50% and 75% points.
  static constexpr size_t test_max_iterations = 4;
  static constexpr size_t test_batch_size = 2;
  const std::vector<std::string> test_class_labels = {"label1", "label2"};
  static constexpr size_t test_num_rows = 100;

  mock_iterator->class_labels_ = test_class_labels;
  
  const std::string test_target_name = "test_target";
  const std::string test_image_name = "test_image";

  // The following callbacks capture by reference so that they can transfer
  // ownership of the mocks created above.
  auto create_iterator_impl = [&](data_iterator::parameters iterator_params) {
    TS_ASSERT(iterator_params.class_labels
                  .empty());  // Should infer class labels from data.
    TS_ASSERT(iterator_params.repeat);

    return std::move(mock_iterator);
  };

  model.create_iterator_calls_.push_back(create_iterator_impl);

  model.init_model_calls_.emplace_back([=](bool use_random_init) {

    std::unique_ptr<model_spec> nn_spec(new model_spec);
    nn_spec->add_convolution("test_layer", "test_input", 16, 16, 3, 3, 1, 1,
                             model_spec::padding_type::SAME,
                             /* weight_init_fn */ [](float* w, float* w_end) {
                               for (int i = 0; i < w_end - w; ++i) {
                                 w[i] = static_cast<float>(i);
                               }
                             });
    return nn_spec;
  });

  auto create_drawing_classifier_impl =
      [&](/* TODO: const float_array_map& weights, const float_array_map& config.
           * Until the nn_spec in C++ isn't ready, do not pass in any weights.
           */
          size_t batch_size, size_t num_classes) {
        TS_ASSERT_EQUALS(batch_size, test_batch_size);
        TS_ASSERT_EQUALS(num_classes, test_class_labels.size());
        
        /* TODO: Uncomment when we start passing weights around */
        // weights should be what we returned from init_model, as copied by
        // neural_net::wrap_network_params
        // TS_ASSERT_EQUALS(weights.size(), 1);
        // auto it = weights.find("test_layer_weight");
        // TS_ASSERT(it != weights.end());
        // for (size_t i = 0; i < it->second.size(); ++i) {
        //   TS_ASSERT_EQUALS(it->second.data()[i], static_cast<float>(i));
        // }

        // TODO: Assert the config values?

        return std::move(mock_nn_model);
      };
  mock_context->create_drawing_classifier_calls_.push_back(
      create_drawing_classifier_impl);

  auto create_compute_context_impl = [&] { return std::move(mock_context); };
  model.create_compute_context_calls_.push_back(create_compute_context_impl);

  // Create an arbitrary SFrame with test_num_rows rows.
  drawing_data_generator data_generator(test_num_rows, test_class_labels);
  gl_sframe data = data_generator.get_data();
  
  // Now, actually invoke drawing_classifier::init_train. This will trigger all
  // the assertions registered above.
  model.init_train(data, test_target_name, test_image_name, gl_sframe(),
                      {
                          {"batch_size", test_batch_size},
                          {"max_iterations", test_max_iterations},
                      });

  // Verify model fields.
  TS_ASSERT_EQUALS(model.get_field<flex_int>("batch_size"), test_batch_size);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("max_iterations"),
                   test_max_iterations);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("target"),
                   test_target_name);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("feature"), test_image_name);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("num_classes"),
                   test_class_labels.size());
  TS_ASSERT_EQUALS(model.get_field<flex_int>("training_iterations"), 0);

  // Deconstructing `model` here will assert that every expected call to a
  // mocked-out method has been called.
}

// BOOST_AUTO_TEST_CASE(test_drawing_classifier_perform_training_iteration) {
//   // Most of this test body will be spent setting up the mock objects that we'll
//   // inject into the object_detector implementation. These mock objects will
//   // make assertions about their inputs along the way and provide the outputs
//   // that we manually pre-program. At the end will be the calls to
//   // object_detector::iterate_training that will trigger all the actual testing.

//   // Allocate the mock dependencies. We'll transfer ownership when the toolkit
//   // code attempts to instantiate these dependencies.
//   std::unique_ptr<mock_data_iterator> mock_iterator(new mock_data_iterator);
//   std::unique_ptr<mock_image_augmenter> mock_augmenter(
//       new mock_image_augmenter);
//   std::unique_ptr<mock_model_backend> mock_nn_model(new mock_model_backend);
//   std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

//   // We'll request 4 training iterations, since the learning rate schedule
//   // kicks in at the 50% and 75% points.
//   static constexpr size_t test_max_iterations = 4;
//   static constexpr size_t test_batch_size = 2;
//   const std::vector<std::string> test_class_labels = {"label1", "label2"};
//   static constexpr size_t test_num_rows = 100;
//   static constexpr float test_loss = 5.f;

//   mock_iterator->class_labels_ = test_class_labels;
  
//   auto num_iterations_submitted = std::make_shared<size_t>(0);
//   for (size_t i = 0; i < test_max_iterations; ++i) {
//     // Program the mock_iterator to return two arbitrary images, each with one
//     // unique annotation. We'll store a copy of the annotations for later
//     // comparison.
//     auto test_annotations =
//         std::make_shared<std::vector<std::vector<image_annotation>>>();
//     auto next_batch_impl = [=](size_t batch_size) {
//       TS_ASSERT_EQUALS(batch_size, test_batch_size);

//       std::vector<labeled_image> result(test_batch_size);
//       for (size_t j = 0; j < result.size(); ++j) {
//         // The actual contents of the image and the annotations are irrelevant
//         // for the purposes of this test. But encode the batch index and row
//         // index into the bounding box so that we can verify this data is passed
//         // into the image augmenter.
//         image_annotation annotation;
//         annotation.bounding_box.x = i;
//         annotation.bounding_box.y = j;

//         result[j].annotations.push_back(annotation);
//         test_annotations->push_back(result[j].annotations);
//       }

//       return result;
//     };
//     mock_iterator->next_batch_calls_.push_back(next_batch_impl);

//     // The mock_model_backend should expect calls to set_learning_rate just at
//     // the 50% and 75% marks.
//     if (i == test_max_iterations / 2 || i == test_max_iterations * 3 / 4) {
//       auto set_learning_rate_impl = [=](float lr) {
//         TS_ASSERT_EQUALS(*num_iterations_submitted, i);
//       };
//       mock_nn_model->set_learning_rate_calls_.push_back(set_learning_rate_impl);
//     }

//     // The mock_model_backend should expect `train` calls on every iteration.
//     auto train_impl = [=](const float_array_map& inputs) {
//       // The input_batch should just be whatever the image_augmenter returned.
//       shared_float_array input_batch = inputs.at("input");
//       TS_ASSERT_EQUALS(input_batch.data(), test_image_batch->data());

//       // Track how many calls we've had.
//       *num_iterations_submitted += 1;

//       // Multiply loss by 8 to offset the "mps_loss_mult" factor currently
//       // hardwired in to avoid fp16 underflow in MPS.
//       std::map<std::string, shared_float_array> result;
//       result["loss"] = shared_float_array::wrap(8 * test_loss);
//       return result;
//     };
//     mock_nn_model->train_calls_.push_back(train_impl);
//   }

//   test_drawing_classifier model(
//       {{"batch_size", test_batch_size},
//        {"max_iterations", 4},
//        {"num_examples", test_num_examples},
//        {"training_iterations", 0}},
//       nullptr, std::move(mock_context), std::move(mock_iterator),
//       std::move(mock_augmenter), std::move(mock_nn_model));

//   // Now, actually invoke object_detector::iterate_training. This will trigger
//   // all the assertions registered above.
//   for (size_t i = 0; i < test_max_iterations; ++i) {
//     model.perform_training_iteration();
//   }

//   TS_ASSERT_EQUALS(model.get_field<flex_int>("training_iterations"),
//                    test_max_iterations);
//   TS_ASSERT_EQUALS(model.get_field<flex_int>("training_epochs"),
//                    test_max_iterations * test_batch_size / test_num_examples);

//   // Deconstructing `model` here will assert that every expected call to a
//   // mocked-out method has been called.

// }


}  // namespace
}  // drawing_classifier
}  // turi

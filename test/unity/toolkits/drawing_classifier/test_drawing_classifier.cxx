/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_drawing_classifier

#include <toolkits/drawing_classifier/drawing_classifier.hpp>

#include <algorithm>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <model_server/lib/image_util.hpp>

#include "../neural_net/neural_net_mocks.hpp"
#include "dc_data_utils.hpp"
#include "dc_mock_interface.hpp"

namespace turi {
namespace drawing_classifier {

namespace {

using turi::neural_net::compute_context;
using turi::neural_net::float_array_map;
using turi::neural_net::mock_compute_context;
using turi::neural_net::mock_model_backend;
using turi::neural_net::model_backend;
using turi::neural_net::model_spec;
using turi::neural_net::shared_float_array;


void test_init_training(bool with_bitmap_based_data) {
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
  const flex_list test_class_labels = {"label1", "label2"};
  static constexpr size_t test_num_rows = 100;

  mock_iterator->class_labels_ = test_class_labels;

  const std::string test_target_name = "test_target";
  const std::string test_feature_name = "test_feature";

  // The following callbacks capture by reference so that they can transfer
  // ownership of the mocks created above.

  auto create_iterator_impl = [&](data_iterator::parameters iterator_params) {
    // Should infer class labels from data.
    TS_ASSERT(iterator_params.class_labels.empty());
    TS_ASSERT(iterator_params.repeat);
    TS_ASSERT_EQUALS(iterator_params.feature_column_name, test_feature_name);

    gl_sframe data = iterator_params.data;
    TS_ASSERT(data.contains_column(iterator_params.feature_column_name));
    TS_ASSERT_EQUALS(data[iterator_params.feature_column_name].dtype(),
                     flex_type_enum::IMAGE);

    return std::move(mock_iterator);
  };

  model.create_iterator_calls_.push_back(create_iterator_impl);

  model.init_model_calls_.emplace_back([=]() {
    std::unique_ptr<model_spec> nn_spec(new model_spec);
    auto weight_init_fn = [](float* w, float* w_end) {
      for (int i = 0; i < w_end - w; ++i) {
        w[i] = static_cast<float>(i);
      }
    };
    nn_spec->add_convolution("test_layer", "test_input", 16, 16, 3, 3, 1, 1,
                             model_spec::padding_type::SAME, weight_init_fn);
    return nn_spec;
  });

  auto create_drawing_classifier_impl = [&](const float_array_map& weights,
                                            size_t batch_size,
                                            size_t num_classes) {
    TS_ASSERT_EQUALS(batch_size, test_batch_size);
    TS_ASSERT_EQUALS(num_classes, test_class_labels.size());

    // weights should be what we returned from init_model, as copied by
    // neural_net::wrap_network_params
    TS_ASSERT_EQUALS(weights.size(), 1);
    auto it = weights.find("test_layer_weight");
    TS_ASSERT(it != weights.end());
    for (size_t i = 0; i < it->second.size(); ++i) {
      TS_ASSERT_EQUALS(it->second.data()[i], static_cast<float>(i));
    }

    // TODO: Assert the config values?

    return std::move(mock_nn_model);
  };

  mock_context->create_drawing_classifier_calls_.push_back(
      create_drawing_classifier_impl);

  auto create_compute_context_impl = [&] { return std::move(mock_context); };
  model.create_compute_context_calls_.push_back(create_compute_context_impl);

  // Create an arbitrary SFrame with test_num_rows rows.
  drawing_data_generator data_generator(
    /* is_bitmap_based     */ with_bitmap_based_data,
    /* num_rows            */ test_num_rows,
    /* class_labels        */ test_class_labels,
    /* target_column_name  */ test_target_name, 
    /* feature_column_name */ test_feature_name);
  
  gl_sframe data = data_generator.get_data();
  TS_ASSERT_EQUALS(data.size(), test_num_rows);

  std::string feature_column_name = data_generator.get_feature_column_name();
  std::string target_column_name = data_generator.get_target_column_name();
  TS_ASSERT_EQUALS(feature_column_name, test_feature_name);
  TS_ASSERT_EQUALS(target_column_name, test_target_name);

  
  if (!with_bitmap_based_data) {
    TS_ASSERT_EQUALS(data[feature_column_name].dtype(), flex_type_enum::LIST);
  }

  // Now, actually invoke drawing_classifier::init_training. This will trigger
  // all the assertions registered above.
  model.init_training(data, test_target_name, test_feature_name, gl_sframe(),
                      {
                          {"batch_size", test_batch_size},
                          {"max_iterations", test_max_iterations},
                      });

  // Verify model fields.
  TS_ASSERT_EQUALS(model.get_field<flex_int>("batch_size"), test_batch_size);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("max_iterations"),
                   test_max_iterations);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("target"), test_target_name);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("feature"), test_feature_name);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("num_classes"),
                   test_class_labels.size());
  TS_ASSERT_EQUALS(model.get_field<flex_int>("training_iterations"), 0);

  // Deconstructing `model` here will assert that every expected call to a
  // mocked-out method has been called.
}

/**
 * Most of this test body will be spent setting up the mock objects that we'll
 * inject into the drawing_classifier implementation. These mock objects will
 * make assertions about their inputs along the way and provide the outputs
 * that we manually pre-program. At the end will be a single call to
 * drawing_classifier::init_training that will trigger all the actual testing.
 */
BOOST_AUTO_TEST_CASE(test_drawing_classifier_init_training) {
  test_init_training(/* with_bitmap_based_data */ true);
}

BOOST_AUTO_TEST_CASE(test_init_training_with_stroke_based_conversion) {
  test_init_training(/* with_bitmap_based_data */ false);
}

/**
 * Most of this test body will be spent setting up the mock objects that we'll
 * inject into the drawing_classifier implementation. These mock objects will
 * make assertions about their inputs along the way and provide the outputs
 * that we manually pre-program. At the end will be a single call to
 * drawing_classifier::init_training that will trigger all the actual testing.
 */
BOOST_AUTO_TEST_CASE(test_drawing_classifier_iterate_training) {
  // Allocate the mock dependencies. We'll transfer ownership when the toolkit
  // code attempts to instantiate these dependencies.
  std::unique_ptr<mock_data_iterator> mock_iterator(new mock_data_iterator);
  std::unique_ptr<mock_model_backend> mock_nn_model(new mock_model_backend);
  std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

  // We'll request 4 training iterations, since the learning rate schedule
  // kicks in at the 50% and 75% points.
  static constexpr size_t test_max_iterations = 4;
  static constexpr size_t test_batch_size = 2;
  const flex_list test_class_labels = {"label1", "label2"};
  // static constexpr size_t test_num_rows = 100;
  static constexpr float test_loss = 5.f;

  mock_iterator->class_labels_ = test_class_labels;

  auto num_iterations_submitted = std::make_shared<size_t>(0);
  for (size_t i = 0; i < test_max_iterations; ++i) {
    // Program the mock_iterator to return nothing.
    auto next_batch_impl = [=](size_t batch_size) {
      TS_ASSERT_EQUALS(batch_size, test_batch_size);

      data_iterator::batch result;
      result.num_samples = batch_size;

      return result;
    };
    mock_iterator->next_batch_calls_.push_back(next_batch_impl);

    // Since has_next_batch is the loop guard in iterate_training,
    // it will be called twice, and we need to push two implementations,
    // one that returns true, and one that returns false.
    auto has_next_batch_true_impl = [=]() { return true; };

    auto has_next_batch_false_impl = [=]() { return false; };

    mock_iterator->has_next_batch_calls_.push_back(has_next_batch_true_impl);
    mock_iterator->has_next_batch_calls_.push_back(has_next_batch_false_impl);

    auto reset_impl = [=]() { return; };
    mock_iterator->reset_calls_.push_back(reset_impl);

    // The mock_model_backend should expect calls to set_learning_rate just at
    // the 50% and 75% marks.
    if (i == test_max_iterations / 2 || i == test_max_iterations * 3 / 4) {
      auto set_learning_rate_impl = [=](float lr) {
        TS_ASSERT_EQUALS(*num_iterations_submitted, i);
      };
      mock_nn_model->set_learning_rate_calls_.push_back(set_learning_rate_impl);
    }

    // The mock_model_backend should expect `train` calls on every iteration.
    auto train_impl = [=](const float_array_map& inputs) {
      // shared_float_array input_batch = inputs.at("input");
      // TS_ASSERT_EQUALS(input_batch.data(), test_image_batch->data());

      // Track how many calls we've had.
      *num_iterations_submitted += 1;

      // Multiply loss by 8 to offset the "mps_loss_mult" factor currently
      // hardwired in to avoid fp16 underflow in MPS.
      neural_net::float_array_map result;

      result["loss"] = shared_float_array::wrap(8 * test_loss);
      result["output"] = shared_float_array::wrap(.5);

      return result;
    };

    mock_nn_model->train_calls_.push_back(train_impl);
  }

  test_drawing_classifier model({{"batch_size", test_batch_size},
                                 {"max_iterations", test_max_iterations},
                                 {"num_classes", test_class_labels.size()},
                                 {"training_iterations", 0}},
                                nullptr, std::move(mock_context),
                                std::move(mock_iterator),
                                std::move(mock_nn_model));

  // Now, actually invoke drawing_classifier::iterate_training.
  // This will trigger all the assertions registered above.
  for (size_t i = 0; i < test_max_iterations; ++i) {
    model.iterate_training(false);
  }

  TS_ASSERT_EQUALS(model.get_field<flex_int>("training_iterations"),
                   test_max_iterations);

  // Deconstructing `model` here will assert that every expected call to a
  // mocked-out method has been called.
}

}  // namespace
}  // namespace drawing_classifier
}  // namespace turi

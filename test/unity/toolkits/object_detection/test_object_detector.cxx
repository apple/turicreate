/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_object_detector

#include <toolkits/object_detection/object_detector.hpp>

#include <array>
#include <deque>
#include <memory>
#include <numeric>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#include <toolkits/object_detection/od_darknet_yolo_model_trainer.hpp>

#include "../neural_net/neural_net_mocks.hpp"

namespace turi {
namespace object_detection {
namespace {

using CoreML::Specification::NeuralNetwork;
using turi::neural_net::compute_context;
using turi::neural_net::deferred_float_array;
using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::image_annotation;
using turi::neural_net::image_augmenter;
using turi::neural_net::labeled_image;
using turi::neural_net::mock_compute_context;
using turi::neural_net::mock_image_augmenter;
using turi::neural_net::mock_model_backend;
using turi::neural_net::model_backend;
using turi::neural_net::model_spec;
using turi::neural_net::shared_float_array;

// First, define mock implementations of the key object_detector dependencies.
// These implementations allow the test to define a callback for each call to
// these classes' method, to make assertions on the inputs and to provide
// canned outputs. The production implementations should have their own
// separate unit tests.

// TODO: Adopt a real mocking library. Or at least factor out the shared
// boilerplate into some utility templates or macros. Yes, if necessary, create
// our own simplistic mocking tools.

std::map<std::string, variant_type> get_basic_state() {
  return {{"annotation_origin", "top_left"},
          {"annotation_position", "center"},
          {"annotation_scale", "pixel"},
          {"annotations", "annotations"},
          {"feature", "feature"},
          {"grid_height", 13},
          {"grid_width", 13},
          {"random_seed", 7},
          {"verbose", false}};
}

class mock_data_iterator: public data_iterator {
public:
  using next_batch_call =
      std::function<std::vector<labeled_image>(size_t batch_size)>;

  ~mock_data_iterator() {
    TS_ASSERT(next_batch_calls_.empty());
  }

  bool has_next_batch() const override { return !next_batch_calls_.empty(); }

  std::vector<labeled_image> next_batch(size_t batch_size) override {
    TS_ASSERT(!next_batch_calls_.empty());
    next_batch_call expected_call = std::move(next_batch_calls_.front());
    next_batch_calls_.pop_front();
    return expected_call(batch_size);
  }

  const std::vector<std::string>& class_labels() const override {
    return class_labels_;
  }

  size_t num_instances() const override {
    return num_instances_;
  }

  std::deque<next_batch_call> next_batch_calls_;

  std::vector<std::string> class_labels_;
  size_t num_instances_ = 0;
};

// Subclass of DarknetYOLOModelTrainer that mocks out the decoding of
// inference batches.
class TestDarknetYOLOModelTrainer : public DarknetYOLOModelTrainer {
 public:
  using decode_output_batch_call = std::function<InferenceOutputBatch(
      EncodedBatch batch, float confidence_threshold, float iou_threshold)>;

  TestDarknetYOLOModelTrainer(const DarknetYOLOCheckpoint& checkpoint,
                              neural_net::compute_context* context)
      : DarknetYOLOModelTrainer(checkpoint, context) {}

  InferenceOutputBatch DecodeOutputBatch(EncodedBatch batch,
                                         float confidence_threshold,
                                         float iou_threshold) override {
    TS_ASSERT(!decode_output_batch_calls_.empty());
    decode_output_batch_call expected_call =
        std::move(decode_output_batch_calls_.front());
    decode_output_batch_calls_.pop_front();
    return expected_call(std::move(batch), confidence_threshold, iou_threshold);
  }

  mutable std::deque<decode_output_batch_call> decode_output_batch_calls_;
};

// Subclass of object_detector that mocks out the methods that inject the
// object_detector dependencies.
class test_object_detector: public object_detector {
public:
  using create_iterator_call =
      std::function<std::unique_ptr<data_iterator>(
        data_iterator::parameters iterator_params)>;

  using create_compute_context_call =
      std::function<std::unique_ptr<compute_context>()>;

  using create_trainer_call = std::function<std::unique_ptr<ModelTrainer>(
      const Config& config, const std::string& pretrained_model_path,
      std::unique_ptr<neural_net::compute_context> context)>;

  using create_inference_trainer_call =
      std::function<std::unique_ptr<ModelTrainer>(
          const Checkpoint& checkpoint,
          std::unique_ptr<neural_net::compute_context> context)>;

  using perform_evaluation_call =
      std::function<variant_type(gl_sframe data, std::string metric,
                                 std::string output_type, float, float)>;

  using convert_yolo_to_annotations_call =
      std::function<std::vector<image_annotation>(
          const neural_net::float_array& yolo_map,
          const std::vector<std::pair<float, float>>& anchor_boxes,
          float min_confidence)>;

  test_object_detector() = default;
  test_object_detector(std::map<std::string, variant_type> initial_state,
                       float_array_map initial_weights)
      : object_detector(std::move(initial_state), std::move(initial_weights)) {}

  ~test_object_detector() {
    TS_ASSERT(create_iterator_calls_.empty());
    TS_ASSERT(create_compute_context_calls_.empty());
    TS_ASSERT(create_trainer_calls_.empty());
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

  std::unique_ptr<ModelTrainer> create_trainer(
      const Config& config, const std::string& pretrained_model_path,
      std::unique_ptr<neural_net::compute_context> context) const override {
    TS_ASSERT(!create_trainer_calls_.empty());
    create_trainer_call expected_call =
        std::move(create_trainer_calls_.front());
    create_trainer_calls_.pop_front();
    return expected_call(config, pretrained_model_path, std::move(context));
  }

  std::unique_ptr<ModelTrainer> create_inference_trainer(
      const Checkpoint& checkpoint,
      std::unique_ptr<neural_net::compute_context> context) const override {
    TS_ASSERT(!create_inference_trainer_calls_.empty());
    create_inference_trainer_call expected_call =
        std::move(create_inference_trainer_calls_.front());
    create_inference_trainer_calls_.pop_front();
    return expected_call(checkpoint, std::move(context));
  }

  variant_type perform_evaluation(gl_sframe data, std::string metric,
                                  std::string output_type,
                                  float confidence_threshold,
                                  float iou_threshold) override {
    TS_ASSERT(!perform_evaluation_calls_.empty());
    perform_evaluation_call expected_call =
        std::move(perform_evaluation_calls_.front());
    perform_evaluation_calls_.pop_front();
    return expected_call(data, metric, output_type, confidence_threshold,
                         iou_threshold);
  }

  std::vector<neural_net::image_annotation> convert_yolo_to_annotations(
      const neural_net::float_array& yolo_map,
      const std::vector<std::pair<float, float>>& anchor_boxes,
      float min_confidence) override {
    TS_ASSERT(!convert_yolo_to_annotations_calls_.empty());
    convert_yolo_to_annotations_call expected_call =
        std::move(convert_yolo_to_annotations_calls_.front());
    convert_yolo_to_annotations_calls_.pop_front();
    return expected_call(yolo_map, anchor_boxes, min_confidence);
  }

  template <class T>
  T get_field(const std::string& name) {
    return variant_get_value<T>(get_value_from_state(name));
  }

  mutable std::deque<create_iterator_call> create_iterator_calls_;
  mutable std::deque<create_compute_context_call> create_compute_context_calls_;
  mutable std::deque<create_trainer_call> create_trainer_calls_;
  mutable std::deque<create_inference_trainer_call>
      create_inference_trainer_calls_;
  mutable std::deque<perform_evaluation_call> perform_evaluation_calls_;
  mutable std::deque<convert_yolo_to_annotations_call>
      convert_yolo_to_annotations_calls_;
};

BOOST_AUTO_TEST_CASE(test_object_detector_iterate_training) {
  // Most of this test body will be spent setting up the mock objects that we'll
  // inject into the object_detector implementation. These mock objects will
  // make assertions about their inputs along the way and provide the outputs
  // that we manually pre-program. At the end will be the calls to
  // object_detector::iterate_training that will trigger all the actual testing.

  // Allocate the mock dependencies. We'll transfer ownership when the toolkit
  // code attempts to instantiate these dependencies.
  std::unique_ptr<mock_data_iterator> mock_iterator(new mock_data_iterator);
  std::unique_ptr<mock_image_augmenter> mock_augmenter(
      new mock_image_augmenter);
  std::unique_ptr<mock_model_backend> mock_nn_model(new mock_model_backend);
  std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

  // We'll request 4 training iterations, since the learning rate schedule
  // kicks in at the 50% and 75% points.
  static constexpr size_t test_max_iterations = 4;
  static constexpr size_t test_batch_size = 2;
  const std::vector<std::string> test_class_labels = {"label1", "label2"};
  static constexpr size_t test_num_instances = 123;
  static constexpr size_t test_num_examples = 100;
  static constexpr float test_loss = 5.f;

  mock_iterator->class_labels_ = test_class_labels;
  mock_iterator->num_instances_ = test_num_instances;

  auto num_iterations_submitted = std::make_shared<size_t>(0);
  for (size_t i = 0; i < test_max_iterations; ++i) {
    // Program the mock_iterator to return two arbitrary images, each with one
    // unique annotation. We'll store a copy of the annotations for later
    // comparison.
    auto test_annotations =
        std::make_shared<std::vector<std::vector<image_annotation>>>();
    auto next_batch_impl = [=](size_t batch_size) {
      TS_ASSERT_EQUALS(batch_size, test_batch_size);

      std::vector<labeled_image> result(test_batch_size);
      for (size_t j = 0; j < result.size(); ++j) {
        // The actual contents of the image and the annotations are irrelevant
        // for the purposes of this test. But encode the batch index and row
        // index into the bounding box so that we can verify this data is passed
        // into the image augmenter.
        image_annotation annotation;
        annotation.bounding_box.x = i;
        annotation.bounding_box.y = j;

        result[j].annotations.push_back(annotation);
        test_annotations->push_back(result[j].annotations);
      }

      return result;
    };
    mock_iterator->next_batch_calls_.push_back(next_batch_impl);

    // Program the mock_augmenter to return an arbitrary float_array, and to
    // pass through the annotations.
    auto test_image_batch = std::make_shared<shared_float_array>();
    auto prepare_images_impl = [=](std::vector<labeled_image> source_batch) {
      // The source batch should batch what we returned from the mock_iterator.
      TS_ASSERT_EQUALS(source_batch.size(), test_batch_size);
      for (size_t j = 0; j < source_batch.size(); ++j) {
        TS_ASSERT_EQUALS(source_batch[j].annotations, (*test_annotations)[j]);
      }

      // Just return a zero array with the correct shape for this test.
      image_augmenter::result res;
      res.image_batch = shared_float_array::wrap(
          std::vector<float>(test_batch_size * 416 * 416 * 3),
          {test_batch_size, 416, 416, 3});
      res.annotations_batch = *test_annotations;

      // Save the image_batch for downstream validation.
      *test_image_batch = res.image_batch;

      return res;
    };
    mock_augmenter->prepare_images_calls_.push_back(prepare_images_impl);

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
      // The input_batch should just be whatever the image_augmenter returned.
      shared_float_array input_batch = inputs.at("input");
      TS_ASSERT_EQUALS(input_batch.data(), test_image_batch->data());

      // Track how many calls we've had.
      *num_iterations_submitted += 1;

      // Multiply loss by 8 to offset the "mps_loss_mult" factor currently
      // hardwired in to avoid fp16 underflow in MPS.
      std::map<std::string, shared_float_array> result;
      result["loss"] = shared_float_array::wrap(8 * test_loss);
      return result;
    };
    mock_nn_model->train_calls_.push_back(train_impl);
  }

  std::map<std::string, variant_type> state = get_basic_state();
  state["batch_size"] = test_batch_size;
  state["classes"] =
      flex_list(test_class_labels.begin(), test_class_labels.end());
  state["max_iterations"] = 4;
  state["num_classes"] = 2;
  state["num_examples"] = test_num_examples;
  state["training_iterations"] = 0;
  test_object_detector model(state, {});

  // Program the mock_compute_context to emit the other mocks.
  mock_context->create_augmenter_calls_.push_back(
      [&](const image_augmenter::options& opts) {
        return std::move(mock_augmenter);
      });
  mock_context->create_augmenter_calls_.push_back(
      [&](const image_augmenter::options& opts) {
        // The ModelTrainer will instantiate an augmenter for inference, but
        // we won't use it in this test.
        return std::unique_ptr<image_augmenter>(new mock_image_augmenter);
      });
  mock_context->create_object_detector_calls_.push_back(
      [&](int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
          const float_array_map& config,
          const float_array_map& weights) { return std::move(mock_nn_model); });
  model.create_iterator_calls_.push_back(
      [&](data_iterator::parameters iterator_params) {
        return std::move(mock_iterator);
      });
  model.create_compute_context_calls_.push_back(
      [&] { return std::move(mock_context); });

  // Trigger the injection of the mocks.
  model.resume_training(gl_sframe(), gl_sframe());

  // Now, actually invoke object_detector::iterate_training. This will trigger
  // all the assertions registered above.
  for (size_t i = 0; i < test_max_iterations; ++i) {
    model.iterate_training();
  }

  TS_ASSERT_EQUALS(model.get_field<flex_int>("training_iterations"),
                   test_max_iterations);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("training_epochs"),
                   test_max_iterations * test_batch_size / test_num_examples);

  // Deconstructing `model` here will assert that every expected call to a
  // mocked-out method has been called.
}

BOOST_AUTO_TEST_CASE(test_object_detector_init_training) {
  // Most of this test body will be spent setting up the mock objects that we'll
  // inject into the object_detector implementation. These mock objects will
  // make assertions about their inputs along the way and provide the outputs
  // that we manually pre-program. At the end will be a single call to
  // object_detector::init_training that will trigger all the actual testing.
  test_object_detector model;

  // Allocate the mock dependencies. We'll transfer ownership when the toolkit
  // code attempts to instantiate these dependencies.
  std::unique_ptr<mock_data_iterator> mock_iterator(new mock_data_iterator);
  std::unique_ptr<mock_image_augmenter> mock_augmenter(
      new mock_image_augmenter);
  std::unique_ptr<mock_model_backend> mock_nn_model(new mock_model_backend);
  std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

  // We'll request 4 training iterations, since the learning rate schedule
  // kicks in at the 50% and 75% points.
  static constexpr size_t test_max_iterations = 4;
  static constexpr size_t test_batch_size = 2;
  const std::vector<std::string> test_class_labels = {"label1", "label2"};
  static constexpr size_t test_num_instances = 123;
  static constexpr size_t test_num_examples = 100;

  mock_iterator->class_labels_ = test_class_labels;
  mock_iterator->num_instances_ = test_num_instances;

  const std::string test_annotations_name = "test_annotations";
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

  auto create_augmenter_impl = [&](const image_augmenter::options& opts) {
    TS_ASSERT_EQUALS(opts.output_height, 416);
    TS_ASSERT_EQUALS(opts.output_width, 416);
    return std::move(mock_augmenter);
  };
  mock_context->create_augmenter_calls_.push_back(create_augmenter_impl);
  mock_context->create_augmenter_calls_.push_back(
      [&](const image_augmenter::options& opts) {
        // The ModelTrainer will instantiate an augmenter for inference, but
        // we won't use it in this test.
        return std::unique_ptr<image_augmenter>(new mock_image_augmenter);
      });

  // We'll provide this path for the "mlmodel_path" option. When the
  // object_detector attempts to initialize weights from that path, just return
  // some arbitrary dummy params.
  const std::string test_mlmodel_path = "/test/foo.mlmodel";
  model.create_trainer_calls_.emplace_back(
      [=](const Config& config, const std::string& pretrained_model_path,
          std::unique_ptr<neural_net::compute_context> context) {
        TS_ASSERT_EQUALS(pretrained_model_path, test_mlmodel_path);
        TS_ASSERT_EQUALS(config.num_classes, test_class_labels.size());

        float_array_map weights;
        std::vector<float> buffer(16 * 16 * 3 * 3);
        std::iota(buffer.begin(), buffer.end(), 0);  // buffer[i] = i
        weights["test_layer_weight"] = shared_float_array::wrap(
            std::move(buffer), std::vector<size_t>{16, 16, 3, 3});

        DarknetYOLOCheckpoint checkpoint(config, std::move(weights));
        return checkpoint.CreateModelTrainer(context.get());
      });

  auto create_object_detector_impl =
      [&](int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
          const float_array_map& config, const float_array_map& weights) {
        TS_ASSERT_EQUALS(n, test_batch_size);
        TS_ASSERT_EQUALS(c_in, 3);
        TS_ASSERT_EQUALS(h_in, 416);
        TS_ASSERT_EQUALS(w_in, 416);
        TS_ASSERT_EQUALS(c_out, 15 * (5 + test_class_labels.size()));
        TS_ASSERT_EQUALS(h_out, 13);
        TS_ASSERT_EQUALS(w_out, 13);

        // weights should be what we returned from create_trainer
        TS_ASSERT_EQUALS(weights.size(), 1);
        auto it = weights.find("test_layer_weight");
        TS_ASSERT(it != weights.end());
        for (size_t i = 0; i < it->second.size(); ++i) {
          TS_ASSERT_EQUALS(it->second.data()[i], static_cast<float>(i));
        }

        // TODO: Assert the config values?

        return std::move(mock_nn_model);
      };
  mock_context->create_object_detector_calls_.push_back(
      create_object_detector_impl);

  auto create_compute_context_impl = [&] { return std::move(mock_context); };
  model.create_compute_context_calls_.push_back(create_compute_context_impl);

  // Create an arbitrary SFrame with test_num_examples rows, since
  // object_detector uses the number of rows to compute num_examples, which is
  // used as a normalizer.
  gl_sframe data({{"ignored", gl_sarray::from_sequence(0, test_num_examples)}});

  // Now, actually invoke object_detector::init_training. This will trigger all
  // the assertions registered above.
  model.init_training(data, test_annotations_name, test_image_name, gl_sframe(),
                      {{"mlmodel_path", test_mlmodel_path},
                       {"batch_size", test_batch_size},
                       {"max_iterations", test_max_iterations}});

  // Verify model fields.
  TS_ASSERT_EQUALS(model.get_field<flex_int>("batch_size"), test_batch_size);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("max_iterations"),
                   test_max_iterations);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("annotations"),
                   test_annotations_name);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("feature"), test_image_name);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("model"), "darknet-yolo");
  TS_ASSERT_EQUALS(model.get_field<flex_int>("num_bounding_boxes"),
                   test_num_instances);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("num_classes"),
                   test_class_labels.size());
  TS_ASSERT_EQUALS(model.get_field<flex_int>("num_examples"),
                   test_num_examples);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("training_iterations"), 0);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("training_epochs"), 0);

  // Deconstructing `model` here will assert that every expected call to a
  // mocked-out method has been called.
}

BOOST_AUTO_TEST_CASE(test_object_detector_finalize_training) {
  // Most of this test body will be spent setting up the mock objects that we'll
  // inject into the object_detector implementation. These mock objects will
  // make assertions about their inputs along the way and provide the outputs
  // that we manually pre-program. At the end will be a single call to
  // object_detector::finalize_training that will trigger the actual testing.

  // Allocate the mock dependencies. We'll transfer ownership when the toolkit
  // code attempts to instantiate these dependencies.
  std::unique_ptr<mock_data_iterator> mock_iterator(new mock_data_iterator);
  std::unique_ptr<mock_image_augmenter> mock_augmenter(
      new mock_image_augmenter);
  std::unique_ptr<mock_model_backend> mock_nn_model(new mock_model_backend);
  std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

  // We'll request 4 training iterations, since the learning rate schedule
  // kicks in at the 50% and 75% points.
  static constexpr size_t test_batch_size = 2;
  const std::vector<std::string> test_class_labels = {"label1", "label2"};
  static constexpr size_t test_num_instances = 123;
  static constexpr size_t test_num_examples = 100;

  mock_iterator->class_labels_ = test_class_labels;
  mock_iterator->num_instances_ = test_num_instances;

  std::map<std::string, variant_type> state = get_basic_state();
  state["batch_size"] = test_batch_size;
  state["classes"] =
      flex_list(test_class_labels.begin(), test_class_labels.end());
  state["grid_height"] = 13;
  state["grid_width"] = 13;
  state["max_iterations"] = 4;
  state["num_classes"] = 2;
  state["num_examples"] = test_num_examples;
  state["training_iterations"] = 0;
  test_object_detector model(state, {});

  // Program the mock_compute_context to emit the other mocks.
  mock_context->create_augmenter_calls_.push_back(
      [&](const image_augmenter::options& opts) {
        return std::move(mock_augmenter);
      });
  mock_context->create_augmenter_calls_.push_back(
      [&](const image_augmenter::options& opts) {
        // The ModelTrainer will instantiate an augmenter for inference, but
        // we won't use it in this test.
        return std::unique_ptr<image_augmenter>(new mock_image_augmenter);
      });
  mock_context->create_object_detector_calls_.push_back(
      [&](int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
          const float_array_map& config,
          const float_array_map& weights) { return std::move(mock_nn_model); });
  model.create_iterator_calls_.push_back(
      [&](data_iterator::parameters iterator_params) {
        return std::move(mock_iterator);
      });
  model.create_compute_context_calls_.push_back(
      [&] { return std::move(mock_context); });

  // Trigger the injection of the mocks.
  model.resume_training(gl_sframe(), gl_sframe());

  // finalize_training will trigger a call to evaluation, to compute training
  // metrics.
  auto perform_evaluation_impl =
      [&](gl_sframe data, std::string metric, std::string output_type,
          float confidence_threshold, float iou_threshold) {
        std::map<std::string, variant_type> result;
        result["mean_average_precision"] = 0.80f;
        return to_variant(result);
      };
  model.perform_evaluation_calls_.push_back(perform_evaluation_impl);

  // Now, actually invoke object_detector::finalize_training. This will trigger
  // all the assertions registered above.
  model.finalize_training(true);

  // Verify model fields.
  TS_ASSERT_EQUALS(
      model.get_field<flex_float>("training_mean_average_precision"), 0.8f);

  // Deconstructing `model` here will assert that every expected call to a
  // mocked-out method has been called.
}

// TODO: Add a unit test for object_detector::train, in terms of the
// checkpointing API.

BOOST_AUTO_TEST_CASE(test_object_detector_auto_split) {

  // Most of this test body will be spent setting up the mock objects that we'll
  // inject into the object_detector implementation. These mock objects will
  // make assertions about their inputs along the way and provide the outputs
  // that we manually pre-program. At the end will be a single call to
  // object_detector::train that will trigger all the actual testing.
  test_object_detector model;

  // Allocate the mock dependencies. We'll transfer ownership when the toolkit
  // code attempts to instantiate these dependencies.
  std::unique_ptr<mock_data_iterator> mock_iterator(new mock_data_iterator);
  std::unique_ptr<mock_image_augmenter> mock_augmenter(
      new mock_image_augmenter);
  std::unique_ptr<mock_model_backend> mock_nn_model(
      new mock_model_backend);
  std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

  // We'll request 4 training iterations, since the learning rate schedule
  // kicks in at the 50% and 75% points.
  static constexpr size_t test_max_iterations = 4;
  static constexpr size_t test_batch_size = 2;
  const std::vector<std::string> test_class_labels = { "label1", "label2" };
  static constexpr size_t test_num_instances = 123;
  static constexpr size_t test_num_examples = 200;
  static constexpr float test_loss = 5.f;

  mock_iterator->class_labels_ = test_class_labels;
  mock_iterator->num_instances_ = test_num_instances;

  auto num_iterations_submitted = std::make_shared<size_t>(0);
  for (size_t i = 0; i < test_max_iterations; ++i) {

    // Program the mock_iterator to return two arbitrary images, each with one
    // unique annotation. We'll store a copy of the annotations for later
    // comparison.
    auto test_annotations =
        std::make_shared<std::vector<std::vector<image_annotation>>>();
    auto next_batch_impl = [=](size_t batch_size) {

      TS_ASSERT_EQUALS(batch_size, test_batch_size);

      std::vector<labeled_image> result(test_batch_size);
      for (size_t j = 0; j < result.size(); ++j) {

        // The actual contents of the image and the annotations are irrelevant
        // for the purposes of this test. But encode the batch index and row
        // index into the bounding box so that we can verify this data is passed
        // into the image augmenter.
        image_annotation annotation;
        annotation.bounding_box.x = i;
        annotation.bounding_box.y = j;

        result[j].annotations.push_back(annotation);
        test_annotations->push_back(result[j].annotations);
      }

      return result;
    };
    mock_iterator->next_batch_calls_.push_back(next_batch_impl);

    // Program the mock_augmenter to return an arbitrary float_array, and to
    // pass through the annotations.
    auto test_image_batch = std::make_shared<shared_float_array>();
    auto prepare_images_impl = [=](std::vector<labeled_image> source_batch) {

      // The source batch should batch what we returned from the mock_iterator.
      TS_ASSERT_EQUALS(source_batch.size(), test_batch_size);
      for (size_t j = 0; j < source_batch.size(); ++j) {
        TS_ASSERT_EQUALS(source_batch[j].annotations, (*test_annotations)[j]);
      }

      // Just return a zero array with the correct shape for this test.
      image_augmenter::result res;
      res.image_batch = shared_float_array::wrap(
          std::vector<float>(test_batch_size * 416 * 416 * 3),
          {test_batch_size, 416, 416, 3});
      res.annotations_batch = *test_annotations;

      // Save the image_batch for downstream validation.
      *test_image_batch = res.image_batch;

      return res;
    };
    mock_augmenter->prepare_images_calls_.push_back(prepare_images_impl);

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

      // The input_batch should just be whatever the image_augmenter returned.
      shared_float_array input_batch = inputs.at("input");
      TS_ASSERT_EQUALS(input_batch.data(), test_image_batch->data());

      // Track how many calls we've had.
      *num_iterations_submitted += 1;

      // Multiply loss by 8 to offset the "mps_loss_mult" factor currently
      // hardwired in to avoid fp16 underflow in MPS.
      std::map<std::string, shared_float_array> result;
      result["loss"] = shared_float_array::wrap(8 * test_loss);
      return result;
    };
    mock_nn_model->train_calls_.push_back(train_impl);
  }

  const std::string test_annotations_name = "test_annotations";
  const std::string test_image_name = "test_image";

  // The following callbacks capture by reference so that they can transfer
  // ownership of the mocks created above.
  auto create_iterator_impl = [&](data_iterator::parameters iterator_params) {
    // The train data is smaller than the original dataset
    TS_ASSERT(test_num_examples > iterator_params.data.size());
    TS_ASSERT(iterator_params.class_labels.empty());  // Should infer class labels from data.
    TS_ASSERT(iterator_params.repeat);

    return std::move(mock_iterator);
  };
  model.create_iterator_calls_.push_back(create_iterator_impl);

  auto create_augmenter_impl = [&](const image_augmenter::options& opts) {

    TS_ASSERT_EQUALS(opts.output_height, 416);
    TS_ASSERT_EQUALS(opts.output_width, 416);
    return std::move(mock_augmenter);
  };
  mock_context->create_augmenter_calls_.push_back(create_augmenter_impl);
  mock_context->create_augmenter_calls_.push_back(
      [&](const image_augmenter::options& opts) {
        // The ModelTrainer will instantiate an augmenter for inference, but
        // we won't use it in this test.
        return std::unique_ptr<image_augmenter>(new mock_image_augmenter);
      });

  // We'll provide this path for the "mlmodel_path" option. When the
  // object_detector attempts to initialize weights from that path, just return
  // some arbitrary dummy params.
  const std::string test_mlmodel_path = "/test/foo.mlmodel";
  model.create_trainer_calls_.emplace_back(
      [=](const Config& config, const std::string& pretrained_model_path,
          std::unique_ptr<neural_net::compute_context> context) {
        TS_ASSERT_EQUALS(pretrained_model_path, test_mlmodel_path);
        TS_ASSERT_EQUALS(config.num_classes, test_class_labels.size());

        float_array_map weights;
        std::vector<float> buffer(16 * 16 * 3 * 3);
        std::iota(buffer.begin(), buffer.end(), 0);  // buffer[i] = i
        weights["test_layer_weight"] = shared_float_array::wrap(
            std::move(buffer), std::vector<size_t>{16, 16, 3, 3});

        DarknetYOLOCheckpoint checkpoint(config, std::move(weights));
        return checkpoint.CreateModelTrainer(context.get());
      });

  auto create_object_detector_impl = [&](int n, int c_in, int h_in, int w_in,
                                    int c_out, int h_out, int w_out,
                                    const float_array_map& config,
                                    const float_array_map& weights) {

    TS_ASSERT_EQUALS(n, test_batch_size);
    TS_ASSERT_EQUALS(c_in, 3);
    TS_ASSERT_EQUALS(h_in, 416);
    TS_ASSERT_EQUALS(w_in, 416);
    TS_ASSERT_EQUALS(c_out, 15 * (5 + test_class_labels.size()));
    TS_ASSERT_EQUALS(h_out, 13);
    TS_ASSERT_EQUALS(w_out, 13);

    // weights should be what we returned from create_trainer
    TS_ASSERT_EQUALS(weights.size(), 1);
    auto it = weights.find("test_layer_weight");
    TS_ASSERT(it != weights.end());
    for (size_t i = 0; i < it->second.size(); ++i) {
      TS_ASSERT_EQUALS(it->second.data()[i], static_cast<float>(i));
    }

    // TODO: Assert the config values?

    return std::move(mock_nn_model);
  };
  mock_context->create_object_detector_calls_.push_back(
      create_object_detector_impl);

  auto create_compute_context_impl = [&] { return std::move(mock_context); };
  model.create_compute_context_calls_.push_back(create_compute_context_impl);

  // Training will trigger a call to evaluation, to compute training metrics.
  auto perform_evaluation_impl =
      [&](gl_sframe data, std::string metric, std::string output_type,
          float confidence_threshold, float iou_threshold) {
        std::map<std::string, variant_type> result;
        result["mean_average_precision"] = 0.80f;
        return to_variant(result);
      };

  // The two evaluation calls are performed for train data as well as validation
  // data.
  model.perform_evaluation_calls_.resize(2, perform_evaluation_impl);

  // Create an arbitrary SFrame with test_num_examples rows, since
  // object_detector uses the number of rows to compute num_examples, which is
  // used as a normalizer.
  gl_sframe data({{"ignored", gl_sarray::from_sequence(0, test_num_examples)}});

  // Now, actually invoke object_detector::train. This will trigger all the
  // assertions registered above.
  model.train(data, test_annotations_name, test_image_name, "auto",
              { { "mlmodel_path",   test_mlmodel_path   },
                { "batch_size",     test_batch_size     },
                { "max_iterations", test_max_iterations }, });

  // Verify model fields.
  TS_ASSERT_EQUALS(model.get_field<flex_int>("batch_size"), test_batch_size);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("max_iterations"),
                   test_max_iterations);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("annotations"),
                   test_annotations_name);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("feature"), test_image_name);
  TS_ASSERT_EQUALS(model.get_field<flex_string>("model"), "darknet-yolo");
  TS_ASSERT_EQUALS(model.get_field<flex_int>("num_bounding_boxes"),
                   test_num_instances);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("num_classes"),
                   test_class_labels.size());
  TS_ASSERT_LESS_THAN_EQUALS(model.get_field<flex_int>("num_examples"),
                   test_num_examples);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("training_iterations"),
                   test_max_iterations);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("training_epochs"),
                   test_max_iterations * test_batch_size / test_num_examples);
  TS_ASSERT_EQUALS(
      model.get_field<flex_float>("training_mean_average_precision"), 0.8f);

  // Deconstructing `model` here will assert that every expected call to a
  // mocked-out method has been called.
}

BOOST_AUTO_TEST_CASE(test_object_detector_predict) {
  static constexpr size_t test_num_examples = 100;
  static constexpr size_t test_max_iterations = 2;
  static constexpr size_t test_batch_size = 2;
  const std::string test_annotations_name = "test_annotations";
  const std::string test_image_name = "test_image";
  static constexpr float test_loss = 5.f;
  const std::vector<std::string> test_class_labels = {"label1", "label2"};
  static constexpr size_t test_num_instances = 123;

  // Create an arbitrary SFrame with test_num_examples rows, since
  // object_detector uses the number of rows to compute num_examples, which is
  // used as a normalizer.
  gl_sframe data(
      {{test_image_name, gl_sarray::from_sequence(0, test_num_examples)}});

  test_object_detector model;
  // Allocate the mock dependencies. We'll transfer ownership when the toolkit
  // code attempts to instantiate these dependencies.
  std::unique_ptr<mock_data_iterator> mock_iterator(new mock_data_iterator);
  std::unique_ptr<mock_image_augmenter> mock_augmenter(
      new mock_image_augmenter);
  std::unique_ptr<mock_model_backend> mock_nn_model(new mock_model_backend);
  std::unique_ptr<mock_compute_context> mock_context(new mock_compute_context);

  mock_iterator->class_labels_ = test_class_labels;
  mock_iterator->num_instances_ = test_num_instances;

  auto num_iterations_submitted = std::make_shared<size_t>(0);
  for (size_t i = 0; i < test_max_iterations; ++i) {
    // Program the mock_iterator to return two arbitrary images, each with one
    // unique annotation. We'll store a copy of the annotations for later
    // comparison.
    auto test_annotations =
        std::make_shared<std::vector<std::vector<image_annotation>>>();

    auto next_batch_impl = [=](size_t batch_size) {
      TS_ASSERT_EQUALS(batch_size, test_batch_size);
      std::vector<labeled_image> result(test_batch_size);
      for (size_t j = 0; j < result.size(); ++j) {
        // The actual contents of the image and the annotations are irrelevant
        // for the purposes of this test. But encode the batch index and row
        // index into the bounding box so that we can verify this data is passed
        // into the image augmenter.
        image_annotation annotation;
        annotation.bounding_box.x = i;
        annotation.bounding_box.y = j;

        result[j].annotations.push_back(annotation);
        test_annotations->push_back(result[j].annotations);
      }

      return result;
    };
    mock_iterator->next_batch_calls_.push_back(next_batch_impl);

    // Program the mock_augmenter to return an arbitrary float_array, and to
    // pass through the annotations.
    auto test_image_batch = std::make_shared<shared_float_array>();
    auto prepare_images_impl = [=](std::vector<labeled_image> source_batch) {
      // The source batch should batch what we returned from the mock_iterator.
      TS_ASSERT_EQUALS(source_batch.size(), test_batch_size);
      for (size_t j = 0; j < source_batch.size(); ++j) {
        TS_ASSERT_EQUALS(source_batch[j].annotations, (*test_annotations)[j]);
      }

      // Just return a zero array with the correct shape for this test.
      image_augmenter::result res;
      res.image_batch = shared_float_array::wrap(
          std::vector<float>(test_batch_size * 416 * 416 * 3),
          {test_batch_size, 416, 416, 3});
      res.annotations_batch = *test_annotations;

      // Save the image_batch for downstream validation.
      *test_image_batch = res.image_batch;

      return res;
    };
    mock_augmenter->prepare_images_calls_.push_back(prepare_images_impl);

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
      // The input_batch should just be whatever the image_augmenter returned.
      shared_float_array input_batch = inputs.at("input");
      TS_ASSERT_EQUALS(input_batch.data(), test_image_batch->data());

      // Track how many calls we've had.
      *num_iterations_submitted += 1;

      // Multiply loss by 8 to offset the "mps_loss_mult" factor currently
      // hardwired in to avoid fp16 underflow in MPS.
      std::map<std::string, shared_float_array> result;
      result["loss"] = shared_float_array::wrap(8 * test_loss);
      return result;
    };
    mock_nn_model->train_calls_.push_back(train_impl);

  }  // End of test_max_iterations for loop

  auto create_iterator_impl = [&](data_iterator::parameters iterator_params) {
    return std::move(mock_iterator);
  };

  model.create_iterator_calls_.push_back(create_iterator_impl);

  auto create_augmenter_impl = [&](const image_augmenter::options& opts) {
    return std::move(mock_augmenter);
  };
  mock_context->create_augmenter_calls_.push_back(create_augmenter_impl);
  mock_context->create_augmenter_calls_.push_back(
      [&](const image_augmenter::options& opts) {
        // The ModelTrainer will instantiate an augmenter for inference, but
        // we won't use it in this test.
        return std::unique_ptr<image_augmenter>(new mock_image_augmenter);
      });

  // We'll provide this path for the "mlmodel_path" option. When the
  // object_detector attempts to initialize weights from that path, just return
  // some arbitrary dummy params.
  const std::string test_mlmodel_path = "/test/foo.mlmodel";
  model.create_trainer_calls_.emplace_back(
      [=](const Config& config, const std::string& pretrained_model_path,
          std::unique_ptr<neural_net::compute_context> context) {
        TS_ASSERT_EQUALS(pretrained_model_path, test_mlmodel_path);
        TS_ASSERT_EQUALS(config.num_classes, test_class_labels.size());

        float_array_map weights;
        std::vector<float> buffer(16 * 16 * 3 * 3);
        std::iota(buffer.begin(), buffer.end(), 0);  // buffer[i] = i
        weights["test_layer_weight"] = shared_float_array::wrap(
            std::move(buffer), std::vector<size_t>{16, 16, 3, 3});

        DarknetYOLOCheckpoint checkpoint(config, std::move(weights));
        return checkpoint.CreateModelTrainer(context.get());
      });

  auto create_object_detector_impl =
      [&](int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
          const float_array_map& config,
          const float_array_map& weights) { return std::move(mock_nn_model); };
  mock_context->create_object_detector_calls_.push_back(
      create_object_detector_impl);

  auto create_compute_context_impl = [&] { return std::move(mock_context); };
  model.create_compute_context_calls_.push_back(create_compute_context_impl);

  // Training will trigger a call to evaluation, to compute training metrics.
  auto perform_evaluation_impl =
      [&](gl_sframe data, std::string metric, std::string output_type,
          float confidence_threshold, float iou_threshold) {
        std::map<std::string, variant_type> result;
        result["mean_average_precision"] = 0.80f;
        return to_variant(result);
      };
  model.perform_evaluation_calls_.push_back(perform_evaluation_impl);

  // Now, actually invoke object_detector::train. This will trigger all the
  // assertions registered above.
  model.train(data, test_annotations_name, test_image_name, gl_sframe(),
              {
                  {"mlmodel_path", test_mlmodel_path},
                  {"batch_size", test_batch_size},
                  {"max_iterations", test_max_iterations},
              });

  TS_ASSERT_EQUALS(model.get_field<flex_int>("num_bounding_boxes"),
                   test_num_instances);
  TS_ASSERT_EQUALS(model.get_field<flex_int>("num_classes"),
                   test_class_labels.size());

  mock_iterator.reset(new mock_data_iterator);
  mock_iterator->class_labels_ = test_class_labels;
  mock_iterator->num_instances_ = test_num_instances;
  model.create_iterator_calls_.push_back(create_iterator_impl);

  mock_context.reset(new mock_compute_context);
  model.create_compute_context_calls_.push_back(create_compute_context_impl);

  mock_augmenter.reset(new mock_image_augmenter);
  mock_context->create_augmenter_calls_.push_back(
      [&](const image_augmenter::options& opts) {
        // The ModelTrainer will instantiate an augmenter for training, but
        // we won't use it in this test.
        return std::unique_ptr<image_augmenter>(new mock_image_augmenter);
      });
  mock_context->create_augmenter_calls_.push_back(create_augmenter_impl);

  mock_nn_model.reset(new mock_model_backend);
  mock_context->create_object_detector_calls_.push_back(
      create_object_detector_impl);

  static constexpr size_t num_prediction_instances = 2;
  auto test_annotations =
      std::make_shared<std::vector<std::vector<image_annotation>>>();
  auto next_batch_impl = [=](size_t batch_size) {
    TS_ASSERT_EQUALS(batch_size, test_batch_size);

    std::vector<labeled_image> result(test_batch_size);
    for (size_t j = 0; j < result.size(); ++j) {
      // The actual contents of the image and the annotations are irrelevant
      // for the purposes of this test. But encode the batch index and row
      // index into the bounding box so that we can verify this data is passed
      // into the image augmenter.
      image_annotation annotation;
      annotation.bounding_box.x = 0;
      annotation.bounding_box.y = j;

      result[j].annotations.push_back(annotation);
      test_annotations->push_back(result[j].annotations);
    }

    return result;
  };
  mock_iterator->next_batch_calls_.push_back(next_batch_impl);

  // Program the mock_augmenter to return an arbitrary float_array, and to
  // pass through the annotations.
  auto test_image_batch = std::make_shared<shared_float_array>();
  auto prepare_images_impl = [=](std::vector<labeled_image> source_batch) {
    // Return an arbitrary float_array, just a scalar encoding the iteration
    // index.
    image_augmenter::result res;
    res.image_batch = shared_float_array::wrap(static_cast<float>(0));
    res.annotations_batch = *test_annotations;

    // Save the image_batch for downstream validation.
    *test_image_batch = res.image_batch;

    return res;
  };
  mock_augmenter->prepare_images_calls_.push_back(prepare_images_impl);

  auto predict_impl = [](const float_array_map& inputs) {
    float_array_map result;

    static constexpr size_t OUTPUT_GRID_SIZE = 2;
    static constexpr size_t NUM_CLASSES = 2;
    static constexpr size_t NUM_PREDS = NUM_CLASSES + 5;  // 4 for bbox, 1 conf

    const std::vector<std::pair<float, float>> anchor_boxes = {
        {1.f, 2.f},
        {1.f, 1.f},
    };

    // Allocate a YOLO map and define a setter function so we can populate it
    // relatively conveniently.
    size_t buffer_size = test_batch_size * anchor_boxes.size() * NUM_PREDS *
                         OUTPUT_GRID_SIZE * OUTPUT_GRID_SIZE;
    std::vector<float> buffer(buffer_size, 0.f);
    auto set_val = [&](size_t b_num, size_t h, size_t w, size_t b, size_t p,
                       float val) {
      float* out = buffer.data();
      out += h * NUM_PREDS * anchor_boxes.size() * OUTPUT_GRID_SIZE;
      out += w * NUM_PREDS * anchor_boxes.size();
      out += b * NUM_PREDS;
      out += p;
      *out = val;
    };

    for (size_t batch_num = 0; batch_num < test_batch_size;
         ++batch_num) {  // batch size
      // Initialize all confidence scores to large negative values, which
      // correspond to zero confidence (after passing through the sigmoid
      // function).
      for (size_t h = 0; h < OUTPUT_GRID_SIZE; ++h) {
        for (size_t w = 0; w < OUTPUT_GRID_SIZE; ++w) {
          for (size_t b = 0; b < anchor_boxes.size(); ++b) {
            set_val(batch_num, h, w, b, 4, -1000.f);  // conf
          }
        }
      }

      // Predict class 0 at the center of output cell (1,0) with exactly the
      // size of anchor box 0.
      set_val(batch_num, 0, 1, 0, 0, 0.f);     // x
      set_val(batch_num, 0, 1, 0, 1, 0.f);     // y
      set_val(batch_num, 0, 1, 0, 2, 0.f);     // w
      set_val(batch_num, 0, 1, 0, 3, 0.f);     // h
      set_val(batch_num, 0, 1, 0, 4, 1000.f);  // conf
      set_val(batch_num, 0, 1, 0, 5, 1000.f);  // class 0
      set_val(batch_num, 0, 1, 0, 6, 0.f);     // class 1

      // Predict class 1 at the upper-left corner of output cell (0,1) at half
      // the size of anchor box 1, with confidence 0.5.
      set_val(batch_num, 1, 0, 1, 0, -1000.f);         // x
      set_val(batch_num, 1, 0, 1, 1, -1000.f);         // y
      set_val(batch_num, 1, 0, 1, 2, std::log(0.5f));  // w
      set_val(batch_num, 1, 0, 1, 3, std::log(0.5f));  // h
      set_val(batch_num, 1, 0, 1, 4, 0.f);             // conf
      set_val(batch_num, 1, 0, 1, 5, 0.f);             // class 0
      set_val(batch_num, 1, 0, 1, 6, 1000.f);          // class 1
    }

    shared_float_array arr = shared_float_array::wrap(
        buffer, {test_batch_size, OUTPUT_GRID_SIZE, OUTPUT_GRID_SIZE,
                 anchor_boxes.size() * NUM_PREDS});
    result["output"] = arr;
    return result;
  };
  mock_nn_model->predict_calls_.push_back(predict_impl);

  mock_iterator->next_batch_calls_.push_back(next_batch_impl);
  mock_augmenter->prepare_images_calls_.push_back(prepare_images_impl);
  mock_nn_model->predict_calls_.push_back(predict_impl);

  auto decode_output_batch_impl =
      [](EncodedBatch batch, float confidence_threshold, float iou_threshold) {
        const float_array& yolo_map = batch.encoded_data["output"];
        ASSERT_EQ(yolo_map.dim(), 4);
        const size_t* const shape = yolo_map.shape();
        const size_t batch_size = shape[0];
        const size_t output_height = shape[1];
        const size_t output_width = shape[2];
        const size_t num_channels = shape[3];

        size_t num_anchor_boxes = 2;
        static constexpr size_t NUM_CLASSES = 2;
        static constexpr size_t NUM_PREDS =
            NUM_CLASSES + 5;  // 4 for bbox, 1 conf
        ASSERT_EQ(num_channels, num_anchor_boxes * NUM_PREDS);

        static constexpr size_t OUTPUT_GRID_SIZE = 2;
        ASSERT_EQ(output_height, OUTPUT_GRID_SIZE);
        ASSERT_EQ(output_width, OUTPUT_GRID_SIZE);

        InferenceOutputBatch result;
        result.predictions.resize(batch_size);
        for (size_t i = 0; i < batch_size; ++i) {
          for (size_t j = 0; j < num_prediction_instances; ++j) {
            // The actual contents of the image and the annotations are
            // irrelevant for the purposes of this test. But encode the batch
            // index and row index into the bounding box so that we can verify
            // this data is passed into the image augmenter.
            image_annotation annotation;
            annotation.bounding_box.x = 0;
            annotation.bounding_box.y = j;
            result.predictions[i].push_back(annotation);
          }
          result.image_sizes.emplace_back(416, 416);
        }
        return result;
      };
  model.create_inference_trainer_calls_.emplace_back(
      [=](const Checkpoint& checkpoint,
          std::unique_ptr<neural_net::compute_context> context) {
        std::unique_ptr<TestDarknetYOLOModelTrainer> result;
        DarknetYOLOCheckpoint darknet_yolo_checkpoint(checkpoint.config(),
                                                      checkpoint.weights());
        result.reset(new TestDarknetYOLOModelTrainer(darknet_yolo_checkpoint,
                                                     context.get()));

        // Two calls for two batches
        for (size_t i = 0; i < test_batch_size; ++i) {
          result->decode_output_batch_calls_.push_back(
              decode_output_batch_impl);
        }
        return result;
      });

  std::map<std::string, flexible_type> opts{{"confidence_threshold",0.25}, {"iou_threshold",0.45}};
  variant_type result_variant = model.predict(data, opts);
  gl_sarray result = variant_get_value<gl_sarray>(result_variant);
  for (size_t j = 0; j < result.size(); ++j) {
    TS_ASSERT_EQUALS(result[j].size(), num_prediction_instances);
  }
}

}  // namespace
}  // namespace object_detection
}  // namespace turi

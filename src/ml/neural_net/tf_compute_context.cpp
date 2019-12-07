/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/
#include <ml/neural_net/tf_compute_context.hpp>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <core/parallel/lambda_omp.hpp>
#include <core/parallel/thread_pool.hpp>
#include <core/util/try_finally.hpp>
#include <ml/neural_net/image_augmentation.hpp>
#include <ml/neural_net/model_backend.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace turi {
namespace neural_net {

using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::labeled_image;
using turi::neural_net::shared_float_array;
using turi::neural_net::float_array_image_augmenter;

template <typename CallFunc>
void call_pybind_function(const CallFunc& func) {
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  turi::scoped_finally gstate_restore([&]() { PyGILState_Release(gstate); });

  try {
    func();
  } catch (const pybind11::error_already_set& e) {
    log_and_throw(std::string("An error occurred: ") + e.what());
  } catch (...) {
    log_and_throw("Unknown error occurred");
  }

}

static std::vector<size_t> get_shape(const float_array& num) {
  return std::vector<size_t>(num.shape(), num.shape() + num.dim());
}

static std::vector<size_t> get_strides(const float_array& num) {
  if (num.dim() == 0) {
    return {};
  }
  std::vector<size_t> result(num.dim());
  const size_t* shape = num.shape();
  result[num.dim() - 1] = sizeof(float);
  for (size_t i = num.dim() - 1; i > 0; --i) {
    result[i - 1] = result[i] * shape[i];
  }
  return result;
}

PYBIND11_MODULE(libtctensorflow, m) {
  pybind11::class_<float_array>(m, "FloatArray", pybind11::buffer_protocol())
      .def_buffer([](float_array& m) -> pybind11::buffer_info {
        return pybind11::buffer_info(
            const_cast<float*>(m.data()), /* Pointer to buffer */
            sizeof(float),                /* Size of one scalar */
            pybind11::format_descriptor<float>::format(),  /* Python struct-style format descriptor */
            m.dim(),              /* Number of dimensions  */
            get_shape(m),         /* Buffer dimensions */
            get_strides(m)        /* Strides (in bytes) for each index */

        );
      });
  pybind11::class_<shared_float_array>(m, "SharedFloatArray",
                                       pybind11::buffer_protocol())
      .def_buffer([](shared_float_array& m) -> pybind11::buffer_info {
        return pybind11::buffer_info(
            const_cast<float*>(m.data()), /* Pointer to buffer */
            sizeof(float),                /* Size of one scalar */
            pybind11::format_descriptor<float>::format(), /* Python struct-style format descriptor */
            m.dim(),              /* Number of dimensions */
            get_shape(m),         /* Buffer dimensions */
            get_strides(m)        /* Strides (in bytes) for each index */

        );
      });
}


class tf_model_backend : public model_backend {
 public:
  tf_model_backend(pybind11::object model);

  ~tf_model_backend();

  // model_backend interface
  float_array_map train(const float_array_map& inputs) override;
  float_array_map predict(const float_array_map& inputs) const override;
  float_array_map export_weights() const override;
  void set_learning_rate(float lr) override;

  // Setters for values needed to enable asynchronous computation, using
  // deferred_float_array.
  void set_train_output_shapes(
      std::map<std::string, std::vector<size_t>> output_shapes) {
    train_output_shapes_ = std::move(output_shapes);
  }

 private:
  float_array_map train_sync(const float_array_map& inputs);

  pybind11::object model_;
  thread_pool thread_pool_;
  parallel_task_queue task_queue_;

  std::map<std::string, std::vector<size_t>> train_output_shapes_;
};

tf_model_backend::tf_model_backend(pybind11::object model)
    : model_(model), thread_pool_(1), task_queue_(thread_pool_) {}

float_array_map tf_model_backend::train_sync(const float_array_map& inputs) {
  // Call train method on the TensorflowModel
  float_array_map result;

  call_pybind_function([&]() {
    pybind11::object output = model_.attr("train")(inputs);

    std::map<std::string, pybind11::buffer> buf_output =
        output.cast<std::map<std::string, pybind11::buffer>>();

    for (auto& kv : buf_output) {
      pybind11::buffer_info buf = kv.second.request();
      turi::neural_net::shared_float_array value =
          turi::neural_net::shared_float_array::copy(
              static_cast<float*>(buf.ptr),
              std::vector<size_t>(buf.shape.begin(), buf.shape.end()));
      result[kv.first] = value;
    }
  });

  return result;
}

float_array_map tf_model_backend::train(const float_array_map& inputs) {
  // We can only use asynchronous training if we know the shapes of the outputs.
  if (train_output_shapes_.empty()) {
    return train_sync(inputs);
  }

  // Create a promise for each expected output.
  std::map<std::string, std::shared_ptr<std::promise<shared_float_array>>>
      promises;
  for (const auto& kv : train_output_shapes_) {
    promises[kv.first] = std::make_shared<std::promise<shared_float_array>>();
  }

  // Dispatch the call to TF to our worker thread.
  auto perform_train = [inputs, promises, this] {
    // Invoke TensorFlow.
    float_array_map local_result = this->train_sync(inputs);

    // Fulfill the promises we made.
    for (const auto& kv : promises) {
      kv.second->set_value(local_result.at(kv.first));
    }
  };
  task_queue_.launch(perform_train);

  // Return a result dictionary wrapping the futures for the promises dispatched
  // to TensorFlow.
  float_array_map result;
  for (const auto& kv : train_output_shapes_) {
    const std::string& key = kv.first;
    const std::vector<size_t>& shape = kv.second;
    result[key] = shared_float_array(std::make_shared<deferred_float_array>(
        promises.at(key)->get_future(), shape));
  }

  return result;
}

float_array_map tf_model_backend::predict(const float_array_map& inputs) const {
  float_array_map result;

  // Call predict method on the TensorFlowModel
  call_pybind_function([&]() {
    pybind11::object output = model_.attr("predict")(inputs);
    std::map<std::string, pybind11::buffer> buf_output =
        output.cast<std::map<std::string, pybind11::buffer>>();

    for (auto& kv : buf_output) {
      pybind11::buffer_info buf = kv.second.request();
      turi::neural_net::shared_float_array value =
          turi::neural_net::shared_float_array::copy(
              static_cast<float*>(buf.ptr),
              std::vector<size_t>(buf.shape.begin(), buf.shape.end()));
      result[kv.first] = value;
    }
  });

  return result;
}

float_array_map tf_model_backend::export_weights() const {
  float_array_map result;
  call_pybind_function([&]() {
    // Call export_weights method on the TensorFLowModel
    pybind11::dict exported_weights = model_.attr("export_weights")();

    // it should be trivial to call this if we use the same interpreter process
    pybind11::module np = pybind11::module::import("numpy");
    for (auto& kv : exported_weights) {
      // defensively call ascontiguousarray to force to reorganize
      // underlying numpy memory layout using the real strides
      exported_weights[kv.first] = np.attr("ascontiguousarray")(kv.second);
    }

    std::map<std::string, pybind11::buffer> buf_output =
        exported_weights.cast<std::map<std::string, pybind11::buffer>>();

    for (auto& kv : buf_output) {
      pybind11::buffer_info buf = kv.second.request();
      turi::neural_net::shared_float_array value =
          turi::neural_net::shared_float_array::copy(
              static_cast<float*>(buf.ptr),
              std::vector<size_t>(buf.shape.begin(), buf.shape.end()));
      result[kv.first] = value;
    }
  });

  return result;
}

void tf_model_backend::set_learning_rate(float lr) {
  float_array_map result;

  // Call set_learning_rate method on the TensorFLowModel
  call_pybind_function([&]() { model_.attr("set_learning_rate")(lr); });
}

tf_model_backend::~tf_model_backend() {
  call_pybind_function([&]() { model_ = pybind11::object(); });
}

class tf_image_augmenter : public float_array_image_augmenter {
 public:
  tf_image_augmenter(const options& opts);
  ~tf_image_augmenter() override = default;

  labeled_float_image prepare_augmented_images(
      labeled_float_image data_to_augment) override;

 private:
  labeled_float_image prepare_augmented_images_sync(
      labeled_float_image data_to_augment);
};

tf_image_augmenter::tf_image_augmenter(const options& opts) : float_array_image_augmenter(opts) {}

float_array_image_augmenter::labeled_float_image
tf_image_augmenter::prepare_augmented_images(
    labeled_float_image data_to_augment) {
  size_t batch_size = data_to_augment.images.size();

  // Allocate a result into which the worker threads can write their
  // thread-local results in parallel.
  labeled_float_image result;
  result.images.resize(thread::cpu_count());
  result.annotations.resize(batch_size);

  auto perform_augmentations = [&](size_t thread_id, size_t num_threads) {
    size_t range_start = batch_size * thread_id / num_threads;
    size_t range_end = batch_size * (thread_id + 1) / num_threads;

    // Slice out the inputs we need to augment.
    labeled_float_image local_data_to_augment;
    auto first_image_it = data_to_augment.images.begin();
    auto first_annotation_it = data_to_augment.annotations.begin();
    local_data_to_augment.images = std::vector<shared_float_array>(
        first_image_it + range_start, first_image_it + range_end);
    local_data_to_augment.annotations = std::vector<shared_float_array>(
        first_annotation_it + range_start, first_annotation_it + range_end);

    // Augment the slice.
    labeled_float_image local_result =
        this->prepare_augmented_images_sync(local_data_to_augment);

    // Write the result into the appropriate slice of the shared output.
    result.images[thread_id] = local_result.images[0];
    for (size_t i = range_start; i < range_end; ++i) {
      result.annotations[i] = local_result.annotations[i - range_start];
    }
  };
  in_parallel(perform_augmentations);

  // Trim the result images, which are populated at one element per thread, not
  // one element per image.
  auto unused_output = [](const shared_float_array& image) {
    return image.dim() == 0;
  };
  auto new_end =
      std::remove_if(result.images.begin(), result.images.end(), unused_output);
  result.images.erase(new_end, result.images.end());

  return result;
}

float_array_image_augmenter::labeled_float_image
tf_image_augmenter::prepare_augmented_images_sync(
    float_array_image_augmenter::labeled_float_image data_to_augment) {
  options opts = get_options();
  float_array_image_augmenter::labeled_float_image image_annotations;

  call_pybind_function([&]() {
    // Import the module from python that does data augmentation
    pybind11::module tf_aug = pybind11::module::import(
        "turicreate.toolkits.object_detector._tf_image_augmenter");

    const size_t output_height = opts.output_height;
    const size_t output_width = opts.output_width;

    // TODO: Remove resize_only by passing all the augmentation options
    bool resize_only = false;
    if (opts.crop_prob == 0.f) {
      resize_only = true;
    }

    // Get augmented images and annotations from tensorflow
    pybind11::object augmented_data = tf_aug.attr("get_augmented_data")(
        data_to_augment.images, data_to_augment.annotations, output_height,
        output_width, resize_only);
    std::pair<pybind11::buffer, std::vector<pybind11::buffer>> aug_data =
        augmented_data
            .cast<std::pair<pybind11::buffer, std::vector<pybind11::buffer>>>();

    pybind11::buffer_info buf_img = std::get<0>(aug_data).request();
    image_annotations.images.push_back(shared_float_array::copy(
        static_cast<float*>(buf_img.ptr),
        std::vector<size_t>(buf_img.shape.begin(), buf_img.shape.end())));
    std::vector<turi::neural_net::shared_float_array> annotations_per_batch;
    std::vector<pybind11::buffer> aug_annotations = std::get<1>(aug_data);

    for (size_t i = 0; i < aug_annotations.size(); i++) {
      pybind11::buffer_info buf_ann = aug_annotations[i].request();
      size_t num_annotations = buf_ann.shape[0];
      turi::neural_net::shared_float_array annotation;
      if (num_annotations > 0) {
        annotation = turi::neural_net::shared_float_array::copy(
            static_cast<float*>(buf_ann.ptr),
            std::vector<size_t>(buf_ann.shape.begin(), buf_ann.shape.end()));
      } else {
        annotation = turi::neural_net::shared_float_array();
      }
      annotations_per_batch.push_back(annotation);
    }
    image_annotations.annotations = annotations_per_batch;
  });

  return image_annotations;
}

namespace {

std::unique_ptr<compute_context> create_tf_compute_context() {
  return std::unique_ptr<compute_context>(new tf_compute_context);
}

// At static-init time, register create_tf_compute_context().
// TODO: Codify priority levels?
static auto* tf_registration = new compute_context::registration(
    /* priority */ 1, &create_tf_compute_context, &create_tf_compute_context);

}  // namespace

tf_compute_context::tf_compute_context() {
  call_pybind_function([&]() {
    pybind11::module os = pybind11::module::import("os");
    os.attr("environ")["TF_CPP_MIN_LOG_LEVEL"] = "2";
  });
};

tf_compute_context::~tf_compute_context() = default;

size_t tf_compute_context::memory_budget() const {
  // TODO: Returns 4GB as that makes sure default batch size is used.
  // Do something that makes more sense like MPS later.
  return 4294967296lu;
}

std::vector<std::string> tf_compute_context::gpu_names() const {
  std::vector<std::string> gpu_device_names;

  call_pybind_function([&]() {
    pybind11::module tf_gpu_devices =
        pybind11::module::import("turicreate.toolkits._tf_utils");
    // Get the names from tf utilities function
    pybind11::object gpu_devices = tf_gpu_devices.attr("get_gpu_names")();
    gpu_device_names = gpu_devices.cast<std::vector<std::string>>();
  });

  return gpu_device_names;
}


std::unique_ptr<model_backend> tf_compute_context::create_object_detector(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {
  std::unique_ptr<tf_model_backend> result;
  call_pybind_function([&]() {
      pybind11::module tf_od_backend = pybind11::module::import(
          "turicreate.toolkits.object_detector._tf_model_architecture");

      // Make an instance of python object
      pybind11::object object_detector = tf_od_backend.attr(
          "ODTensorFlowModel")(h_in, w_in, n, c_out, h_out, w_out, weights, config);
      result.reset(new tf_model_backend(object_detector));
    });

  // Enable asynchronous training.
  // TODO: Match the MPS implementation, which has loss shape {batch_size}
  std::map<std::string, std::vector<size_t>> output_shapes;
  output_shapes["loss"] = {1};
  result->set_train_output_shapes(std::move(output_shapes));

  return result;
}

std::unique_ptr<model_backend> tf_compute_context::create_activity_classifier(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {
  shared_float_array prediction_window = config.at("ac_pred_window");
  const float* pred_window = prediction_window.data();
  int pw = static_cast<int>(*pred_window);

  std::unique_ptr<tf_model_backend> result;
  call_pybind_function([&]() {
    pybind11::module tf_ac_backend = pybind11::module::import(
        "turicreate.toolkits.activity_classifier._tf_model_architecture");

    // Make an instance of python object
    pybind11::object activity_classifier = tf_ac_backend.attr(
        "ActivityTensorFlowModel")(weights, n, c_in, c_out, pw, w_out);
    result.reset(new tf_model_backend(activity_classifier));
  });
  return result;
}

std::unique_ptr<image_augmenter> tf_compute_context::create_image_augmenter(
    const image_augmenter::options& opts) {
  return std::unique_ptr<image_augmenter>(new tf_image_augmenter(opts));
}

std::unique_ptr<model_backend> tf_compute_context::create_style_transfer(
      const float_array_map& config, const float_array_map& weights) {
  std::unique_ptr<tf_model_backend> result;
  call_pybind_function([&]() {
    pybind11::module tf_st_backend = pybind11::module::import(
        "turicreate.toolkits.style_transfer._tf_model_architecture");

    // Make an instance of python object
    pybind11::object style_transfer =
        tf_st_backend.attr("StyleTransferTensorFlowModel")(config, weights);
    result.reset(new tf_model_backend(style_transfer));
  });
  return result;
}

/**
 * TODO: Add proper arguments to create_drawing_classifier
 */
std::unique_ptr<model_backend> tf_compute_context::create_drawing_classifier(
    const float_array_map& weights,
    size_t batch_size, size_t num_classes) {
  std::unique_ptr<tf_model_backend> result;
  call_pybind_function([&]() {
    pybind11::module tf_dc_backend = pybind11::module::import(
        "turicreate.toolkits.drawing_classifier._tf_drawing_classifier");

    // Make an instance of python object
    pybind11::object drawing_classifier = tf_dc_backend.attr(
        "DrawingClassifierTensorFlowModel")(weights, batch_size, num_classes);
    result.reset(new tf_model_backend(drawing_classifier));
  });
  return result;
}

}  // namespace neural_net
}  // namespace turi

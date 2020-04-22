/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/
#include <ml/neural_net/tf_compute_context.hpp>

#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

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

 private:
  pybind11::object model_;
};

tf_model_backend::tf_model_backend(pybind11::object model) : model_(model) {}

float_array_map tf_model_backend::train(const float_array_map& inputs) {
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
  tf_image_augmenter(const options& opts, pybind11::object augmenter);

  ~tf_image_augmenter();

  float_array_result prepare_augmented_images(
      labeled_float_image data_to_augment) override;
 private:
  pybind11::object augmenter_;
  int random_seed_ = 0;
  int iteration_id_ = 0;
};

tf_image_augmenter::tf_image_augmenter(const options& opts,
                                       pybind11::object augmenter)
    : float_array_image_augmenter(opts),
      augmenter_(augmenter),
      random_seed_(opts.random_seed) {}

float_array_image_augmenter::float_array_result
tf_image_augmenter::prepare_augmented_images(
    float_array_image_augmenter::labeled_float_image data_to_augment) {
  float_array_image_augmenter::float_array_result image_annotations;

  call_pybind_function([&]() {
    // Use std::seed_seq to hash the iteration index into our random seed. Note
    // that the result must be unsigned, since numpy requires nonnegative seeds.
    std::seed_seq seq{random_seed_, ++iteration_id_};
    std::array<std::uint32_t, 1> random_seed;
    seq.generate(random_seed.begin(), random_seed.end());

    // Get augmented images and annotations from tensorflow
    pybind11::object augmented_data = augmenter_.attr("get_augmented_data")(
        data_to_augment.images, data_to_augment.annotations, random_seed[0]);
    std::pair<pybind11::buffer, std::vector<pybind11::buffer>> aug_data =
        augmented_data
            .cast<std::pair<pybind11::buffer, std::vector<pybind11::buffer>>>();

    pybind11::buffer_info buf_img = std::get<0>(aug_data).request();
    image_annotations.images = turi::neural_net::shared_float_array::copy(
        static_cast<float*>(buf_img.ptr),
        std::vector<size_t>(buf_img.shape.begin(), buf_img.shape.end()));
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

tf_image_augmenter::~tf_image_augmenter() {
  call_pybind_function([&]() { augmenter_ = pybind11::object(); });
}

namespace {

std::unique_ptr<compute_context> create_tf_compute_context() {
  return std::unique_ptr<compute_context>(new tf_compute_context);
}

// At static-init time, register create_tf_compute_context().
// TODO: Codify priority levels?
static auto* tf_registration __attribute__((unused)) =
  new compute_context::registration(
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

void tf_compute_context::print_training_device_info() const {
  bool has_gpu;

  call_pybind_function([&]() {
      pybind11::module tf_gpu_devices =
        pybind11::module::import("turicreate.toolkits._tf_utils");
      pybind11::object resp = tf_gpu_devices.attr("is_gpu_available")();
      has_gpu = resp.cast<bool>();
    });

  if (!has_gpu) {
    logprogress_stream << "Using CPU to create model.";
  } else {
    logprogress_stream << "Using a GPU to create model.";
  }
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
  return result;
}

std::unique_ptr<model_backend> tf_compute_context::create_activity_classifier(
    const ac_parameters& ac_params) {
  std::unique_ptr<tf_model_backend> result;
  call_pybind_function([&]() {
    pybind11::module tf_ac_backend = pybind11::module::import(
        "turicreate.toolkits.activity_classifier._tf_model_architecture");

    // Make an instance of python object
    pybind11::object activity_classifier =
        tf_ac_backend.attr("ActivityTensorFlowModel")(
            ac_params.weights, ac_params.batch_size, ac_params.num_features,
            ac_params.num_classes, ac_params.prediction_window,
            ac_params.num_predictions_per_chunk, ac_params.random_seed);
    result.reset(new tf_model_backend(activity_classifier));
  });

  return result;
}

std::unique_ptr<image_augmenter> tf_compute_context::create_image_augmenter(
    const image_augmenter::options& opts) {
  std::unique_ptr<tf_image_augmenter> result;
  
  call_pybind_function([&]() {

    const size_t output_height = opts.output_height;
    const size_t output_width = opts.output_width;
    const size_t batch_size = opts.batch_size;

    // TODO: Remove resize_only by passing all the augmentation options
    bool resize_only = false;
    if (opts.crop_prob == 0.f) {
      resize_only = true;
    }

    pybind11::module tf_aug = pybind11::module::import(
        "turicreate.toolkits.object_detector._tf_image_augmenter");

    // Make an instance of python object
    pybind11::object image_augmenter =
        tf_aug.attr("DataAugmenter")(output_height, output_width, batch_size, resize_only);
    result.reset(new tf_image_augmenter(opts, image_augmenter));
  });
  return result;
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
    const float_array_map& weights, size_t batch_size, size_t num_classes) {
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

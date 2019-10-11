/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
*/
#include <ml/neural_net/tf_compute_context.hpp>

#include <iostream>
#include <vector>

#include <core/util/try_finally.hpp>
#include <ml/neural_net/image_augmentation.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <model_server/extensions/additional_sframe_utilities.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace turi {
namespace neural_net {

using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::shared_float_array;
using turi::neural_net::labeled_image;

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

class tf_image_augmenter : public processed_image_augmenter {
 public:
  tf_image_augmenter(const options& opts);
  ~tf_image_augmenter();

  const options& get_options() const override { return opts_; }

  image_augmenter::result prepare_images(
      std::vector<labeled_image> source_batch) override;

  image_augmenter::intermediate_result prepare_augmented_images(
    image_augmenter::intermediate_labeled_image data_to_augment) override;

 private:
  options opts_;
};

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
  pybind11::class_<image_box>(m, "ImageBox")
    .def_readwrite("x", &image_box::x)
    .def_readwrite("y", &image_box::y)
    .def_readwrite("height", &image_box::height)
    .def_readwrite("width", &image_box::width);
  pybind11::class_<image_annotation>(m, "ImageAnnotation")
    .def_readwrite("identifier", &image_annotation::identifier)
    .def_readwrite("bounding_box", &image_annotation::bounding_box)
    .def_readwrite("confidence", &image_annotation::confidence);
  // pybind11::class_<image_type>(m, "ImageType");
  //   .def()
  // pybind11::class_<labeled_image>(m, "LabeledImage");
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

tf_compute_context::tf_compute_context() = default;

tf_compute_context::~tf_compute_context() = default;

size_t tf_compute_context::memory_budget() const {
  return 0;
}

std::vector<std::string> tf_compute_context::gpu_names() const {
  return std::vector<std::string>();
}

std::unique_ptr<model_backend> tf_compute_context::create_object_detector(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {

  pybind11::object object_detector;
  call_pybind_function([&]() {
      pybind11::module tf_od_backend = pybind11::module::import(
          "turicreate.toolkits.object_detector._tf_model_architecture");

      // Make an instance of python object
      object_detector = tf_od_backend.attr("ODTensorFlowModel")(h_in, w_in, n, c_out, weights, config);
    });
  return std::unique_ptr<tf_model_backend>(
      new tf_model_backend(object_detector));
}

std::unique_ptr<model_backend> tf_compute_context::create_activity_classifier(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {
  shared_float_array prediction_window = config.at("ac_pred_window");
  const float* pred_window = prediction_window.data();
  int pw = static_cast<int>(*pred_window);
  pybind11::object activity_classifier;
  call_pybind_function([&]() {
    pybind11::module tf_ac_backend = pybind11::module::import(
        "turicreate.toolkits.activity_classifier._tf_model_architecture");

    // Make an instance of python object
    activity_classifier = tf_ac_backend.attr("ActivityTensorFlowModel")(
        weights, n, c_in, c_out, pw, w_out);
  });
  return std::unique_ptr<tf_model_backend>(
      new tf_model_backend(activity_classifier));
  
}

std::unique_ptr<image_augmenter> tf_compute_context::create_image_augmenter(
    const image_augmenter::options& opts) {
  return std::unique_ptr<image_augmenter>(new tf_image_augmenter(opts));
}

tf_model_backend::tf_model_backend(pybind11::object model): model_(model) {}

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
    pybind11::object exported_weights = model_.attr("export_weights")();
    std::map<std::string, pybind11::buffer> buf_output =
        exported_weights.cast<std::map<std::string, pybind11::buffer>>();

    for (auto& kv : buf_output) {
      pybind11::buffer_info buf = kv.second.request();
      turi::neural_net::shared_float_array value =
          turi::neural_net::shared_float_array::copy(static_cast<float*>(buf.ptr),
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

tf_image_augmenter::tf_image_augmenter(const options& opts) : processed_image_augmenter(opts) {}

image_augmenter::result tf_image_augmenter::prepare_images( std::vector<labeled_image> source_batch) {
  return processed_image_augmenter::prepare_images(source_batch);
}

image_augmenter::intermediate_result tf_image_augmenter::prepare_augmented_images(
    image_augmenter::intermediate_labeled_image data_to_augment) {

  // const size_t n = opts_.batch_size;
  image_augmenter::intermediate_result image_annotations;

  call_pybind_function([&]() {

    // Pass the required data to tensorflow
    pybind11::module tf_aug = pybind11::module::import(
        "turicreate.toolkits.object_detector._tf_image_augmenter");

    // Get augmented images and annotations from tensorflow
    pybind11::object tf_augmentation = tf_aug.attr("DataAugmenter")(data_to_augment.images, data_to_augment.annotations, data_to_augment.predictions);
    pybind11::object augmented_img = tf_augmentation.attr("get_augmented_images")();
    pybind11::buffer aug_images = augmented_img.cast<pybind11::buffer>();
    pybind11::buffer_info buf_img = aug_images.request();
    image_annotations.images = 
          turi::neural_net::shared_float_array::copy(static_cast<float*>(buf_img.ptr),
              std::vector<size_t>(buf_img.shape.begin(), buf_img.shape.end()));
    pybind11::object augmented_ann = tf_augmentation.attr("get_augmented_annotations")();
    std::vector<pybind11::buffer> aug_annotations = augmented_ann.cast<std::vector<pybind11::buffer>>();
    std::vector<turi::neural_net::shared_float_array> annotations_per_batch;
    for (size_t i=0; i< aug_annotations.size(); i++) {
      pybind11::buffer_info buf_ann = aug_annotations[i].request();
      turi::neural_net::shared_float_array annotate = turi::neural_net::shared_float_array::copy(static_cast<float*>(buf_ann.ptr),
        std::vector<size_t>(buf_ann.shape.begin(), buf_ann.shape.end()));
      annotations_per_batch.push_back(annotate);
    }
    image_annotations.annotations = annotations_per_batch;

  });

  return image_annotations;
}

tf_image_augmenter::~tf_image_augmenter() {}

}  // namespace neural_net
}  // namespace turi

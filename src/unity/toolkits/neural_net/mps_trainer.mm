#import "mps_trainer.h"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include <logger/logger.hpp>
#include <unity/toolkits/neural_net/float_array.hpp>

#import "mps_cnnmodule.h"
#import "mps_device_manager.h"
#import "mps_utils.h"

using turi::neural_net::external_float_array;
using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::float_array_map_iterator;
using turi::neural_net::make_array_map;
using turi::neural_net::mps_cnn_module;
using turi::neural_net::shared_float_array;

int TCMPSCreateCNNModule(MPSHandle *out) {

  API_BEGIN();

  id <MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
  if (!dev) {
    log_and_throw("No valid Metal device.");
  }

  mps_cnn_module *mps = new mps_cnn_module(dev);
  *out = (void *)mps;

  API_END();
}

int TCMPSDeleteCNNModule(MPSHandle handle) {
  API_BEGIN();
  mps_cnn_module *obj = reinterpret_cast<mps_cnn_module*>(handle);
  delete obj;
  API_END();
}

int TCMPSCreateFloatArray(TCMPSFloatArrayRef *array_out, float* data,
                          size_t size, size_t* shape, size_t dim) {
  API_BEGIN();

  float_array* array = new external_float_array(data, size, shape, dim);
  *array_out = reinterpret_cast<TCMPSFloatArrayRef>(array);

  API_END();
}

int TCMPSDeleteFloatArray(TCMPSFloatArrayRef array) {
  API_BEGIN();

  delete reinterpret_cast<float_array*>(array);

  API_END();
}

int TCMPSGetFloatArrayShape(TCMPSFloatArrayRef array_ref,
                            size_t** shape_out, size_t* dim_out) {
  API_BEGIN();

  float_array* array = reinterpret_cast<float_array*>(array_ref);
  *shape_out = const_cast<size_t*>(array->shape());
  *dim_out = array->dim();

  API_END();
}

int TCMPSReadFloatArray(TCMPSFloatArrayRef array_ref, float** data_out,
                        size_t** shape_out, size_t* dim_out) {
  API_BEGIN();

  float_array* array = reinterpret_cast<float_array*>(array_ref);
  *data_out = const_cast<float*>(array->data());
  if (shape_out) {
    *shape_out = const_cast<size_t*>(array->shape());
  }
  if (dim_out) {
    *dim_out = array->dim();
  }

  API_END();
}

int TCMPSNextFloatArray(
    TCMPSFloatArrayMapIteratorRef iter_ref, char** name_out, float** data_out,
    size_t** shape_out, size_t* dim_out) {

  auto* iter = reinterpret_cast<float_array_map_iterator*>(iter_ref);
  if (!iter->has_next()) return -1;

  // Note that it is crucial that we do not copy anything out of the
  // float_array_map_iterator, so that the raw pointers we return point to
  // memory owned by the client via the float_array_map_iterator.
  const auto& name_and_float_array = iter->next();
  const std::string& name = name_and_float_array.first;
  const shared_float_array& float_array = name_and_float_array.second;
  *name_out = const_cast<char*>(name.c_str());
  *data_out = const_cast<float*>(float_array.data());
  *shape_out = const_cast<size_t*>(float_array.shape());
  *dim_out = float_array.dim();

  return 0;
}

int TCMPSDeleteFloatArrayMapIterator(TCMPSFloatArrayMapIteratorRef iter) {
  API_BEGIN();

  delete reinterpret_cast<float_array_map_iterator*>(iter);

  API_END();
}

int TCMPSInit(MPSHandle handle, int network_id, int n, int c_in, int h_in,
              int w_in, int c_out, int h_out, int w_out,  int updater_id,
              char **config_names, void **config_arrays, int config_len)
{
  API_BEGIN();

  float_array_map config =
      make_array_map(config_names, config_arrays, config_len);

  mps_cnn_module *obj = reinterpret_cast<mps_cnn_module*>(handle);
  obj->init(network_id, n, c_in, h_in, w_in, c_out, h_out, w_out, updater_id,
            config);

  API_END();
}

int TCMPSLoad(MPSHandle handle, char **names, void **arrs, int len) {
  API_BEGIN();

  float_array_map weights = make_array_map(names, arrs, len);

  mps_cnn_module *obj = reinterpret_cast<mps_cnn_module*>(handle);
  obj->load(weights);

  API_END();
}

int TCMPSExport(MPSHandle handle,
                TCMPSFloatArrayMapIteratorRef* float_array_map_out) {
  API_BEGIN();
  mps_cnn_module *obj = reinterpret_cast<mps_cnn_module*>(handle);
  auto* float_array_map = new float_array_map_iterator(obj->export_weights());
  *float_array_map_out =
      reinterpret_cast<TCMPSFloatArrayMapIteratorRef>(float_array_map);
  API_END();
}

int TCMPSPredict(MPSHandle handle, TCMPSFloatArrayRef input,
                 TCMPSFloatArrayRef labels, TCMPSFloatArrayRef weights,
                 TCMPSFloatArrayRef* fwd_out, TCMPSFloatArrayRef* loss_out) {

  API_BEGIN();

  mps_cnn_module *obj = reinterpret_cast<mps_cnn_module*>(handle);

  // This function can be called either on the training model (to compute
  // validation statistics), or on the prediction model. In the former case, we
  // expect validation labels and want to compute validation loss.
  float_array_map inputs;
  float_array* input_ptr = reinterpret_cast<float_array*>(input);
  inputs.emplace("input", std::make_shared<external_float_array>(*input_ptr));

  bool loss_image_required = labels != nullptr && weights != nullptr;
  if (loss_image_required) {
    float_array* labels_ptr = reinterpret_cast<float_array*>(labels);
    float_array* weights_ptr = reinterpret_cast<float_array*>(weights);
    inputs.emplace("labels",
                   std::make_shared<external_float_array>(*labels_ptr));
    inputs.emplace("weights",
                   std::make_shared<external_float_array>(*weights_ptr));
  }

  // Perform inference.
  float_array_map outputs = obj->predict(inputs);

  // Extract the desired results.
  shared_float_array* output = new shared_float_array(outputs.at("output"));
  *fwd_out = reinterpret_cast<TCMPSFloatArrayRef>(output);
  if (loss_out != nullptr) {
    shared_float_array* loss = new shared_float_array(outputs.at("loss"));
    *loss_out = reinterpret_cast<TCMPSFloatArrayRef>(loss);
  }

  API_END();
}

int TCMPSTrain(MPSHandle handle, TCMPSFloatArrayRef inputs,
               TCMPSFloatArrayRef labels, TCMPSFloatArrayRef weights,
               TCMPSFloatArrayRef* fwd_out, TCMPSFloatArrayRef* loss_out) {

  API_BEGIN();
  mps_cnn_module *obj = reinterpret_cast<mps_cnn_module*>(handle);
  float_array* inputs_ptr = reinterpret_cast<float_array*>(inputs);
  float_array* labels_ptr = reinterpret_cast<float_array*>(labels);
  float_array* weights_ptr = reinterpret_cast<float_array*>(weights);
  shared_float_array inputs_array(
      std::make_shared<external_float_array>(*inputs_ptr));
  shared_float_array labels_array(
      std::make_shared<external_float_array>(*labels_ptr));
  shared_float_array weights_array(
      std::make_shared<external_float_array>(*weights_ptr));
  auto outputs = obj->train({ { "input",   inputs_array  },
                              { "labels",  labels_array  },
                              { "weights", weights_array }  });
  shared_float_array* output = new shared_float_array(outputs.at("output"));
  shared_float_array* loss = new shared_float_array(outputs.at("loss"));
  *fwd_out = reinterpret_cast<TCMPSFloatArrayRef>(output);
  *loss_out = reinterpret_cast<TCMPSFloatArrayRef>(loss);
  API_END();
}

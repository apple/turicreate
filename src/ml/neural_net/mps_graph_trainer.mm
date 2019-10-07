#import "mps_graph_trainer.h"

#include <tuple>

#include <ml/neural_net/float_array.hpp>

#import "mps_device_manager.h"
#import "mps_graph_cnnmodule.h"

#include "mps_graph_networks.h"
#include "style_transfer/mps_style_transfer_backend.hpp"

using turi::neural_net::deferred_float_array;
using turi::neural_net::external_float_array;
using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::float_array_map_iterator;
using turi::neural_net::float_scalar;
using turi::neural_net::make_array_map;
using turi::neural_net::mps_graph_cnn_module;
using turi::neural_net::shared_float_array;

#ifdef HAS_MACOS_10_15
using turi::style_transfer::mps_style_transfer;
#endif

int TCMPSHasHighPowerMetalDevice(bool *has_device) {
  API_BEGIN();
  if (has_device) {
    id<MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
    *has_device = ((dev != nil) && (!dev.isLowPower));
  }
  API_END();
}

int TCMPSMetalDeviceName(char *name, int max_len) {
  API_BEGIN();
  id<MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
  if (dev == nil) {
    return 1;
  }
  strlcpy(name, [dev.name cStringUsingEncoding:NSUTF8StringEncoding], max_len);
  API_END();
}

int TCMPSMetalDeviceMemoryLimit(uint64_t *size) {
  API_BEGIN();

  id<MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
  if (dev == nil) {
    return 1;
  }

  *size = dev.recommendedMaxWorkingSetSize;

  API_END();
}

int TCMPSDeleteGraphModule(MPSHandle handle) {
  API_BEGIN();
  auto *obj = reinterpret_cast<turi::neural_net::model_backend *>(handle);
  delete obj;
  API_END();
}

EXPORT int TCMPSTrainStyleTransferGraph(MPSHandle handle, int index, TCMPSFloatArrayRef inputs,
                                        TCMPSFloatArrayRef labels, TCMPSFloatArrayRef *loss_out) {
  API_BEGIN();
#ifdef HAS_MACOS_10_15
  mps_style_transfer *obj = reinterpret_cast<mps_style_transfer *>(handle);
  float_array *inputs_ptr = reinterpret_cast<float_array *>(inputs);
  float_array *labels_ptr = reinterpret_cast<float_array *>(labels);

  shared_float_array inputs_array(std::make_shared<external_float_array>(*inputs_ptr));
  shared_float_array labels_array(std::make_shared<external_float_array>(*labels_ptr));
  shared_float_array index_array(std::make_shared<float_scalar>(index));

  auto outputs =
      obj->train({{"input", inputs_array}, {"labels", labels_array}, {"index", index_array}});
  shared_float_array *loss = new shared_float_array(outputs.at("loss"));
  *loss_out = reinterpret_cast<TCMPSFloatArrayRef>(loss);
#else
  log_and_throw("Can't construct GPU Style Transfer Network for MacOS \
                     platform lower than 10.15");
#endif
  API_END();
}

int TCMPSTrainGraph(MPSHandle handle, TCMPSFloatArrayRef inputs, TCMPSFloatArrayRef labels,
                    TCMPSFloatArrayRef *loss_out) {
  API_BEGIN();
  mps_graph_cnn_module *obj = (mps_graph_cnn_module *)handle;
  float_array *inputs_ptr = reinterpret_cast<float_array *>(inputs);
  float_array *labels_ptr = reinterpret_cast<float_array *>(labels);
  shared_float_array inputs_array(std::make_shared<external_float_array>(*inputs_ptr));
  shared_float_array labels_array(std::make_shared<external_float_array>(*labels_ptr));
  auto outputs = obj->train({{"input", inputs_array}, {"labels", labels_array}});
  shared_float_array *loss = new shared_float_array(outputs.at("loss"));
  *loss_out = reinterpret_cast<TCMPSFloatArrayRef>(loss);
  API_END();
}

int TCMPSPredictGraph(MPSHandle handle, TCMPSFloatArrayRef inputs,
                      TCMPSFloatArrayRef *outputs_ptr) {
  API_BEGIN();
  mps_graph_cnn_module *obj = (mps_graph_cnn_module *)handle;
  float_array *inputs_ptr = reinterpret_cast<float_array *>(inputs);
  shared_float_array inputs_array(std::make_shared<external_float_array>(*inputs_ptr));
  auto outputs = obj->predict({{"input", inputs_array}});
  shared_float_array *output = new shared_float_array(outputs.at("output"));
  *outputs_ptr = reinterpret_cast<TCMPSFloatArrayRef>(output);
  API_END();
}

int TCMPSTrainReturnGradGraph(MPSHandle handle, TCMPSFloatArrayRef inputs,
                              TCMPSFloatArrayRef gradient, TCMPSFloatArrayRef *outputs_ptr) {
  API_BEGIN();
  mps_graph_cnn_module *obj = (mps_graph_cnn_module *)handle;
  float_array *inputs_ptr = reinterpret_cast<float_array *>(inputs);
  float_array *gradient_ptr = reinterpret_cast<float_array *>(gradient);
  deferred_float_array *outputs =
      new deferred_float_array(obj->train_return_grad(*inputs_ptr, *gradient_ptr));
  *outputs_ptr = reinterpret_cast<TCMPSFloatArrayRef>(outputs);
  API_END();
}

int TCMPSCreateGraphModule(MPSHandle *handle, int network_id, int n, int c_in, int h_in, int w_in,
                           int c_out, int h_out, int w_out, char **config_names,
                           void **config_arrays, int config_len, char **weight_names,
                           void **weight_arrays, int weight_len) {
  API_BEGIN();
  float_array_map config = make_array_map(config_names, config_arrays, config_len);
  float_array_map weights = make_array_map(weight_names, weight_arrays, weight_len);

  if (network_id != static_cast<int>(turi::neural_net::kSTGraphNet)) {
    mps_graph_cnn_module *mps = new mps_graph_cnn_module();

    mps->init(network_id, n, c_in, h_in, w_in, c_out, h_out, w_out, config, weights);

    *handle = (void *)mps;
  } else {
#ifdef HAS_MACOS_10_15
    mps_style_transfer *mps = new mps_style_transfer(config, weights);

    *handle = (void *)mps;
#else
    log_and_throw("Can't construct GPU Style Transfer Network for MacOS \
                     platform lower than 10.15");
#endif
  }
  API_END();
}

int TCMPSExportGraph(MPSHandle handle, TCMPSFloatArrayMapIteratorRef *float_array_map_out) {
  API_BEGIN();
  mps_graph_cnn_module *obj = (mps_graph_cnn_module *)handle;
  auto *float_array_map = new float_array_map_iterator(obj->export_weights());
  *float_array_map_out = reinterpret_cast<TCMPSFloatArrayMapIteratorRef>(float_array_map);
  API_END();
}

int TCMPSSetLearningRateGraph(MPSHandle handle, float new_lr) {
  API_BEGIN();
  mps_graph_cnn_module *obj = (mps_graph_cnn_module *)handle;
  obj->set_learning_rate(new_lr);
  API_END();
}

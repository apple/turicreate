#import "mps_graph_trainer.h"

#include <tuple>

#include "mps_float_array.hpp"

#import "mps_device_manager.h"
#import "mps_graph_cnnmodule.h"

using turi::mps::MPSGraphModule;
using turi::mps::deferred_float_array;
using turi::mps::float_array;
using turi::mps::float_array_map;
using turi::mps::float_array_map_iterator;
using turi::mps::make_array_map;

int TCMPSHasHighPowerMetalDevice(bool *has_device) {
  API_BEGIN();
  if (has_device) {
    id <MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
    *has_device = ((dev != nil) && (!dev.isLowPower));
  }
  API_END();
}

int TCMPSMetalDeviceName(char *name, int max_len) {
  API_BEGIN();
  id <MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
  if (dev == nil) {
    return 1;
  }
  strlcpy(name, [dev.name cStringUsingEncoding:NSUTF8StringEncoding], max_len);
  API_END();
}

int TCMPSMetalDeviceMemoryLimit(uint64_t *size) {
  API_BEGIN();

  id <MTLDevice> dev = [[TCMPSDeviceManager sharedInstance] preferredDevice];
  if (dev == nil) {
    return 1;
  }

  *size = dev.recommendedMaxWorkingSetSize;

  API_END();
}

int TCMPSCreateGraphModule(MPSHandle *out) {
  API_BEGIN();
  MPSGraphModule *mps = new MPSGraphModule();
  *out = (void *)mps;
  API_END();
}

int TCMPSDeleteGraphModule(MPSHandle handle) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  delete obj;
  API_END();
}

int TCMPSTrainGraph(MPSHandle handle, TCMPSFloatArrayRef inputs,
                    TCMPSFloatArrayRef labels, TCMPSFloatArrayRef* loss_out) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  float_array* inputs_ptr = reinterpret_cast<float_array*>(inputs);
  float_array* labels_ptr = reinterpret_cast<float_array*>(labels);
  deferred_float_array* loss =
      new deferred_float_array(obj->Train(*inputs_ptr, *labels_ptr));
  *loss_out = reinterpret_cast<TCMPSFloatArrayRef>(loss);
  API_END();
}

int TCMPSPredictGraph(MPSHandle handle, TCMPSFloatArrayRef inputs,
                      TCMPSFloatArrayRef* outputs_ptr) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  float_array* inputs_ptr = reinterpret_cast<float_array*>(inputs);
  deferred_float_array* outputs =
      new deferred_float_array(obj->Predict(*inputs_ptr));
  *outputs_ptr = reinterpret_cast<TCMPSFloatArrayRef>(outputs);
  API_END();
}

int TCMPSTrainReturnGradGraph(
    MPSHandle handle, TCMPSFloatArrayRef inputs, TCMPSFloatArrayRef gradient,
    TCMPSFloatArrayRef* outputs_ptr) {

  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  float_array* inputs_ptr = reinterpret_cast<float_array*>(inputs);
  float_array* gradient_ptr = reinterpret_cast<float_array*>(gradient);
  deferred_float_array* outputs = new deferred_float_array(
      obj->TrainReturnGrad(*inputs_ptr, *gradient_ptr));
  *outputs_ptr = reinterpret_cast<TCMPSFloatArrayRef>(outputs);
  API_END();
}

int TCMPSInitGraph(MPSHandle handle, int network_id, int n, int c_in, int h_in, int w_in,
              int c_out, int h_out, int w_out,
              char **config_names, void **config_arrays,
              int64_t *config_sizes, int config_len,
              char **weight_names, void **weight_arrays,
              int64_t *weight_sizes, int weight_len) {
  API_BEGIN();
  
  float_array_map config =
      make_array_map(config_names, config_arrays, config_sizes, config_len);
  float_array_map weights =
      make_array_map(weight_names, weight_arrays, weight_sizes, weight_len);

  MPSGraphModule *obj = (MPSGraphModule *)handle;
  obj->Init(network_id, n, c_in, h_in, w_in, c_out, h_out, w_out,
            config, weights);
  API_END();
}

int TCMPSExportGraph(MPSHandle handle,
                     TCMPSFloatArrayMapIteratorRef* float_array_map_out) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  auto* float_array_map = new float_array_map_iterator(obj->Export());
  *float_array_map_out =
      reinterpret_cast<TCMPSFloatArrayMapIteratorRef>(float_array_map);
  API_END();
}

int TCMPSSetLearningRateGraph(MPSHandle handle, float new_lr) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  obj->SetLearningRate(new_lr);
  API_END();
}

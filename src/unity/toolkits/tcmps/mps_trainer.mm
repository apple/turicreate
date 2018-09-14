#import "mps_trainer.h"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "mps_float_array.hpp"

#import "mps_cnnmodule.h"
#import "mps_utils.h"

using turi::mps::external_float_array;
using turi::mps::float_array;
using turi::mps::FloatArrayMap;
using turi::mps::MPSCNNModule;
using turi::mps::make_array_map;

int TCMPSCreateCNNModule(MPSHandle *out) {
  API_BEGIN();
  MPSCNNModule *mps = new MPSCNNModule();
  *out = (void *)mps;
  API_END();
}

int TCMPSDeleteCNNModule(MPSHandle handle) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
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

int TCMPSForward(MPSHandle handle, TCMPSFloatArrayRef inputs, float *out,
                 bool is_train) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  float_array* inputs_array = reinterpret_cast<float_array*>(inputs);
  obj->Forward(*inputs_array, out, is_train);
  API_END();
}

int TCMPSBackward(MPSHandle handle, TCMPSFloatArrayRef gradient,
             float *out) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  float_array* gradient_array = reinterpret_cast<float_array*>(gradient);
  obj->Backward(*gradient_array, out);
  API_END();
}

int TCMPSLoss(MPSHandle handle, TCMPSFloatArrayRef inputs,
              TCMPSFloatArrayRef labels, TCMPSFloatArrayRef weights,
              bool loss_image_required, float *out) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  float_array* inputs_array = reinterpret_cast<float_array*>(inputs);
  float_array* labels_array = reinterpret_cast<float_array*>(labels);
  float_array* weights_array = reinterpret_cast<float_array*>(weights);
  obj->Loss(*inputs_array, *labels_array, *weights_array, loss_image_required,
            out);
  API_END();
}

int TCMPSForwardBackward(
    MPSHandle handle, TCMPSFloatArrayRef inputs, TCMPSFloatArrayRef labels,
    TCMPSFloatArrayRef weights, bool loss_image_required, float *out) {

  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  float_array* inputs_array = reinterpret_cast<float_array*>(inputs);
  float_array* labels_array = reinterpret_cast<float_array*>(labels);
  float_array* weights_array = reinterpret_cast<float_array*>(weights);
  obj->ForwardBackward(*inputs_array, *labels_array, *weights_array,
                       loss_image_required, out);
  API_END();
}

int TCMPSForwardWithLoss(
    MPSHandle handle, TCMPSFloatArrayRef inputs, TCMPSFloatArrayRef labels,
    TCMPSFloatArrayRef weights, bool loss_image_required, bool is_train,
    float *out) {

  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  float_array* inputs_array = reinterpret_cast<float_array*>(inputs);
  float_array* labels_array = reinterpret_cast<float_array*>(labels);
  float_array* weights_array = reinterpret_cast<float_array*>(weights);
  obj->Forward(*inputs_array, *labels_array, *weights_array,
               loss_image_required, is_train, out);
  API_END();
}

int TCMPSGetLossImages(MPSHandle handle, float *out) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->GetLossImages(out);
  API_END();
}

int TCMPSBeginForwardBatch(
    MPSHandle handle, int batch_id, TCMPSFloatArrayRef inputs,
    TCMPSFloatArrayRef labels, TCMPSFloatArrayRef weights,
    bool loss_image_required, bool is_train) {

  API_BEGIN();
  MPSCNNModule *obj = reinterpret_cast<MPSCNNModule *>(handle);
  float_array* inputs_array = reinterpret_cast<float_array*>(inputs);
  float_array* labels_array = reinterpret_cast<float_array*>(labels);
  float_array* weights_array = reinterpret_cast<float_array*>(weights);
  obj->BeginForwardBatch(batch_id, *inputs_array, *labels_array, *weights_array,
                         loss_image_required, is_train);
  API_END();
}

int TCMPSBeginForwardBackwardBatch(
    MPSHandle handle, int batch_id, TCMPSFloatArrayRef inputs,
    TCMPSFloatArrayRef labels, TCMPSFloatArrayRef weights,
    bool loss_image_required) {

  API_BEGIN();
  MPSCNNModule *obj = reinterpret_cast<MPSCNNModule *>(handle);
  float_array* inputs_array = reinterpret_cast<float_array*>(inputs);
  float_array* labels_array = reinterpret_cast<float_array*>(labels);
  float_array* weights_array = reinterpret_cast<float_array*>(weights);
  obj->BeginForwardBackwardBatch(batch_id, *inputs_array, *labels_array,
                                 *weights_array, loss_image_required);
  API_END();
}

int TCMPSWaitForBatch(MPSHandle handle, int batch_id, float *forward_out,
                 float *loss_out) {
  API_BEGIN();
  MPSCNNModule *obj = reinterpret_cast<MPSCNNModule *>(handle);
  obj->WaitForBatch(batch_id, forward_out, loss_out);
  API_END();
}

int TCMPSInit(MPSHandle handle, int network_id, int n, int c_in, int h_in, int w_in,
         int c_out, int h_out, int w_out,  int updater_id,
         char **config_names, void **config_arrays,
         int64_t *config_sizes, int config_len) {
  API_BEGIN();

  FloatArrayMap config = make_array_map(config_names, config_arrays, config_sizes, config_len);

  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->Init(network_id, n, c_in, h_in, w_in, c_out, h_out, w_out, updater_id, config);
  API_END();
}

int TCMPSLoad(MPSHandle handle, char **names, void **arrs, int64_t *sz, int len) {
  API_BEGIN();

  FloatArrayMap weights = make_array_map(names, arrs, sz, len);

  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->Load(weights);
  API_END();
}

int TCMPSNumParams(MPSHandle handle, int *num) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  *num = obj->NumParams();
  API_END();
}

int TCMPSExport(MPSHandle handle, char **names, void **arrs, int64_t *dim,
           int **shape) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->Export();
  auto &table = obj->table_;
  int cnt = 0;
  for (auto &p : table) {
    names[cnt] = (char *)std::get<0>(p.second).c_str();
    arrs[cnt] = (void *)std::get<1>(p.second);
    dim[cnt] = std::get<2>(p.second);
    shape[cnt++] = &(std::get<3>(p.second)[0]);
  }
  API_END();
}

int TCMPSCpuUpdate(MPSHandle handle) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->Update();
  API_END();
}

int TCMPSUpdate(MPSHandle handle){
    API_BEGIN();
    MPSCNNModule *obj = (MPSCNNModule *)handle;
    obj->GpuUpdate();
    API_END();
}

int TCMPSSetLearningRate(MPSHandle handle, float new_lr) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->SetLearningRate(new_lr);
  API_END();
}

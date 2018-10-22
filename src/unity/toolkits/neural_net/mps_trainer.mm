#import "mps_trainer.h"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include <unity/toolkits/neural_net/float_array.hpp>

#import "mps_cnnmodule.h"
#import "mps_utils.h"

using turi::neural_net::MPSCNNModule;
using turi::neural_net::external_float_array;
using turi::neural_net::float_array;
using turi::neural_net::float_array_map;
using turi::neural_net::float_array_map_iterator;
using turi::neural_net::make_array_map;
using turi::neural_net::shared_float_array;

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

  float_array_map config =
      make_array_map(config_names, config_arrays, config_sizes, config_len);

  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->Init(network_id, n, c_in, h_in, w_in, c_out, h_out, w_out, updater_id, config);
  API_END();
}

int TCMPSLoad(MPSHandle handle, char **names, void **arrs, int64_t *sz, int len) {
  API_BEGIN();

  float_array_map weights = make_array_map(names, arrs, sz, len);

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

int TCMPSExport(MPSHandle handle,
                TCMPSFloatArrayMapIteratorRef* float_array_map_out) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  auto* float_array_map = new float_array_map_iterator(obj->Export());
  *float_array_map_out =
      reinterpret_cast<TCMPSFloatArrayMapIteratorRef>(float_array_map);
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

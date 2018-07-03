#include "mps_trainer.h"
#import "iostream"
#include "mps_cnnmodule.h"
#import "string"
#import "unordered_map"
#import "mps_utils.h"

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

int TCMPSForward(MPSHandle handle, void *ptr, int64_t sz, int64_t *shape, int dim,
            float *out, bool is_train) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->Forward(ptr, sz, shape, dim, out, is_train);
  API_END();
}

int TCMPSBackward(MPSHandle handle, void *ptr, int64_t sz, int64_t *shape, int dim,
             float *out) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->Backward(ptr, sz, shape, dim, out);
  API_END();
}

int TCMPSLoss(MPSHandle handle, void *ptr, size_t sz, int64_t *shape, int dim,
         void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
         void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
         bool loss_image_required,
         float *out) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->Loss(ptr, sz, shape, dim,
            label_ptr, label_sz, label_shape, label_dim,
            weight_ptr, weight_sz, weight_shape, weight_dim,
            loss_image_required,
            out);
  API_END();
}

int TCMPSForwardBackward(MPSHandle handle, void *ptr, size_t sz, int64_t *shape, int dim,
         void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
         void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
         bool loss_image_required,
         float *out) {
    API_BEGIN();
    MPSCNNModule *obj = (MPSCNNModule *)handle;
    obj->ForwardBackward(ptr, sz, shape, dim,
                         label_ptr, label_sz, label_shape, label_dim,
                         weight_ptr, weight_sz, weight_shape, weight_dim,
                         loss_image_required,
                         out);
    API_END();
}

int TCMPSForwardWithLoss(MPSHandle handle, void *ptr, size_t sz, int64_t *shape, int dim,
                    void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
                    void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
                    bool loss_image_required, bool is_train,
                    float *out) {
    API_BEGIN();
    MPSCNNModule *obj = (MPSCNNModule *)handle;
    obj->Forward(ptr, sz, shape, dim,
                 label_ptr, label_sz, label_shape, label_dim,
                 weight_ptr, weight_sz, weight_shape, weight_dim,
                 loss_image_required, is_train,
                 out);
    API_END();
}

int TCMPSGetLossImages(MPSHandle handle, float *out) {
  API_BEGIN();
  MPSCNNModule *obj = (MPSCNNModule *)handle;
  obj->GetLossImages(out);
  API_END();
}

int TCMPSBeginForwardBatch(
    MPSHandle handle, int batch_id, void *ptr, size_t sz, int64_t *shape, int dim,
    void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
    void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
    bool loss_image_required, bool is_train) {
  API_BEGIN();
  MPSCNNModule *obj = reinterpret_cast<MPSCNNModule *>(handle);
  obj->BeginForwardBatch(batch_id, ptr, sz, shape, dim,
                         label_ptr, label_sz, label_shape, label_dim,
                         weight_ptr, weight_sz, weight_shape, weight_dim,
                         loss_image_required, is_train);
  API_END();
}

int TCMPSBeginForwardBackwardBatch(
    MPSHandle handle, int batch_id, void *ptr, size_t sz, int64_t *shape, int dim,
    void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
    void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
    bool loss_image_required) {
  API_BEGIN();
  MPSCNNModule *obj = reinterpret_cast<MPSCNNModule *>(handle);
  obj->BeginForwardBackwardBatch(batch_id, ptr, sz, shape, dim,
                                 label_ptr, label_sz, label_shape, label_dim,
                                 weight_ptr, weight_sz, weight_shape, weight_dim,
                                 loss_image_required);
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

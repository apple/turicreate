#include "mps_graph_trainer.h"
#import "iostream"
#include "mps_graph_cnnmodule.h"
#import "string"
#import "unordered_map"

int HasHighPowerMetalDevice(bool *has_device) {
  API_BEGIN();
  if (has_device) {
    id<MTLDevice> dev = MetalDevice::Get()->dev;
    *has_device = ((dev != nil) && (!dev.isLowPower));
  }
  API_END();
}

int MetalDeviceName(char *name, int max_len) {
  API_BEGIN();
  id<MTLDevice> dev = MetalDevice::Get()->dev;
  if (dev == nil) {
    return 1;
  }
  strlcpy(name, [dev.name cStringUsingEncoding:NSUTF8StringEncoding], max_len);
  API_END();
}

int CreateMPSGraph(MPSHandle *out) {
  API_BEGIN();
  MPSGraphModule *mps = new MPSGraphModule();
  *out = (void *)mps;
  API_END();
}

int DeleteMPSGraph(MPSHandle handle) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  delete obj;
  API_END();
}

int SetInputGraph(MPSHandle handle, void *ptr, int64_t sz, int64_t *shape, int dim,
             int flag) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  obj->SetInput(ptr, sz, shape, dim, flag);
  API_END();
}

int SetLossStateGraph(MPSHandle handle, void *ptr) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  obj->SetLossState((float *)ptr);
  API_END();
}

int RunGraph(MPSHandle handle, float *out, float *loss) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  obj->RunGraph(out, loss);
  API_END();
}

int WaitUntilCompletedGraph(MPSHandle handle) {
    API_BEGIN();
    MPSGraphModule *obj = (MPSGraphModule *)handle;
    obj->WaitUntilCompleted();
    API_END();
}

int InitGraph(MPSHandle handle, int network_id, int n, int c_in, int h_in, int w_in,
              int c_out, int h_out, int w_out,
              char **config_names, void **config_arrays,
              int64_t *config_sizes, int config_len,
              char **weight_names, void **weight_arrays,
              int64_t *weight_sizes, int weight_len) {
  API_BEGIN();
  
  FloatArrayMap config = make_array_map(config_names, config_arrays, config_sizes, config_len);
  FloatArrayMap weights = make_array_map(weight_names, weight_arrays, weight_sizes, weight_len);

  MPSGraphModule *obj = (MPSGraphModule *)handle;
  obj->Init(network_id, n, c_in, h_in, w_in, c_out, h_out, w_out,
            config, weights);
  API_END();
}

int NumParamsGraph(MPSHandle handle, int *num) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  *num = obj->NumParams();
  API_END();
}

int ExportGraph(MPSHandle handle, char **names, void **arrs, int64_t *dim,
           int **shape) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
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

int SetLearningRateGraph(MPSHandle handle, float new_lr) {
  API_BEGIN();
  MPSGraphModule *obj = (MPSGraphModule *)handle;
  obj->SetLearningRate(new_lr);
  API_END();
}

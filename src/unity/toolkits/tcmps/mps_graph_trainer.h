#ifndef GRAPH_C_API_H_
#define GRAPH_C_API_H_

#import "mps_cnnmodule.h"
#import "mps_graph_cnnmodule.h"
#import "mps_trainer.h"
#import <exception>

#define EXPORT __attribute__((visibility("default")))

/*! \brief  macro to guard beginning and end section of all functions */
#define API_BEGIN() try {
//\brief every function starts with API_BEGIN();
//     and finishes with API_END() or API_END_HANDLE_ERROR
#define API_END()                                                              \
  }                                                                            \
  catch (...) {                                                                \
    NSLog(@"Error");                                                           \
    return -1;                                                                 \
  }                                                                            \
  return 0; // NOLINT(*)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void *MPSHandle;

EXPORT int HasHighPowerMetalDevice(bool *has_device);
EXPORT int MetalDeviceName(char *name, int max_len);

EXPORT int CreateMPSGraph(MPSHandle *handle);
EXPORT int DeleteMPSGraph(MPSHandle handle);

EXPORT int SetInputGraph(MPSHandle handle, void *ptr, int64_t sz, int64_t *shape, int dim, int flag);
EXPORT int SetLossStateGraph(MPSHandle handle, void *ptr);

EXPORT int RunGraph(MPSHandle handle, float *out, float *loss);
EXPORT int WaitUntilCompletedGraph(MPSHandle handle);

EXPORT int InitGraph(MPSHandle handle, int network_id, int n, int c_in, int h_in, int w_in,
                     int c_out, int h_out, int w_out,
                     char **config_names, void **config_arrays,
                     int64_t *config_sizes, int config_len,
                     char **weight_names, void **weight_arrays,
                     int64_t *weight_sizes, int weight_len);

EXPORT int NumParamsGraph(MPSHandle handle, int *num);

EXPORT int ExportGraph(MPSHandle handle, char **names, void **arrs, int64_t *dim,
           int **shape);

EXPORT int SetLearningRateGraph(MPSHandle handle, float new_lr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif

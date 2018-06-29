#ifndef C_API_H_
#define C_API_H_

#import "mps_cnnmodule.h"
#import <exception>

#define EXPORT __attribute__((visibility("default")))

/*! \brief  macro to guard beginning and end section of all functions */
#define API_BEGIN()                                                            \
  @autoreleasepool {                                                           \
  try {

/*! \brief every function starts with API_BEGIN();
     and finishes with API_END() or API_END_HANDLE_ERROR */
#define API_END()                                                              \
  }  /* try */                                                                 \
  catch (...) {                                                                \
    NSLog(@"Error");                                                           \
    return -1;                                                                 \
  }                                                                            \
  return 0;                                                                    \
  }  /* @autoreleasepool */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void *MPSHandle;

EXPORT int TCMPSCreateCNNModule(MPSHandle *handle);
EXPORT int TCMPSDeleteCNNModule(MPSHandle handle);

EXPORT int TCMPSForward(MPSHandle handle, void *ptr, int64_t sz, int64_t *shape, int dim,
                   float *out, bool is_train);

EXPORT int TCMPSBackward(MPSHandle handle, void *ptr, int64_t sz, int64_t *shape, int dim,
                    float *out);

EXPORT int TCMPSLoss(MPSHandle handle, void *ptr, size_t sz, int64_t *shape, int dim,
                void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
                void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
                bool loss_image_required,
                float *out);
    
EXPORT int TCMPSForwardBackward(MPSHandle handle, void *ptr, size_t sz, int64_t *shape, int dim,
                           void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
                           void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
                           bool loss_image_required,
                           float *out);

EXPORT int TCMPSForwardWithLoss(MPSHandle handle, void *ptr, size_t sz, int64_t *shape, int dim,
                           void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
                           void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
                           bool loss_image_required, bool is_train,
                           float *out);

EXPORT int TCMPSGetLossImages(MPSHandle handle, float *out);

EXPORT int TCMPSBeginForwardBatch(
    MPSHandle handle, int batch_id, void *ptr, size_t sz, int64_t *shape, int dim,
    void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
    void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
    bool loss_image_required, bool is_train);

EXPORT int TCMPSBeginForwardBackwardBatch(
    MPSHandle handle, int batch_id, void *ptr, size_t sz, int64_t *shape, int dim,
    void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
    void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
    bool loss_image_required);

EXPORT int TCMPSWaitForBatch(MPSHandle handle, int batch_id, float *forward_out,
                        float *loss_out);

EXPORT int TCMPSInit(MPSHandle handle, int network_id, int n, int c_in, int h_in, int w_in,
                int c_out, int h_out, int w_out, int updater_id,
                char **config_names, void **config_arrays,
                int64_t *config_sizes, int config_len);

EXPORT int TCMPSLoad(MPSHandle handle, char **names, void **arrs, int64_t *sz, int len);

EXPORT int TCMPSNumParams(MPSHandle handle, int *num);

EXPORT int TCMPSExport(MPSHandle handle, char **names, void **arrs, int64_t *dim,
                  int **shape);

EXPORT int TCMPSCpuUpdate(MPSHandle handle);
EXPORT int TCMPSUpdate(MPSHandle handle);

EXPORT int TCMPSSetLearningRate(MPSHandle handle, float new_lr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif

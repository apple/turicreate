#ifndef C_API_H_
#define C_API_H_

#import "mps_cnnmodule.h"
#import <exception>

#define EXPORT __attribute__((visibility("default")))

/*! \brief  macro to guard beginning and end section of all functions */
#define API_BEGIN() try {
/*! \brief every function starts with API_BEGIN();
     and finishes with API_END() or API_END_HANDLE_ERROR */
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

EXPORT int CreateMPS(MPSHandle *handle);
EXPORT int DeleteMPS(MPSHandle handle);

EXPORT int Forward(MPSHandle handle, void *ptr, int64_t sz, int64_t *shape, int dim,
                   float *out, bool is_train);

EXPORT int Backward(MPSHandle handle, void *ptr, int64_t sz, int64_t *shape, int dim,
                    float *out);

EXPORT int Loss(MPSHandle handle, void *ptr, size_t sz, int64_t *shape, int dim,
                void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
                void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
                bool loss_image_required,
                float *out);
    
EXPORT int ForwardBackward(MPSHandle handle, void *ptr, size_t sz, int64_t *shape, int dim,
                           void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
                           void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
                           bool loss_image_required,
                           float *out);

EXPORT int ForwardWithLoss(MPSHandle handle, void *ptr, size_t sz, int64_t *shape, int dim,
                           void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
                           void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
                           bool loss_image_required, bool is_train,
                           float *out);

EXPORT int GetLossImages(MPSHandle handle, float *out);

EXPORT int BeginForwardBatch(
    MPSHandle handle, int batch_id, void *ptr, size_t sz, int64_t *shape, int dim,
    void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
    void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
    bool loss_image_required, bool is_train);

EXPORT int BeginForwardBackwardBatch(
    MPSHandle handle, int batch_id, void *ptr, size_t sz, int64_t *shape, int dim,
    void *label_ptr, size_t label_sz, int64_t *label_shape, int label_dim,
    void *weight_ptr, size_t weight_sz, int64_t *weight_shape, int weight_dim,
    bool loss_image_required);

EXPORT int WaitForBatch(MPSHandle handle, int batch_id, float *forward_out,
                        float *loss_out);

EXPORT int Init(MPSHandle handle, int network_id, int n, int c_in, int h_in, int w_in,
                int c_out, int h_out, int w_out, int updater_id,
                char **config_names, void **config_arrays,
                int64_t *config_sizes, int config_len);

EXPORT int Load(MPSHandle handle, char **names, void **arrs, int64_t *sz, int len);

EXPORT int NumParams(MPSHandle handle, int *num);

EXPORT int Export(MPSHandle handle, char **names, void **arrs, int64_t *dim,
                  int **shape);

EXPORT int CpuUpdate(MPSHandle handle);
EXPORT int Update(MPSHandle handle);

EXPORT int SetLearningRate(MPSHandle handle, float new_lr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif

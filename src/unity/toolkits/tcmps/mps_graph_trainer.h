#ifndef GRAPH_C_API_H_
#define GRAPH_C_API_H_

#import "mps_trainer.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

EXPORT int TCMPSHasHighPowerMetalDevice(bool *has_device);
EXPORT int TCMPSMetalDeviceName(char *name, int max_len);
EXPORT int TCMPSMetalDeviceMemoryLimit(uint64_t *size);

EXPORT int TCMPSCreateGraphModule(MPSHandle *handle);
EXPORT int TCMPSDeleteGraphModule(MPSHandle handle);

EXPORT int TCMPSStartTrainingBatchGraph(MPSHandle handle, void *ptr, int64_t sz,
                                   int64_t *shape, int dim, float *labels_ptr);
EXPORT int TCMPSWaitForTrainingBatchGraph(MPSHandle handle, float *loss);

EXPORT int TCMPSStartInferenceBatchGraph(MPSHandle handle, void *ptr, int64_t sz,
                                    int64_t *shape, int dim);
EXPORT int TCMPSWaitForInferenceBatchGraph(MPSHandle handle, float *out_ptr);

EXPORT int TCMPSStartTrainReturnGradBatchGraph(
    MPSHandle handle, void *ptr, int64_t sz, int64_t *shape, int dim,
    void *grad_ptr, int64_t grad_sz, int64_t *grad_shape, int grad_dim);
EXPORT int TCMPSWaitForTrainReturnGradBatchGraph(MPSHandle handle, float *out_ptr);

EXPORT int TCMPSInitGraph(MPSHandle handle, int network_id, int n, int c_in, int h_in, int w_in,
                     int c_out, int h_out, int w_out,
                     char **config_names, void **config_arrays,
                     int64_t *config_sizes, int config_len,
                     char **weight_names, void **weight_arrays,
                     int64_t *weight_sizes, int weight_len);

EXPORT int TCMPSNumParamsGraph(MPSHandle handle, int *num);

EXPORT int TCMPSExportGraph(MPSHandle handle, char **names, void **arrs, int64_t *dim,
           int **shape);

EXPORT int TCMPSSetLearningRateGraph(MPSHandle handle, float new_lr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif

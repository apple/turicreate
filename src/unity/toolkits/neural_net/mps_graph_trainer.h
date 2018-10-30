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

EXPORT int TCMPSInitGraph(MPSHandle handle, int network_id, int n, int c_in, int h_in, int w_in,
                     int c_out, int h_out, int w_out,
                     char **config_names, void **config_arrays, int config_len,
                     char **weight_names, void **weight_arrays, int weight_len);

EXPORT int TCMPSSetLearningRateGraph(MPSHandle handle, float new_lr);

EXPORT int TCMPSTrainGraph(MPSHandle handle,
                           TCMPSFloatArrayRef inputs, TCMPSFloatArrayRef labels,
                           TCMPSFloatArrayRef* loss_out);

EXPORT int TCMPSPredictGraph(
    MPSHandle handle, TCMPSFloatArrayRef inputs, TCMPSFloatArrayRef* outputs);

EXPORT int TCMPSTrainReturnGradGraph(
    MPSHandle handle, TCMPSFloatArrayRef inputs, TCMPSFloatArrayRef gradient,
    TCMPSFloatArrayRef* outputs);

EXPORT int TCMPSExportGraph(MPSHandle handle,
                            TCMPSFloatArrayMapIteratorRef* float_array_map_out);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif

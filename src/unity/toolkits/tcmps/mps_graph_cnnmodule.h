#ifndef MPS_GRAPH_MODULE_H_
#define MPS_GRAPH_MODULE_H_

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#import "mps_utils.h"
#import "mps_graph_networks.h"

NS_ASSUME_NONNULL_BEGIN

// Forward declaration of internal Objective-C class used in the implementation
// of MPSGraphModule below.
@class TCMPSGraphModuleBatch;

namespace turi {
namespace mps {

class MPSGraphModule {
public:
  MPSGraphModule();

  void Init(int network_id, int n, int c_in, int h_in, int w_in, int c_out,
            int h_out, int w_out,
            const FloatArrayMap &config,
            const FloatArrayMap &weights);

  // Each call to a Start*Batch function must be paired with a call to a
  // WaitFor*Batch function. These functions must match the graph mode used to
  // initialize the module.
  //
  // TODO: Replace this API with a more user-friendly one: each Start*Batch
  // function could return a handle wrapping the MPSImage containing the
  // expected output. Attempting to read from the MPSImage would trigger a wait
  // if necessary, a la MXNet NDArray.

  // Training
  void SetLearningRate(float lr);
  void StartTrainingBatch(void *ptr, int64_t sz, int64_t *shape, int dim,
                          float *labels_ptr);
  void WaitForTrainingBatch(float *loss);

  // Inference
  void StartInferenceBatch(void *ptr, int64_t sz, int64_t *shape, int dim);
  void WaitForInferenceBatch(float *out_ptr);

  // Forward-backward pass with specified input and top-gradient images
  void StartTrainReturnGradBatch(void *ptr, int64_t sz, int64_t *shape, int dim,
                                 void *grad_ptr, int64_t grad_sz,
                                 int64_t *grad_shape, int grad_dim);
  void WaitForTrainReturnGradBatch(float *out_ptr);

  void Export();
  int NumParams();

  std::unordered_map<std::string,
                     std::tuple<std::string, float *, int, std::vector<int>>>
      table_;

private:
  MPSImageBatch *CreateImageBatch(MPSImageDescriptor *desc);
  MPSImageBatch *CopyInput(void *ptr, int64_t sz, int64_t *shape, int dim);
  MPSImageBatch *CopyGrad(void *ptr, int64_t sz, int64_t *shape, int dim);
  MPSCNNLossLabelsBatch *CopyLabels(float *ptr);

  void Blob2MPSImage(float *ptr, MPSImageBatch *batch);
  void MPSImage2Blob(float *ptr, MPSImageBatch *batch);

  id<MTLDevice> dev_;
  id<MTLCommandQueue> cmd_queue_;
  NSMutableArray<TCMPSGraphModuleBatch *> *pending_batches_;

  GraphMode mode_;
  std::unique_ptr<MPSGraphNetwork> network_;

  MPSImageDescriptor * _Nullable input_desc_ = nil;
  MPSImageDescriptor * _Nullable output_desc_ = nil;

  MPSImageBatch * _Nullable recycled_input_ = nil;
  MPSImageBatch * _Nullable recycled_grad_ = nil;
};

}  // namespace mps
}  // namespace turi

NS_ASSUME_NONNULL_END

#endif

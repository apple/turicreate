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

#include "mps_float_array.hpp"

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
  void StartTrainingBatch(const float_array& input_batch,
                          const float_array& label_batch);
  void WaitForTrainingBatch(float *loss);

  // Inference
  void StartInferenceBatch(const float_array& input_batch);
  void WaitForInferenceBatch(float *out_ptr);

  // Forward-backward pass with specified input and top-gradient images
  void StartTrainReturnGradBatch(const float_array& input_batch,
                                 const float_array& gradient_batch);
  void WaitForTrainReturnGradBatch(float *out_ptr);

  void Export();
  int NumParams();

  std::unordered_map<std::string,
                     std::tuple<std::string, float *, int, std::vector<int>>>
      table_;

private:
  MPSImageBatch *CreateImageBatch(MPSImageDescriptor *desc);
  MPSImageBatch *CopyInput(const float_array& input);
  MPSImageBatch *CopyGrad(const float_array& gradient);
  MPSCNNLossLabelsBatch *CopyLabels(const float_array& labels);

  void Blob2MPSImage(const float_array& blob, MPSImageBatch *batch);
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

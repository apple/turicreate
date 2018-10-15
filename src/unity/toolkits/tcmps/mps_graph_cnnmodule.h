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

namespace turi {
namespace mps {

class MPSGraphModule {
public:
  MPSGraphModule();

  void Init(int network_id, int n, int c_in, int h_in, int w_in, int c_out,
            int h_out, int w_out,
            const float_array_map& config, const float_array_map& weights);

  // The graph mode used to initialize the module determines which of the
  // following functions are allowed.
  // TODO: Parameters such as graph mode should be more explicit than just a
  // component of the `config` map.

  // Training
  void SetLearningRate(float lr);
  deferred_float_array Train(const float_array& input_batch,
                             const float_array& label_batch);

  // Inference
  deferred_float_array Predict(const float_array& input_batch);

  // Forward-backward pass with specified input and top-gradient images
  deferred_float_array TrainReturnGrad(const float_array& input_batch,
                                       const float_array& gradient_batch);

  float_array_map Export() const;

private:
  MPSImageBatch *CreateImageBatch(MPSImageDescriptor *desc);
  MPSImageBatch *CopyInput(const float_array& input);
  MPSImageBatch *CopyGrad(const float_array& gradient);
  MPSCNNLossLabelsBatch *CopyLabels(const float_array& labels);

  id<MTLDevice> dev_;
  id<MTLCommandQueue> cmd_queue_;

  GraphMode mode_;
  std::unique_ptr<MPSGraphNetwork> network_;
  std::vector<size_t> result_shape_;

  MPSImageDescriptor * _Nullable input_desc_ = nil;
  MPSImageDescriptor * _Nullable output_desc_ = nil;

  NSMutableArray<MPSImageBatch *> *recycled_inputs_ = nil;
  NSMutableArray<MPSImageBatch *> *recycled_grads_ = nil;
};

}  // namespace mps
}  // namespace turi

NS_ASSUME_NONNULL_END

#endif

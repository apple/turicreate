#ifndef MPS_MODULE_H_
#define MPS_MODULE_H_

#include <map>
#include <memory>
#include <string>

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <ml/neural_net/float_array.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <ml/neural_net/mps_command_queue.hpp>

#import "mps_networks.h"
#import "mps_updater.h"
#import "mps_utils.h"

NS_ASSUME_NONNULL_BEGIN

namespace turi {
namespace neural_net {

class mps_cnn_module: public model_backend {
public:

  mps_cnn_module(const mps_command_queue& command_queue);

  void init(int network_id, int n, int c_in, int h_in, int w_in, int c_out,
            int h_out, int w_out, int updater_id,
            const float_array_map& config);
  void load(const float_array_map& weights);

  float_array_map export_weights() const override;
  float_array_map predict(const float_array_map& inputs) const override;

  void set_learning_rate(float lr) override;
  float_array_map train(const float_array_map& inputs) override;

private:
  void SetupUpdater(int updater_id);

  static NSData *EncodeLabels(const float* labels, NSUInteger sequenceLength,
                              NSUInteger numClasses);
  static NSData *EncodeWeights(const float* weights, NSUInteger sequenceLength,
                               NSUInteger numClasses);

  MPSImageBatch *copy_input(const float_array& input) const;
  MPSCNNLossLabelsBatch *copy_labels(const float_array& labels,
                                     const float_array& weights) const;

  MPSCNNLossLabelsBatch *initLossLabelsBatch(
      id<MTLDevice> device, const float_array& labels,
      const float_array& weights, int batch_size, int seq_len, int num_classes);
  static void FillLossLabelsBatch(
      MPSCNNLossLabelsBatch *labelsBatch, id <MTLDevice> device,
      const float_array& labels, const float_array& weights,
      int batch_size, int seq_len, int num_classes);
  static MPSImageBatch *ExtractLossImages(MPSCNNLossLabelsBatch *labelsBatch,
                                          id<MTLCommandBuffer> cb);

  float_array_map perform_batch(const float_array_map& inputs,
                                bool do_backward) const;

  id <MTLDevice> dev_ = nil;
  id <MTLCommandQueue> cmd_queue_ = nil;
  MPSImageDescriptor *input_desc_ = nil;
  std::unique_ptr<MPSNetwork> network_;
  std::unique_ptr<MPSUpdater> updater_;
  size_t output_chn_ = 0;
  size_t output_width_ = 0;

  NSMutableArray<MPSImageBatch *> *recycled_inputs_ = nil;
  NSMutableArray<MPSCNNLossLabelsBatch *> *recycled_labels_ = nil;
};

}  // namespace neural_net
}  // namespace turi

NS_ASSUME_NONNULL_END

#endif

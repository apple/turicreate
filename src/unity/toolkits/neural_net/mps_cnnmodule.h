#ifndef MPS_MODULE_H_
#define MPS_MODULE_H_

#include <assert.h>
#include <map>
#include <vector>

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <unity/toolkits/neural_net/float_array.hpp>

#import "mps_networks.h"
#import "mps_updater.h"
#import "mps_utils.h"

namespace turi {
namespace neural_net {

class MPSCNNModule {
public:
  MPSCNNModule();
  ~MPSCNNModule();
  void Init(int network_id, int n, int c_in, int h_in, int w_in, int c_out,
            int h_out, int w_out, int updater_id,
            const float_array_map& config);
  void Forward(const float_array& inputs, float* out, bool is_train = true);
  void Backward(const float_array& gradient, float* out);
  void ForwardBackward(const float_array& inputs, const float_array& labels,
                       const float_array& weights, bool loss_image_required,
                       float* out);
  void Forward(const float_array& inputs, const float_array& labels,
               const float_array& weights, bool loss_image_required,
               bool is_train, float* out);
  void Loss(const float_array& inputs, const float_array& labels,
            const float_array& weights, bool loss_image_required,
            float* out);
  void BeginForwardBatch(
      int batch_id, const float_array& inputs, const float_array& labels,
      const float_array& weights, bool loss_image_required, bool is_train);
  void BeginForwardBackwardBatch(
      int batch_id, const float_array& inputs, const float_array& labels,
      const float_array& weights, bool loss_image_required);
  void WaitForBatch(int batch_id, float *forward_out, float *loss_out);
  void GetLossImages(float *_Nonnull out);
  void Update();
  void GpuUpdate();
  void Load(const float_array_map& weights);
  float_array_map Export() const;
  void SetLearningRate(float new_lr) {
    if (updater_ != nil) {
      updater_->SetLearningRate(new_lr);
    }
  }
  int NumParams();

private:
  // Used by the asynchronous API.
  struct Batch {
    id<MTLCommandBuffer> _Nullable command_buffer = nil;
    MPSImageBatch * _Nonnull input = nil;
    MPSImageBatch * _Nonnull output = nil;
    MPSImageBatch * _Nonnull top_grad = nil;
    MPSImageBatch * _Nullable loss_images = nil;
    MPSCNNLossLabelsBatch * _Nullable loss_labels = nil;
  };

  id<MTLDevice> _Nonnull dev_;
  id<MTLCommandQueue> _Nonnull cmd_queue_;
  MPSImageDescriptor *_Nonnull input_desc_ = nil;
  MPSImageDescriptor *_Nonnull output_desc_ = nil;
  MPSImageBatch *_Nonnull input_;
  MPSImageBatch *_Nonnull output_;
  MPSImageBatch *_Nonnull top_grad_;
  MPSImageBatch *_Nullable loss_images_{nil};
  MPSCNNLossLabelsBatch *_Nullable loss_labels_ = nil;
  MPSNetwork *_Nonnull network_{nil};
  MPSUpdater *_Nonnull updater_{nil};
  int output_chn_;
  int output_width_;

  // Used by the asynchronous API.
  std::map<int, Batch> active_batches_;  // Keyed by batch ID
  std::vector<Batch> free_batches_;

private:
  Batch* StartBatch(int batch_id);  // Throws if ID is already in use

  void SetupUpdater(int updater_id);
  void Blob2MPSImage(const float_array& blob, MPSImageBatch *batch);
  void MPSImage2Blob(float *_Nonnull ptr, MPSImageBatch *_Nonnull batch);

  static NSData *EncodeLabels(const float* labels, NSUInteger sequenceLength,
                              NSUInteger numClasses);
  static NSData *EncodeWeights(const float* weights, NSUInteger sequenceLength,
                               NSUInteger numClasses);

  MPSCNNLossLabelsBatch *initLossLabelsBatch(
      id<MTLDevice> device, const float_array& labels,
      const float_array& weights, int batch_size, int seq_len, int num_classes);
  static void FillLossLabelsBatch(
      MPSCNNLossLabelsBatch *labelsBatch, id <MTLDevice> device,
      const float_array& labels, const float_array& weights,
      int batch_size, int seq_len, int num_classes);
  static MPSImageBatch *ExtractLossImages(MPSCNNLossLabelsBatch *labelsBatch,
                                          id<MTLCommandBuffer> cb);
    
  void TrainingWithLoss(
      Batch *batch, const float_array& inputs, const float_array& labels,
      const float_array& weights,
      bool loss_image_required, bool wait_until_completed, float *out,
      bool do_backward, bool is_train = true);
};

}  // namespace neural_net
}  // namespace turi

#endif

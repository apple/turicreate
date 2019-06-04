#include "mps_cnnmodule.h"

#include <algorithm>
#include <iostream>

#include <logger/assertions.hpp>

#import "mps_device_manager.h"

namespace turi {
namespace neural_net {

namespace {

MPSImageBatch * _Nonnull CreateImageBatch(id<MTLDevice> _Nonnull device,
                                          MPSImageDescriptor * _Nonnull desc,
                                          NSUInteger batchSize) {
  NSMutableArray<MPSImage*> *result = [[NSMutableArray alloc] initWithCapacity:batchSize];
  for (NSUInteger i = 0; i < batchSize; ++i) {
    [result addObject:[[MPSImage alloc] initWithDevice:device imageDescriptor:desc]];
  }
  return [result copy];
}

}  // anonymous namespace

mps_cnn_module::mps_cnn_module(const mps_command_queue& command_queue) {

  cmd_queue_ = command_queue.impl;
  dev_ = cmd_queue_.device;

  ASSERT_TRUE(dev_ != nil);
  ASSERT_TRUE(cmd_queue_ != nil);
}

void mps_cnn_module::init(int network_id, int n, int c_in, int h_in, int w_in,
                          int c_out, int h_out, int w_out, int updater_id,
                          const float_array_map& config) {

  @autoreleasepool {

  // Save output shape, used for initializing the labels (that can not
  // be pre-initialized without the data)
  output_chn_ = static_cast<size_t>(c_out);
  output_width_ = static_cast<size_t>(w_out);

  input_desc_ = [MPSImageDescriptor
      imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                 width:w_in
                                height:h_in
                       featureChannels:c_in
                        numberOfImages:1
                                 usage:MTLTextureUsageShaderWrite |
                                       MTLTextureUsageShaderRead];

  network_.reset(createNetwork(static_cast<NetworkType>(network_id),
                               {n, h_in, w_in, c_in, h_out, w_out, c_out},
                               config));
  network_->batch_size = n;
  network_->Init(dev_, cmd_queue_, config);
  SetupUpdater(updater_id);

  }  // @autoreleasepool
}

void mps_cnn_module::load(const float_array_map& weights) {

  @autoreleasepool {

  network_->Load(weights);

  }  // @autoreleasepool
}

float_array_map mps_cnn_module::export_weights() const {

  @autoreleasepool {

  return network_->Export();

  }  // @autoreleasepool
}

float_array_map mps_cnn_module::predict(const float_array_map& inputs) const {

  @autoreleasepool {

  return perform_batch(inputs, /* do_backward */ false);

  }  // @autoreleasepool
}

void mps_cnn_module::set_learning_rate(float lr) {

  @autoreleasepool {

  if (updater_ != nullptr) {
    updater_->SetLearningRate(lr);
  }

  }  // @autoreleasepool
}

float_array_map mps_cnn_module::train(const float_array_map& inputs) {

  @autoreleasepool {

  return perform_batch(inputs, /* do_backward */ true);

  }  // @autoreleasepool
}

float_array_map mps_cnn_module::perform_batch(const float_array_map &inputs,
                                              bool do_backward) const {

  const bool loss_image_required = network_->is_train_;
  ASSERT_TRUE(!do_backward || loss_image_required);

  // TODO: The names should somehow be a parameter of this class.
  auto input_iter = inputs.find("input");
  auto labels_iter = inputs.find("labels");
  auto weights_iter = inputs.find("weights");
  if (input_iter == inputs.end()) {
    log_and_throw("Cannot run model without argument named \"input\".");
  }
  if (loss_image_required) {
    if (labels_iter == inputs.end()) {
      log_and_throw("Cannot run model without argument named \"labels\".");
    }
    if (weights_iter == inputs.end()) {
      log_and_throw("Cannot run model without argument named \"weights\".");
    }
  }

  // Convert input from float_array to MPSImageBatch.
  MPSImageBatch *inputBatch = copy_input(input_iter->second);

  // Convert labels from float_array to MPSCNNLossLabelsBatch if needed.
  MPSCNNLossLabelsBatch *labelsBatch = nil;
  if (loss_image_required) {
    labelsBatch = copy_labels(labels_iter->second, weights_iter->second);
  }

  // Schedule forward pass.
  id <MTLCommandBuffer> commandBuffer = [cmd_queue_ commandBuffer];
  MPSImageBatch *output =
      network_->Forward(inputBatch, commandBuffer, do_backward);

  MPSImageBatch *lossImages = nil;
  if (loss_image_required) {

    // Schedule computation of loss.
    MPSImageBatch *topGrad =
        network_->Loss(output, labelsBatch, commandBuffer);
    lossImages = ExtractLossImages(labelsBatch, commandBuffer);

    if (do_backward) {

      // Schedule backward pass.
      MPSImageBatch *bottomGrad = network_->Backward(topGrad, commandBuffer);

      // No one reads the result images from the backward pass. Decrement the
      // read count now so that MPS can deallocate them, and to prevent
      // assertion failures with Metal API validation enabled.
      // TODO: Images intended for clients to (optionally) read should be
      // non-temporary.
      MPSImageBatchIncrementReadCount(bottomGrad, -1);

      // Schedule the actual weight update.
      network_->GpuUpdate(commandBuffer);
    }
  }

  // Copy the network output from GPU to CPU.
  for (MPSImage *out in output) {
    [out synchronizeOnCommandBuffer:commandBuffer];
  }

  // Set up promises/futures to receive asynchronous results.
  // n.b. The block below must not capture `this`.
  std::vector<size_t> output_shape =
      { static_cast<size_t>(network_->batch_size), 1, output_width_,
        output_chn_ };
  auto output_promise = std::make_shared<std::promise<shared_float_array>>();
  auto loss_promise = std::make_shared<std::promise<shared_float_array>>();
  NSMutableArray<MPSImageBatch *> *recycled_inputs = recycled_inputs_;
  NSMutableArray<MPSCNNLossLabelsBatch *> *recycled_labels = recycled_labels_;
  [commandBuffer addCompletedHandler:^(id <MTLCommandBuffer> cmdBuf) {

      // Propagate Metal errors as C++ exceptions.
      if (cmdBuf.status == MTLCommandBufferStatusError) {
        output_promise->set_exception(std::make_exception_ptr(
            std::runtime_error(cmdBuf.error.localizedDescription.UTF8String)));
        loss_promise->set_exception(std::make_exception_ptr(
            std::runtime_error(cmdBuf.error.localizedDescription.UTF8String)));
        return;
      }

      // Recycle reusable allocations.
      @synchronized(recycled_inputs) {
        [recycled_inputs addObject:inputBatch];
      }
      if (labelsBatch) {
        @synchronized(recycled_labels) {
          [recycled_labels addObject:labelsBatch];
        }
      }

      // Fulfill promises, copying from Metal CPU buffers to float_array.
      output_promise->set_value(copy_image_batch(output_shape, output));
      if (lossImages) {
        loss_promise->set_value(copy_image_batch({ output_shape[0], 1, 1, 1 },
                                                 lossImages));
      }
  }];

  // Dispatch the scheduled work to Metal.
  [commandBuffer commit];

  // Return the deferred_float_array values wrapping the future results.
  float_array_map result;
  if (lossImages) {
    std::vector<size_t> loss_shape = { output_shape[0], 1, 1, 1 };
    result.emplace("loss", std::make_shared<deferred_float_array>(
        loss_promise->get_future(), std::move(loss_shape)));
  }
  result.emplace("output", std::make_shared<deferred_float_array>(
      output_promise->get_future(), std::move(output_shape)));
  return result;
}

void mps_cnn_module::SetupUpdater(int updater_id) {
  updater_.reset(createUpdater(updater_id));
  updater_->Init(network_->layers, {1e-3});
}

MPSImageBatch *mps_cnn_module::copy_input(const float_array& input) const {

  // TODO: Recycle non-temporary MPSImage instances using image allocators
  // wrapping pools of recycled instances. Something similar can be done for
  // MPSCNNLossLabels instances.
  MPSImageBatch *batch = nil;
  @synchronized(recycled_inputs_) {
    if (recycled_inputs_.count > 0) {
      batch = recycled_inputs_.lastObject;
      [recycled_inputs_ removeLastObject];
    }
  }
  if (!batch) {
    batch = CreateImageBatch(dev_, input_desc_, network_->batch_size);
  }

  fill_image_batch(input, batch);

  return batch;
}

// static
NSData *mps_cnn_module::EncodeLabels(
    const float* labels, NSUInteger sequenceLength, NSUInteger numClasses) {

  NSUInteger dataLength = sequenceLength * numClasses * sizeof(float);
  NSMutableData *result = [NSMutableData dataWithLength:dataLength];  // Zeroed

  // Create a hot one encoded representation of the label, per sequence
  float* output = (float*)result.mutableBytes;
  for (NSUInteger i = 0; i < sequenceLength; ++i) {
    // Interpret the label as an index to a class
    NSUInteger classIndex = (NSUInteger)labels[i];

    // Set the corresponding float value to 1
    output[classIndex] = 1.f;

    // Advance output pointer to encoding of the next sequence element.
    output += numClasses;
  }

  return result;
}

// static
NSData *mps_cnn_module::EncodeWeights(
      const float* weights, NSUInteger sequenceLength, NSUInteger numClasses) {

  NSUInteger dataLength = sequenceLength * numClasses * sizeof(float);
  NSMutableData *result = [NSMutableData dataWithLength:dataLength];

  // Repeat each weight value numClasses times, so each of the channels in the
  // one-hot encoded representation of the labels will have the same weight.
  float* output = (float*)result.mutableBytes;
  for (NSUInteger i = 0; i < sequenceLength; ++i) {
    std::fill(output, output + numClasses, weights[i]);
    output += numClasses;
  }

  return result;
}

MPSCNNLossLabelsBatch *mps_cnn_module::copy_labels(
    const float_array& labels_array, const float_array& weights_array) const {

  const int batch_size = network_->batch_size;
  const int seq_len = output_width_;
  const int num_classes = output_chn_;

  @synchronized(recycled_labels_) {
    // Reuse an existing allocation if possible.
    if (recycled_labels_.count > 0) {
      MPSCNNLossLabelsBatch *batchLabels = recycled_labels_.lastObject;
      [recycled_labels_ removeLastObject];

      FillLossLabelsBatch(batchLabels, dev_, labels_array, weights_array,
                          batch_size, seq_len, num_classes);
      return batchLabels;
    }
  }

  // TODO: Factor out more shared code from FillLossLabelsBatch and the below?

  const float* labels_ptr = labels_array.data();
  const float* weights_ptr = weights_array.data();

  MPSCNNLossLabelsBatch *labels = @[];

  // label size in each batch is seq_len (width) * 1 (height) * num_classes
  // (output channels)
  MTLSize labelMtlSize = MTLSizeMake(seq_len, 1, num_classes);

  for (int batch_idx = 0; batch_idx < batch_size; batch_idx++) {

    // labels_ptr and weights_ptr are each C arrays of length
    // `batch_size * seq_len`
    int ptr_offset_for_batch = batch_idx * seq_len;

    NSData *labelsData = EncodeLabels(labels_ptr + ptr_offset_for_batch,
                                      seq_len, num_classes);
    MPSCNNLossDataDescriptor *labelsDescriptor = [MPSCNNLossDataDescriptor
        cnnLossDataDescriptorWithData:labelsData
                               layout:MPSDataLayoutHeightxWidthxFeatureChannels
                                 size:labelMtlSize];
    
    NSData *weightsData = EncodeWeights(weights_ptr + ptr_offset_for_batch,
                                        seq_len, num_classes);
    MPSCNNLossDataDescriptor *weightsDescriptor = [MPSCNNLossDataDescriptor
                                                   cnnLossDataDescriptorWithData:weightsData
                                                                          layout:MPSDataLayoutHeightxWidthxFeatureChannels
                                                                            size:labelMtlSize];
      
    MPSCNNLossLabels *lossState =
      [[MPSCNNLossLabels alloc] initWithDevice:dev_
                                 lossImageSize:{1, 1, 1}
                              labelsDescriptor:labelsDescriptor
                             weightsDescriptor:weightsDescriptor];

    labels = [labels arrayByAddingObject:lossState];
  }

  return labels;
}

// static
void mps_cnn_module::FillLossLabelsBatch(
    MPSCNNLossLabelsBatch *labelsBatch, id <MTLDevice> device,
    const float_array& labels_array, const float_array& weights_array,
    int batch_size, int seq_len, int num_classes) {

  const float* labels_ptr = labels_array.data();
  const float* weights_ptr = weights_array.data();

  MPSImageReadWriteParams labelsFeatureChannelInfo = {
      .featureChannelOffset = 0,
      .numberOfFeatureChannelsToReadWrite =
          (NSUInteger)num_classes  // featureChannels
  };

  MTLRegion labelsRegion = {
      .origin = { .x = 0, .y = 0, .z = 0 },
      .size   = { .width = (NSUInteger)seq_len, .height = 1, .depth = 1 }
  };

  for (NSUInteger i = 0; i < (NSUInteger)batch_size; ++i) {

    MPSCNNLossLabels *lossLabels = labelsBatch[i];

    // labels_ptr and weights_ptr are each C arrays of length
    // `batch_size * seq_len`
    NSUInteger ptrOffsetForBatch = i * (NSUInteger)seq_len;
    NSData *labelsData = EncodeLabels(labels_ptr + ptrOffsetForBatch,
                                      seq_len, num_classes);
    NSData *weightsData = EncodeWeights(weights_ptr + ptrOffsetForBatch,
                                        seq_len, num_classes);

    // Overwrite the data in the existing labelsImage and weightsImage from this
    // batch's lossLabels.

    MPSImage *labelsImage = [[MPSImage alloc] initWithTexture:lossLabels.labelsImage.texture
                                              featureChannels:num_classes];
    MPSImage *weightsImage = [[MPSImage alloc] initWithTexture:lossLabels.weightsImage.texture
                                               featureChannels:num_classes];
    [labelsImage writeBytes:(float*)labelsData.bytes
                 dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                bytesPerRow:seq_len * sizeof(float)
              bytesPerImage:num_classes * seq_len * sizeof(float)
                     region:labelsRegion
         featureChannelInfo:labelsFeatureChannelInfo
                 imageIndex:0];
    [weightsImage writeBytes:(float*)weightsData.bytes
                  dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
                 bytesPerRow:seq_len * sizeof(float)
               bytesPerImage:num_classes * seq_len * sizeof(float)
                      region:labelsRegion
          featureChannelInfo:labelsFeatureChannelInfo
                  imageIndex:0];
  }
}

// static
MPSImageBatch *mps_cnn_module::ExtractLossImages(
    MPSCNNLossLabelsBatch *labelsBatch, id<MTLCommandBuffer> cb) {
  NSMutableArray<MPSImage *> *lossImages = [[NSMutableArray alloc] initWithCapacity:labelsBatch.count];

  for (NSUInteger i = 0; i < labelsBatch.count; ++i) {
    MPSImage* lossImage = labelsBatch[i].lossImage;
    [lossImages addObject:lossImage];
    [lossImage synchronizeOnCommandBuffer:cb];
  }

  return lossImages;
}

}  // namespace neural_net
}  // namespace turi

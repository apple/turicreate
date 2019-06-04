#include "mps_graph_cnnmodule.h"

#include <logger/logger.hpp>

#import "mps_device_manager.h"

NS_ASSUME_NONNULL_BEGIN

// Represents one batch submitted to MPS.
@interface TCMPSGraphModuleBatch : NSObject

- (instancetype)initWithCommandBuffer:(id<MTLCommandBuffer>)commandBuffer;

@property (nonatomic, readonly) id<MTLCommandBuffer> commandBuffer;
@property (nonatomic, nullable) MPSImageBatch *input;
@property (nonatomic, nullable) MPSImageBatch *grad;
@property (nonatomic, nullable) MPSCNNLossLabelsBatch *lossState;
@property (nonatomic, nullable) MPSImageBatch *output;

@end

NS_ASSUME_NONNULL_END

@implementation TCMPSGraphModuleBatch

- (instancetype)initWithCommandBuffer:(id<MTLCommandBuffer>)commandBuffer {
  self = [super init];
  if (self) {
    _commandBuffer = commandBuffer;
  }
  return self;
}

@end

namespace turi {
namespace neural_net {

mps_graph_cnn_module::mps_graph_cnn_module() {
  @autoreleasepool {
    dev_ = [[TCMPSDeviceManager sharedInstance] preferredDevice];
    assert(dev_ && "No valid Metal device. Availability should be checked before creating mps_graph_cnn_module.");
    id<MTLCommandQueue> cq = [dev_ newCommandQueue];
    assert(cq);
    cmd_queue_ = cq;

#if VERBOSE
    NSLog(@"Selected dev: %@", dev_.name);
#endif
  }
}

mps_graph_cnn_module::mps_graph_cnn_module(
    const mps_command_queue& command_queue) {
  @autoreleasepool {

  cmd_queue_ = command_queue.impl;
  dev_ = cmd_queue_.device;

  }  // @autoreleasepool
}

void mps_graph_cnn_module::init(
    int network_id, int n, int c_in, int h_in, int w_in, int c_out, int h_out,
    int w_out, const float_array_map& config, const float_array_map& weights) {

  @autoreleasepool {
    mode_ = (GraphMode)get_array_map_scalar(config, "mode", kGraphModeTrainReturnGrad);
    
    input_desc_ = [MPSImageDescriptor
        imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                   width:w_in
                                  height:h_in
                         featureChannels:c_in
                          numberOfImages:1
                                   usage:MTLTextureUsageShaderWrite |
                                         MTLTextureUsageShaderRead];
    output_desc_ = [MPSImageDescriptor
        imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                   width:w_out
                                  height:h_out
                         featureChannels:c_out
                          numberOfImages:1
                                   usage:MTLTextureUsageShaderWrite |
                                         MTLTextureUsageShaderRead];

    network_ = createNetworkGraph((GraphNetworkType)network_id, {n, h_in, w_in, c_in, h_out, w_out, c_out}, config);
    network_->batch_size = n;
    network_->Init(dev_, cmd_queue_, mode_, config, weights);

    switch (mode_) {
    case kGraphModeTrain:
      result_shape_ = {static_cast<size_t>(n)};  // Loss for each instance
      break;
    case kGraphModeTrainReturnGrad:
      // Gradient for input layer
      result_shape_ = {static_cast<size_t>(n), static_cast<size_t>(h_in),
                       static_cast<size_t>(w_in), static_cast<size_t>(c_in)};
      break;
    case kGraphModeInference:
      // Result image from output layer
      result_shape_ = {static_cast<size_t>(n), static_cast<size_t>(h_out),
                       static_cast<size_t>(w_out), static_cast<size_t>(c_out)};
      break;
    default:
      break;
    }

    recycled_inputs_ = [[NSMutableArray alloc] initWithCapacity:2];
    recycled_grads_ = [[NSMutableArray alloc] initWithCapacity:2];
  }
}

float_array_map mps_graph_cnn_module::train(const float_array_map& inputs) {

  @autoreleasepool {

  assert(mode_ == kGraphModeTrain);

  id<MTLCommandBuffer> cb = [cmd_queue_ commandBuffer];
  TCMPSGraphModuleBatch *batch = [[TCMPSGraphModuleBatch alloc] initWithCommandBuffer:cb];

  // TODO: The names should somehow be a parameter of this class.
  auto input_iter = inputs.find("input");
  auto labels_iter = inputs.find("labels");
  if (input_iter == inputs.end()) {
    log_and_throw("Cannot train without argument named \"input\".");
  }
  if (labels_iter == inputs.end()) {
    log_and_throw("Cannot train without argument named \"labels\".");
  }
  const shared_float_array& input_batch = input_iter->second;
  const shared_float_array& label_batch = labels_iter->second;

  // Copy from raw C inputs to MPS images and loss labels.
  batch.input = copy_input(input_batch);
  batch.lossState = copy_labels(label_batch);

  // Encode the forward-backward pass.
  batch.output = network_->RunGraph(cb, batch.input, batch.lossState);

  // Schedule synchronization of the loss from GPU to CPU.
  for (MPSCNNLossLabels *lossState in batch.lossState) {
    [lossState synchronizeOnCommandBuffer:cb];
  }

  // Schedule copying of the output into a float_array promise.
  // n.b. The block below must not capture `this`.
  size_t loss_size = static_cast<size_t>(batch.lossState.count);
  auto loss_promise = std::make_shared<std::promise<shared_float_array>>();
  NSMutableArray<MPSImageBatch *> *recycled_inputs = recycled_inputs_;
  [cb addCompletedHandler:^(id <MTLCommandBuffer> cmdBuf) {

      // Propagate Metal errors as C++ exceptions.
      if (cmdBuf.status == MTLCommandBufferStatusError) {
        loss_promise->set_exception(std::make_exception_ptr(std::runtime_error(
            cmdBuf.error.localizedDescription.UTF8String)));
        return;
      }

      // Copy out the loss data and compute the scalar loss for each training
      // instance.
      std::vector<float> loss(loss_size);
      auto loss_it = loss.begin();
      for (MPSCNNLossLabels *lossState in batch.lossState) {
        *loss_it = sumImage([lossState lossImage]);
        ++loss_it;
      }

      // Recycle the MPSImage instances used for inputs.
      @synchronized(recycled_inputs) {
        [recycled_inputs addObject:batch.input];
      }

      // Fulfill the promise last (potentially triggering context switch).
      loss_promise->set_value(shared_float_array::wrap(std::move(loss),
                                                       {loss_size}));
  }];

  // Dispatch this batch to MPS.
  [cb commit];

  // Return the wrapped future from the promise.
  // TODO: The names should somehow be a parameter of this class.
  shared_float_array loss(std::make_shared<deferred_float_array>(
      loss_promise->get_future(), std::vector<size_t>({loss_size})));
  return { { "loss", loss } };

  }  // @autoreleasepool
}

float_array_map
mps_graph_cnn_module::predict(const float_array_map& inputs) const {
  @autoreleasepool {

  assert(mode_ == kGraphModeInference);

  id<MTLCommandBuffer> cb = [cmd_queue_ commandBuffer];
  TCMPSGraphModuleBatch *batch = [[TCMPSGraphModuleBatch alloc] initWithCommandBuffer:cb];

  // TODO: The names should somehow be a parameter of this class.
  auto input_iter = inputs.find("input");
  if (input_iter == inputs.end()) {
    log_and_throw("Cannot train without argument named \"input\".");
  }
  const shared_float_array& input_batch = input_iter->second;

  // Copy from raw C inputs to MPS images. Encode the forward pass.
  batch.input = copy_input(input_batch);
  batch.output = network_->RunGraph(cb, @{@"input" : batch.input});

  // Schedule synchronization of the output from GPU to CPU.
  for (MPSImage *image in batch.output) {
    [image synchronizeOnCommandBuffer:cb];
  }

  // Schedule copying of the output into a float_array promise.
  // n.b. The block below must not capture `this`.
  std::vector<size_t> result_shape = result_shape_;
  auto result_promise = std::make_shared<std::promise<shared_float_array>>();
  NSMutableArray<MPSImageBatch *> *recycled_inputs = recycled_inputs_;
  [cb addCompletedHandler:^(id <MTLCommandBuffer> cmdBuf) {

      // Propagate Metal errors as C++ exceptions.
      if (cmdBuf.status == MTLCommandBufferStatusError) {
        result_promise->set_exception(std::make_exception_ptr(
            std::runtime_error(cmdBuf.error.localizedDescription.UTF8String)));
        return;
      }

      // Copy out the results.
      shared_float_array result = copy_image_batch_float16(result_shape,
                                                           batch.output);
      // Recycle the MPSImage instances used for inputs.
      @synchronized(recycled_inputs) {
        [recycled_inputs addObject:batch.input];
      }

      // Fulfill the promise last (potentially triggering context switch).
      result_promise->set_value(std::move(result));
  }];

  // Dispatch this batch to MPS.
  [cb commit];

  // Return the wrapped future from the promise.
  // TODO: The names should somehow be a parameter of this class.
  shared_float_array output(std::make_shared<deferred_float_array>(
      result_promise->get_future(), result_shape_));
  return { { "output", output } };

  }  // @autoreleasepool
}

deferred_float_array mps_graph_cnn_module::train_return_grad(
    const float_array& input_batch, const float_array& gradient_batch) {

  @autoreleasepool {

  assert(mode_ == kGraphModeTrainReturnGrad);

  id<MTLCommandBuffer> cb = [cmd_queue_ commandBuffer];
  TCMPSGraphModuleBatch *batch = [[TCMPSGraphModuleBatch alloc] initWithCommandBuffer:cb];

  // Copy from raw C inputs to MPS images. Encode the forward-backward pass.
  batch.input = copy_input(input_batch);
  batch.grad = copy_grad(gradient_batch);
  batch.output = network_->RunGraph(cb, @{@"input" : batch.input,
                                          @"grad"  : batch.grad   });

  // Schedule synchronization of the output from GPU to CPU.
  for (MPSImage *image in batch.output) {
    [image synchronizeOnCommandBuffer:cb];
  }

  // Schedule copying of the output into a float_array promise.
  // n.b. The block below must not capture `this`.
  std::vector<size_t> result_shape = result_shape_;
  auto result_promise = std::make_shared<std::promise<shared_float_array>>();
  NSMutableArray<MPSImageBatch *> *recycled_inputs = recycled_inputs_;
  NSMutableArray<MPSImageBatch *> *recycled_grads = recycled_grads_;
  [cb addCompletedHandler:^(id <MTLCommandBuffer> cmdBuf) {

      // Propagate Metal errors as C++ exceptions.
      if (cmdBuf.status == MTLCommandBufferStatusError) {
        result_promise->set_exception(std::make_exception_ptr(
            std::runtime_error(cmdBuf.error.localizedDescription.UTF8String)));
        return;
      }

      // Copy out the results.
      shared_float_array result = copy_image_batch_float16(result_shape,
                                                           batch.output);
      // Recycle the MPSImage instances used for inputs.
      @synchronized(recycled_inputs) {
        [recycled_inputs addObject:batch.input];
      }
      @synchronized(recycled_grads) {
        [recycled_grads addObject:batch.grad];
      }

      // Fulfill the promise last (potentially triggering context switch).
      result_promise->set_value(std::move(result));
  }];

  // Dispatch this batch to MPS.
  [cb commit];

  // Return the wrapped future from the promise.
  return deferred_float_array(result_promise->get_future(), result_shape_);

  }  // @autoreleasepool
}

float_array_map mps_graph_cnn_module::export_weights() const {
  @autoreleasepool {
    return network_->Export();
  }
}

void mps_graph_cnn_module::set_learning_rate(float lr) {
  @autoreleasepool {
    for (size_t i = 0; i < network_->layers.size(); ++i) {
      network_->layers[i]->SetLearningRate(lr);
    }
  }
}

MPSImageBatch *
mps_graph_cnn_module::create_image_batch(MPSImageDescriptor *desc) const {
  NSUInteger batchSize = (NSUInteger)network_->batch_size;
  NSMutableArray<MPSImage *> *result = [[NSMutableArray alloc] initWithCapacity:batchSize];
  for (NSUInteger i = 0; i < batchSize; ++i) {
    [result addObject:[[MPSImage alloc] initWithDevice:dev_ imageDescriptor:desc]];
  }
  return [result copy];
}

MPSImageBatch *
mps_graph_cnn_module::copy_input(const float_array& input) const {
  @autoreleasepool {
    // may check shape

    // Use a recycled MPSImageBatch if available.
    MPSImageBatch *batch = nil;
    @synchronized(recycled_inputs_) {
      if (recycled_inputs_.count > 0) {
        batch = recycled_inputs_.lastObject;
        [recycled_inputs_ removeLastObject];
      }
    }
    if (!batch) {
      // Allocate a new MPSImageBatch if necessary.
      batch = create_image_batch(input_desc_);
    }
    fill_image_batch(input, batch);
    return batch;
  }
}

MPSImageBatch *
mps_graph_cnn_module::copy_grad(const float_array& gradient) const {
  @autoreleasepool {
    // may check shape

    // Use a recycled MPSImageBatch if available.
    MPSImageBatch *batch = nil;
    @synchronized(recycled_grads_) {
      if (recycled_grads_.count > 0) {
        batch = recycled_grads_.lastObject;
        [recycled_grads_ removeLastObject];
      }
    }
    if (!batch) {
      // Allocate a new MPSImageBatch if necessary.
      batch = create_image_batch(output_desc_);
    }
    fill_image_batch(gradient, batch);
    return batch;
  }
}

MPSCNNLossLabelsBatch *
mps_graph_cnn_module::copy_labels(const float_array& labels) const {
  @autoreleasepool {
    return network_->loss_layer_->CreateLossState(dev_, labels);
  }
}

}  // namespace neural_net
}  // namespace turi

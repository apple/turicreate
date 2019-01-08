#include "mps_cnnmodule.h"

#include <algorithm>
#include <iostream>

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

MPSCNNModule::MPSCNNModule() {
  dev_ = [[TCMPSDeviceManager sharedInstance] preferredDevice];
  assert(dev_ && "No valid Metal device. Availability should be checked before creating MPSCNNModule.");
  id<MTLCommandQueue> cq = [dev_ newCommandQueue];
  assert(cq);
  cmd_queue_ = cq;

#if VERBOSE
  NSLog(@"Selected dev: %@", dev_.name);
#endif
}

void MPSCNNModule::Init(int network_id, int n, int c_in, int h_in, int w_in,
                        int c_out, int h_out, int w_out, int updater_id,
                        const float_array_map& config) {

  // Save output shape, used for initializing the labels (that can not
  // be pre-initialized without the data)
  output_chn_ = c_out;
  output_width_ = w_out;

  input_desc_ = [MPSImageDescriptor
      imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                 width:w_in
                                height:h_in
                       featureChannels:c_in
                        numberOfImages:1
                                 usage:MTLTextureUsageShaderWrite |
                                       MTLTextureUsageShaderRead];

  input_ = CreateImageBatch(dev_, input_desc_, n);

  // output_ and top_grad_ should only be allocated in Test mode, where they are used
  // as inputs coming externally from python
  LowLevelMode network_mode = (LowLevelMode) get_array_map_scalar(config, "mode", kLowLevelModeTrain);

  if (kLowLevelModeTest == network_mode){
      output_desc_ = [MPSImageDescriptor
          imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                     width:w_out
                                    height:h_out
                           featureChannels:c_out
                            numberOfImages:1
                                     usage:MTLTextureUsageShaderWrite |
                                           MTLTextureUsageShaderRead];

      output_ = CreateImageBatch(dev_, output_desc_, n);
      top_grad_ = CreateImageBatch(dev_, output_desc_, n);
  }

  network_ = createNetwork((NetworkType)network_id, {n, h_in, w_in, c_in, h_out, w_out, c_out}, config);
  network_->batch_size = n;
  network_->Init(dev_, cmd_queue_, config);
  SetupUpdater(updater_id);
}

MPSCNNModule::~MPSCNNModule() {
  delete network_;
  delete updater_;
}

MPSCNNModule::Batch* MPSCNNModule::StartBatch(int batch_id) {
  if (active_batches_.find(batch_id) != active_batches_.end()) {
    throw std::logic_error("Cannot start batch with ID already in use");
  }

  Batch& batch = active_batches_[batch_id];
  if (free_batches_.empty()) {
    // Allocate a new input MPSImageBatch.
    batch.input = CreateImageBatch(dev_, input_desc_, network_->batch_size);
  } else {
    batch = std::move(free_batches_.back());
    free_batches_.pop_back();

    // Recycle MPSImageBatch allocations from a previous batch.
    assert(batch.input);
  }

  return &batch;
}

void MPSCNNModule::WaitForBatch(int batch_id, float *forward_out,
                                float *loss_out) {
  std::map<int, Batch>::iterator it = active_batches_.find(batch_id);
  if (it == active_batches_.end()) {
    throw std::logic_error("Cannot wait for batch with unknown ID");
  }

  Batch &batch = it->second;
  assert(batch.command_buffer);
  [batch.command_buffer waitUntilCompleted];

  if (forward_out) {
    MPSImage2Blob(forward_out, batch.output);
  }

  if (loss_out) {
    MPSImage2Blob(loss_out, batch.loss_images);
  }

  batch.command_buffer = nil;
  batch.output = nil;
  batch.top_grad = nil;
  batch.loss_images = nil;
  batch.loss_labels = nil;

  free_batches_.push_back(std::move(batch));
  active_batches_.erase(it);
}

void MPSCNNModule::Forward(const float_array& inputs, float *out,
                           bool is_train) {
  // may check shape here
  Blob2MPSImage(inputs, input_);
  @autoreleasepool {
    id<MTLCommandBuffer> commandBuffer = [cmd_queue_ commandBuffer];


    // run network
    output_ = network_->Forward(input_, commandBuffer, is_train);

    for (NSUInteger i = 0; i < [output_ count]; ++i) {
        [output_[i] synchronizeOnCommandBuffer:commandBuffer];
    }

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];

    // copy output
    MPSImage2Blob(out, output_);
  }
}

void MPSCNNModule::Backward(const float_array& gradient, float *out) {
  Blob2MPSImage(gradient, top_grad_);
  @autoreleasepool {
    id<MTLCommandBuffer> commandBuffer = [cmd_queue_ commandBuffer];


    // run backward
    MPSImageBatch *bottom_grad = network_->Backward(top_grad_, commandBuffer);
    for (NSUInteger i = 0; i < [bottom_grad count]; ++i) {
        [bottom_grad[i] synchronizeOnCommandBuffer:commandBuffer];
    }
    network_->SyncState(commandBuffer);
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    // copy output
    MPSImage2Blob(out, bottom_grad);
  }
}

void MPSCNNModule::Loss(const float_array& inputs, const float_array& labels,
                        const float_array& weights, bool loss_image_required,
                        float* out) {
  Blob2MPSImage(inputs, output_);
  @autoreleasepool {
      id<MTLCommandBuffer> commandBuffer = [cmd_queue_ commandBuffer];


      // Creating labels batch
      loss_labels_ =
          initLossLabelsBatch(dev_, labels, weights,
                              network_->batch_size, output_width_, output_chn_);

      // Calc loss
      top_grad_ = network_->Loss(output_, loss_labels_, commandBuffer);

      for (NSUInteger i = 0; i < [top_grad_ count]; ++i) {
        [top_grad_[i] synchronizeOnCommandBuffer:commandBuffer];
      }
      
      if (loss_image_required){
        loss_images_ = ExtractLossImages(loss_labels_, commandBuffer);
      }

      [commandBuffer commit];
      [commandBuffer waitUntilCompleted];

      // copy output
      MPSImage2Blob(out, top_grad_);
  }
}

void MPSCNNModule::Forward(const float_array& inputs, const float_array& labels,
                           const float_array& weights, bool loss_image_required,
                           bool is_train, float* out) {
    
    TrainingWithLoss(/* batch */ nullptr, inputs, labels, weights,
                     loss_image_required, /* wait_until_completed */ true, out,
                     /* do_backward */ false, is_train);
}
void MPSCNNModule::ForwardBackward(
    const float_array& inputs, const float_array& labels,
    const float_array& weights, bool loss_image_required, float* out) {
    
    TrainingWithLoss(/* batch */ nullptr, inputs, labels, weights,
                     loss_image_required, /* wait_until_completed */ true, out,
                     /* do_backward */ true);
}

void MPSCNNModule::BeginForwardBatch(
    int batch_id, const float_array& inputs, const float_array& labels,
    const float_array& weights, bool loss_image_required, bool is_train) {
  Batch* batch = StartBatch(batch_id);
  TrainingWithLoss(batch, inputs, labels, weights,
                   loss_image_required, /* wait_until_completed */ false,
                   /* out */ nullptr, /* do_backward */ false, is_train);
}

void MPSCNNModule::BeginForwardBackwardBatch(
    int batch_id, const float_array& inputs, const float_array& labels,
    const float_array& weights, bool loss_image_required) {
  Batch* batch = StartBatch(batch_id);
  TrainingWithLoss(batch, inputs, labels, weights,
                   loss_image_required, /* wait_until_completed */ false,
                   /* out */ nullptr, /* do_backward */ true);
}

void MPSCNNModule::TrainingWithLoss(
      Batch *batch, const float_array& inputs_array,
      const float_array& labels_array, const float_array& weights_array,
      bool loss_image_required, bool wait_until_completed, float *out,
      bool do_backward, bool is_train) {
    // may check shape here
    if (batch) {
      // TODO: Recycle non-temporary MPSImage instances using image allocators
      // wrapping pools of recycled instances. Something similar can be done for
      // MPSCNNLossLabels instances.
      input_ = batch->input;
      loss_images_ = batch->loss_images;
      loss_labels_ = batch->loss_labels;
    }
    Blob2MPSImage(inputs_array, input_);
    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [cmd_queue_ commandBuffer];

        // Creating labels batch
        if (loss_labels_) {
          // Recycle existing allocations
          FillLossLabelsBatch(
              loss_labels_, dev_, labels_array, weights_array,
              network_->batch_size, output_width_, output_chn_);
        } else {
          loss_labels_ = initLossLabelsBatch(
              dev_, labels_array, weights_array,
              network_->batch_size, output_width_, output_chn_);
        }

        // run foward pass
        output_ = network_->Forward(input_, commandBuffer, is_train);
        
        // Calc loss
        top_grad_ = network_->Loss(output_, loss_labels_, commandBuffer);
        
        
        if (loss_image_required){
          loss_images_ = ExtractLossImages(loss_labels_, commandBuffer);
        }
        
        if (do_backward){
            // run backward pass
            MPSImageBatch *bottom_grad = network_->Backward(top_grad_, commandBuffer);
            if (kLowLevelModeTest == network_->network_mode_){
                for (NSUInteger i = 0; i < [bottom_grad count]; ++i) {
                    [bottom_grad[i] synchronizeOnCommandBuffer:commandBuffer];
                }
            } else {
                // No one reads the result images from the backward pass.
                // Decrement the read count now so that MPS can deallocate them,
                // and to prevent assertion failures with Metal API validation
                // enabled.
                // TODO: Images intended for clients to (optionally) read should
                // be non-temporary.
                MPSImageBatchIncrementReadCount(bottom_grad, -1);
            }
        }
        
        for (NSUInteger i = 0; i < [output_ count]; ++i) {
            [output_[i] synchronizeOnCommandBuffer:commandBuffer];
        }

        [commandBuffer commit];
        
        if (wait_until_completed) {
          [commandBuffer waitUntilCompleted];
        }

        if (out) {
          assert(wait_until_completed && "Error: Must wait for completion before reading output.");
          MPSImage2Blob(out, output_);
        }

        if (batch) {
          batch->command_buffer = commandBuffer;
          batch->output = output_;
          batch->top_grad = top_grad_;
          batch->loss_images = loss_images_;
          batch->loss_labels = loss_labels_;
        }
    }
}

void MPSCNNModule::GetLossImages(float *_Nonnull out) {
  assert(loss_images_ && "Error: No Loss image found. Please call Loss() with loss_image_required=true.");
    
  // copy output
  MPSImage2Blob(out, loss_images_);
}

void MPSCNNModule::Update() { network_->Update(updater_); }

void MPSCNNModule::GpuUpdate(){
    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [cmd_queue_ commandBuffer];
        network_->GpuUpdate(commandBuffer);
        
        [commandBuffer commit];

        // Don't bother waiting for this command buffer to complete. Any
        // observation or dependency on the results of this update must entail
        // another later command, for which the observer must wait.
    }
}

void MPSCNNModule::Load(const float_array_map& weights) {
  network_->Load(weights);
}

float_array_map MPSCNNModule::Export() const {
  return network_->Export();
}
int MPSCNNModule::NumParams() { return network_->NumParams(); }

void MPSCNNModule::SetupUpdater(int updater_id) {
  updater_ = createUpdater(updater_id);
  updater_->Init(network_->layers, {1e-3});
}

void MPSCNNModule::Blob2MPSImage(const float_array& blob,
                                 MPSImageBatch *batch) {
  // add size chcek later
  assert([batch count] > 0);
  const float* ptr = blob.data();
  MPSImage *img = batch[0];
  int stride = [img width] * [img height] * [img featureChannels];
  for (int i = 0; i < [batch count]; ++i) {
    MPSImage *img = batch[i];
    [img writeBytes:ptr + stride * i
         dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
  }
}

void MPSCNNModule::MPSImage2Blob(float *ptr, MPSImageBatch *batch) {
  // add size chcek later
  assert([batch count] > 0);
  MPSImage *img = batch[0];
  int stride = [img width] * [img height] * [img featureChannels];
  for (int i = 0; i < [batch count]; ++i) {
    MPSImage *img = batch[i];
    [img readBytes:ptr + stride * i
        dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
  }
}

// static
NSData *MPSCNNModule::EncodeLabels(
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
NSData *MPSCNNModule::EncodeWeights(
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

MPSCNNLossLabelsBatch *MPSCNNModule::initLossLabelsBatch(
      id<MTLDevice> device, const float_array& labels_array,
      const float_array& weights_array, int batch_size, int seq_len,
      int num_classes) {

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
      [[MPSCNNLossLabels alloc] initWithDevice:device
                                 lossImageSize:{1, 1, 1}
                              labelsDescriptor:labelsDescriptor
                             weightsDescriptor:weightsDescriptor];

    labels = [labels arrayByAddingObject:lossState];
  }

  return labels;
}

// static
void MPSCNNModule::FillLossLabelsBatch(
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
MPSImageBatch *MPSCNNModule::ExtractLossImages(
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

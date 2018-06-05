#include "mps_graph_cnnmodule.h"
#include "mps_utils.h"

MPSGraphModule::MPSGraphModule() {
  @autoreleasepool {
    dev_ = MetalDevice::Get()->dev;
    assert(dev_ && "No valid Metal device. Availability should be checked before creating MPSGraphModule.");
    id<MTLCommandQueue> cq = [dev_ newCommandQueue];
    assert(cq);
    cmd_queue_ = cq;
    double_buffering_semaphore_ = dispatch_semaphore_create(2);

#if VERBOSE
    NSLog(@"Selected dev: %@", dev_.name);
#endif
  }
}

void MPSGraphModule::Init(int network_id, int n, int c_in, int h_in, int w_in,
                          int c_out, int h_out, int w_out,
                          const FloatArrayMap &config,
                          const FloatArrayMap &weights) {
  @autoreleasepool {
    mode_ = (GraphMode)get_array_map_scalar(config, "mode", kGraphModeTrainReturnGrad);
    
    MPSImageDescriptor *input_desc = [MPSImageDescriptor
        imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                   width:w_in
                                  height:h_in
                         featureChannels:c_in
                          numberOfImages:1
                                   usage:MTLTextureUsageShaderWrite |
                                         MTLTextureUsageShaderRead];
    MPSImageDescriptor *output_desc = [MPSImageDescriptor
        imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                   width:w_out
                                  height:h_out
                         featureChannels:c_out
                          numberOfImages:1
                                   usage:MTLTextureUsageShaderWrite |
                                         MTLTextureUsageShaderRead];
    input_ = @[];
    output_ = @[];
    grad_ = @[];
    for (int i = 0; i < n; ++i) {
      MPSImage *img =
          [[MPSImage alloc] initWithDevice:dev_ imageDescriptor:input_desc];
      input_ = [input_ arrayByAddingObject:img];
    }

    for (int i = 0; i < n; ++i) {
      MPSImage *img =
          [[MPSImage alloc] initWithDevice:dev_ imageDescriptor:output_desc];
      grad_ = [grad_ arrayByAddingObject:img];
    }
    
    network_ = createNetworkGraph((GraphNetworkType)network_id, {n, h_in, w_in, c_in, h_out, w_out, c_out}, config);
    network_->batch_size = n;
    network_->Init(dev_, cmd_queue_, mode_, config, weights);
  }
}

void MPSGraphModule::RunGraph(float *out, float *loss) {
  @autoreleasepool {
    id<MTLCommandBuffer> cb = [cmd_queue_ commandBuffer];

    dispatch_semaphore_wait(double_buffering_semaphore_, DISPATCH_TIME_FOREVER);
    last_cmd_buf_ = cb;

    if (loss_state_ && network_->loss_layer_) {
      output_ = network_->RunGraph(cb, input_, loss_state_);
    } else if (mode_ == kGraphModeInference) {
      NSDictionary *inputs = @{@"input" : input_};
      output_ = network_->RunGraph(cb, inputs);
    } else {
      NSDictionary *inputs = @{@"input" : input_, @"grad" : grad_};
      output_ = network_->RunGraph(cb, inputs);
    }

    // sync resource
    if (out) {
      for (int i = 0; i < [output_ count]; ++i) {
        [output_[i] synchronizeOnCommandBuffer:cb];
      }
    }

    if (loss_state_ && loss) {
      for(NSUInteger i = 0; i < [loss_state_ count]; i++){
        [loss_state_[i] synchronizeOnCommandBuffer:cb];
      }
    }
    
    [cb addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
      dispatch_semaphore_signal(double_buffering_semaphore_);
    }];
    [cb commit];
    
    if (loss_state_ && loss) {
      [cb waitUntilCompleted];
      for(NSUInteger i = 0; i < [loss_state_ count]; i++){
        MPSImage *img = [loss_state_[i] lossImage];
        loss[i] = sumImage(img);
      }
    }
    
    if (out) {
      [cb waitUntilCompleted];
      MPSImage2Blob(out, output_);
    }
  }
}

void MPSGraphModule::WaitUntilCompleted() {
    @autoreleasepool {
        [last_cmd_buf_ waitUntilCompleted];
        last_cmd_buf_ = nil;
    }
}

MPSGraphModule::~MPSGraphModule() {
  delete network_;
}

void MPSGraphModule::Export() {
  @autoreleasepool {
    table_.clear();
    network_->Export(table_);
  }
}

int MPSGraphModule::NumParams() { return network_->NumParams(); }

void MPSGraphModule::SetLearningRate(float lr) {
  @autoreleasepool {
    for (int i = 0; i < network_->layers.size(); ++i) {
      network_->layers[i]->SetLearningRate(lr);
    }
  }
}

void MPSGraphModule::SetInput(void *ptr, int64_t sz, int64_t *shape, int dim,
                            int flag) {
  @autoreleasepool {
    // may check shape
    MPSImageBatch *batch;
    if (flag == 0) {
      batch = input_;
    } else {
      batch = grad_;
    }
    Blob2MPSImage((float *)ptr, batch);
  }
}

void MPSGraphModule::SetLossState(float *ptr) {
  @autoreleasepool {
    if (!network_->loss_layer_) return;

    loss_state_ = network_->loss_layer_->CreateLossState(dev_, ptr);
  }
}

void MPSGraphModule::Blob2MPSImage(float *ptr, MPSImageBatch *batch) {
  // add size chcek later
  assert([batch count] > 0);
  MPSImage *img = batch[0];
  int stride = [img width] * [img height] * [img featureChannels];
  for (int i = 0; i < [batch count]; ++i) {
    MPSImage *img = batch[i];
    [img writeBytes:ptr + stride * i
         dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
  }
}

void MPSGraphModule::MPSImage2Blob(float *ptr, MPSImageBatch *batch) {
  // add size chcek later
  assert([batch count] > 0);
  __fp16 *dptr = (__fp16 *)ptr;
  MPSImage *img = batch[0];
  int stride = [img width] * [img height] * [img featureChannels];
  for (int i = 0; i < [batch count]; ++i) {
    MPSImage *img = batch[i];
    [img readBytes:dptr + stride * i
        dataLayout:(MPSDataLayoutHeightxWidthxFeatureChannels)imageIndex:0];
  }
}



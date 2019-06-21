#include <ml/neural_net/mps_layer_fully_connected_data_loader.h>

@implementation FullyConnectedDataLoader

- (id _Nonnull) initWithParams:(NSString * _Nonnull)name
          inputFeatureChannels:(int)inputFeatureChannels
         outputFeatureChannels:(int)outputFeatureChannels
                   inputHeight:(int)inputHeight
                    inputWidth:(int)inputWidth
                       weights:(float * _Nonnull)weights
                        biases:(float * _Nonnull)biases
                 updateWeights:(bool)updateWeights
                        device:(id<MTLDevice> _Nonnull)dev
                     cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q {
    
  self = [super init];
    
  mName = name;
  mInputFeatureChannels = inputFeatureChannels;
  mOutputFeatureChannels = outputFeatureChannels;
    
  mKernelWidth = inputWidth;
  mKernelHeight = inputHeight;

  mUpdate = updateWeights; 
    
  mDev = dev;
  mCq = cmd_q;
  
  if (weights != NULL) {
        
  mWeights = [dev newBufferWithBytes:weights
                              length:sizeof(float) * mInputFeatureChannels * mKernelHeight * mKernelHeight * mOutputFeatureChannels
                             options:MTLResourceStorageModeManaged];
  } else {
      mWeights = [dev newBufferWithLength:sizeof(float) * mInputFeatureChannels *  mKernelHeight * mKernelWidth * mOutputFeatureChannels
                                  options:MTLResourceStorageModeManaged];
  }
    
  if (biases != NULL) {
      mBiases = [dev newBufferWithBytes:biases
                                 length:sizeof(float) * mOutputFeatureChannels
                                options:MTLResourceStorageModeManaged];
  } else {
      mBiases = [dev newBufferWithLength:sizeof(float) * mOutputFeatureChannels
                                 options:MTLResourceStorageModeManaged];
  }
    
    
  MTLResourceOptions storageMode;
    
  storageMode = MTLResourceStorageModeManaged;
    
  mWeightMomentumBuffer = [dev newBufferWithLength:sizeof(float) * mInputFeatureChannels * mKernelHeight * mKernelWidth * mOutputFeatureChannels
                                           options:storageMode];
    
  mWeightVelocityBuffer = [dev newBufferWithLength:sizeof(float) * mInputFeatureChannels * mKernelHeight * mKernelWidth * mOutputFeatureChannels
                                           options:storageMode];
    
  MPSVectorDescriptor *vDescWeights = [MPSVectorDescriptor vectorDescriptorWithLength:mInputFeatureChannels * mKernelHeight * mKernelWidth * mOutputFeatureChannels
                                                                             dataType:(MPSDataTypeFloat32)];
    
    
  mWeightMomentumVector = [[MPSVector alloc] initWithBuffer:mWeightMomentumBuffer
                                                 descriptor:vDescWeights];
    
  mWeightVelocityVector = [[MPSVector alloc] initWithBuffer:mWeightVelocityBuffer
                                                 descriptor:vDescWeights];
    
    
  mBiasMomentumBuffer =
      [dev newBufferWithLength:sizeof(float) * mOutputFeatureChannels
                       options:storageMode];
    
  mBiasVelocityBuffer =
      [dev newBufferWithLength:sizeof(float) * mOutputFeatureChannels
                       options:storageMode];
    
    
  MPSVectorDescriptor *vDescBiases =
      [MPSVectorDescriptor vectorDescriptorWithLength:mOutputFeatureChannels
                                             dataType:(MPSDataTypeFloat32)];
    
  mBiasMomentumVector = [[MPSVector alloc] initWithBuffer:mBiasMomentumBuffer
                                               descriptor:vDescBiases];
    
  mBiasVelocityVector = [[MPSVector alloc] initWithBuffer:mBiasVelocityBuffer
                                               descriptor:vDescBiases];
    
  mState = [[MPSCNNConvolutionWeightsAndBiasesState alloc] initWithWeights:mWeights
                                                                    biases:mBiases];
    
  mOptimizer = [[MPSNNOptimizerAdam alloc] initWithDevice:mDev
                                             learningRate:0.001f];
    
  return self;
}

- (MPSDataType) dataType {
    return MPSDataTypeFloat32;
}

- (MPSCNNConvolutionDescriptor *__nonnull) descriptor {
  MPSCNNConvolutionDescriptor *convDesc = [MPSCNNConvolutionDescriptor cnnConvolutionDescriptorWithKernelWidth:mKernelWidth
                                                                                                  kernelHeight:mKernelHeight
                                                                                          inputFeatureChannels:mInputFeatureChannels
                                                                                         outputFeatureChannels:mOutputFeatureChannels];
    
  return convDesc;
}

- (MPSCNNConvolutionWeightsAndBiasesState * _Nullable)updateWithCommandBuffer:(id<MTLCommandBuffer> _Nonnull)commandBuffer
                                                                gradientState:(MPSCNNConvolutionGradientState * _Nullable)gradientState
                                                                  sourceState:(MPSCNNConvolutionWeightsAndBiasesState * _Nullable)sourceState {
  if (mUpdate) {
    [mOptimizer encodeToCommandBuffer:commandBuffer
             convolutionGradientState:gradientState
               convolutionSourceState:mState
                 inputMomentumVectors:@[mWeightMomentumVector, mBiasMomentumVector]
                 inputVelocityVectors:@[mWeightVelocityVector, mBiasVelocityVector]
                          resultState:mState];
      
    return mState;
  }

  return nil; 
}

- (size_t) weight_size {
  return mKernelWidth * mKernelHeight * mInputFeatureChannels * mOutputFeatureChannels * sizeof(float);
}

- (void) loadWeight:(float *__nullable) src {
  memcpy(mWeights.contents, (void *)src, [self weight_size]);
  [mWeights didModifyRange:NSMakeRange(0, [self weight_size])];
}

- (void *__nonnull) weights {
  return (float *)mWeights.contents;
}

- (size_t) bias_size {
  return mOutputFeatureChannels * sizeof(float);
}

- (void) loadBias:(float *__nullable) src {
  float *dst = (float *)mBiases.contents;
  memcpy((void *)dst, (void *)src, [self bias_size]);
  [mBiases didModifyRange:NSMakeRange(0, [self  bias_size])];
}

- (float *__nullable) biasTerms {
  return (float *)mBiases.contents;
}

- (BOOL) load {
  [self checkpointWithCommandQueue:mCq];
  return YES;
}

- (void) purge {}

- (NSString*__nullable) label {
  return mName;
}

- (id _Nonnull) copyWithZone:(nullable NSZone *) zone {
  return self;
}

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue {
  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLBlitCommandEncoder> blit = commandBuffer.blitCommandEncoder;
    
  [blit synchronizeResource:mWeightMomentumBuffer];
  [blit synchronizeResource:mWeightVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(mWeights)];
    
  [blit synchronizeResource:mBiasMomentumBuffer];
  [blit synchronizeResource:mBiasVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(mBiases)];
    
  [blit endEncoding];
    
  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];
}

@end

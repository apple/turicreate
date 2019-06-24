#include <ml/neural_net/mps_layer_instance_norm_data_loader.h>
#include <stdio.h>
#include <stdlib.h>

#define nonnull_cast_a(_a)                                                       \
  ({                                                                           \
    __typeof__(*(_a)) *__nullable a = (__typeof__(*(a)) *__nullable)(_a);      \
    assert((a) != NULL);                                                       \
    /*return*/ ((__typeof__(*(a)) *__nonnull)(a));                             \
  })


@implementation InstanceNormDataLoader

@synthesize numberOfFeatureChannels = mNumberOfFeatureChannels;

- (id) initWithParams:(NSString *)name
         gammaWeights:(float **)gammaWeights
          betaWeights:(float **)betaWeights
numberFeatureChannels:(int)numberFeatureChannels
               styles:(int)styles
               device:(id<MTLDevice> _Nonnull)dev 
            cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q {
  @autoreleasepool{ 
    self = [self init];
    
    mName = name;
    mStyles = styles;
    mNumberOfFeatureChannels = (NSUInteger) numberFeatureChannels;
    
    mCurrentStyle = 0;  
  
    mGamma = gammaWeights;
    mBeta = betaWeights;

    mCq = cmd_q;

    float *zeros_ptr = (float*) calloc(mNumberOfFeatureChannels, sizeof(float));
    float *ones_ptr = (float*) malloc(mNumberOfFeatureChannels * sizeof(float));

    for(size_t x = 0; x < mNumberOfFeatureChannels; x ++){
      ones_ptr[x] = 1.0f;
    }
    
    runningUpdatePipelineState = nil;

    vDesc = [MPSVectorDescriptor vectorDescriptorWithLength:mNumberOfFeatureChannels
                                                   dataType:(MPSDataTypeFloat32)];

    mGammaBuffer = [dev newBufferWithBytes:mGamma[0]
                                    length:sizeof(float) * mNumberOfFeatureChannels
                                   options:MTLResourceStorageModeManaged];

    mBetaBuffer = [dev newBufferWithBytes:mBeta[0]
                                   length:sizeof(float) * mNumberOfFeatureChannels
                                  options:MTLResourceStorageModeManaged];


    mGammaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                            length:sizeof(float) * mNumberOfFeatureChannels
                                           options:MTLResourceStorageModeManaged];

    mGammaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                            length:sizeof(float) * mNumberOfFeatureChannels
                                           options:MTLResourceStorageModeManaged];

    mBetaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                           length:sizeof(float) * mNumberOfFeatureChannels
                                          options:MTLResourceStorageModeManaged];

    mBetaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                           length:sizeof(float) * mNumberOfFeatureChannels
                                          options:MTLResourceStorageModeManaged];

    mMovingMeanBuffer = [dev newBufferWithBytes:zeros_ptr
                                         length:sizeof(float) * mNumberOfFeatureChannels
                                        options:MTLResourceStorageModeManaged];

    mMovingVarianceBuffer = [dev newBufferWithBytes:ones_ptr
                                             length:sizeof(float) * mNumberOfFeatureChannels
                                            options:MTLResourceStorageModeManaged];
    
    adamGamma = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                              learningRate:0.001f];
        
    adamBeta = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                             learningRate:0.001f];
      
    mGammaVector = [[MPSVector alloc] initWithBuffer:mGammaBuffer
                                          descriptor:vDesc];

    mGammaMomentumVector = [[MPSVector alloc] initWithBuffer:mGammaMomentumBuffer
                                                  descriptor:vDesc];

    mGammaVelocityVector = [[MPSVector alloc] initWithBuffer:mGammaVelocityBuffer
                                                  descriptor:vDesc];

    mBetaVector = [[MPSVector alloc] initWithBuffer:mBetaBuffer
                                         descriptor:vDesc];

    mBetaMomentumVector = [[MPSVector alloc] initWithBuffer:mBetaMomentumBuffer
                                                 descriptor:vDesc];

    mBetaVelocityVector = [[MPSVector alloc] initWithBuffer:mBetaVelocityBuffer
                                                 descriptor:vDesc];


    mState = [[MPSCNNNormalizationGammaAndBetaState alloc] initWithGamma:mGammaBuffer
                                                                    beta:mBetaBuffer];

    return self;
  }
}

- (void) updateNumberOfStyles:(int)styles {
  mCurrentStyle = 0;
  mStyles = styles;
}

- (void) updateCurrentStyle:(int)style {
  mCurrentStyle = style;
}

- (int) getCurrentStyle {
  return mCurrentStyle;
}

- (void) loadBeta:(float **)beta {
  mBeta = beta;
}

- (float *) beta {
  [self checkpointWithCommandQueue:mCq];
  return (float *) [[mBetaVector data] contents];
}

- (void) loadGamma:(float **)gamma {
  mGamma = gamma;
}

- (float *) gamma {
  [self checkpointWithCommandQueue:mCq];
  return (float*) [[mGammaVector data] contents];
}

- (MPSCNNNormalizationGammaAndBetaState *)updateGammaAndBetaWithCommandBuffer:(id<MTLCommandBuffer>)commandBuffer 
                                              instanceNormalizationStateBatch:(MPSCNNInstanceNormalizationGradientStateBatch *)instanceNormalizationStateBatch {
    
  NSUInteger t1 = [adamGamma timeStep];
  NSUInteger t2 = [adamBeta timeStep];

    for (MPSCNNInstanceNormalizationGradientState *instanceNormalizationState in instanceNormalizationStateBatch) {
      MPSVector *gradientWeightsVector = [[MPSVector alloc] initWithBuffer:nonnull_cast_a(instanceNormalizationState.gradientForGamma)
                                                                descriptor:vDesc];

      MPSVector *inputWeightsVector = [[MPSVector alloc] initWithBuffer:nonnull_cast_a(instanceNormalizationState.gamma)
                                                             descriptor:vDesc];
      adamGamma.timeStep = t1;
      [adamGamma encodeToCommandBuffer:commandBuffer
                   inputGradientVector:gradientWeightsVector
                     inputValuesVector:inputWeightsVector
                   inputMomentumVector:mGammaMomentumVector
                   inputVelocityVector:mGammaVelocityVector
                    resultValuesVector:mGammaVector];

      MPSVector *gradientBiasesVector = [[MPSVector alloc] initWithBuffer:nonnull_cast_a(instanceNormalizationState.gradientForBeta)
                                                               descriptor:vDesc];

      MPSVector *inputBiasesVector = [[MPSVector alloc] initWithBuffer:nonnull_cast_a(instanceNormalizationState.beta)
                                                            descriptor:vDesc];
      adamBeta.timeStep = t2;
      [adamBeta encodeToCommandBuffer:commandBuffer
                  inputGradientVector:gradientBiasesVector
                    inputValuesVector:inputBiasesVector
                  inputMomentumVector:mBetaMomentumVector
                  inputVelocityVector:mBetaVelocityVector
                   resultValuesVector:mBetaVector];

  }

    return mState;
}

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue {
  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLBlitCommandEncoder> blit = commandBuffer.blitCommandEncoder;

  [blit synchronizeResource:mBetaMomentumBuffer];
  [blit synchronizeResource:mBetaVelocityBuffer];
  [blit synchronizeResource:nonnull_cast_a(mState.beta)];

  [blit synchronizeResource:mGammaMomentumBuffer];
  [blit synchronizeResource:mGammaVelocityBuffer];
  [blit synchronizeResource:nonnull_cast_a(mState.gamma)];

  [blit synchronizeResource:mMovingMeanBuffer];
  [blit synchronizeResource:mMovingVarianceBuffer];

  [blit endEncoding];

  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];
}

- (NSString*__nullable) label {
  return mName;
}

- (id) copyWithZone:(nullable NSZone *) zone {
  return self;
}

@end

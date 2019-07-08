#include <ml/neural_net/mps_layer_instance_norm_data_loader.h>
#include <ml/neural_net/mps_weight.h>

@interface TCMPSInstanceNormDataLoader () {
  float *_gamma_weights;
  float *_beta_weights;
  
  NSString *_name;
  NSUInteger _styles;

  id<MTLBuffer> _gammaBuffer;
  id<MTLBuffer> _betaBuffer;

  MPSVector *_gammaVector;
  MPSVector *_betaVector;
  MPSVector *_gammaMomentumVector;
  MPSVector *_gammaVelocityVector;
  MPSVector *_betaMomentumVector;
  MPSVector *_betaVelocityVector;

  id<MTLBuffer> _gammaMomentumBuffer;
  id<MTLBuffer> _gammaVelocityBuffer;
  id<MTLBuffer> _betaMomentumBuffer;
  id<MTLBuffer> _betaVelocityBuffer;
  id<MTLBuffer> _movingMeanBuffer;
  id<MTLBuffer> _movingVarianceBuffer;

  MPSVectorDescriptor *_vDesc;
  MPSCNNNormalizationGammaAndBetaState *_state;

  id<MTLCommandQueue> _cq;
  MPSNNOptimizerAdam *_adamGamma;
  MPSNNOptimizerAdam *_adamBeta;
}

@end

@implementation TCMPSInstanceNormDataLoader

- (instancetype) initWithParams:(NSString *)name
                   gammaWeights:(float *)gammaWeights
                    betaWeights:(float *)betaWeights
          numberFeatureChannels:(NSUInteger)numberFeatureChannels
                         styles:(NSUInteger)styles
                         device:(id<MTLDevice>)dev 
                      cmd_queue:(id<MTLCommandQueue>) cmd_q {
  self = [self init];
    
  if (self) {
    _name = name;
    _numberOfFeatureChannels = numberFeatureChannels;

    _styles = styles;
    
    _currentStyle = 0;  
    
    _gamma_weights = (float *) malloc(numberFeatureChannels * styles * sizeof(float));
    _beta_weights = (float *) malloc(numberFeatureChannels * styles * sizeof(float));

    memcpy(_gamma_weights, gammaWeights, numberFeatureChannels * styles * sizeof(float));
    memcpy(_beta_weights, betaWeights, numberFeatureChannels * styles * sizeof(float));

    _cq = cmd_q;

    float *zeros_ptr = (float*) calloc(_numberOfFeatureChannels, sizeof(float));
    float *ones_ptr = (float*) malloc(_numberOfFeatureChannels * sizeof(float));

    for(size_t x = 0; x < _numberOfFeatureChannels; x ++){
      ones_ptr[x] = 1.0f;
    }

    _vDesc = [MPSVectorDescriptor vectorDescriptorWithLength:_numberOfFeatureChannels
                                                   dataType:(MPSDataTypeFloat32)];

    _gammaBuffer = [dev newBufferWithBytes:_gamma_weights
                                    length:sizeof(float) * _numberOfFeatureChannels
                                   options:MTLResourceStorageModeManaged];

    _betaBuffer = [dev newBufferWithBytes:_beta_weights
                                   length:sizeof(float) * _numberOfFeatureChannels
                                  options:MTLResourceStorageModeManaged];


    _gammaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                            length:sizeof(float) * _numberOfFeatureChannels
                                           options:MTLResourceStorageModeManaged];

    _gammaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                            length:sizeof(float) * _numberOfFeatureChannels
                                           options:MTLResourceStorageModeManaged];

    _betaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                           length:sizeof(float) * _numberOfFeatureChannels
                                          options:MTLResourceStorageModeManaged];

    _betaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                           length:sizeof(float) * _numberOfFeatureChannels
                                          options:MTLResourceStorageModeManaged];

    _movingMeanBuffer = [dev newBufferWithBytes:zeros_ptr
                                         length:sizeof(float) * _numberOfFeatureChannels
                                        options:MTLResourceStorageModeManaged];

    _movingVarianceBuffer = [dev newBufferWithBytes:ones_ptr
                                             length:sizeof(float) * _numberOfFeatureChannels
                                            options:MTLResourceStorageModeManaged];
    
    _adamGamma = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                              learningRate:0.001f];
        
    _adamBeta = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                             learningRate:0.001f];
      
    _gammaVector = [[MPSVector alloc] initWithBuffer:_gammaBuffer
                                          descriptor:_vDesc];

    _gammaMomentumVector = [[MPSVector alloc] initWithBuffer:_gammaMomentumBuffer
                                                  descriptor:_vDesc];

    _gammaVelocityVector = [[MPSVector alloc] initWithBuffer:_gammaVelocityBuffer
                                                  descriptor:_vDesc];

    _betaVector = [[MPSVector alloc] initWithBuffer:_betaBuffer
                                         descriptor:_vDesc];

    _betaMomentumVector = [[MPSVector alloc] initWithBuffer:_betaMomentumBuffer
                                                 descriptor:_vDesc];

    _betaVelocityVector = [[MPSVector alloc] initWithBuffer:_betaVelocityBuffer
                                                 descriptor:_vDesc];


    _state = [[MPSCNNNormalizationGammaAndBetaState alloc] initWithGamma:_gammaBuffer
                                                                    beta:_betaBuffer];
    
    free(zeros_ptr);
    free(ones_ptr);
  }
  return self;
}

-(void)dealloc {
  free(_gamma_weights);
  free(_beta_weights);
}

- (void) updateCurrentStyle:(NSUInteger)style {
  _currentStyle = style;
}

- (void) loadBeta:(float *)beta {
  _beta_weights = beta;
}

- (float *) beta {
  [self checkpointWithCommandQueue:_cq];
  return (float *) [[_betaVector data] contents];
}

- (void) loadGamma:(float *)gamma {
  _gamma_weights = gamma;
}

- (float *) gamma {
  [self checkpointWithCommandQueue:_cq];
  return (float*) [[_gammaVector data] contents];
}

- (MPSCNNNormalizationGammaAndBetaState *)updateGammaAndBetaWithCommandBuffer:(id<MTLCommandBuffer>)commandBuffer 
                                              instanceNormalizationStateBatch:(MPSCNNInstanceNormalizationGradientStateBatch *)instanceNormalizationStateBatch {
    
  NSUInteger t1 = [_adamGamma timeStep];
  NSUInteger t2 = [_adamBeta timeStep];

    for (MPSCNNInstanceNormalizationGradientState *instanceNormalizationState in instanceNormalizationStateBatch) {
      MPSVector *gradientWeightsVector = [[MPSVector alloc] initWithBuffer:nonnull_cast(instanceNormalizationState.gradientForGamma)
                                                                descriptor:_vDesc];

      MPSVector *inputWeightsVector = [[MPSVector alloc] initWithBuffer:nonnull_cast(instanceNormalizationState.gamma)
                                                             descriptor:_vDesc];
      _adamGamma.timeStep = t1;
      [_adamGamma encodeToCommandBuffer:commandBuffer
                    inputGradientVector:gradientWeightsVector
                      inputValuesVector:inputWeightsVector
                    inputMomentumVector:_gammaMomentumVector
                    inputVelocityVector:_gammaVelocityVector
                     resultValuesVector:_gammaVector];

      MPSVector *gradientBiasesVector = [[MPSVector alloc] initWithBuffer:nonnull_cast(instanceNormalizationState.gradientForBeta)
                                                               descriptor:_vDesc];

      MPSVector *inputBiasesVector = [[MPSVector alloc] initWithBuffer:nonnull_cast(instanceNormalizationState.beta)
                                                            descriptor:_vDesc];
      _adamBeta.timeStep = t2;
      [_adamBeta encodeToCommandBuffer:commandBuffer
                   inputGradientVector:gradientBiasesVector
                     inputValuesVector:inputBiasesVector
                   inputMomentumVector:_betaMomentumVector
                   inputVelocityVector:_betaVelocityVector
                    resultValuesVector:_betaVector];

  }

    return _state;
}

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue {
  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLBlitCommandEncoder> blit = commandBuffer.blitCommandEncoder;

  [blit synchronizeResource:_betaMomentumBuffer];
  [blit synchronizeResource:_betaVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(_state.beta)];

  [blit synchronizeResource:_gammaMomentumBuffer];
  [blit synchronizeResource:_gammaVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(_state.gamma)];

  [blit synchronizeResource:_movingMeanBuffer];
  [blit synchronizeResource:_movingVarianceBuffer];

  [blit endEncoding];

  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];
}

- (NSString*__nullable) label {
  return _name;
}

- (id) copyWithZone:(nullable NSZone *) zone {
  return self;
}

@end

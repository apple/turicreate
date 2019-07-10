#include <ml/neural_net/mps_layer_instance_norm_data_loader.h>
#include <ml/neural_net/mps_weight.h>

@interface TCMPSInstanceNormDataLoader () {
  NSMutableData *_gamma_weights;
  NSMutableData *_beta_weights;
  
  NSString *_name;
  NSUInteger _styles;

  NSMutableArray<MPSVector *> *_gammaVectorArray;
  NSMutableArray<MPSVector *> *_betaVectorArray;
  NSMutableArray<MPSVector *> *_gammaMomentumVectorArray;
  NSMutableArray<MPSVector *> *_gammaVelocityVectorArray;
  NSMutableArray<MPSVector *> *_betaMomentumVectorArray;
  NSMutableArray<MPSVector *> *_betaVelocityVectorArray;

  NSMutableArray<id<MTLBuffer>> *_movingMeanBufferArray;
  NSMutableArray<id<MTLBuffer>> *_movingVarianceBufferArray;
  NSMutableArray<id<MTLBuffer>> *_gammaMomentumBufferArray;
  NSMutableArray<id<MTLBuffer>> *_gammaVelocityBufferArray;
  NSMutableArray<id<MTLBuffer>> *_betaMomentumBufferArray;
  NSMutableArray<id<MTLBuffer>> *_betaVelocityBufferArray;

  MPSVectorDescriptor *_vDesc;
  NSMutableArray<MPSCNNNormalizationGammaAndBetaState *> *stateArray;

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
    
    _gamma_weights = [NSMutableData dataWithLength:numberFeatureChannels * styles * sizeof(float)];
    _beta_weights = [NSMutableData dataWithLength:numberFeatureChannels * styles * sizeof(float)];

    _gamma_weights = [NSMutableData dataWithBytes:gammaWeights length:numberFeatureChannels * styles * sizeof(float)];
    _beta_weights = [NSMutableData dataWithBytes:betaWeights length:numberFeatureChannels * styles * sizeof(float)];

    _cq = cmd_q;

    float *zeros_ptr = (float*) calloc(_numberOfFeatureChannels, sizeof(float));
    float *ones_ptr = (float*) malloc(_numberOfFeatureChannels * sizeof(float));

    for(size_t x = 0; x < _numberOfFeatureChannels; x ++){
      ones_ptr[x] = 1.0f;
    }

    _adamGamma = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                              learningRate:0.001f];
        
    _adamBeta = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                             learningRate:0.001f];

    _vDesc = [MPSVectorDescriptor vectorDescriptorWithLength:_numberOfFeatureChannels
                                                   dataType:(MPSDataTypeFloat32)];

    _gammaVectorArray = [[NSMutableArray alloc] init];
    _betaVectorArray = [[NSMutableArray alloc] init];
    _gammaMomentumVectorArray = [[NSMutableArray alloc] init];
    _gammaVelocityVectorArray = [[NSMutableArray alloc] init];
    _betaMomentumVectorArray = [[NSMutableArray alloc] init];
    _betaVelocityVectorArray = [[NSMutableArray alloc] init];

    _movingMeanBufferArray = [[NSMutableArray alloc] init];
    _movingVarianceBufferArray = [[NSMutableArray alloc] init];
    _gammaMomentumBufferArray = [[NSMutableArray alloc] init];
    _gammaVelocityBufferArray = [[NSMutableArray alloc] init];
    _betaMomentumBufferArray = [[NSMutableArray alloc] init];
    _betaVelocityBufferArray = [[NSMutableArray alloc] init];

    for (NSUInteger index = 0; index < styles; index ++) {
      id<MTLBuffer> gammaBuffer = [dev newBufferWithBytes:_gamma_weights.mutableBytes
                                                   length:sizeof(float) * _numberOfFeatureChannels
                                                  options:MTLResourceStorageModeManaged];

      id<MTLBuffer> betaBuffer = [dev newBufferWithBytes:_beta_weights.mutableBytes
                                                  length:sizeof(float) * _numberOfFeatureChannels
                                                 options:MTLResourceStorageModeManaged];

      id<MTLBuffer> gammaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                                           length:sizeof(float) * _numberOfFeatureChannels
                                                          options:MTLResourceStorageModeManaged];

      [_gammaMomentumBufferArray addObject:gammaMomentumBuffer];

      id<MTLBuffer> gammaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                                           length:sizeof(float) * _numberOfFeatureChannels
                                                          options:MTLResourceStorageModeManaged];

      [_gammaVelocityBufferArray addObject:gammaVelocityBuffer];

      id<MTLBuffer> betaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                                          length:sizeof(float) * _numberOfFeatureChannels
                                                         options:MTLResourceStorageModeManaged];

      [_betaMomentumBufferArray addObject:betaMomentumBuffer];

      id<MTLBuffer> betaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                                          length:sizeof(float) * _numberOfFeatureChannels
                                                         options:MTLResourceStorageModeManaged];

      [_betaVelocityBufferArray addObject:betaVelocityBuffer];

      id<MTLBuffer> movingMeanBuffer = [dev newBufferWithBytes:zeros_ptr
                                                        length:sizeof(float) * _numberOfFeatureChannels
                                                       options:MTLResourceStorageModeManaged];

      [_movingMeanBufferArray addObject:movingMeanBuffer];

      id<MTLBuffer> movingVarianceBuffer = [dev newBufferWithBytes:ones_ptr
                                                            length:sizeof(float) * _numberOfFeatureChannels
                                                           options:MTLResourceStorageModeManaged];

      [_movingVarianceBufferArray addObject:movingVarianceBuffer];

      MPSVector *gammaVector = [[MPSVector alloc] initWithBuffer:gammaBuffer
                                                      descriptor:_vDesc];

      [_gammaVectorArray addObject:gammaVector];

      MPSVector *gammaMomentumVector = [[MPSVector alloc] initWithBuffer:gammaMomentumBuffer
                                                              descriptor:_vDesc];

      [_gammaMomentumVectorArray addObject:gammaMomentumVector];

      MPSVector *gammaVelocityVector = [[MPSVector alloc] initWithBuffer:gammaVelocityBuffer
                                                              descriptor:_vDesc];

      [_gammaVelocityVectorArray addObject:gammaVelocityVector];

      MPSVector *betaVector = [[MPSVector alloc] initWithBuffer:betaBuffer
                                                     descriptor:_vDesc];

      [_betaVectorArray addObject:betaVector];

      MPSVector *betaMomentumVector = [[MPSVector alloc] initWithBuffer:betaMomentumBuffer
                                                             descriptor:_vDesc];

      [_betaMomentumVectorArray addObject:betaMomentumVector];

      MPSVector *betaVelocityVector = [[MPSVector alloc] initWithBuffer:betaVelocityBuffer
                                                             descriptor:_vDesc];
      [_betaVelocityVectorArray addObject:betaVelocityVector];

      MPSCNNNormalizationGammaAndBetaState* state = [[MPSCNNNormalizationGammaAndBetaState alloc] 
                                                          initWithGamma:gammaBuffer
                                                                   beta:betaBuffer];

      [stateArray addObject:state];
    }
    
    free(zeros_ptr);
    free(ones_ptr);
  }
  return self;
}

- (void) updateCurrentStyle:(NSUInteger)style {
  _currentStyle = style;
}

- (void) loadBeta:(float *)beta {
  memcpy(_beta_weights.mutableBytes, beta, _numberOfFeatureChannels * _styles * sizeof(float));
}

- (float *) beta {
  [self checkpointWithCommandQueue:_cq];
  return (float *) [[[_betaVectorArray objectAtIndex: _currentStyle] data] contents];
}

- (void) loadGamma:(float *)gamma {
  memcpy(_gamma_weights.mutableBytes, gamma, _numberOfFeatureChannels * _styles * sizeof(float));
}

- (float *) gamma {
  [self checkpointWithCommandQueue:_cq];
  return (float*) [[[_gammaVectorArray objectAtIndex: _currentStyle] data] contents];
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
                    inputMomentumVector:_gammaMomentumVectorArray[_currentStyle]
                    inputVelocityVector:_gammaVelocityVectorArray[_currentStyle]
                     resultValuesVector:_gammaVectorArray[_currentStyle]];

      MPSVector *gradientBiasesVector = [[MPSVector alloc] initWithBuffer:nonnull_cast(instanceNormalizationState.gradientForBeta)
                                                               descriptor:_vDesc];

      MPSVector *inputBiasesVector = [[MPSVector alloc] initWithBuffer:nonnull_cast(instanceNormalizationState.beta)
                                                            descriptor:_vDesc];
      _adamBeta.timeStep = t2;
      [_adamBeta encodeToCommandBuffer:commandBuffer
                   inputGradientVector:gradientBiasesVector
                     inputValuesVector:inputBiasesVector
                   inputMomentumVector:_betaMomentumVectorArray[_currentStyle]
                   inputVelocityVector:_betaVelocityVectorArray[_currentStyle]
                    resultValuesVector:_betaVectorArray[_currentStyle]];

  }

    return stateArray[_currentStyle];
}

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue {
  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLBlitCommandEncoder> blit = commandBuffer.blitCommandEncoder;

  for (size_t index = 0; index < _styles; index ++){
    [blit synchronizeResource:_betaMomentumBufferArray[index]];
    [blit synchronizeResource:_betaVelocityBufferArray[index]];
    [blit synchronizeResource:nonnull_cast(stateArray[index].beta)];

    [blit synchronizeResource:_gammaMomentumBufferArray[index]];
    [blit synchronizeResource:_gammaVelocityBufferArray[index]];
    [blit synchronizeResource:nonnull_cast(stateArray[index].gamma)];

    [blit synchronizeResource:_movingMeanBufferArray[index]];
    [blit synchronizeResource:_movingVarianceBufferArray[index]];
  }

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

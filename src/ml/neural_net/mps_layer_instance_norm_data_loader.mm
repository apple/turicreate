/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/mps_layer_instance_norm_data_loader.h>
#include <ml/neural_net/mps_weight.h>

API_AVAILABLE(macos(10.14))
@interface TCMPSInstanceNormDataLoaderProps : NSObject
  @property (nonatomic) MPSVector *gammaVector;
  @property (nonatomic) MPSVector *betaVector;
  @property (nonatomic) MPSVector *gammaMomentumVector;
  @property (nonatomic) MPSVector *gammaVelocityVector;
  @property (nonatomic) MPSVector *betaMomentumVector;
  @property (nonatomic) MPSVector *betaVelocityVector;

  @property (nonatomic) id<MTLBuffer> movingMeanBuffer;
  @property (nonatomic) id<MTLBuffer> movingVarianceBuffer;
  @property (nonatomic) id<MTLBuffer> gammaMomentumBuffer;
  @property (nonatomic) id<MTLBuffer> gammaVelocityBuffer;
  @property (nonatomic) id<MTLBuffer> betaMomentumBuffer;
  @property (nonatomic) id<MTLBuffer> betaVelocityBuffer;
  @property (nonatomic) id<MTLBuffer> gammaBuffer;
  @property (nonatomic) id<MTLBuffer> betaBuffer;

  @property (nonatomic) MPSCNNNormalizationGammaAndBetaState *state;
@end

@implementation TCMPSInstanceNormDataLoaderProps 
@end

@interface TCMPSInstanceNormDataLoader () {
  NSMutableData *_gamma_weights;
  NSMutableData *_beta_weights;
  
  NSString *_name;
  NSMutableArray<TCMPSInstanceNormDataLoaderProps *> *_style_props;

  NSUInteger _styleIndex;

  NSMutableData * _gammaPlaceHolder;
  NSMutableData * _betaPlaceHolder;

  MPSVectorDescriptor *_vDesc;

  MPSCNNInstanceNormalization* _instanceNormFilter;

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
    
    _styleIndex = 0;  
    
    _gammaPlaceHolder = [NSMutableData data];
    _betaPlaceHolder = [NSMutableData data];

    
    _gamma_weights = [NSMutableData dataWithLength:numberFeatureChannels * styles * sizeof(float)];
    _beta_weights = [NSMutableData dataWithLength:numberFeatureChannels * styles * sizeof(float)];

    _gamma_weights = [NSMutableData dataWithBytes:gammaWeights length:numberFeatureChannels * styles * sizeof(float)];
    _beta_weights = [NSMutableData dataWithBytes:betaWeights length:numberFeatureChannels * styles * sizeof(float)];

    _cq = cmd_q;

    float *zeros_ptr = (float*) calloc(_numberOfFeatureChannels, sizeof(float));
    float *ones_ptr = (float*) malloc(_numberOfFeatureChannels * sizeof(float));

    for(NSUInteger x = 0; x < _numberOfFeatureChannels; x ++) ones_ptr[x] = 1.0f;

    _adamGamma = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                              learningRate:0.001f];
        
    _adamBeta = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                             learningRate:0.001f];

    _vDesc = [MPSVectorDescriptor vectorDescriptorWithLength:_numberOfFeatureChannels
                                                   dataType:(MPSDataTypeFloat32)];

    _style_props = [[NSMutableArray alloc] init];

    for (NSUInteger index = 0; index < styles; index ++) {
      TCMPSInstanceNormDataLoaderProps *style_property = [[TCMPSInstanceNormDataLoaderProps alloc] init];

      size_t offset = index * _numberOfFeatureChannels * sizeof(float);

      style_property.gammaBuffer = [dev newBufferWithBytes:(char *) _gamma_weights.bytes + offset
                                                   length:sizeof(float) * _numberOfFeatureChannels
                                                  options:MTLResourceStorageModeManaged];

      style_property.betaBuffer = [dev newBufferWithBytes:(char *) _beta_weights.bytes + offset
                                                  length:sizeof(float) * _numberOfFeatureChannels
                                                 options:MTLResourceStorageModeManaged];

      style_property.gammaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                                            length:sizeof(float) * _numberOfFeatureChannels
                                                           options:MTLResourceStorageModeManaged];

      style_property.gammaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                                            length:sizeof(float) * _numberOfFeatureChannels
                                                           options:MTLResourceStorageModeManaged];

      style_property.betaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                                           length:sizeof(float) * _numberOfFeatureChannels
                                                          options:MTLResourceStorageModeManaged];

      style_property.betaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                                           length:sizeof(float) * _numberOfFeatureChannels
                                                          options:MTLResourceStorageModeManaged];

      style_property.movingMeanBuffer = [dev newBufferWithBytes:zeros_ptr
                                                         length:sizeof(float) * _numberOfFeatureChannels
                                                        options:MTLResourceStorageModeManaged];

      style_property.movingVarianceBuffer = [dev newBufferWithBytes:ones_ptr
                                                             length:sizeof(float) * _numberOfFeatureChannels
                                                            options:MTLResourceStorageModeManaged];

      style_property.gammaVector = [[MPSVector alloc] initWithBuffer:style_property.gammaBuffer
                                                          descriptor:_vDesc];

      style_property.gammaMomentumVector = [[MPSVector alloc] initWithBuffer:style_property.gammaMomentumBuffer
                                                                  descriptor:_vDesc];

      style_property.gammaVelocityVector = [[MPSVector alloc] initWithBuffer:style_property.gammaVelocityBuffer
                                                                  descriptor:_vDesc];

      style_property.betaVector = [[MPSVector alloc] initWithBuffer:style_property.betaBuffer
                                                         descriptor:_vDesc];

      style_property.betaMomentumVector = [[MPSVector alloc] initWithBuffer:style_property.betaMomentumBuffer
                                                                 descriptor:_vDesc];

      style_property.betaVelocityVector = [[MPSVector alloc] initWithBuffer:style_property.betaVelocityBuffer
                                                                 descriptor:_vDesc];

      style_property.state = [[MPSCNNNormalizationGammaAndBetaState alloc] initWithGamma:style_property.gammaBuffer
                                                                                    beta:style_property.betaBuffer];

      [_style_props addObject:style_property];
    }
    
    free(zeros_ptr);
    free(ones_ptr);
  }
  return self;
}

- (void) setLearningRate:(float)lr {
  [_adamGamma setLearningRate:lr];
  [_adamBeta setLearningRate:lr];
}

- (void) loadBeta:(float *)beta {
  float* betaWeights = (float *) [[[[_style_props objectAtIndex: _styleIndex] betaVector] data] contents];
  memcpy(betaWeights, beta, _numberOfFeatureChannels * _styles * sizeof(float));
}

- (float *) beta {
  NSUInteger previousStyle = _styleIndex;
  _betaPlaceHolder.length = 0;
  for (NSUInteger index = 0; index < _styles; index++) {
    _styleIndex = index;
    [self checkpointWithCommandQueue:_cq];
    float* betaWeights = (float *) [_style_props[_styleIndex].betaBuffer contents];
    [_betaPlaceHolder appendBytes:betaWeights length:sizeof(float)*_numberOfFeatureChannels];
  }
  _styleIndex = previousStyle;

  return (float *) (_betaPlaceHolder.bytes);
}

- (void) loadGamma:(float *)gamma {
  float* gammaWeights = (float*) [[[[_style_props objectAtIndex: _styleIndex] gammaVector] data] contents];
  memcpy(gammaWeights, gamma, _numberOfFeatureChannels * _styles * sizeof(float));
}

// TODO: refactor for multiple indicies
- (float *) gamma {
  NSUInteger previousStyle = _styleIndex;
  _gammaPlaceHolder.length = 0;
  for (NSUInteger index = 0; index < _styles; index++) { 
    _styleIndex = index; 
    [self checkpointWithCommandQueue:_cq];
    float* gammaWeights = (float *) [_style_props[_styleIndex].gammaBuffer contents];
    [_gammaPlaceHolder appendBytes:gammaWeights length:sizeof(float)*_numberOfFeatureChannels];
  }
  _styleIndex = previousStyle;

  return (float *) (_gammaPlaceHolder.bytes);
}

- (MPSCNNNormalizationGammaAndBetaState *)updateGammaAndBetaWithCommandBuffer:(id<MTLCommandBuffer>)commandBuffer 
                                              instanceNormalizationStateBatch:(MPSCNNInstanceNormalizationGradientStateBatch *)instanceNormalizationStateBatch {
  NSUInteger t1 = [_adamGamma timeStep];
  NSUInteger t2 = [_adamBeta timeStep];

  _instanceNormFilter = instanceNormalizationStateBatch[0].instanceNormalization;

  for (MPSCNNInstanceNormalizationGradientState *instanceNormalizationState in instanceNormalizationStateBatch) {
    MPSVector *gradientWeightsVector = [[MPSVector alloc] initWithBuffer:nonnull_cast(instanceNormalizationState.gradientForGamma)
                                                              descriptor:_vDesc];
    _adamGamma.timeStep = t1;
    [_adamGamma encodeToCommandBuffer:commandBuffer
                  inputGradientVector:gradientWeightsVector
                    inputValuesVector:[[_style_props objectAtIndex: _styleIndex] gammaVector]
                  inputMomentumVector:[[_style_props objectAtIndex: _styleIndex] gammaMomentumVector]
                  inputVelocityVector:[[_style_props objectAtIndex: _styleIndex] gammaVelocityVector]
                   resultValuesVector:[[_style_props objectAtIndex: _styleIndex] gammaVector]];

    MPSVector *gradientBiasesVector = [[MPSVector alloc] initWithBuffer:nonnull_cast(instanceNormalizationState.gradientForBeta)
                                                             descriptor:_vDesc];

    _adamBeta.timeStep = t2;
    [_adamBeta encodeToCommandBuffer:commandBuffer
                 inputGradientVector:gradientBiasesVector
                   inputValuesVector:[[_style_props objectAtIndex: _styleIndex] betaVector]
                 inputMomentumVector:[[_style_props objectAtIndex: _styleIndex] betaMomentumVector] 
                 inputVelocityVector:[[_style_props objectAtIndex: _styleIndex] betaVelocityVector]
                  resultValuesVector:[[_style_props objectAtIndex: _styleIndex] betaVector]];

  }

  return [[_style_props objectAtIndex: _styleIndex] state];
}

- (void)checkpoint {
  if (_instanceNormFilter) {
    id <MTLCommandBuffer> cmdBuf = [_cq commandBuffer];
    [_instanceNormFilter reloadGammaAndBetaWithCommandBuffer: cmdBuf
                                           gammaAndBetaState: [[_style_props objectAtIndex: _styleIndex] state]];
    [cmdBuf commit];
  }
}

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue {
  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLBlitCommandEncoder> blit = commandBuffer.blitCommandEncoder;

  for (size_t index = 0; index < _styles; index ++) {
    TCMPSInstanceNormDataLoaderProps *style_property = [_style_props objectAtIndex: index];

    [blit synchronizeResource:[style_property betaMomentumBuffer]];
    [blit synchronizeResource:[style_property betaVelocityBuffer]];
    [blit synchronizeResource:nonnull_cast([style_property state].beta)];

    [blit synchronizeResource:[style_property gammaMomentumBuffer]];
    [blit synchronizeResource:[style_property gammaVelocityBuffer]];
    [blit synchronizeResource:nonnull_cast([style_property state].gamma)];

    [blit synchronizeResource:[style_property movingMeanBuffer]];
    [blit synchronizeResource:[style_property movingVarianceBuffer]];
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

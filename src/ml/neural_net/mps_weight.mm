//
//  mps_mm.c
//  tcmps
//
//  Copyright © 2018 Turi. All rights reserved.
//

#include "mps_weight.h"

using turi::neural_net::OptimizerOptions;

NS_ASSUME_NONNULL_BEGIN

// Protocol generalizing across MPSNNOptimizerStochasticGradientDescent and
// MPSNNOptimizerAdam, for convenience.
API_AVAILABLE(macos(10.14))
@protocol TCMPSConvolutionWeightsOptimizing <NSObject>

// Conv update
-(void)encodeToCommandBuffer:(id<MTLCommandBuffer>)commandBuffer
    convolutionGradientState:(MPSCNNConvolutionGradientState *)gradientState
          convolutionWeights:(TCMPSConvolutionWeights *)weights;

// Batch norm update
-(void)encodeToCommandBuffer:(id<MTLCommandBuffer>)commandBuffer
     batchNormalizationState:(MPSCNNBatchNormalizationState *)state
               batchNormData:(TCMPSBatchNormWeights *)weights;

-(void)setLearningRate:(float)newLearningRate;

@end

// Category adding TCMPSConvolutionWeightsOptimizing to MPSNNOptimizerAdam
@interface MPSNNOptimizerAdam (TCMPSWeightsOptimizing) <TCMPSConvolutionWeightsOptimizing>
@end

// Category adding TCMPSConvolutionWeightsOptimizing to
// MPSNNOptimizerStochasticGradientDescent
@interface MPSNNOptimizerStochasticGradientDescent (TCMPSWeightsOptimizing) <TCMPSConvolutionWeightsOptimizing>
@end

// Class extension for TCMPSConvolutionWeights internal implementation
@interface TCMPSConvolutionWeights ()

@property (nonatomic, readonly) id<TCMPSConvolutionWeightsOptimizing> optimizer;

@property (nonatomic, readonly) MPSVector *weightMomentumVector;
@property (nonatomic, readonly) MPSVector *weightVelocityVector;
@property (nonatomic, readonly) MPSVector *biasMomentumVector;
@property (nonatomic, readonly) MPSVector *biasVelocityVector;

@end

// Class extension for TCMPSBatchNormWeights internal implementation
@interface TCMPSBatchNormWeights ()

@property (nonatomic, readonly) id<TCMPSConvolutionWeightsOptimizing> optimizer;

@property (nonatomic, readonly) MPSVector *gammaMomentumVector;
@property (nonatomic, readonly) MPSVector *betaMomentumVector;
@property (nonatomic, readonly) MPSVector *gammaVelocityVector;
@property (nonatomic, readonly) MPSVector *betaVelocityVector;

@property (nonatomic, readonly) MPSVector *movingMeanVector;
@property (nonatomic, readonly) MPSVector *movingVarianceVector;

@end

NS_ASSUME_NONNULL_END

@implementation MPSNNOptimizerAdam (TCMPSWeightsOptimizing)

-(void)encodeToCommandBuffer:(id<MTLCommandBuffer>)commandBuffer
    convolutionGradientState:(MPSCNNConvolutionGradientState *)gradientState
          convolutionWeights:(TCMPSConvolutionWeights *)weights
{
  [self encodeToCommandBuffer:commandBuffer
     convolutionGradientState:gradientState
       convolutionSourceState:weights.state
         inputMomentumVectors:@[weights.weightMomentumVector, weights.biasMomentumVector]
         inputVelocityVectors:@[weights.weightVelocityVector, weights.biasVelocityVector]
                  resultState:weights.state];
}

-(void)encodeToCommandBuffer:(id<MTLCommandBuffer>)commandBuffer
     batchNormalizationState:(MPSCNNBatchNormalizationState *)state
               batchNormData:(TCMPSBatchNormWeights *)weights
{
  [self encodeToCommandBuffer:commandBuffer
      batchNormalizationState:state
         inputMomentumVectors:@[weights.gammaMomentumVector, weights.betaMomentumVector]
         inputVelocityVectors:@[weights.gammaVelocityVector, weights.betaVelocityVector]
                  resultState:weights.gammaBetaState];
}

@end

@implementation MPSNNOptimizerStochasticGradientDescent (TCMPSWeightsOptimizing)

-(void)encodeToCommandBuffer:(id<MTLCommandBuffer>)commandBuffer
    convolutionGradientState:(MPSCNNConvolutionGradientState *)gradientState
          convolutionWeights:(TCMPSConvolutionWeights *)weights
{
  [self encodeToCommandBuffer:commandBuffer
     convolutionGradientState:gradientState
       convolutionSourceState:weights.state
         inputMomentumVectors:@[weights.weightMomentumVector, weights.biasMomentumVector]
                  resultState:weights.state];
}

-(void)encodeToCommandBuffer:(id<MTLCommandBuffer>)commandBuffer
     batchNormalizationState:(MPSCNNBatchNormalizationState *)state
               batchNormData:(TCMPSBatchNormWeights *)weights
{
  [self encodeToCommandBuffer:commandBuffer
      batchNormalizationState:state
         inputMomentumVectors:@[weights.gammaMomentumVector, weights.betaMomentumVector]
                  resultState:weights.gammaBetaState];
}

@end

@implementation TCMPSConvolutionWeights

- (nonnull instancetype)initWithKernelWidth:(NSUInteger)kernelWidth
                               kernelHeight:(NSUInteger)kernelHeight
                       inputFeatureChannels:(NSUInteger)inputFeatureChannels
                      outputFeatureChannels:(NSUInteger)outputFeatureChannels
                                 neuronType:(MPSCNNNeuronType)neuronType
                                    neuronA:(float)neuronA
                                    neuronB:(float)neuronB
                                     stride:(NSUInteger)stride
                     kernelParamsBinaryName:(const char *__nonnull)kernelParamsBinaryName
                                     device:(id<MTLDevice> _Nonnull ) dev
                                  cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q
                            init_weight_ptr:(float* __nullable) w_ptr
                              init_bias_ptr:(float* __nullable) b_ptr
                           optimizerOptions:(OptimizerOptions)optimizerOptions
{
    return [self initWithKernelWidth:kernelWidth
                        kernelHeight:kernelHeight
                inputFeatureChannels:inputFeatureChannels
               outputFeatureChannels:outputFeatureChannels
                          neuronType:neuronType
                             strideX:stride
                             strideY:stride
                             neuronA:neuronA
                             neuronB:neuronB
              kernelParamsBinaryName:kernelParamsBinaryName
                              device:dev
                           cmd_queue:cmd_q
                     init_weight_ptr:w_ptr
                       init_bias_ptr:b_ptr
                    optimizerOptions:optimizerOptions];
}


- (nonnull instancetype)initWithKernelWidth:(NSUInteger)kernelWidth
                               kernelHeight:(NSUInteger)kernelHeight
                       inputFeatureChannels:(NSUInteger)inputFeatureChannels
                      outputFeatureChannels:(NSUInteger)outputFeatureChannels
                                 neuronType:(MPSCNNNeuronType)neuronType
                                    strideX:(NSUInteger)stride_x
                                    strideY:(NSUInteger)stride_y
                                    neuronA:(float)neuronA
                                    neuronB:(float)neuronB
                     kernelParamsBinaryName:(const char *__nonnull)kernelParamsBinaryName
                                     device:(id<MTLDevice> _Nonnull ) dev
                                  cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q
                            init_weight_ptr:(float* __nullable) w_ptr
                              init_bias_ptr:(float* __nullable) b_ptr
                           optimizerOptions:(OptimizerOptions)optimizerOptions
{
  self = [super init];
  if (nil == self)
    return nil;

  cq = cmd_q;
  _optimizerOptions = optimizerOptions;

  _outputFeatureChannels = outputFeatureChannels;
  _inputFeatureChannels = inputFeatureChannels;
  _kernelWidth = kernelWidth;
  _kernelHeight = kernelHeight;
  _kernelParamsBinaryName = kernelParamsBinaryName;
  _convDesc = [MPSCNNConvolutionDescriptor
      cnnConvolutionDescriptorWithKernelWidth:kernelWidth
                                 kernelHeight:kernelHeight
                         inputFeatureChannels:inputFeatureChannels
                        outputFeatureChannels:outputFeatureChannels];

  _convDesc.strideInPixelsX = stride_x;
  _convDesc.strideInPixelsY = stride_y;
  MPSNNNeuronDescriptor *neuronDesc =
      [MPSNNNeuronDescriptor cnnNeuronDescriptorWithType:neuronType
                                                       a:neuronA
                                                       b:neuronB];
  _convDesc.fusedNeuronDescriptor = neuronDesc;

  sizeBias = _outputFeatureChannels * sizeof(float);
  sizeWeights = _inputFeatureChannels * _kernelHeight * _kernelWidth *
                _outputFeatureChannels * sizeof(float);

  MPSNNOptimizerDescriptor *desc = _optimizerOptions.mpsDescriptor();

  if (_optimizerOptions.useSGD) {
    _optimizer = [[MPSNNOptimizerStochasticGradientDescent alloc] initWithDevice:dev
                                                                 momentumScale:_optimizerOptions.sgdMomentum useNestrovMomentum:NO optimizerDescriptor:desc];
  } else {
    _optimizer =  [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                                       beta1:_optimizerOptions.adamBeta1
                                                       beta2:_optimizerOptions.adamBeta2
                                                     epsilon:_optimizerOptions.adamEpsilon
                                                    timeStep:0
                                         optimizerDescriptor:desc];
  }

  MTLResourceOptions storageMode;
  storageMode = MTLResourceStorageModeManaged;

  weightMomentumBuffer = [dev
      newBufferWithLength:sizeof(float) * _inputFeatureChannels *
                          _kernelHeight * _kernelWidth * _outputFeatureChannels
                  options:storageMode];

  weightVelocityBuffer = [dev
      newBufferWithLength:sizeof(float) * _inputFeatureChannels *
                          _kernelHeight * _kernelWidth * _outputFeatureChannels
                  options:storageMode];

  if (w_ptr != NULL) {

    weightBuffer = [dev
      newBufferWithBytes:w_ptr
                  length:sizeof(float) * _inputFeatureChannels * _kernelHeight *
                         _kernelWidth * _outputFeatureChannels
                 options:storageMode];
  } else {
    weightBuffer = [dev
                    newBufferWithLength:sizeof(float) * _inputFeatureChannels *
                    _kernelHeight * _kernelWidth * _outputFeatureChannels
                    options:storageMode];
  }

  if (b_ptr != NULL) {

    biasBuffer = [dev newBufferWithBytes:b_ptr
                                length:sizeof(float) * _outputFeatureChannels
                               options:storageMode];
  } else {
    biasBuffer = [dev newBufferWithLength:sizeof(float) * _outputFeatureChannels
                                 options:storageMode];
  }


  biasMomentumBuffer =
      [dev newBufferWithLength:sizeof(float) * _outputFeatureChannels
                       options:storageMode];

  biasVelocityBuffer =
      [dev newBufferWithLength:sizeof(float) * _outputFeatureChannels
                       options:storageMode];

  _state = [[MPSCNNConvolutionWeightsAndBiasesState alloc] initWithWeights:weightBuffer
                                                                    biases:biasBuffer];

  MPSVectorDescriptor *vDescWeights = [MPSVectorDescriptor
      vectorDescriptorWithLength:_inputFeatureChannels * _kernelHeight *
                                 _kernelWidth * _outputFeatureChannels
                        dataType:(MPSDataTypeFloat32)];

  _weightMomentumVector = [[MPSVector alloc] initWithBuffer:weightMomentumBuffer
                                                 descriptor:vDescWeights];

  _weightVelocityVector = [[MPSVector alloc] initWithBuffer:weightVelocityBuffer
                                                 descriptor:vDescWeights];

  MPSVectorDescriptor *vDescBiases =
      [MPSVectorDescriptor vectorDescriptorWithLength:_outputFeatureChannels
                                             dataType:(MPSDataTypeFloat32)];

  _biasMomentumVector = [[MPSVector alloc] initWithBuffer:biasMomentumBuffer
                                               descriptor:vDescBiases];

  _biasVelocityVector = [[MPSVector alloc] initWithBuffer:biasVelocityBuffer
                                               descriptor:vDescBiases];

  return self;
}

// We don't yet trigger any copies of this data source, but real implementations
// here will be necessary to support training with additional GPUs
- (instancetype)copyWithZone:(nullable NSZone *)zone {
  assert(false && "NSCopying not implemented for TCMPSConvolutionWeights");
  return self;
}

- (instancetype)copyWithZone:(nullable NSZone *)zone device:(nullable id <MTLDevice>)device {
  assert(false && "NSCopying not implemented for TCMPSConvolutionWeights");
  return self;
}

- (MPSDataType)dataType {
  return MPSDataTypeFloat32;
}

- (void)setLearningRate:(float)lr {
  [_optimizer setLearningRate:lr];
  _optimizerOptions.learningRate = lr;
}

- (size_t)weightSize{
    return sizeWeights / sizeof(float);
}
- (size_t)biasSize{
    return sizeBias / sizeof(float);
}

- (MPSCNNConvolutionDescriptor *__nonnull)descriptor {
  return _convDesc;
}

- (void *__nonnull)weights {
  return (float *)weightBuffer.contents;
}

- (float *__nullable)biasTerms {
  return (float *)biasBuffer.contents;
};

- (BOOL)load {
  [self checkpointWithCommandQueue:cq];
  return YES;
}

- (void)purge{};

- (MPSCNNConvolutionWeightsAndBiasesState *__nullable)
updateWithCommandBuffer:(__nonnull id<MTLCommandBuffer>)commandBuffer
          gradientState:
              (MPSCNNConvolutionGradientState *__nonnull)gradientState {

  [self.optimizer encodeToCommandBuffer:commandBuffer
               convolutionGradientState:gradientState
                     convolutionWeights:self];

  return self.state;
}

- (MPSCNNConvolutionWeightsAndBiasesState *__nullable)
updateWithCommandBuffer:(__nonnull id<MTLCommandBuffer>)commandBuffer
          gradientState:(MPSCNNConvolutionGradientState *__nonnull)gradientState
            sourceState:
                (MPSCNNConvolutionWeightsAndBiasesState *__nonnull)sourceState {

  // Note: we ignore sourceState here, as self.state has the same data, with
  // possibly higher precision.
  return [self updateWithCommandBuffer:commandBuffer
                         gradientState:gradientState];
}

- (void)set_cq:(id<MTLCommandQueue>)cmd_queue {
  cq = cmd_queue;
}

- (void)checkpoint {
}

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue {

  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLBlitCommandEncoder> blit = commandBuffer.blitCommandEncoder;
  assert(self.state.weights);
  [blit synchronizeResource:weightMomentumBuffer];
  [blit synchronizeResource:weightVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(self.state.weights)];

  assert(self.state.biases);
  [blit synchronizeResource:biasMomentumBuffer];
  [blit synchronizeResource:biasVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(self.state.biases)];

  [blit endEncoding];

  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];
}

- (void)loadWeight:(float *)src {
  memcpy(weightBuffer.contents, (void *)src, sizeWeights);
  [weightBuffer didModifyRange:NSMakeRange(0, sizeWeights)];
}

- (void)loadBias:(float *)src {
  float *dst = (float *)biasBuffer.contents;
  memcpy((void *)dst, (void *)src, sizeBias);
  [biasBuffer didModifyRange:NSMakeRange(0, sizeBias)];
}

- (NSString *_Nullable)label {
  return [NSString stringWithUTF8String:_kernelParamsBinaryName.c_str()];
}

@end  // TCMPSConvolutionWeights

@implementation TCMPSBatchNormWeights

@synthesize internalLabel = _label;

- (nonnull instancetype)initWithChannels:(NSUInteger)channels
                  kernelParamsBinaryName:
                      (const char *__nonnull)kernelParamsBinaryName
                                  device:(id<MTLDevice> _Nonnull ) dev
                               cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q
                                   gamma:(float *__nullable)g_ptr
                                    beta:(float *__nullable)b_ptr
                              moving_avg:(float *__nullable)ma_ptr
                              moving_var:(float *__nullable)mv_ptr
                        optimizerOptions:(OptimizerOptions)optimizerOptions
                        batchNormEpsilon:(float)batchNormEpsilon {
  self = [super init];
  if (nil == self)
    return nil;

  _optimizerOptions = optimizerOptions;
  _batchNormEpsilon = batchNormEpsilon;

  cq = cmd_q;

  _kernelParamsBinaryName = kernelParamsBinaryName;

  // memory map the parameters
  std::vector<float> zeros(channels, 0);
  float *zeros_ptr = zeros.data();
  std::vector<float> ones(channels, 1);
  float *ones_ptr = ones.data();

  t = 0.f;
  float bnRunningUpdateMomentum = 0.9f;

  _channels = channels;

  MPSNNOptimizerDescriptor *desc = _optimizerOptions.mpsDescriptor();

  if (_optimizerOptions.useSGD) {
    _optimizer = [[MPSNNOptimizerStochasticGradientDescent alloc] initWithDevice:dev
                                                                   momentumScale:_optimizerOptions.sgdMomentum
                                                              useNestrovMomentum:NO
                                                             optimizerDescriptor:desc];
  } else {
    _optimizer = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                                      beta1:_optimizerOptions.adamBeta1
                                                      beta2:_optimizerOptions.adamBeta2
                                                    epsilon:_optimizerOptions.adamEpsilon
                                                   timeStep:0
                                        optimizerDescriptor:desc];
  }

  /*
    A note on how the batch norm update works.

    What we want is to perform:

        value(t+1) = mu * value(t) + (1 - mu) * statistic(t)

    Value is the batch norm statistics (global mean or variance), mu is the
    momentum of the update, and statistic is either the mean or variance of the
    current batch.

    We use an SGD optimizer without moment and L2 weight decay, which performs:

        value(t+1) = value(t) - learningRate * (gradient(t) + value(t) * regularizationScale)

    Solving this gives:

        learningRate = -(1 - mu)
        regularizationScale = -1
        gradient(t) = statistic(t)
   */

  MPSNNOptimizerDescriptor *bnRunningOptDesc = [MPSNNOptimizerDescriptor
                                    optimizerDescriptorWithLearningRate:-(1 - bnRunningUpdateMomentum)
                                    gradientRescale:1.0f
                                    regularizationType:MPSNNRegularizationTypeL2
                                    regularizationScale:-1.0f];

  mov_avg_updater = [[MPSNNOptimizerStochasticGradientDescent alloc] initWithDevice:dev momentumScale:0.0 useNestrovMomentum:NO optimizerDescriptor:bnRunningOptDesc];
  mov_var_updater = [[MPSNNOptimizerStochasticGradientDescent alloc] initWithDevice:dev momentumScale:0.0 useNestrovMomentum:NO optimizerDescriptor:bnRunningOptDesc];

  MTLResourceOptions storageMode = MTLResourceStorageModeManaged;

  gammaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                         length:sizeof(float) * channels
                                        options:storageMode];

  gammaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                         length:sizeof(float) * channels
                                        options:storageMode];

  if (g_ptr != NULL) {

    gammaBuffer = [dev newBufferWithBytes:g_ptr
                                 length:sizeof(float) * channels
                                options:storageMode];
  } else {
    gammaBuffer = [dev newBufferWithBytes:ones_ptr
                                   length:sizeof(float) * channels
                                  options:storageMode];
  }

  betaMomentumBuffer = [dev newBufferWithBytes:zeros_ptr
                                        length:sizeof(float) * channels
                                       options:storageMode];

  betaVelocityBuffer = [dev newBufferWithBytes:zeros_ptr
                                        length:sizeof(float) * channels
                                       options:storageMode];
  if (b_ptr != NULL) {
    betaBuffer = [dev newBufferWithBytes:b_ptr
                                length:sizeof(float) * channels
                               options:storageMode];
  } else {
    betaBuffer = [dev newBufferWithBytes:zeros_ptr
                                  length:sizeof(float) * channels
                                 options:storageMode];
  }

  if (ma_ptr != NULL) {
    movingMeanBuffer = [dev newBufferWithBytes:ma_ptr
                                        length:sizeof(float) * channels
                                       options:storageMode];
  } else {
    movingMeanBuffer = [dev newBufferWithBytes:zeros_ptr
                       length:sizeof(float) * channels
                      options:storageMode];
  }

  if (mv_ptr != NULL) {
    movingVarianceBuffer = [dev newBufferWithBytes:mv_ptr
                                        length:sizeof(float) * channels
                                       options:storageMode];
  } else {
    movingVarianceBuffer = [dev newBufferWithBytes:ones_ptr
                                        length:sizeof(float) * channels
                                       options:storageMode];
  }



  _gammaBetaState =
      [[MPSCNNNormalizationGammaAndBetaState alloc] initWithGamma:gammaBuffer
                                                             beta:betaBuffer];
  _meanVarianceState =
      [[MPSCNNNormalizationMeanAndVarianceState alloc] initWithMean:movingMeanBuffer
                                                           variance:movingVarianceBuffer];

  _betaMomentumPointer = (float *)betaMomentumBuffer.contents;
  _betaVelocityPointer = (float *)betaVelocityBuffer.contents;
  _betaPointer = (float *)betaBuffer.contents;
  _gammaMomentumPointer = (float *)gammaMomentumBuffer.contents;
  _gammaVelocityPointer = (float *)gammaVelocityBuffer.contents;
  _gammaPointer = (float *)gammaBuffer.contents;
  _movingMeanPointer = (float *)movingMeanBuffer.contents;
  _movingVariancePointer = (float *)movingVarianceBuffer.contents;

  vDesc = [MPSVectorDescriptor vectorDescriptorWithLength:channels
                                                 dataType:(MPSDataTypeFloat32)];

  _gammaMomentumVector =
      [[MPSVector alloc] initWithBuffer:gammaMomentumBuffer descriptor:vDesc];

  _gammaVelocityVector =
      [[MPSVector alloc] initWithBuffer:gammaVelocityBuffer descriptor:vDesc];

  _betaMomentumVector =
      [[MPSVector alloc] initWithBuffer:betaMomentumBuffer descriptor:vDesc];

  _betaVelocityVector =
      [[MPSVector alloc] initWithBuffer:betaVelocityBuffer descriptor:vDesc];

  _movingMeanVector = [[MPSVector alloc] initWithBuffer:movingMeanBuffer descriptor:vDesc];
  _movingVarianceVector = [[MPSVector alloc] initWithBuffer:movingVarianceBuffer descriptor:vDesc];


  _label = [NSString stringWithCString:kernelParamsBinaryName
                              encoding:[NSString defaultCStringEncoding]];

  return self;
}

// We don't yet trigger any copies of this data source, but real implementations
// here will be necessary to support training with additional GPUs
- (instancetype)copyWithZone:(nullable NSZone *)zone {
  assert(false && "NSCopying not implemented for TCMPSBatchNormWeights");
  return self;
}

- (instancetype)copyWithZone:(nullable NSZone *)zone device:(nullable id <MTLDevice>)device {
  assert(false && "NSCopying not implemented for TCMPSBatchNormWeights");
  return self;
}

- (void)set_cq:(id<MTLCommandQueue>)cmd_queue {
  cq = cmd_queue;
}

- (NSUInteger)numberOfFeatureChannels {
  return _channels;
}
- (float *__nonnull)gamma {
  return _gammaPointer;
}
- (float *__nullable)beta {
  return _betaPointer;
}
- (float *__nullable)mean {
  return _movingMeanPointer;
}
- (float *__nullable)variance {
  return _movingVariancePointer;
}
- (float)epsilon {
  return _batchNormEpsilon;
}

- (BOOL)load {
  [self checkpointWithCommandQueue:cq];
  return YES;
}

- (void)purge {
}

- (NSString *_Nullable)label {
  return _label;
}

- (void)setLearningRate:(float)lr {
  [_optimizer setLearningRate:lr];
  _optimizerOptions.learningRate = lr;
}

- (BOOL)updateGammaAndBetaWithBatchNormalizationState:
    (MPSCNNBatchNormalizationState *__nonnull)batchNormalizationState {

  float *betaGradient =
      (float *)(batchNormalizationState.gradientForBeta.contents);
  float *gammaGradient =
      (float *)(batchNormalizationState.gradientForGamma.contents);

  float *mean = (float *)(batchNormalizationState.mean.contents);
  float *variance = (float *)(batchNormalizationState.variance.contents);

  float beta1 = _optimizerOptions.adamBeta1;
  float beta2 = _optimizerOptions.adamBeta2;

  t++;
  float lr_t =
      _optimizerOptions.learningRate * sqrtf(1.0f - powf(beta2, t)) / (1.f - powf(beta1, t));

  for (NSUInteger i = 0; i < _channels; i++) {
    _betaMomentumPointer[i] =
        (beta1 * _betaMomentumPointer[i]) + ((1 - beta1) * betaGradient[i]);
    _betaVelocityPointer[i] = (beta2 * _betaVelocityPointer[i]) +
                              ((1 - beta2) * betaGradient[i] * betaGradient[i]);
    _betaPointer[i] -= lr_t * (_betaMomentumPointer[i] /
                               (sqrt(_betaVelocityPointer[i]) + _optimizerOptions.adamEpsilon));

    _gammaMomentumPointer[i] =
        (beta1 * _gammaMomentumPointer[i]) + ((1 - beta1) * gammaGradient[i]);
    _gammaVelocityPointer[i] =
        (beta2 * _gammaVelocityPointer[i]) +
        ((1 - beta2) * gammaGradient[i] * gammaGradient[i]);
    _gammaPointer[i] -= lr_t * (_gammaMomentumPointer[i] /
                                (sqrt(_gammaVelocityPointer[i]) + _optimizerOptions.adamEpsilon));

    // update moving mean and variance
    _movingMeanPointer[i] -= (_movingMeanPointer[i] - mean[i]) * (1 - beta2);
    _movingVariancePointer[i] -=
        (_movingVariancePointer[i] - variance[i]) * (1 - beta2);

    assert(_movingVariancePointer[i] >= 0);
  }
  return YES;
}

- (MPSCNNNormalizationGammaAndBetaState *__nullable)
updateGammaAndBetaWithCommandBuffer:(nonnull id<MTLCommandBuffer>)commandBuffer
            batchNormalizationState:(MPSCNNBatchNormalizationState *__nonnull)
                                        batchNormalizationState {

  // Update mean and variance.
  assert(batchNormalizationState.mean);
  MPSVector *meanVec= [[MPSVector alloc]
                       initWithBuffer:batchNormalizationState.mean
                                                    descriptor:vDesc];

  MPSVector *varVector = [[MPSVector alloc]
                          initWithBuffer:batchNormalizationState.variance
                                                descriptor:vDesc];

  [mov_avg_updater encodeToCommandBuffer:commandBuffer
                     inputGradientVector:meanVec
                       inputValuesVector:self.movingMeanVector
                     inputMomentumVector:nil
                      resultValuesVector:self.movingMeanVector];
  [mov_var_updater encodeToCommandBuffer:commandBuffer
                     inputGradientVector:varVector
                       inputValuesVector:self.movingVarianceVector
                     inputMomentumVector:nil
                      resultValuesVector:self.movingVarianceVector];

  // Update gamma and beta. This update must come last, since the MPS API we use
  // will decrement read counts.
  [_optimizer encodeToCommandBuffer:commandBuffer
            batchNormalizationState:batchNormalizationState
                      batchNormData:self];

  return self.gammaBetaState;
}

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue {

#if TARGET_OS_OSX
  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLBlitCommandEncoder> blit = commandBuffer.blitCommandEncoder;
  assert(self.gammaBetaState.beta);
  [blit synchronizeResource:betaMomentumBuffer];
  [blit synchronizeResource:betaVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(self.gammaBetaState.beta)];

  assert(self.gammaBetaState.gamma);
  [blit synchronizeResource:gammaMomentumBuffer];
  [blit synchronizeResource:gammaVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(self.gammaBetaState.gamma)];

  [blit synchronizeResource:movingMeanBuffer];
  [blit synchronizeResource:movingVarianceBuffer];

  [blit endEncoding];

  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];
#endif // TARGET_OS_OSX
}

- (void)loadBeta:(float * _Nullable)src {
    memcpy((void *)_betaPointer, (void *)src, _channels * sizeof(float));
    [betaBuffer didModifyRange:NSMakeRange(0, _channels * sizeof(float))];
}

- (void)loadGamma:(float *)src{
    memcpy((void *)_gammaPointer, (void *)src, _channels * sizeof(float));
    [gammaBuffer didModifyRange:NSMakeRange(0, _channels * sizeof(float))];
}

- (void)loadMovingAvg:(float * _Nullable)src {
    memcpy((void *)_movingMeanPointer, (void *)src, _channels * sizeof(float));
    [movingMeanBuffer didModifyRange:NSMakeRange(0, _channels * sizeof(float))];
}

- (void)loadMovingVar:(float *)src{
    memcpy((void *)_movingVariancePointer, (void *)src, _channels * sizeof(float));
    [movingVarianceBuffer didModifyRange:NSMakeRange(0, _channels * sizeof(float))];
}

@end  // TCMPSBatchNormWeights

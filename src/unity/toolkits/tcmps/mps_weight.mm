//
//  mps_mm.c
//  tcmps
//
//  Copyright © 2018 Turi. All rights reserved.
//

#include "mps_weight.h"

using turi::mps::OptimizerOptions;



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

  t = 0.f;

  MPSNNOptimizerDescriptor *desc = _optimizerOptions.mpsDescriptor();
  
  if (_optimizerOptions.useSGD) {
    sgdWeights = [[MPSNNOptimizerStochasticGradientDescent alloc] initWithDevice:dev
                                                                 momentumScale:_optimizerOptions.sgdMomentum useNestrovMomentum:NO optimizerDescriptor:desc];

    sgdBias = [[MPSNNOptimizerStochasticGradientDescent alloc] initWithDevice:dev
                                                                momentumScale:_optimizerOptions.sgdMomentum useNestrovMomentum:NO optimizerDescriptor:desc];
  } else {
    adamWeights = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                                       beta1:_optimizerOptions.adamBeta1
                                                       beta2:_optimizerOptions.adamBeta2
                                                     epsilon:_optimizerOptions.adamEpsilon
                                                    timeStep:0
                                         optimizerDescriptor:desc];

    adamBias = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
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

  convWtsAndBias = [[MPSCNNConvolutionWeightsAndBiasesState alloc]
      initWithWeights:weightBuffer
               biases:biasBuffer];

  vDescWeights = [MPSVectorDescriptor
      vectorDescriptorWithLength:_inputFeatureChannels * _kernelHeight *
                                 _kernelWidth * _outputFeatureChannels
                        dataType:(MPSDataTypeFloat32)];

  weightMomentumVector = [[MPSVector alloc] initWithBuffer:weightMomentumBuffer
                                                descriptor:vDescWeights];

  weightVelocityVector = [[MPSVector alloc] initWithBuffer:weightVelocityBuffer
                                                descriptor:vDescWeights];

  weightVector =
      [[MPSVector alloc] initWithBuffer:weightBuffer descriptor:vDescWeights];

  vDescBiases =
      [MPSVectorDescriptor vectorDescriptorWithLength:_outputFeatureChannels
                                             dataType:(MPSDataTypeFloat32)];

  biasMomentumVector = [[MPSVector alloc] initWithBuffer:biasMomentumBuffer
                                              descriptor:vDescBiases];

  biasVelocityVector = [[MPSVector alloc] initWithBuffer:biasVelocityBuffer
                                              descriptor:vDescBiases];

  biasVector =
      [[MPSVector alloc] initWithBuffer:biasBuffer descriptor:vDescBiases];

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
  if (_optimizerOptions.useSGD) {
    [sgdBias setLearningRate:lr];
    [sgdWeights setLearningRate:lr];
  } else {
    [adamBias setLearningRate:lr];
    [adamWeights setLearningRate:lr];
  }
  _optimizerOptions.learningRate = lr;
}

- (size_t)weight_size{
    return sizeWeights / sizeof(float);
}
- (size_t)bias_size{
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

  MPSCNNConvolutionWeightsAndBiasesState *__nonnull sourceState =
      convWtsAndBias;

  return [self updateWithCommandBuffer:commandBuffer
                         gradientState:gradientState
                           sourceState:sourceState];
}

- (MPSCNNConvolutionWeightsAndBiasesState *__nullable)
updateWithCommandBuffer:(__nonnull id<MTLCommandBuffer>)commandBuffer
          gradientState:(MPSCNNConvolutionGradientState *__nonnull)gradientState
            sourceState:
                (MPSCNNConvolutionWeightsAndBiasesState *__nonnull)sourceState {

  t++;

  MPSVector *gradientWeightsVector =
      [[MPSVector alloc] initWithBuffer:gradientState.gradientForWeights
                             descriptor:vDescWeights];

  if (_optimizerOptions.useSGD) {
    [sgdWeights encodeToCommandBuffer:commandBuffer
                  inputGradientVector:gradientWeightsVector
                    inputValuesVector:weightVector
                  inputMomentumVector:weightMomentumVector
                   resultValuesVector:weightVector];
  } else {
    [adamWeights encodeToCommandBuffer:commandBuffer
                 inputGradientVector:gradientWeightsVector
                   inputValuesVector:weightVector
                 inputMomentumVector:weightMomentumVector
                 inputVelocityVector:weightVelocityVector
                  resultValuesVector:weightVector];
  }

  MPSVector *gradientBiasesVector =
      [[MPSVector alloc] initWithBuffer:gradientState.gradientForBiases
                             descriptor:vDescBiases];

  assert(sourceState.biases);

  if (_optimizerOptions.useSGD) {
    [sgdBias encodeToCommandBuffer:commandBuffer
               inputGradientVector:gradientBiasesVector
                 inputValuesVector:biasVector
               inputMomentumVector:biasMomentumVector
                resultValuesVector:biasVector];
  } else {
    [adamBias encodeToCommandBuffer:commandBuffer
              inputGradientVector:gradientBiasesVector
                inputValuesVector:biasVector
              inputMomentumVector:biasMomentumVector
              inputVelocityVector:biasVelocityVector
               resultValuesVector:biasVector];
  }

  // TODO: Adopt the convenience MPSNNOptimizer API that handles more of these
  // fiddly details.
  if (gradientState.isTemporary) {
    gradientState.readCount -= 1;
  }
  if (sourceState.isTemporary) {
    sourceState.readCount -= 1;
  }

  return convWtsAndBias;
}

- (void)set_cq:(id<MTLCommandQueue>)cmd_queue {
  cq = cmd_queue;
}

- (void)checkpoint {
}

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue {

  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLBlitCommandEncoder> blit = commandBuffer.blitCommandEncoder;
  assert(convWtsAndBias.weights);
  [blit synchronizeResource:weightMomentumBuffer];
  [blit synchronizeResource:weightVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(convWtsAndBias.weights)];

  assert(convWtsAndBias.biases);
  [blit synchronizeResource:biasMomentumBuffer];
  [blit synchronizeResource:biasVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(convWtsAndBias.biases)];

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
  [weightBuffer didModifyRange:NSMakeRange(0, sizeBias)];
}

- (NSString *_Nullable)label {
  return [NSString stringWithUTF8String:_kernelParamsBinaryName.c_str()];
}

- (void)dealloc {
}
@end  // TCMPSConvolutionWeights

@implementation TCMPSBatchNormData

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
    sgdGamma = [[MPSNNOptimizerStochasticGradientDescent alloc] initWithDevice:dev
                                                                 momentumScale:_optimizerOptions.sgdMomentum useNestrovMomentum:NO optimizerDescriptor:desc];
    sgdBeta = [[MPSNNOptimizerStochasticGradientDescent alloc] initWithDevice:dev
                                                                momentumScale:_optimizerOptions.sgdMomentum useNestrovMomentum:NO optimizerDescriptor:desc];

  } else {
    adamGamma = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
                                                     beta1:_optimizerOptions.adamBeta1
                                                     beta2:_optimizerOptions.adamBeta2
                                                   epsilon:_optimizerOptions.adamEpsilon
                                                  timeStep:0
                                       optimizerDescriptor:desc];

    adamBeta = [[MPSNNOptimizerAdam alloc] initWithDevice:dev
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



  gammaBetaState =
      [[MPSCNNNormalizationGammaAndBetaState alloc] initWithGamma:gammaBuffer
                                                             beta:betaBuffer];
  meanVarianceState =
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

  gammaMomentumVector =
      [[MPSVector alloc] initWithBuffer:gammaMomentumBuffer descriptor:vDesc];

  gammaVelocityVector =
      [[MPSVector alloc] initWithBuffer:gammaVelocityBuffer descriptor:vDesc];

  gammaVector = [[MPSVector alloc] initWithBuffer:gammaBuffer descriptor:vDesc];

  betaMomentumVector =
      [[MPSVector alloc] initWithBuffer:betaMomentumBuffer descriptor:vDesc];

  betaVelocityVector =
      [[MPSVector alloc] initWithBuffer:betaVelocityBuffer descriptor:vDesc];

  betaVector = [[MPSVector alloc] initWithBuffer:betaBuffer descriptor:vDesc];

  movingMeanVector = [[MPSVector alloc] initWithBuffer:movingMeanBuffer descriptor:vDesc];
  movingVarianceVector = [[MPSVector alloc] initWithBuffer:movingVarianceBuffer descriptor:vDesc];


  _label = [NSString stringWithCString:kernelParamsBinaryName
                              encoding:[NSString defaultCStringEncoding]];

  return self;
}

// We don't yet trigger any copies of this data source, but real implementations
// here will be necessary to support training with additional GPUs
- (instancetype)copyWithZone:(nullable NSZone *)zone {
  assert(false && "NSCopying not implemented for TCMPSBatchNormData");
  return self;
}

- (instancetype)copyWithZone:(nullable NSZone *)zone device:(nullable id <MTLDevice>)device {
  assert(false && "NSCopying not implemented for TCMPSBatchNormData");
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
  if (_optimizerOptions.useSGD) {
    [sgdGamma setLearningRate:lr];
    [sgdBeta setLearningRate:lr];
  } else {
    [adamGamma setLearningRate:lr];
    [adamBeta setLearningRate:lr];
  }
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

  t++;

  assert(batchNormalizationState.gradientForGamma);
  MPSVector *gradientWeightsVector = [[MPSVector alloc]
      initWithBuffer:nonnull_cast(batchNormalizationState.gradientForGamma)
          descriptor:vDesc];

  assert(batchNormalizationState.gamma);

  if (_optimizerOptions.useSGD) {
    [sgdGamma encodeToCommandBuffer:commandBuffer
                inputGradientVector:gradientWeightsVector
                  inputValuesVector:gammaVector
                inputMomentumVector:gammaMomentumVector
                 resultValuesVector:gammaVector];
  } else {

    [adamGamma encodeToCommandBuffer:commandBuffer
               inputGradientVector:gradientWeightsVector
                 inputValuesVector:gammaVector
               inputMomentumVector:gammaMomentumVector
               inputVelocityVector:gammaVelocityVector
                resultValuesVector:gammaVector];
  }


  assert(batchNormalizationState.gradientForBeta);
  MPSVector *gradientBiasesVector = [[MPSVector alloc]
      initWithBuffer:nonnull_cast(batchNormalizationState.gradientForBeta)
          descriptor:vDesc];

  assert(batchNormalizationState.beta);

  if (_optimizerOptions.useSGD) {
    [sgdBeta encodeToCommandBuffer:commandBuffer
               inputGradientVector:gradientBiasesVector
                 inputValuesVector:betaVector
               inputMomentumVector:betaMomentumVector
                resultValuesVector:betaVector];

  } else {
    [adamBeta encodeToCommandBuffer:commandBuffer
              inputGradientVector:gradientBiasesVector
                inputValuesVector:betaVector
              inputMomentumVector:betaMomentumVector
              inputVelocityVector:betaVelocityVector
               resultValuesVector:betaVector];
  }
  
  assert(batchNormalizationState.mean);
  MPSVector *meanVec= [[MPSVector alloc]
                       initWithBuffer:batchNormalizationState.mean
                                                    descriptor:vDesc];

  MPSVector *varVector = [[MPSVector alloc]
                          initWithBuffer:batchNormalizationState.variance
                                                descriptor:vDesc];

  [mov_avg_updater encodeToCommandBuffer:commandBuffer
                     inputGradientVector:meanVec
                       inputValuesVector:movingMeanVector
                     inputMomentumVector:nil
                      resultValuesVector:movingMeanVector];
  [mov_var_updater encodeToCommandBuffer:commandBuffer
                     inputGradientVector:varVector
                       inputValuesVector:movingVarianceVector
                     inputMomentumVector:nil
                      resultValuesVector:movingVarianceVector];
  return gammaBetaState;
}

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue {

#if TARGET_OS_OSX
  id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  id<MTLBlitCommandEncoder> blit = commandBuffer.blitCommandEncoder;
  assert(gammaBetaState.beta);
  [blit synchronizeResource:betaMomentumBuffer];
  [blit synchronizeResource:betaVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(gammaBetaState.beta)];

  assert(gammaBetaState.gamma);
  [blit synchronizeResource:gammaMomentumBuffer];
  [blit synchronizeResource:gammaVelocityBuffer];
  [blit synchronizeResource:nonnull_cast(gammaBetaState.gamma)];

  [blit synchronizeResource:movingMeanBuffer];
  [blit synchronizeResource:movingVarianceBuffer];

  [blit endEncoding];

  [commandBuffer commit];
  [commandBuffer waitUntilCompleted];
#endif // TARGET_OS_OSX
}

- (void)dealloc {
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

@end  // TCMPSBatchNormData

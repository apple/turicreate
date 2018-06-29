//
//  mps_mm.h
//  tcmps
//
//  Copyright © 2018 Turi. All rights reserved.
//

#ifndef MPS_WEIGHT_H_
#define MPS_WEIGHT_H_

#define ADVANCE_PTR(_a, _size)                                                 \
  (__typeof__(_a))((uintptr_t)(_a) + (size_t)(_size))

#if DEBUG
#define nonnull_cast(_a)                                                       \
  ({                                                                           \
    __typeof__(*(_a)) *__nullable a = (__typeof__(*(a)) *__nullable)(_a);      \
    assert((a) != NULL);                                                       \
    /*return*/ ((__typeof__(*(a)) *__nonnull)(a));                             \
  })
#else
#define nonnull_cast(_a)                                                       \
  ({                                                                           \
    __typeof__(*(_a)) *__nullable a = (__typeof__(*(a)) *__nullable)(_a);      \
    /*return*/ ((__typeof__(*(a)) *__nonnull)(a));                             \
  })
#endif

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <vector>
#import "mps_utils.h"

API_AVAILABLE(macos(10.14))
@interface TCMPSConvolutionWeights : NSObject <MPSCNNConvolutionDataSource> {
@private
  NSUInteger _outputFeatureChannels;
  NSUInteger _inputFeatureChannels;
  NSUInteger _kernelHeight;
  NSUInteger _kernelWidth;
  MPSCNNConvolutionDescriptor *_convDesc;
  std::string _kernelParamsBinaryName;

  size_t sizeBias, sizeWeights;
  unsigned _seed;
  turi::mps::OptimizerOptions _optimizerOptions;
  float t;

  id<MTLBuffer> weightMomentumBuffer, biasMomentumBuffer, weightVelocityBuffer,
      biasVelocityBuffer, weightBuffer, biasBuffer;
  MPSVector *weightMomentumVector, *biasMomentumVector, *weightVelocityVector,
      *biasVelocityVector, *weightVector, *biasVector;

  MPSNNOptimizerAdam *adamWeights, *adamBias;
  MPSNNOptimizerStochasticGradientDescent *sgdWeights, *sgdBias;

  MPSVectorDescriptor *vDescWeights;
  MPSVectorDescriptor *vDescBiases;

  id<MTLCommandQueue> cq;

@public
  MPSCNNConvolutionWeightsAndBiasesState *convWtsAndBias;
}
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
                           optimizerOptions:(turi::mps::OptimizerOptions)optimizerOptions;

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
                           optimizerOptions:(turi::mps::OptimizerOptions)optimizerOptions;

- (MPSDataType)dataType;
- (MPSCNNConvolutionDescriptor *__nonnull)descriptor;
- (void *__nonnull)weights;
- (float *__nullable)biasTerms;
- (size_t)weight_size;
- (size_t)bias_size;
- (void)loadWeight:(float *__nullable)src;
- (void)loadBias:(float *__nullable)src;
- (BOOL)load;
- (void)purge;
- (void)setLearningRate:(float)lr;
- (MPSCNNConvolutionWeightsAndBiasesState *__nullable)
updateWithCommandBuffer:(__nonnull id<MTLCommandBuffer>)commandBuffer
          gradientState:
              (MPSCNNConvolutionGradientState *__nonnull)gradientState;
- (MPSCNNConvolutionWeightsAndBiasesState *__nullable)
updateWithCommandBuffer:(__nonnull id<MTLCommandBuffer>)commandBuffer
          gradientState:(MPSCNNConvolutionGradientState *__nonnull)gradientState
            sourceState:
                (MPSCNNConvolutionWeightsAndBiasesState *__nonnull)sourceState;
- (void)checkpoint;
- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue;

@end  // TCMPSConvolutionWeights

API_AVAILABLE(macos(10.14))
@interface TCMPSBatchNormData : NSObject <MPSCNNBatchNormalizationDataSource> {
@private
  NSUInteger _channels;
  float *_betaPointer, *_gammaPointer, *_betaMomentumPointer,
      *_betaVelocityPointer, *_gammaVelocityPointer, *_gammaMomentumPointer,
      *_movingVariancePointer, *_movingMeanPointer;
  turi::mps::OptimizerOptions _optimizerOptions;
  float t;
  float _batchNormEpsilon;

  std::string _kernelParamsBinaryName;

  MPSVectorDescriptor *vDesc;
  MPSNNOptimizerAdam *adamGamma, *adamBeta;
  MPSNNOptimizerStochasticGradientDescent *sgdGamma, *sgdBeta, *mov_avg_updater, *mov_var_updater;
  id<MTLDevice> dev;
  id<MTLCommandQueue> cq;
  bool use_sgd_opt;

  id<MTLBuffer> gammaMomentumBuffer, betaMomentumBuffer, gammaVelocityBuffer,
      betaVelocityBuffer, gammaBuffer, betaBuffer, movingVarianceBuffer,
      movingMeanBuffer;
  MPSVector *gammaMomentumVector, *betaMomentumVector, *gammaVelocityVector,
      *betaVelocityVector, *gammaVector, *betaVector, *movingVarianceVector,
      *movingMeanVector;

  NSString *_label;
@public
  MPSCNNNormalizationGammaAndBetaState *gammaBetaState;
  MPSCNNNormalizationMeanAndVarianceState *meanVarianceState;
}

@property(readwrite, retain, nonatomic, nonnull) NSString *internalLabel;

- (nonnull instancetype)initWithChannels:(NSUInteger)channels
                  kernelParamsBinaryName:
                      (const char *__nonnull)kernelParamsBinaryName
                                  device:(id<MTLDevice> _Nonnull ) dev
                               cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q
                               gamma:(float *__nullable)g_ptr
                                    beta:(float *__nullable)b_ptr
                              moving_avg:(float *__nullable)ma_ptr
                              moving_var:(float *__nullable)mv_ptr
                        optimizerOptions:(turi::mps::OptimizerOptions)optimizerOptions
                        batchNormEpsilon:(float)batchNormEpsilon;

// MPSCNNBatchNormalizationDataSource interface methods
- (float *__nonnull)gamma;
- (float *__nullable)beta;
- (float *__nullable)mean;
- (float *__nullable)variance;
- (float)epsilon;
- (void)setLearningRate:(float)lr;
- (BOOL)load;
- (void)purge;

- (void)loadGamma:(float *__nullable)src;
- (void)loadBeta:(float *__nullable)src;
- (void)loadMovingAvg:(float *__nullable)src;
- (void)loadMovingVar:(float *__nullable)src;
- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue;
- (MPSCNNNormalizationGammaAndBetaState *__nullable) updateGammaAndBetaWithCommandBuffer:(nonnull id<MTLCommandBuffer>)commandBuffer
                                                                 batchNormalizationState:(MPSCNNBatchNormalizationState *__nonnull)batchNormalizationState;
- (BOOL)updateGammaAndBetaWithBatchNormalizationState:
    (MPSCNNBatchNormalizationState *__nonnull)batchNormalizationState;

@end  // TCMPSBatchNormData


#endif /* MPS_WEIGHT_H_ */

#ifndef convolution_data_loader_h
#define convolution_data_loader_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <unity/toolkits/neural_net/mps_weight.h>

API_AVAILABLE(macos(10.14))
@interface ConvolutionDataLoader : NSObject <MPSCNNConvolutionDataSource> {
  NSString *mName;

  int mKernelWidth;
  int mKernelHeight;

  int mInputFeatureChannels;
  int mOutputFeatureChannels;

  int mStrideWidth;
  int mStrideHeight;

  id<MTLBuffer> mWeights;
  id<MTLBuffer> mBiases;

  id<MTLBuffer> mWeightMomentumBuffer;
  MPSVector *mWeightMomentumVector;

  id<MTLBuffer> mBiasMomentumBuffer;
  MPSVector *mBiasMomentumVector;

  id<MTLBuffer> mWeightVelocityBuffer;
  MPSVector *mWeightVelocityVector;

  id<MTLBuffer> mBiasVelocityBuffer;
  MPSVector *mBiasVelocityVector;

  MPSCNNConvolutionWeightsAndBiasesState *mState;

  MPSNNOptimizerAdam *mOptimizer;

  id<MTLDevice> mDev;
  id<MTLCommandQueue> mCq;
}

- (id _Nonnull)initWithParams:(NSString *_Nullable)name
                  kernelWidth:(int)kernelWidth
                 kernelHeight:(int)kernelHeight
         inputFeatureChannels:(int)inputFeatureChannels
        outputFeatureChannels:(int)outputFeatureChannels
                  strideWidth:(int)strideWidth
                 strideHeight:(int)strideHeight
                      weights:(float *_Nonnull)weights
                       biases:(float *_Nonnull)biases
                       device:(id<MTLDevice> _Nonnull)dev
                    cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q;

- (MPSDataType)dataType;
- (MPSCNNConvolutionDescriptor *__nonnull)descriptor;

- (MPSCNNConvolutionWeightsAndBiasesState *_Nonnull)
    updateWithCommandBuffer:(id<MTLCommandBuffer> _Nonnull)commandBuffer
              gradientState:
                  (MPSCNNConvolutionGradientState *_Nonnull)gradientState
                sourceState:(MPSCNNConvolutionWeightsAndBiasesState *_Nonnull)
                                sourceState;

- (size_t)weight_size;
- (void)loadWeight:(float *__nullable)src;
- (void *__nonnull)weights;

- (size_t)bias_size;
- (void)loadBias:(float *__nullable)src;
- (float *__nullable)biasTerms;

- (BOOL)load;
- (void)purge;

- (NSString *__nullable)label;
- (id _Nonnull)copyWithZone:(nullable NSZone *)zone;

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue;

@end

#endif

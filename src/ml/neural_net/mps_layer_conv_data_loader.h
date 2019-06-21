#ifndef mps_layer_conv_data_loader_h
#define mps_layer_conv_data_loader_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>


#define nonnull_cast(_a)                                                       \
  ({                                                                           \
    __typeof__(*(_a)) *__nullable a = (__typeof__(*(a)) *__nullable)(_a);      \
    assert((a) != NULL);                                                       \
    /*return*/ ((__typeof__(*(a)) *__nonnull)(a));                             \
  })

API_AVAILABLE(macos(10.14))
@interface ConvolutionDataLoader: NSObject <MPSCNNConvolutionDataSource> {
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

  bool mUpdate;

  id<MTLDevice> mDev;
  id<MTLCommandQueue> mCq;
}

- (id) initWithParams:(NSString * _Nonnull)name
          kernelWidth:(int)kernelWidth
         kernelHeight:(int)kernelHeight
 inputFeatureChannels:(int)inputFeatureChannels
outputFeatureChannels:(int)outputFeatureChannels
          strideWidth:(int)strideWidth
         strideHeight:(int)strideHeight
              weights:(float * _Nonnull)weights
               biases:(float * _Nonnull)biases
        updateWeights:(bool)updateWeights
               device:(id<MTLDevice> _Nonnull)dev
            cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q;

- (MPSDataType) dataType;
- (MPSCNNConvolutionDescriptor *__nonnull) descriptor;

- (MPSCNNConvolutionWeightsAndBiasesState *)updateWithCommandBuffer:(id<MTLCommandBuffer>)commandBuffer 
                                                      gradientState:(MPSCNNConvolutionGradientState *)gradientState 
                                                        sourceState:(MPSCNNConvolutionWeightsAndBiasesState *)sourceState;

- (size_t) weight_size;
- (void) loadWeight:(float *__nullable) src;
- (void *__nonnull) weights;

- (size_t) bias_size;
- (void) loadBias:(float *__nullable) src;
- (float *__nullable) biasTerms;

- (BOOL) load;
- (void) purge;

- (NSString*__nullable) label;
- (id) copyWithZone:(nullable NSZone *) zone;

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>) commandQueue;

@end 

#endif
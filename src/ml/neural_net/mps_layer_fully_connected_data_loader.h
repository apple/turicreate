#ifndef mps_layer_fully_connected_data_loader_h
#define mps_layer_fully_connected_data_loader_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <ml/neural_net/mps_layer_conv_data_loader.h>

API_AVAILABLE(macos(10.14))
@interface FullyConnectedDataLoader: NSObject <MPSCNNConvolutionDataSource> {
  NSString *mName;
    
  int mKernelWidth;
  int mKernelHeight;
    
  id<MTLBuffer> mWeights;
  id<MTLBuffer> mBiases;
    
  int mInputFeatureChannels;
  int mOutputFeatureChannels;
    
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

- (id _Nonnull) initWithParams:(NSString * _Nonnull)name
          inputFeatureChannels:(int)inputFeatureChannels
         outputFeatureChannels:(int)outputFeatureChannels
                   inputHeight:(int)inputHeight
                    inputWidth:(int)inputWidth
                       weights:(float * _Nonnull)weights
                        biases:(float * _Nonnull)biases
                 updateWeights:(bool)updateWeights
                        device:(id<MTLDevice> _Nonnull)dev
                     cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q;

- (MPSDataType) dataType;
- (MPSCNNConvolutionDescriptor *__nonnull) descriptor;
- (MPSCNNConvolutionWeightsAndBiasesState * _Nullable)updateWithCommandBuffer:(id<MTLCommandBuffer> _Nonnull)commandBuffer
                                                                gradientState:(MPSCNNConvolutionGradientState * _Nullable)gradientState
                                                                  sourceState:(MPSCNNConvolutionWeightsAndBiasesState * _Nullable)sourceState;

- (size_t) weight_size;
- (void) loadWeight:(float *__nullable) src;
- (void *__nonnull) weights;

- (size_t) bias_size;
- (void) loadBias:(float *__nullable) src;
- (float *__nullable) biasTerms;

- (BOOL) load;
- (void) purge;

- (NSString*__nullable) label;
- (id _Nonnull) copyWithZone:(nullable NSZone *) zone;

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>) commandQueue;

@end

#endif

#ifndef mps_layer_instance_norm_data_loader_h
#define mps_layer_instance_norm_data_loader_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

typedef struct{
  int size;
  float beta1;
  float beta2;
  float epsilon;
  float lr_t;
} params_t;

API_AVAILABLE(macos(10.14))
@interface InstanceNormDataLoader: NSObject <MPSCNNInstanceNormalizationDataSource> {
  NSString *mName;
  NSUInteger mNumberOfFeatureChannels;

  int mStyles;  
  int mCurrentStyle;  

  float **mGamma;
  float **mBeta;

  float beta1;
  float beta2;
  float epsilon;
  float learning_rate;

  id<MTLBuffer>mGammaBuffer;
  id<MTLBuffer>mBetaBuffer;

  id<MTLBuffer> mGammaMomentumBuffer;
  id<MTLBuffer> mGammaVelocityBuffer;
  id<MTLBuffer> mBetaMomentumBuffer;
  id<MTLBuffer> mBetaVelocityBuffer;
  id<MTLBuffer> mMovingMeanBuffer;
  id<MTLBuffer> mMovingVarianceBuffer;

  MPSCNNNormalizationMeanAndVarianceState *mMeanVarianceState;

  MPSVector *mGammaVector;
  MPSVector *mBetaVector;
  MPSVector *mGammaMomentumVector;
  MPSVector *mGammaVelocityVector;
  MPSVector *mBetaMomentumVector;
  MPSVector *mBetaVelocityVector;

  MPSVectorDescriptor *vDesc;

  MPSCNNNormalizationGammaAndBetaState *mState;

  MPSNNOptimizerAdam *adamGamma;
  MPSNNOptimizerAdam *adamBeta;

  id<MTLComputePipelineState> runningUpdatePipelineState;

  id<MTLCommandQueue> mCq;
}

- (id) initWithParams:(NSString *)name
     gammaWeights:(float **)gammaWeights
      betaWeights:(float **)betaWeights
numberFeatureChannels:(int)numberFeatureChannels
         styles:(int)styles
         device:(id<MTLDevice> _Nonnull)dev
      cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q;

- (void) updateNumberOfStyles:(int)styles;
- (void) updateCurrentStyle:(int)style;
- (int) getCurrentStyle;

- (void) loadBeta:(float **)beta;
- (float *) beta;

- (void) loadGamma:(float **)gamma;
- (float *) gamma;

- (MPSCNNNormalizationGammaAndBetaState *)updateGammaAndBetaWithCommandBuffer:(id<MTLCommandBuffer>)commandBuffer 
                                              instanceNormalizationStateBatch:(MPSCNNInstanceNormalizationGradientStateBatch *)instanceNormalizationStateBatch;

- (void)checkpointWithCommandQueue:(nonnull id<MTLCommandQueue>)commandQueue;

- (NSString*__nullable) label;
- (id) copyWithZone:(nullable NSZone *) zone;

@end

#endif

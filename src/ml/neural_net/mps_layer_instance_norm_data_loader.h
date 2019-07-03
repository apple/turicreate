#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

NS_ASSUME_NONNULL_BEGIN

API_AVAILABLE(macos(10.14))
@interface InstanceNormDataLoader: NSObject <MPSCNNInstanceNormalizationDataSource>

@property (nonatomic) float **gamma_weights;
@property (nonatomic) float **beta_weights;

@property (nonatomic) NSString *name;
@property (nonatomic) NSUInteger numberOfFeatureChannels;
@property (nonatomic) NSUInteger styles;
@property (nonatomic) NSUInteger currentStyle;

@property (nonatomic) id<MTLBuffer> gammaBuffer;
@property (nonatomic) id<MTLBuffer> betaBuffer;

@property (nonatomic) MPSVector *gammaVector;
@property (nonatomic) MPSVector *betaVector;
@property (nonatomic) MPSVector *gammaMomentumVector;
@property (nonatomic) MPSVector *gammaVelocityVector;
@property (nonatomic) MPSVector *betaMomentumVector;
@property (nonatomic) MPSVector *betaVelocityVector;

@property (nonatomic) id<MTLBuffer> gammaMomentumBuffer;
@property (nonatomic) id<MTLBuffer> gammaVelocityBuffer;
@property (nonatomic) id<MTLBuffer> betaMomentumBuffer;
@property (nonatomic) id<MTLBuffer> betaVelocityBuffer;
@property (nonatomic) id<MTLBuffer> movingMeanBuffer;
@property (nonatomic) id<MTLBuffer> movingVarianceBuffer;

@property (nonatomic) MPSVectorDescriptor *vDesc;
@property (nonatomic) MPSCNNNormalizationGammaAndBetaState *state;
@property (nonatomic) id<MTLCommandQueue> cq;

@property (nonatomic) MPSNNOptimizerAdam *adamGamma;
@property (nonatomic) MPSNNOptimizerAdam *adamBeta;

- (instancetype) initWithParams:(NSString *)name
                   gammaWeights:(float **)gammaWeights
                    betaWeights:(float **)betaWeights
          numberFeatureChannels:(NSUInteger)numberFeatureChannels
                         styles:(NSUInteger)styles
                         device:(id<MTLDevice> _Nonnull)dev
                      cmd_queue:(id<MTLCommandQueue> _Nonnull) cmd_q;

- (void) updateNumberOfStyles:(NSUInteger)styles;
- (void) updateCurrentStyle:(NSUInteger)style;
- (NSUInteger) getCurrentStyle;

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

NS_ASSUME_NONNULL_END

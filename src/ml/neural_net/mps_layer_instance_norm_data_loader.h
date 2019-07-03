#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

NS_ASSUME_NONNULL_BEGIN

/**
 * TCMPSInstanceNormDataLoader
 *
 * SUMMARY
 * -------
 * This class override is used by the MPS api to populate the
 * MPSCNNInstanceNormalizationNode node with data. There are a few key methods
 * on this class that the MPS api calls. These are listed below
 *
 * KEY METHODS 
 * -----------
 *
 * - beta, gamma
 *        These methods return the current values for beta and gamma in the
 *        instanceNorm layer. These methods both call checkpointWithCommandQueue
 *        to synchronize the GPU and the CPU weights before returning their
 *        respective values.
 *
 * - checkpointWithCommandQueue
 *        Given a command_queue on a Metal Device we can synchronize resources
 *        from that command_queue. For beta and gamma those might be their
 *        respective values on the GPU, but for training we would also need
 *        momentum and velocity vectors. We therefore synchronize those as well.
 *
 * - updateGammaAndBetaWithCommandBuffer
 *        MPS calls this method and we need to update the gamma and the beta
 *        weights ourselves. To do this we have an adams optimizer for both the
 *        gamma and the beta values. For each value in the batch we don't want
 *        to increment the adam optimizer;s time step as the learning rate would
 *        be affected. So we store the timestep at the beginning of each batch
 *        and use it across the batch. This `Hack` makes the behavior of the
 *        weight updates, very similar to the behavior we see in MxNet.
 */
API_AVAILABLE(macos(10.14))
@interface TCMPSInstanceNormDataLoader: NSObject <MPSCNNInstanceNormalizationDataSource>

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

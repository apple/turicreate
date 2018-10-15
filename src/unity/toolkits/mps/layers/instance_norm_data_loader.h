#ifndef turi_mps_instance_norm_data_loader_h
#define turi_mps_instance_norm_data_loader_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

API_AVAILABLE(macos(10.13))
@interface InstanceNormDataLoader: NSObject <MPSCNNInstanceNormalizationDataSource> {
    NSString *mName;
    NSUInteger mNumberOfFeatureChannels;

    int mStyles;    
    int mCurrentStyle;  

    float **mGamma;
    float **mBeta;
}

- (id __nonnull) initWithParams:(NSString *__nonnull)name
                   gammaWeights:(float *__nonnull*__nonnull)gammaWeights
                    betaWeights:(float *__nonnull*__nonnull)betaWeights
          numberFeatureChannels:(int)numberFeatureChannels
                         styles:(int)styles;

- (void) updateNumberOfStyles:(int) styles;
- (void) updateCurrentStyle:(int) style;
- (int) getCurrentStyle;

- (void) loadBeta:(float *__nonnull*__nonnull) beta;
- (float *__nonnull) beta;

- (void) loadGamma:(float *__nonnull*__nonnull) gamma;
- (float *__nonnull) gamma;

- (NSString *__nullable) label;
- (id __nonnull) copyWithZone:(NSZone *__nullable) zone;

@end

#endif

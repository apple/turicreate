#import <unity/toolkits/mps/layers/instance_norm_data_loader.h>

@implementation InstanceNormDataLoader

@synthesize numberOfFeatureChannels = mNumberOfFeatureChannels;

- (id __nonnull) initWithParams:(NSString *__nonnull)name
                   gammaWeights:(float *__nonnull*__nonnull)gammaWeights
                    betaWeights:(float *__nonnull*__nonnull)betaWeights
          numberFeatureChannels:(int)numberFeatureChannels
                         styles:(int)styles {
    @autoreleasepool {  
        self = [self init];
        
        mName = name;
        mStyles = styles;
        mNumberOfFeatureChannels = (NSUInteger) numberFeatureChannels;
        
        mCurrentStyle = 0;  
    
        mGamma = gammaWeights;
        mBeta = betaWeights;

        return self;
    }
}

- (void) updateNumberOfStyles:(int) styles {
    mCurrentStyle = 0;
    mStyles = styles;
}

- (void) updateCurrentStyle:(int) style {
    mCurrentStyle = style;
}

- (int) getCurrentStyle {
    return mCurrentStyle;
}

- (void) loadBeta:(float *__nonnull*__nonnull) beta {
    mBeta = beta;
}

- (float *__nonnull) beta {
    return (float *) mBeta[mCurrentStyle];
}

- (void) loadGamma:(float *__nonnull*__nonnull) gamma {
    mGamma = gamma;
}

- (float *__nonnull) gamma {
    return (float*) mGamma[mCurrentStyle];
}

- (NSString*__nullable) label {
    return mName;
}

- (id) copyWithZone:(nullable NSZone *) zone {
    return self;
}

@end

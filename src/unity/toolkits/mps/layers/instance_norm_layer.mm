#import <unity/toolkits/mps/layers/instance_norm_layer.h>

@implementation InstanceNormLayer

- (id __nonnull) initWithParameters:(NSString *__nonnull)name
                 channels:(int)channels
                   styles:(int)styles
                    gamma:(float *__nonnull*__nonnull)gamma
                     beta:(float *__nonnull*__nonnull)beta
                inputNode:(MPSNNImageNode *__nonnull)inputNode
                   device:(id<MTLDevice> __nonnull)dev{
    @autoreleasepool {
        self = [self init];
        
        mName = name;
        mChannels = channels;
        mStyles = styles;
        
        mGamma = gamma;
        mBeta = beta;
        
        mDevice = dev;
        mInputNode = inputNode;
        
        mInstNormDataLoad = [[InstanceNormDataLoader alloc] initWithParams:name
                                                              gammaWeights:gamma
                                                               betaWeights:beta
                                                     numberFeatureChannels:channels
                                                                    styles:styles];
        
        mInstNormNode =  [MPSCNNInstanceNormalizationNode nodeWithSource:mInputNode dataSource:mInstNormDataLoad];
    
        return self;
    }
}

- (MPSCNNInstanceNormalizationNode *__nonnull) underlyingNode {
    return mInstNormNode;
}

- (InstanceNormDataLoader *__nonnull) underlyingDataLoader {
    return mInstNormDataLoad;
}

- (MPSNNImageNode *__nonnull) resultImage {
    return mInstNormNode.resultImage;
}

@end

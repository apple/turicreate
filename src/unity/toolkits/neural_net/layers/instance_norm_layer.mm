#import "instance_norm_layer.h"

@implementation InstanceNormLayer

- (id)initWithParameters:(NSString *)name
                channels:(int)channels
                  styles:(int)styles
                   gamma:(float **)gamma
                    beta:(float **)beta
               inputNode:(MPSNNImageNode *)inputNode
                  device:(id<MTLDevice> _Nonnull)dev
               cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q {
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
                                                                styles:styles
                                                                device:dev
                                                             cmd_queue:cmd_q];

    mInstNormNode =
        [MPSCNNInstanceNormalizationNode nodeWithSource:mInputNode
                                             dataSource:mInstNormDataLoad];

    return self;
  }
}

- (MPSCNNInstanceNormalizationNode *)underlyingNode {
  return mInstNormNode;
}

- (InstanceNormDataLoader *)underlyingDataLoader {
  return mInstNormDataLoad;
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mInstNormNode.resultImage;
}

@end

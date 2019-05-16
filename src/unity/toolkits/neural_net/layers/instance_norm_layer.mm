#import "instance_norm_layer.h"

@implementation InstanceNormLayer

- (id _Nonnull)initWithParameters:(NSString *_Nonnull)name
                         channels:(int)channels
                           styles:(int)styles
                            gamma:(float *_Nonnull *_Nonnull)gamma
                             beta:(float *_Nonnull *_Nonnull)beta
                        inputNode:(MPSNNImageNode *_Nonnull)inputNode
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

- (MPSCNNInstanceNormalizationNode *_Nonnull)underlyingNode {
  return mInstNormNode;
}

- (InstanceNormDataLoader *_Nonnull)underlyingDataLoader {
  return mInstNormDataLoad;
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mInstNormNode.resultImage;
}

@end

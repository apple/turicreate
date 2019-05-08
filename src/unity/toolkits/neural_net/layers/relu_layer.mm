#import "relu_layer.h"

@implementation ReluLayer

- (id)initWithParams:(NSString *)name inputNode:(MPSNNImageNode *)inputNode {
  @autoreleasepool {
    self = [self init];

    mName = name;
    mReluNode = [MPSCNNNeuronReLUNode nodeWithSource:inputNode];

    return self;
  }
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mReluNode.resultImage;
}

- (MPSCNNNeuronReLUNode *)underlyingNode {
  return mReluNode;
}

@end

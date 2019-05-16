#import "relu_layer.h"

@implementation ReluLayer

- (id _Nonnull)initWithParams:(NSString *_Nonnull)name
                    inputNode:(MPSNNImageNode *_Nonnull)inputNode {
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

- (MPSCNNNeuronReLUNode *_Nonnull)underlyingNode {
  return mReluNode;
}

@end

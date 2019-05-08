#import "softmax_layer.h"

@implementation SoftmaxLayer

- (id)initWithParams:(NSString *)name inputNode:(MPSNNImageNode *)inputNode {
  @autoreleasepool {
    self = [self init];

    mName = name;
    mSoftmaxNode = [MPSCNNSoftMaxNode nodeWithSource:inputNode];

    return self;
  }
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mSoftmaxNode.resultImage;
}

- (MPSCNNSoftMaxNode *)underlyingNode {
  return mSoftmaxNode;
}

@end

#import "softmax_layer.h"

@implementation SoftmaxLayer

- (id _Nonnull)initWithParams:(NSString *_Nonnull)name
                    inputNode:(MPSNNImageNode *_Nonnull)inputNode {
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

- (MPSCNNSoftMaxNode *_Nonnull)underlyingNode {
  return mSoftmaxNode;
}

@end

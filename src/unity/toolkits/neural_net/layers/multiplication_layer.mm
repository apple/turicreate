#import "multiplication_layer.h"

@implementation MultiplicationLayer

- (id _Nonnull)initWithParams:(NSString *_Nonnull)name
                     leftNode:(MPSNNImageNode *_Nonnull)leftNode
                    rightNode:(MPSNNImageNode *_Nonnull)rightNode {
  @autoreleasepool {
    self = [self init];

    mName = name;
    mMultiplicationNode =
        [MPSNNMultiplicationNode nodeWithLeftSource:leftNode
                                        rightSource:rightNode];

    return self;
  }
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mMultiplicationNode.resultImage;
}

- (MPSNNMultiplicationNode *_Nonnull)underlyingNode {
  return mMultiplicationNode;
}

@end

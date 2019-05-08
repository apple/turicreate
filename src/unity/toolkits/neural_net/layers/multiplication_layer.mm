#import "multiplication_layer.h"

@implementation MultiplicationLayer

- (id)initWithParams:(NSString *)name
            leftNode:(MPSNNImageNode *)leftNode
           rightNode:(MPSNNImageNode *)rightNode {
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

- (MPSNNMultiplicationNode *)underlyingNode {
  return mMultiplicationNode;
}

@end

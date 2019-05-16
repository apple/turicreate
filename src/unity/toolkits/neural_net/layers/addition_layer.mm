#import "addition_layer.h"

@implementation AdditionLayer

- (id)initWithParams:(NSString * _Nullable)name
            leftNode:(MPSNNImageNode * _Nonnull)leftNode
           rightNode:(MPSNNImageNode * _Nonnull)rightNode {
  @autoreleasepool {
    self = [self init];
    mName = name;
    mAdditionNode =
        [MPSNNAdditionNode nodeWithSources:@[ leftNode, rightNode ]];
    return self;
  }
}

- (MPSNNImageNode *_Nonnull)resultImage{
  return mAdditionNode.resultImage;
}

- (MPSNNAdditionNode * _Nonnull)underlyingNode {
  return mAdditionNode;
}

@end

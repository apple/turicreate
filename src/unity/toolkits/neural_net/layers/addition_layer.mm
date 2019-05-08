#import "addition_layer.h"

@implementation AdditionLayer

- (id)initWithParams:(NSString *)name
            leftNode:(MPSNNImageNode *)leftNode
           rightNode:(MPSNNImageNode *)rightNode {
  @autoreleasepool {
    self = [self init];
    mName = name;
    mAdditionNode =
        [MPSNNAdditionNode nodeWithSources:@[ leftNode, rightNode ]];
    return self;
  }
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mAdditionNode.resultImage;
}

- (MPSNNAdditionNode *)underlyingNode {
  return mAdditionNode;
}

@end

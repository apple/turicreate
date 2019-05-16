#import "subtraction_layer.h"

@implementation SubtractionLayer

- (id _Nonnull)initWithParams:(NSString *_Nonnull)name
                     leftNode:(MPSNNImageNode *_Nonnull)leftNode
                    rightNode:(MPSNNImageNode *_Nonnull)rightNode {
  @autoreleasepool {
    self = [self init];

    mName = name;
    mSubtractionNode = [MPSNNSubtractionNode nodeWithLeftSource:leftNode
                                                    rightSource:rightNode];

    return self;
  }
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mSubtractionNode.resultImage;
}

- (MPSNNSubtractionNode *_Nonnull)underlyingNode {
  return mSubtractionNode;
}

@end

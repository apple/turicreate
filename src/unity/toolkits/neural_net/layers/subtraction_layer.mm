#import "subtraction_layer.h"

@implementation SubtractionLayer

- (id)initWithParams:(NSString *)name
            leftNode:(MPSNNImageNode *)leftNode
           rightNode:(MPSNNImageNode *)rightNode {
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

- (MPSNNSubtractionNode *)underlyingNode {
  return mSubtractionNode;
}

@end

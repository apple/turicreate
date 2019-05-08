#import "sigmoid_layer.h"

@implementation SigmoidLayer

- (id)initWithParams:(NSString *)name inputNode:(MPSNNImageNode *)inputNode {
  @autoreleasepool {
    self = [self init];

    mName = name;
    mSigmoidNode = [MPSCNNNeuronSigmoidNode nodeWithSource:inputNode];

    return self;
  }
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mSigmoidNode.resultImage;
}

- (MPSCNNNeuronSigmoidNode *)underlyingNode {
  return mSigmoidNode;
}

@end

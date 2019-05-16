#import "sigmoid_layer.h"

@implementation SigmoidLayer

- (id _Nonnull)initWithParams:(NSString *_Nonnull)name
                    inputNode:(MPSNNImageNode *_Nonnull)inputNode {
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

- (MPSCNNNeuronSigmoidNode *_Nonnull)underlyingNode {
  return mSigmoidNode;
}

@end

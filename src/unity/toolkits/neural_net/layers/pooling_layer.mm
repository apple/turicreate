#import "pooling_layer.h"

@implementation PoolingLayer

- (id)initWithParams:(NSString *)name
           inputNode:(MPSNNImageNode *)inputNode
         kernelWidth:(int)kernelWidth
        kernelHeight:(int)kernelHeight
         strideWidth:(int)strideWidth
        strideHeight:(int)strideHeight {
  @autoreleasepool {
    self = [self init];

    mName = name;
    mPoolingNode =
        [[MPSCNNPoolingAverageNode alloc] initWithSource:inputNode
                                             kernelWidth:kernelWidth
                                            kernelHeight:kernelHeight
                                         strideInPixelsX:strideWidth
                                         strideInPixelsY:strideHeight];

    return self;
  }
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mPoolingNode.resultImage;
}

- (MPSCNNPoolingAverageNode *)underlyingNode {
  return mPoolingNode;
}

@end

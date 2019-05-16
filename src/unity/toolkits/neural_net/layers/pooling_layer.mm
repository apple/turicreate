#import "pooling_layer.h"

@implementation PoolingLayer

- (id _Nonnull)initWithParams:(NSString *_Nonnull)name
                    inputNode:(MPSNNImageNode *_Nonnull)inputNode
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

- (MPSCNNPoolingAverageNode *_Nonnull)underlyingNode {
  return mPoolingNode;
}

@end

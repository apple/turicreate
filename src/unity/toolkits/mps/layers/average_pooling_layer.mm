#import <unity/toolkits/mps/layers/average_pooling_layer.h>

@implementation AveragePoolingLayer 

- (id __nonnull) initWithParams:(NSString *__nonnull)name
                      inputNode:(MPSNNImageNode *__nonnull)inputNode
                    kernelWidth:(int)kernelWidth
                   kernelHeight:(int)kernelHeight
                    strideWidth:(int)strideWidth
                   strideHeight:(int)strideHeight {
    @autoreleasepool {
        self = [self init];

        mName = name;
        mPoolingNode = [[MPSCNNPoolingAverageNode alloc] initWithSource:inputNode
                                                            kernelWidth:kernelWidth 
                                                           kernelHeight:kernelHeight 
                                                        strideInPixelsX:strideWidth 
                                                        strideInPixelsY:strideHeight];
    
        return self;
    }
}

- (MPSNNImageNode *__nonnull) resultImage {
    return mPoolingNode.resultImage;
}

@end

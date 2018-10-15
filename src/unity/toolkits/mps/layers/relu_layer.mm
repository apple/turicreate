#import <unity/toolkits/mps/layers/relu_layer.h>

@implementation ReluLayer 

- (id __nonnull) initWithParams:(NSString *__nonnull)name
                      inputNode:(MPSNNImageNode *__nonnull)inputNode{
    @autoreleasepool {
        self = [self init];

        mName = name;
        mReluNode = [MPSCNNNeuronReLUNode nodeWithSource:inputNode];
    
        return self;
    }
}

- (MPSNNImageNode *__nonnull) resultImage {
    return mReluNode.resultImage;
}

@end

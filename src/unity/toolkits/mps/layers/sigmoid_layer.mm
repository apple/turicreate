#import <unity/toolkits/mps/layers/sigmoid_layer.h>

@implementation SigmoidLayer 

- (id __nonnull) initWithParams:(NSString *__nonnull)name
                      inputNode:(MPSNNImageNode *__nonnull)inputNode {
    @autoreleasepool{
        self = [self init];

        mName = name;
        mSigmoidNode = [MPSCNNNeuronSigmoidNode nodeWithSource:inputNode];
    
        return self;
    }
}

- (MPSNNImageNode *__nonnull) resultImage {
    return mSigmoidNode.resultImage;
}

@end

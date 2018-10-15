#import <unity/toolkits/mps/layers/convolution_layer.h>

@implementation ConvolutionalLayer

- (id __nonnull) initWithParameters:(NSString *__nonnull)name
                        kernelWidth:(int)kernelWidth
                       kernelHeight:(int)kernelHeight
               inputFeatureChannels:(int)inputFeatureChannels
              outputFeatureChannels:(int)outputFeatureChannels
                        strideWidth:(int)strideWidth
                       strideHeight:(int)strideHeight
                       paddingWidth:(int)paddingWidth
                      paddingHeight:(int)paddingHeight
                            weights:(float *__nonnull)weights
                             biases:(float *__nullable)biases
                          inputNode:(MPSNNImageNode *__nonnull)inputNode
                             device:(id<MTLDevice> __nonnull)dev {
    @autoreleasepool {
        self = [self init];
    
        mName = name;

        mKernelWidth = kernelWidth;
        mKernelHeight = kernelHeight;
    
        mInputFeatureChannels = inputFeatureChannels;
        mOutputFeatureChannels = outputFeatureChannels;
    
        mStrideWidth = strideWidth;
        mStrideHeight = strideHeight;
    
        mPaddingWidth = paddingWidth;
        mPaddingHeight = paddingHeight; 

        mWeight = weights;
        mBiases = biases;
    
        mInputNode = inputNode;
        mDevice = dev;
    
        mConvDataLoad  =  [[ConvolutionDataLoader alloc] initWithParams:name
                                                            kernelWidth:kernelWidth
                                                           kernelHeight:kernelHeight
                                                   inputFeatureChannels:inputFeatureChannels
                                                  outputFeatureChannels:outputFeatureChannels
                                                            strideWidth:strideWidth
                                                           strideHeight:strideHeight
                                                                weights:weights
                                                                 biases:biases
                                                                 device:dev];
        
        
        mConvNode = [MPSCNNConvolutionNode nodeWithSource:mInputNode weights:mConvDataLoad];
        
        mPadding = [[ConvolutionPadding alloc] initWithParams:paddingWidth
                                                paddingHeight:paddingHeight];
        mConvNode.paddingPolicy = mPadding;
        
        free(weights);
        free(biases);

        return self;
    }
}

- (MPSCNNConvolutionNode *__nonnull) underlyingNode {
    return mConvNode;
}

- (ConvolutionDataLoader *__nonnull) underlyingDataLoader {
    return mConvDataLoad;
}

- (MPSNNImageNode *__nonnull) resultImage {
    return mConvNode.resultImage;
}

@end

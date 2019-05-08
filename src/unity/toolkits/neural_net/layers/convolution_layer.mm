#import "convolution_layer.h"

@implementation ConvolutionalLayer

- (id)initWithParameters:(NSString *)name
              kernelWidth:(int)kernelWidth
             kernelHeight:(int)kernelHeight
     inputFeatureChannels:(int)inputFeatureChannels
    outputFeatureChannels:(int)outputFeatureChannels
              strideWidth:(int)strideWidth
             strideHeight:(int)strideHeight
             paddingWidth:(int)paddingWidth
            paddingHeight:(int)paddingHeight
                  weights:(float *)weights
                   biases:(float *)biases
                inputNode:(MPSNNImageNode *)inputNode
                   device:(id<MTLDevice> _Nonnull)dev
                cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q {
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

    mConvDataLoad =
        [[ConvolutionDataLoader alloc] initWithParams:name
                                          kernelWidth:kernelWidth
                                         kernelHeight:kernelHeight
                                 inputFeatureChannels:inputFeatureChannels
                                outputFeatureChannels:outputFeatureChannels
                                          strideWidth:strideWidth
                                         strideHeight:strideHeight
                                              weights:weights
                                               biases:biases
                                               device:dev
                                            cmd_queue:cmd_q];

    mConvNode = [MPSCNNConvolutionNode nodeWithSource:mInputNode
                                              weights:mConvDataLoad];

    mPadding = [[ConvolutionPadding alloc] initWithParams:paddingWidth
                                            paddingHeight:paddingHeight
                                              strideWidth:strideWidth
                                             strideHeight:strideHeight];
    mConvNode.paddingPolicy = mPadding;

    return self;
  }
}

- (MPSCNNConvolutionNode *)underlyingNode {
  return mConvNode;
}

- (ConvolutionDataLoader *)underlyingDataLoader {
  return mConvDataLoad;
}

- (MPSNNImageNode *_Nonnull)resultImage {
  return mConvNode.resultImage;
}

@end

#ifndef convolutional_layer_h
#define convolutional_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#import "convolution_data_loader.h"
#import "convolution_padding.h"

API_AVAILABLE(macos(10.14))
@interface ConvolutionalLayer : NSObject {
  NSString *mName;

  int mKernelWidth;
  int mKernelHeight;

  int mInputFeatureChannels;
  int mOutputFeatureChannels;

  int mStrideWidth;
  int mStrideHeight;

  int mPaddingWidth;
  int mPaddingHeight;

  float *mWeight;
  float *mBiases;

  MPSNNImageNode *mInputNode;
  MPSCNNConvolutionNode *mConvNode;
  ConvolutionDataLoader *mConvDataLoad;
  ConvolutionPadding *mPadding;

  id<MTLDevice> mDevice;
}

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
                cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q;

- (MPSCNNConvolutionNode *)underlyingNode;
- (ConvolutionDataLoader *)underlyingDataLoader;
- (MPSNNImageNode *_Nonnull)resultImage;

@end

#endif

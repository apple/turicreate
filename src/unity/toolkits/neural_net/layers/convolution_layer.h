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

- (id _Nonnull)initWithParameters:(NSString *_Nullable)name
                      kernelWidth:(int)kernelWidth
                     kernelHeight:(int)kernelHeight
             inputFeatureChannels:(int)inputFeatureChannels
            outputFeatureChannels:(int)outputFeatureChannels
                      strideWidth:(int)strideWidth
                     strideHeight:(int)strideHeight
                     paddingWidth:(int)paddingWidth
                    paddingHeight:(int)paddingHeight
                          weights:(const float *_Nonnull)weights
                           biases:(const float *_Nonnull)biases
                        inputNode:(MPSNNImageNode *_Nonnull)inputNode
                           device:(id<MTLDevice> _Nonnull)dev
                        cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q;

- (MPSCNNConvolutionNode *_Nonnull)underlyingNode;
- (ConvolutionDataLoader *_Nonnull)underlyingDataLoader;
- (MPSNNImageNode *_Nonnull)resultImage;

@end

#endif

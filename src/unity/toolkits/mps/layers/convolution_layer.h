#ifndef turi_mps_convolutional_layer_h
#define turi_mps_convolutional_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>	

#import <unity/toolkits/mps/layers/convolution_data_loader.h>
#import <unity/toolkits/mps/layers/convolution_padding.h>

API_AVAILABLE(macos(10.13))
@interface ConvolutionalLayer : NSObject
{
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
							 device:(id<MTLDevice> __nonnull)dev;

- (MPSCNNConvolutionNode *__nonnull) underlyingNode;
- (ConvolutionDataLoader *__nonnull) underlyingDataLoader;
- (MPSNNImageNode *__nonnull) resultImage;

@end

#endif

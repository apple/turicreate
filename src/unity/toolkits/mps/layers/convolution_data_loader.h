#ifndef turi_mps_convolution_data_loader_h
#define turi_mps_convolution_data_loader_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.13))
@interface ConvolutionDataLoader: NSObject <MPSCNNConvolutionDataSource> {
	NSString *mName;
	
	int mKernelWidth;
	int mKernelHeight;
	
	int mInputFeatureChannels;
	int mOutputFeatureChannels;
	
	int mStrideWidth;
	int mStrideHeight;
	
	id<MTLBuffer> mWeights;
	id<MTLBuffer> mBiases;
}

- (id __nonnull) initWithParams:(NSString *__nonnull)name
				   kernelWidth:(int)kernelWidth
				  kernelHeight:(int)kernelHeight
		  inputFeatureChannels:(int)inputFeatureChannels
		 outputFeatureChannels:(int)outputFeatureChannels
				   strideWidth:(int)strideWidth
				  strideHeight:(int)strideHeight
					   weights:(float *__nonnull)weights
						biases:(float *__nullable)biases
						device:(id<MTLDevice> __nonnull)dev;

- (MPSDataType) dataType;
- (MPSCNNConvolutionDescriptor *__nonnull) descriptor;

- (size_t) weight_size;
- (void) loadWeight:(float *__nullable) src;
- (void *__nonnull) weights;

- (size_t) bias_size;
- (void) loadBias:(float *__nullable) src;
- (float *__nullable) biasTerms;

- (BOOL) load;
- (void) purge;

- (NSString*__nullable) label;
- (id __nonnull) copyWithZone:(nullable NSZone *) zone;

@end 

#endif

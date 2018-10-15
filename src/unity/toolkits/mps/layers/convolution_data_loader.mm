#import <unity/toolkits/mps/layers/convolution_data_loader.h>

@implementation ConvolutionDataLoader

- (id __nonnull) initWithParams:(NSString *__nonnull)name
				   kernelWidth:(int)kernelWidth
				  kernelHeight:(int)kernelHeight
		  inputFeatureChannels:(int)inputFeatureChannels
		 outputFeatureChannels:(int)outputFeatureChannels
				   strideWidth:(int)strideWidth
				  strideHeight:(int)strideHeight
					   weights:(float *__nonnull)weights
						biases:(float *__nullable)biases
						device:(id<MTLDevice> __nonnull)dev {
	@autoreleasepool {
		self = [super init];

		mName = name;

		mKernelWidth = kernelWidth;
		mKernelHeight = kernelHeight;

		mInputFeatureChannels = inputFeatureChannels;
		mOutputFeatureChannels = outputFeatureChannels;

		mStrideWidth = strideWidth;
		mStrideHeight = strideHeight;	

		if (weights != NULL) {

			mWeights = [dev
				newBufferWithBytes:weights
							length:sizeof(float) * mInputFeatureChannels * mKernelHeight * mKernelHeight * mOutputFeatureChannels
						   options:MTLResourceStorageModeManaged];
		} else {
			mWeights = [dev
						newBufferWithLength:sizeof(float) * mInputFeatureChannels *  mKernelHeight * mKernelWidth * mOutputFeatureChannels
									options:MTLResourceStorageModeManaged];
		}

		if (biases != NULL) {
			mBiases = [dev newBufferWithBytes:biases
									   length:sizeof(float) * mOutputFeatureChannels
									   options:MTLResourceStorageModeManaged];
		} else {
			mBiases = [dev newBufferWithLength:sizeof(float) * mOutputFeatureChannels
									   options:MTLResourceStorageModeManaged];
		}
		
		return self;
	}
}

- (MPSDataType) dataType {
	return MPSDataTypeFloat32;
}

- (MPSCNNConvolutionDescriptor *__nonnull) descriptor {
	MPSCNNConvolutionDescriptor *convDesc = [MPSCNNConvolutionDescriptor
      cnnConvolutionDescriptorWithKernelWidth:mKernelWidth
                                 kernelHeight:mKernelHeight
                         inputFeatureChannels:mInputFeatureChannels
                        outputFeatureChannels:mOutputFeatureChannels];

	convDesc.strideInPixelsX = mStrideWidth;
	convDesc.strideInPixelsY = mStrideHeight;
	
	return convDesc;	
}

- (size_t) weight_size {
	return mKernelWidth * mKernelHeight * mInputFeatureChannels * mOutputFeatureChannels * sizeof(float);
} 

- (void) loadWeight:(float *__nullable) src {
	memcpy(mWeights.contents, (void *)src, [self weight_size]);
	[mWeights didModifyRange:NSMakeRange(0, [self weight_size])];
}

- (void *__nonnull) weights {
	return (float *)mWeights.contents;	
}

- (size_t) bias_size {
	return mOutputFeatureChannels * sizeof(float);
}

- (void) loadBias:(float *__nullable) src {
	float *dst = (float *)mBiases.contents;
	memcpy((void *)dst, (void *)src, [self bias_size]);
	[mBiases didModifyRange:NSMakeRange(0, [self  bias_size])];
}

- (float *__nullable) biasTerms {
	return (float *)mBiases.contents;
}

- (BOOL) load {
	return YES;
}

- (void) purge {}

- (NSString*__nullable) label {
	return mName;
}

- (id __nonnull) copyWithZone:(nullable NSZone *) zone {
	return self;
}

@end

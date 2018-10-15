#import <unity/toolkits/mps/layers/convolution_padding.h>

@implementation ConvolutionPadding

+ (BOOL) supportsSecureCoding {
	return true;
}

- (id __nonnull) initWithParams:(int)paddingWidth
				  paddingHeight:(int)paddingHeight {
	@autoreleasepool {
		self = [self init];
		mPaddingWidth = paddingWidth;
		mPaddingHeight = paddingHeight;
		return self;
	}
}

- (id __nullable)initWithCoder:(NSCoder * __nonnull)coder {
	@autoreleasepool {
		self = [self init];
		return self;
	}
}

- (MPSNNPaddingMethod) paddingMethod {
	return MPSNNPaddingMethodCustom;
}

- (MPSImageDescriptor *__nonnull) destinationImageDescriptorForSourceImages:(NSArray<MPSImage *> *__nonnull)sourceImages 
															   sourceStates:(NSArray<MPSState *> *__nullable)sourceStates 
																  forKernel:(MPSKernel *__nonnull)kernel 
														suggestedDescriptor:(MPSImageDescriptor *__nonnull)inDescriptor {
	MPSCNNConvolution *layer = (MPSCNNConvolution *) kernel;
	
	MPSOffset pad;
	
	pad.x = (int) ((layer.kernelWidth/2) - mPaddingWidth);
	pad.y = (int) ((layer.kernelHeight/2) - mPaddingHeight);
	pad.z = 0;
	
	layer.offset = pad;
	layer.edgeMode = MPSImageEdgeModeZero;
	
	inDescriptor.width += mPaddingWidth * 2;
	inDescriptor.height += mPaddingHeight * 2;
	
	return inDescriptor;
}

- (void) encodeWithCoder:(NSCoder *__nullable) coder {}


@end

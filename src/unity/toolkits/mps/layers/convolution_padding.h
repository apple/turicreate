#ifndef turi_mps_convolution_padding_h
#define turi_mps_convolution_padding_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.13))
@interface ConvolutionPadding : NSObject <MPSNNPadding>
{
	int mPaddingWidth;
	int mPaddingHeight;
}
@property (class, readonly) BOOL supportsSecureCoding;

- (id __nonnull) initWithParams:(int)paddingWidth
				  paddingHeight:(int)paddingHeight;

- (id __nullable) initWithCoder:(NSCoder * __nonnull)coder;

- (MPSNNPaddingMethod) paddingMethod;
- (MPSImageDescriptor *__nonnull) destinationImageDescriptorForSourceImages:(NSArray<MPSImage *> *__nonnull)sourceImages 
															   sourceStates:(NSArray<MPSState *> *__nullable)sourceStates 
																  forKernel:(MPSKernel *__nonnull)kernel 
														suggestedDescriptor:(MPSImageDescriptor *__nonnull)inDescriptor;

- (void) encodeWithCoder:(NSCoder *__nullable) coder;

@end

#endif

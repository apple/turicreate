#ifndef mps_layer_conv_padding_h
#define mps_layer_conv_padding_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface ConvolutionPadding : NSObject <MPSNNPadding>
{
  int mPaddingWidth;
  int mPaddingHeight;
  int mStrideWidth;
  int mStrideHeight;
}

@property (class, readonly) BOOL supportsSecureCoding;

- (id) initWithParams:(int)paddingWidth
        paddingHeight:(int)paddingHeight
          strideWidth:(int)strideWidth
         strideHeight:(int)strideHeight;

- (nullable instancetype)initWithCoder:(NSCoder *)coder;

- (MPSNNPaddingMethod) paddingMethod;
- (void)encodeWithCoder:(NSCoder *)coder;
- (MPSImageDescriptor *)destinationImageDescriptorForSourceImages:(NSArray<MPSImage *> *)sourceImages 
                                                     sourceStates:(NSArray<MPSState *> *)sourceStates 
                                                        forKernel:(MPSKernel *)kernel 
                                              suggestedDescriptor:(MPSImageDescriptor *)inDescriptor;

@end

#endif

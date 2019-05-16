#ifndef convolution_padding_h
#define convolution_padding_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface ConvolutionPadding : NSObject <MPSNNPadding> {
  int mPaddingWidth;
  int mPaddingHeight;
  int mStrideWidth;
  int mStrideHeight;
}
@property(class, readonly) BOOL supportsSecureCoding;

- (id _Nonnull)initWithParams:(int)paddingWidth
                paddingHeight:(int)paddingHeight
                  strideWidth:(int)strideWidth
                 strideHeight:(int)strideHeight;

- (nullable instancetype)initWithCoder:(NSCoder *_Nonnull)coder;

- (MPSNNPaddingMethod)paddingMethod;
- (void)encodeWithCoder:(NSCoder *_Nonnull)coder;
- (MPSImageDescriptor *_Nonnull)
    destinationImageDescriptorForSourceImages:
        (NSArray<MPSImage *> *_Nonnull)sourceImages
                                 sourceStates:(NSArray<MPSState *> *_Nullable)
                                                  sourceStates
                                    forKernel:(MPSKernel *_Nonnull)kernel
                          suggestedDescriptor:
                              (MPSImageDescriptor *_Nonnull)inDescriptor;

@end

#endif

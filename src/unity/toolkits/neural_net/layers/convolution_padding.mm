#include "convolution_padding.h"

@implementation ConvolutionPadding

+ (BOOL)supportsSecureCoding {
  return true;
}

- (id _Nonnull)initWithParams:(int)paddingWidth
                paddingHeight:(int)paddingHeight
                  strideWidth:(int)strideWidth
                 strideHeight:(int)strideHeight {
  @autoreleasepool {
    self = [self init];
    mPaddingWidth = paddingWidth;
    mPaddingHeight = paddingHeight;
    mStrideWidth = strideWidth;
    mStrideHeight = strideHeight;
    return self;
  }
}

- (nullable instancetype)initWithCoder:(NSCoder *_Nonnull)coder {
  @autoreleasepool {
    self = [self init];
    return self;
  }
}

- (MPSNNPaddingMethod)paddingMethod {
  return MPSNNPaddingMethodCustom;
}

- (MPSImageDescriptor *_Nonnull)
    destinationImageDescriptorForSourceImages:
        (NSArray<MPSImage *> *_Nonnull)sourceImages
                                 sourceStates:(NSArray<MPSState *> *_Nullable)
                                                  sourceStates
                                    forKernel:(MPSKernel *_Nonnull)kernel
                          suggestedDescriptor:
                              (MPSImageDescriptor *_Nonnull)inDescriptor {
  MPSCNNConvolution *layer = (MPSCNNConvolution *)kernel;

  MPSOffset pad;
  pad.x = (int)((layer.kernelWidth / 2) - mPaddingWidth);
  pad.y = (int)((layer.kernelHeight / 2) - mPaddingHeight);
  pad.z = 0;

  layer.offset = pad;
  layer.edgeMode = MPSImageEdgeModeZero;

  inDescriptor.width += (mPaddingWidth * 2) / mStrideWidth;
  inDescriptor.height += (mPaddingHeight * 2) / mStrideHeight;

  return inDescriptor;
}

- (void)encodeWithCoder:(NSCoder *_Nonnull)coder {
}

@end

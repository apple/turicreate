#include <ml/neural_net/mps_layer_conv_padding.h>

@implementation ConvolutionPadding

+ (BOOL) supportsSecureCoding {
  return true;
}

- (id) initWithParams:(int)paddingWidth
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

- (nullable instancetype)initWithCoder:(NSCoder *)coder {
  @autoreleasepool {
    self = [self init];
    return self;
  }
}

- (MPSNNPaddingMethod) paddingMethod {
  return MPSNNPaddingMethodCustom;
}

- (MPSImageDescriptor *)destinationImageDescriptorForSourceImages:(NSArray<MPSImage *> *)sourceImages
                                                     sourceStates:(NSArray<MPSState *> *)sourceStates
                                                        forKernel:(MPSKernel *)kernel
                                              suggestedDescriptor:(MPSImageDescriptor *)inDescriptor {
  MPSCNNConvolution *layer = (MPSCNNConvolution *) kernel;
  
  MPSOffset pad;
  pad.x = (int) ((layer.kernelWidth/2) - mPaddingWidth);
  pad.y = (int) ((layer.kernelHeight/2) - mPaddingHeight);
  pad.z = 0;
  
  layer.offset = pad;
  layer.edgeMode = MPSImageEdgeModeZero;
  
  inDescriptor.width += (mPaddingWidth * 2)/mStrideWidth;
  inDescriptor.height += (mPaddingHeight * 2)/mStrideHeight;
  
  return inDescriptor;
}

- (void)encodeWithCoder:(NSCoder *)coder {}


@end

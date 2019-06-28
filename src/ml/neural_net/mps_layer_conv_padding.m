#include <ml/neural_net/mps_layer_conv_padding.h>

@implementation ConvolutionPadding

+ (BOOL) supportsSecureCoding {
  return YES;
}

- (instancetype) initWithParams:(NSUInteger)paddingWidth
                  paddingHeight:(NSUInteger)paddingHeight
                    strideWidth:(NSUInteger)strideWidth
                   strideHeight:(NSUInteger)strideHeight {
  
  self = [self init];

  if (self) {
    _paddingWidth = paddingWidth;
    _paddingHeight = paddingHeight;
    _strideWidth = strideWidth;
    _strideHeight = strideHeight;
  }

  return self;
}

- (nullable instancetype)initWithCoder:(NSCoder *)coder {
  return self;
}

- (MPSNNPaddingMethod)paddingMethod {
  return MPSNNPaddingMethodCustom;
}

- (MPSImageDescriptor *)destinationImageDescriptorForSourceImages:(NSArray<MPSImage *> *)sourceImages
                                                     sourceStates:(NSArray<MPSState *> *)sourceStates
                                                        forKernel:(MPSKernel *)kernel
                                              suggestedDescriptor:(MPSImageDescriptor *)inDescriptor {
  MPSCNNConvolution *layer = (MPSCNNConvolution *) kernel;

  MPSOffset pad;
  pad.x = (int) ((layer.kernelWidth/2) - _paddingWidth);
  pad.y = (int) ((layer.kernelHeight/2) - _paddingHeight);
  pad.z = 0;
  
  layer.offset = pad;
  layer.edgeMode = MPSImageEdgeModeZero;
  
  inDescriptor.width += (_paddingWidth * 2)/_strideWidth;
  inDescriptor.height += (_paddingHeight * 2)/_strideHeight;
  
  return inDescriptor;
}

- (void)encodeWithCoder:(NSCoder *)coder {}


@end

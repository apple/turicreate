#import <ml/neural_net/mps_layer_conv_padding.h>

@implementation TCMPSConvolutionPadding

+ (BOOL) supportsSecureCoding {
  return NO;
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

/*
*
* Due to the specificities of the MPS API, we modify the kernel and imageDescriptor directly
*
* Since the callee is `MPS` we don't worry about the fact that we directly modify the objects
* passed into this method
*
**/
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

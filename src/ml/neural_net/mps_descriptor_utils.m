/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <ml/neural_net/mps_descriptor_utils.h>

@implementation TCMPSConvolutionDescriptor
- (instancetype) init {
  self = [super init];
  if (self) {
    _label = [[NSString alloc] init];
  }
  return self;
}
@end

@implementation TCMPSInstanceNormalizationDescriptor
- (instancetype) init {
  self = [super init];
  if (self) {
    _label = [[NSString alloc] init];
  }
  return self;
}
@end

@implementation TCMPSUpsamplingDescriptor
@end

@implementation TCMPSPoolingDescriptor
@end

@implementation TCMPSEncodingDescriptor
- (instancetype) init {
  self = [super init];
  if (self) {
    _conv = [[TCMPSConvolutionDescriptor alloc] init];
    _inst = [[TCMPSInstanceNormalizationDescriptor alloc] init];
  }
  return self;
}
@end

@implementation TCMPSResidualDescriptor
- (instancetype) init {
  self = [super init];
  if (self) {
    _conv1 = [[TCMPSConvolutionDescriptor alloc] init];
    _inst1 = [[TCMPSInstanceNormalizationDescriptor alloc] init];
    _conv2 = [[TCMPSConvolutionDescriptor alloc] init];
    _inst2 = [[TCMPSInstanceNormalizationDescriptor alloc] init];
  }
  return self;
}
@end

@implementation TCMPSDecodingDescriptor
- (instancetype) init {
  self = [super init];
  if (self) {
    _conv = [[TCMPSConvolutionDescriptor alloc] init];
    _inst = [[TCMPSInstanceNormalizationDescriptor alloc] init];
    _upsample = [[TCMPSUpsamplingDescriptor alloc] init];
  }
  return self;
}
@end

@implementation TCMPSVgg16Block1Descriptor
- (instancetype) init {
  self = [super init];
  if (self) {
    _conv1 = [[TCMPSConvolutionDescriptor alloc] init];
    _conv2 = [[TCMPSConvolutionDescriptor alloc] init];
    _pooling = [[TCMPSPoolingDescriptor alloc] init];
  }
  return self;
}
@end

@implementation TCMPSVgg16Block2Descriptor
- (instancetype) init {
  self = [super init];
  if (self) {
    _conv1 = [[TCMPSConvolutionDescriptor alloc] init];
    _conv2 = [[TCMPSConvolutionDescriptor alloc] init];
    _conv3 = [[TCMPSConvolutionDescriptor alloc] init];
    _pooling = [[TCMPSPoolingDescriptor alloc] init];
  }
  return self;
}
@end

@implementation TCMPSVgg16Descriptor
- (instancetype) init {
  self = [super init];
  if (self) {
    _block1 = [[TCMPSVgg16Block1Descriptor alloc] init];
    _block2 = [[TCMPSVgg16Block1Descriptor alloc] init];
    _block3 = [[TCMPSVgg16Block2Descriptor alloc] init];
    _block4 = [[TCMPSVgg16Block2Descriptor alloc] init];
  }
  return self;
}
@end

@implementation TCMPSTransformerDescriptor
- (instancetype) init {
  self = [super init];
  if (self) {
    _encode1 = [[TCMPSEncodingDescriptor alloc] init];
    _encode2 = [[TCMPSEncodingDescriptor alloc] init];
    _encode3 = [[TCMPSEncodingDescriptor alloc] init];

    _residual1 = [[TCMPSResidualDescriptor alloc] init];
    _residual2 = [[TCMPSResidualDescriptor alloc] init];
    _residual3 = [[TCMPSResidualDescriptor alloc] init];
    _residual4 = [[TCMPSResidualDescriptor alloc] init];
    _residual5 = [[TCMPSResidualDescriptor alloc] init];

    _decode1 = [[TCMPSDecodingDescriptor alloc] init];
    _decode2 = [[TCMPSDecodingDescriptor alloc] init];

    _conv = [[TCMPSConvolutionDescriptor alloc] init];
    _inst = [[TCMPSInstanceNormalizationDescriptor alloc] init];
  }
  return self;
}
@end
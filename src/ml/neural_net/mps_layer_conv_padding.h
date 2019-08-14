/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface TCMPSConvolutionPadding : NSObject <MPSNNPadding>

@property (nonatomic) NSUInteger paddingWidth;
@property (nonatomic) NSUInteger paddingHeight;
@property (nonatomic) NSUInteger strideWidth;
@property (nonatomic) NSUInteger strideHeight;

@property (class, readonly) BOOL supportsSecureCoding;

- (instancetype) initWithParams:(NSUInteger)paddingWidth
                  paddingHeight:(NSUInteger)paddingHeight
                    strideWidth:(NSUInteger)strideWidth
                   strideHeight:(NSUInteger)strideHeight;

- (nullable instancetype)initWithCoder:(NSCoder *)coder;

- (MPSNNPaddingMethod)paddingMethod;
- (void)encodeWithCoder:(NSCoder *)coder;
- (MPSImageDescriptor *)destinationImageDescriptorForSourceImages:(NSArray<MPSImage *> *)sourceImages 
                                                     sourceStates:(NSArray<MPSState *> *)sourceStates 
                                                        forKernel:(MPSKernel *)kernel 
                                              suggestedDescriptor:(MPSImageDescriptor *)inDescriptor;

@end

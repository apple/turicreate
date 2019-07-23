/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

#import <ml/neural_net/style_transfer/mps_style_transfer.h>
#import <ml/neural_net/mps_descriptor_utils.h>

@interface TCMPSStyleTransfer (TCMPSStyleTransferUtils)
+ (TCMPSTransformerDescriptor *)defineTransformerDescriptor:(NSUInteger)numStyles
                                             tuneAllWeights:(BOOL)tuneAllWeights;
+ (TCMPSVgg16Descriptor *)defineVGG16Descriptor:(NSUInteger)numStyles;

+ (NSDictionary<NSString *, NSDictionary *> *)defineTransformerWeights:(NSDictionary<NSString *, NSData *> *)weights;
+ (NSDictionary<NSString *, NSDictionary *> *)defineVGG16:(NSDictionary<NSString *, NSData *> *)weights;
+ (void)populateMean:(NSMutableData *)data;
+ (void)populateMultiplication:(NSMutableData *)data;
@end
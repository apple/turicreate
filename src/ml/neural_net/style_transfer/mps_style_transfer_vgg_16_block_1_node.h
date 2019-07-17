/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

#import <ml/neural_net/mps_descriptor_utils.h>

NS_ASSUME_NONNULL_BEGIN

API_AVAILABLE(macos(10.14))
@interface TCMPSVgg16Block1 : NSObject 

@property (nonatomic) MPSNNImageNode *features;
@property (nonatomic) MPSNNImageNode *output;

- (instancetype) initWithParameters:(NSString *)name
                          inputNode:(MPSNNImageNode *)inputNode
                             device:(id<MTLDevice>)dev
                           cmdQueue:(id<MTLCommandQueue>)cmdQ
                         descriptor:(TCMPSVgg16Block1Descriptor *)descriptor
                        initWeights:(NSDictionary<NSString *, NSData *> *) weights;

- (MPSNNImageNode * _Nullable) backwardPass:(MPSNNImageNode *) inputNode;

@end

NS_ASSUME_NONNULL_END
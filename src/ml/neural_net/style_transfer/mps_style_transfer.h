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
@interface TCMPSStyleTransfer : NSObject

- (instancetype) initWithParameters:(NSDictionary<NSString *, NSData *> *)weights;

- (NSDictionary<NSString *, NSData *> *)exportWeights;
- (NSDictionary<NSString *, NSData *> *)predict:(NSDictionary<NSString *, NSData *> *)inputs;
- (void) setLearningRate:(NSNumber *) lr;
- (NSDictionary<NSString *, NSData *> *) train:(NSDictionary<NSString *, NSData *> *)inputs;

@end

NS_ASSUME_NONNULL_END
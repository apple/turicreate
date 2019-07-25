/* Copyright © 2019 Apple Inc. All rights reserved.
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

API_AVAILABLE(macos(10.15))
@interface TCMPSStyleTransfer : NSObject

@property (nonatomic) NSUInteger batchSize;
@property (nonatomic) NSNumber *contentLossMultiplier;
@property (nonatomic) NSNumber *styleLossMultiplier;
@property (nonatomic) BOOL finetuneAllParams;
@property (nonatomic) NSUInteger imgWidth;
@property (nonatomic) NSUInteger imgHeight;

- (instancetype) initWithParameters:(NSDictionary<NSString *, NSData *> *)weights
                          numStyles:(NSUInteger)numStyles;

- (NSDictionary<NSString *, NSData *> *) exportWeights;
- (NSDictionary<NSString *, NSData *> *) predict:(NSDictionary<NSString *, NSData *> *)inputs;
- (void) setLearningRate:(float)lr;
- (NSDictionary<NSString *, NSData *> *) train:(NSDictionary<NSString *, NSData *> *)inputs;

@end

NS_ASSUME_NONNULL_END
/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

#ifdef HAS_MACOS_10_15

@interface TCMPSStyleTransferWeights : NSObject 
@property (nonatomic) NSData *data;
@property (nonatomic) NSArray<NSNumber *> *shape;
@end

#endif // #ifdef HAS_MACOS_10_15
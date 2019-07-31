/* Copyright © 2019 Apple Inc. All rights reserved.
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

#ifdef HAS_MACOS_10_15

@interface TCMPSStyleTransfer (TCMPSStyleTransferUtils)
+ (TCMPSTransformerDescriptor *) defineTransformerDescriptor:(NSUInteger)numStyles
                                              tuneAllWeights:(BOOL)tuneAllWeights;
+ (TCMPSVgg16Descriptor *) defineVGG16Descriptor:(NSUInteger)numStyles;
/**
 * populateMean
 *
 * SUMMARY
 * -------
 * This function populates the MPSImage used in the pre-processing of the Neural 
 * Network. Each channel in the Style Transfer network is weighted differently
 * and these channel weights take care of that.
 *
 */
+ (void) populateMean:(NSMutableData *)data;

/**
 * populateMultiplication
 *
 * SUMMARY
 * -------
 * This function populates the MPSImage used in the pre-processing of the Neural
 * Network. The images coming out of the neural network are of values ranging 
 * from 0-1. The preprocessor takes those values and scales them up to 0-255.
 */
+ (void) populateMultiplication:(NSMutableData *)data;
@end

#endif // #ifdef HAS_MACOS_10_15
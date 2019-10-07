/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <boost/property_tree/ptree.hpp>

@interface NeuralNetStyleTransferUtils : NSObject

+ (NSData*) loadData:(NSString*)url;
+ (boost::property_tree::ptree) loadConfig:(NSString*)url;

+ (MPSImageBatch *) defineInput:(NSString *)path
                            dev:(id<MTLDevice>)dev;

+ (NSData *) defineOutput:(NSString *)path;

+ (BOOL) checkData:(NSData *)expected
            actual:(NSData *)actual
           epsilon:(float)epsilon;
@end

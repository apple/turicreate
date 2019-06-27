/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <CoreML/CoreML.h>

NS_ASSUME_NONNULL_BEGIN

API_AVAILABLE(macos(10.13),ios(11.0))
@interface TCSoundClassifierPreprocessing : NSObject<MLCustomModel>

@property(readonly, nullable) NSString* inputFeatureName;
@property(readonly, nullable) NSString* outputFeatureName;

- (nullable instancetype)initWithModelDescription:(MLModelDescription*)modelDescription
                              parameterDictionary:(NSDictionary<NSString *, id>*)parameters
                                            error:(NSError**)error;

- (nullable id<MLFeatureProvider>)predictionFromFeatures:(id<MLFeatureProvider>)input
                                                 options:(MLPredictionOptions*)options
                                                   error:(NSError**)error;

@end

NS_ASSUME_NONNULL_END

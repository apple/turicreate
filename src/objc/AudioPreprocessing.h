/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#import <CoreML/CoreML.h>


__attribute__((visibility("default")))
@interface TCSoundClassifierPreprocessing : NSObject<MLCustomModel>

@property(readonly, nullable) NSString* inputFeatureName;
@property(readonly, nullable) NSString* outputFeatureName;

- (nullable instancetype)initWithModelDescription:(MLModelDescription * _Null_unspecified)modelDescription
                              parameterDictionary:(NSDictionary<NSString *, id> * _Null_unspecified)parameters
                                            error:(NSError * _Nullable * _Nullable)error API_AVAILABLE(macos(10.13));

- (nullable id<MLFeatureProvider>)predictionFromFeatures:(id<MLFeatureProvider> _Nonnull)input
                                                 options:(MLPredictionOptions * _Null_unspecified)options
                                                 error:(NSError * _Nullable * _Nullable)error API_AVAILABLE(macos(10.13));

@end

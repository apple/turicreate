#import <CoreML/CoreML.h>
#import <Foundation/Foundation.h>
#include <capi/TuriCreate.h>

// @TODO: think about side data later
// @TODO: think about rating data later

__attribute__((visibility("default")))
@interface TCRecommenderOutput : NSObject<MLFeatureProvider>

@property(nonatomic, nonnull, readonly) tc_sframe *outputSFrame;
@property(nonatomic, nonnull, readonly) const char *item_id_name;

- (nullable instancetype)initWithSFrame: (tc_sframe * _Nonnull)sf 
                           item_id_name: (const char * _Nonnull)item_id_name
                                  error: (NSError * _Nullable * _Nonnull)error;

@end

__attribute__((visibility("default")))
@interface TCRecommender : NSObject<MLCustomModel>

@property(nonatomic, nullable, readonly) tc_model *model;

@property(nonatomic, nullable, readonly) MLModelDescription *modelDescription
  API_AVAILABLE(macos(10.13));

- (nullable instancetype)initWithModelDescription:(MLModelDescription * _Null_unspecified)modelDescription
                              parameterDictionary:(NSDictionary<NSString *, id> * _Null_unspecified)parameters
                                            error:(NSError * _Nullable * _Nullable)error API_AVAILABLE(macos(10.13));

- (nullable id<MLFeatureProvider>)predictionFromFeatures:(id<MLFeatureProvider> _Nonnull)input
                                                 options:(MLPredictionOptions * _Null_unspecified)options
                                                   error:(NSError * _Nullable * _Nullable)error API_AVAILABLE(macos(10.13));
@end

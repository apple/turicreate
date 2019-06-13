/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#import "TuriCreateObjC.h"

#ifndef __has_feature         // Optional of course.
    #define __has_feature(x) 0  // Compatibility with non-clang compilers.
#endif
#if !__has_feature(objc_arc)
    #error "Expected to compile with -fobjc-arc"
#endif

// Returns true if a TC error was handled (in which case,
// the NSError * will get populated if it's non-null).
API_AVAILABLE(macos(10.13),ios(11.0))
static bool handleError(tc_error* error_ptr, NSError** error) {
    if (error_ptr) {
        if (error) {
            NSMutableDictionary *userInfo = [[NSMutableDictionary alloc] init];
            const char *description = tc_error_message(error_ptr);
            userInfo[NSLocalizedDescriptionKey] = @(description);
            *error = [NSError errorWithDomain:MLModelErrorDomain
                                         code:MLModelErrorGeneric
                                     userInfo:userInfo];
        }
        return true;
    }
    return false;
}

@implementation TCRecommenderOutput

- (NSString *) debugDescription {
    NSString *ret = @"";
    for (NSString * key in self.featureNames) {
        ret = [ret stringByAppendingString:[self featureValueForName:key].debugDescription];
        ret = [ret stringByAppendingString:@"\n"];
    }
    return ret;
}

- (NSArray<NSString *> *)featureNames {
    return @[@"probabilities", @"recommendations"];
}

- (nullable MLFeatureValue *)featureValueForName:(nonnull NSString *)featureName
  API_AVAILABLE(macos(10.13),ios(11.0)) {
    tc_sarray *item_ids_array;
    tc_sarray *scores_array;
    tc_sarray *ranks_array;
    tc_error *error_ptr = NULL;
    NSError *ns_error_ptr;
    NSMutableDictionary<id,NSNumber *> *scores;
    NSMutableDictionary<id,NSNumber *> *ranks;
    MLFeatureValue *return_value = nil;
    
    item_ids_array = tc_sframe_extract_column_by_name(self.outputSFrame,
                                                      self.item_id_name,
                                                      &error_ptr);
    if (handleError(error_ptr, &ns_error_ptr)) {
        assert(false); // no way to propagate error to caller
        return nil;
    }
    scores_array = tc_sframe_extract_column_by_name(self.outputSFrame,
                                                    "score", &error_ptr);
    if (handleError(error_ptr, &ns_error_ptr)) {
        assert(false); // no way to propagate error to caller
        return nil;
    }
    ranks_array = tc_sframe_extract_column_by_name(self.outputSFrame,
                                                   "rank", &error_ptr);
    if (handleError(error_ptr, &ns_error_ptr)) {
        assert(false); // no way to propagate error to caller
        return nil;
    }
    
    if ([featureName  isEqual: @"probabilities"]) {
        scores = [[NSMutableDictionary alloc] init];
        size_t num_rows = tc_sframe_num_rows(self.outputSFrame, &error_ptr);
        if (handleError(error_ptr, &ns_error_ptr)) {
            assert(false); // no way to propagate error to caller
            return nil;
        }
        for (size_t i = 0; i < num_rows; i++) {
            tc_flexible_type* item_ft = tc_sarray_extract_element(
                                                                  item_ids_array, i, &error_ptr);
            if (handleError(error_ptr, &ns_error_ptr)) {
                assert(false); // no way to propagate error to caller
                return nil;
            }
            tc_flexible_type* score_ft = tc_sarray_extract_element(
                                                                   scores_array, i, &error_ptr);
            if (handleError(error_ptr, &ns_error_ptr)) {
                assert(false); // no way to propagate error to caller
                return nil;
            }
            double score = tc_ft_double(score_ft, &error_ptr);
            if (handleError(error_ptr, &ns_error_ptr)) {
                assert(false); // no way to propagate error to caller
                return nil;
            }
            switch (tc_ft_type(item_ft)) {
                case FT_TYPE_INTEGER: {
                    int64_t item_id_key = tc_ft_int64(item_ft, &error_ptr);
                    if (handleError(error_ptr, &ns_error_ptr)) {
                        assert(false); // no way to propagate error to caller
                        return nil;
                    }
                    scores[[NSNumber numberWithInteger:item_id_key]] = @(score);
                    break;
                }
                case FT_TYPE_STRING: {
                    int64_t item_id_string_length = tc_ft_string_length(
                                                                        item_ft, &error_ptr);
                    if (handleError(error_ptr, &ns_error_ptr)) {
                        assert(false); // no way to propagate error to caller
                        return nil;
                    }
                    const char* item_id_string_data = tc_ft_string_data(
                                                                        item_ft, &error_ptr);
                    if (handleError(error_ptr, &ns_error_ptr)) {
                        assert(false); // no way to propagate error to caller
                        return nil;
                    }
                    NSString *item_id_nsstring_key = [[NSString alloc] initWithBytes:item_id_string_data
                                                                              length:item_id_string_length
                                                                            encoding:NSUTF8StringEncoding];
                    scores[item_id_nsstring_key] = @(score);
                    break;
                }
                default:
                    assert(false);
                    return nil;
            }
        }
        tc_release(item_ids_array);
        tc_release(scores_array);
        return_value = [MLFeatureValue featureValueWithDictionary:scores error:&ns_error_ptr];
    } else if ([featureName  isEqual: @"recommendations"]) {
        ranks = [[NSMutableDictionary alloc] init];
        for (size_t i = 0; i < tc_sframe_num_rows(
                                                  self.outputSFrame, &error_ptr); i++) {
            tc_flexible_type* item_ft = tc_sarray_extract_element(
                                                                  item_ids_array, i, &error_ptr);
            if (handleError(error_ptr, &ns_error_ptr)) {
                assert(false); // no way to propagate error to caller
                return nil;
            }
            tc_flexible_type* rank_ft = tc_sarray_extract_element(
                                                                  ranks_array, i, &error_ptr);
            if (handleError(error_ptr, &ns_error_ptr)) {
                assert(false); // no way to propagate error to caller
                return nil;
            }
            int64_t rank = tc_ft_int64(rank_ft, &error_ptr);
            switch (tc_ft_type(item_ft)) {
                case FT_TYPE_INTEGER: {
                    int64_t item_id_key = tc_ft_int64(item_ft, &error_ptr);
                    if (handleError(error_ptr, &ns_error_ptr)) {
                        assert(false); // no way to propagate error to caller
                        return nil;
                    }
                    ranks[[NSNumber numberWithInteger:item_id_key]] = @(rank);
                    break;
                }
                case FT_TYPE_STRING: {
                    int64_t item_id_string_length = tc_ft_string_length(item_ft, &error_ptr);
                    if (handleError(error_ptr, &ns_error_ptr)) {
                        assert(false); // no way to propagate error to caller
                        return nil;
                    }
                    const char* item_id_string_data = tc_ft_string_data(item_ft, &error_ptr);
                    if (handleError(error_ptr, &ns_error_ptr)) {
                        assert(false); // no way to propagate error to caller
                        return nil;
                    }
                    NSString *item_id_nsstring_key = [[NSString alloc] initWithBytes:item_id_string_data
                                                                              length:item_id_string_length
                                                                            encoding:NSUTF8StringEncoding];
                    ranks[item_id_nsstring_key] = @(rank);
                    break;
                }
                default:
                    assert(false);
                    return nil;
            }
        }
        tc_release(item_ids_array);
        tc_release(ranks_array);
        return_value = [MLFeatureValue featureValueWithDictionary:ranks error:&ns_error_ptr];
    }
    assert(return_value != nil);
    return return_value;
}

- (nullable instancetype)initWithSFrame: (tc_sframe *)sf
                           item_id_name: (const char *)item_id_name
                                  error: (NSError * _Nullable *)error {
    _item_id_name = item_id_name;
    _outputSFrame = sf;
    return self;
}

@end



@implementation TCRecommender

- (nullable instancetype)initWithModelDescription:(MLModelDescription *)modelDescription
                              parameterDictionary:(NSDictionary<NSString *, id> *)parameters
                                            error:(NSError **)error {
    self = [super init];
    if (!self) return nil;
    tc_error *error_ptr = NULL;
    NSData *model_data = (NSData *)parameters[@"turi_create_model"];
    _model = tc_model_load_from_data(model_data.bytes, model_data.length, &error_ptr);
    _modelDescription = modelDescription;
    if (handleError(error_ptr, error)) {
        return nil;
    }
    return self;
}

- (nullable id<MLFeatureProvider>)predictionFromFeatures:(id<MLFeatureProvider>)input
                                                 options:(MLPredictionOptions *)options
                                                   error:(NSError **)error {
    
    // Convert input to an sframe that recommend_from_interactions can consume
    tc_error *error_ptr = NULL;
    tc_flex_list *itemValuesFlexList = tc_flex_list_create(&error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_flex_list *ratingValuesFlexList = tc_flex_list_create(&error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    MLFeatureValue *interactions_feature = [input featureValueForName:@"interactions"];
    assert(interactions_feature.type == MLFeatureTypeDictionary);
    NSDictionary<id,NSNumber *> *interactions_dict = interactions_feature.dictionaryValue;
    NSArray<id> *keys = interactions_dict.allKeys;
    for (size_t i=0; i<keys.count; i++) {
        
        id key = keys[i];
        const tc_flexible_type *item_ft;
        MLFeatureType item_id_type = self.modelDescription.inputDescriptionsByName[@"interactions"].dictionaryConstraint.keyType;
        switch (item_id_type) {
            case MLFeatureTypeString: {
                NSString *string_key = keys[i];
                item_ft = tc_ft_create_from_string(string_key.UTF8String, string_key.length, &error_ptr);
                break;
            }
            case MLFeatureTypeInt64: {
                NSNumber *int_key = keys[i];
                item_ft = tc_ft_create_from_int64(int_key.intValue, &error_ptr);
                break;
            }
            default:
                break;
        }
        
        if (handleError(error_ptr, error)) {
            return nil;
        }
        const tc_flexible_type *rating_ft = tc_ft_create_from_double(
            interactions_dict[key].doubleValue, &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
        tc_flex_list_add_element(itemValuesFlexList, item_ft, &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
        tc_flex_list_add_element(ratingValuesFlexList, rating_ft, &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
    }
    MLFeatureValue *top_k_feature = [input featureValueForName:@"k"];
    assert(top_k_feature.type == MLFeatureTypeInt64);
    int64_t top_k = top_k_feature.int64Value;
    tc_sarray *itemValues = tc_sarray_create_from_list(
        itemValuesFlexList, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_sarray *ratingValues = tc_sarray_create_from_list(
        ratingValuesFlexList, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_flexible_type *userValue = tc_ft_create_from_int64(-1, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_sarray *userValues = tc_sarray_create_from_const(userValue, 
        tc_sarray_size(itemValues), &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_sframe *itemValuesSFrame = tc_sframe_create_empty(&error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    
    const char *user_id_name = NULL;
    {
        tc_parameters *args = tc_parameters_create_empty(&error_ptr);
        tc_parameters_add_cstring(args, "field", "user_id", &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
        tc_variant* user_id_variant = tc_model_call_method(
                                                           self.model, "get_value", args, &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
        assert(tc_variant_is_cstring(user_id_variant));
        user_id_name = tc_variant_string_data(user_id_variant, &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
        tc_release(args);
    }
    
    const char *item_id_name = NULL;
    {
        tc_parameters *args = tc_parameters_create_empty(&error_ptr);
        tc_parameters_add_cstring(args, "field", "item_id", &error_ptr);
        tc_variant* item_id_variant = tc_model_call_method(
                                                           self.model, "get_value", args, &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
        assert(tc_variant_is_cstring(item_id_variant));
        item_id_name = tc_variant_string_data(item_id_variant, &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
        tc_release(args);
    }
    const char *ratings_name = NULL;
    {
        tc_parameters *args = tc_parameters_create_empty(&error_ptr);
        tc_parameters_add_cstring(args, "field", "target", &error_ptr);
        tc_variant* ratings_variant = tc_model_call_method(
                                                           self.model, "get_value", args, &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
        if (tc_variant_is_cstring(ratings_variant)) {
            if (ratings_variant != NULL) {// && ratings_variant_length > 0) {
                ratings_name = tc_variant_string_data(ratings_variant, &error_ptr);
                if (handleError(error_ptr, error)) {
                    return nil;
                }
            } else {
                ratings_name = NULL;
            }
            tc_release(args);
        } else {
            assert(tc_variant_is_flexible_type(ratings_variant));
            assert(tc_ft_is_undefined(tc_variant_flexible_type(ratings_variant, NULL)));
        }
        
    }
    
    tc_sframe_add_column(
                         itemValuesSFrame, user_id_name, userValues, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_sframe_add_column(
                         itemValuesSFrame, item_id_name, itemValues, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    if (ratings_name != NULL) {
        tc_sframe_add_column(
                             itemValuesSFrame, ratings_name, ratingValues, &error_ptr);
        if (handleError(error_ptr, error)) {
            return nil;
        }
    }
    tc_parameters *args = tc_parameters_create_empty(&error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_parameters_add_sframe(
                             args, "new_observation_data", itemValuesSFrame, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    
    tc_sarray *singleUserValue = tc_sarray_create_from_const(
                                                             userValue, 1, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_sframe *userValuesSFrame = tc_sframe_create_empty(&error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_sframe_add_column(
                         userValuesSFrame, user_id_name, singleUserValue, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_parameters_add_sframe(
                             args, "reference_data", userValuesSFrame, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    
    tc_parameters_add_int64(args, "top_k", top_k, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    tc_variant* recommendation = tc_model_call_method(
                                                      self.model, "recommend_extension_wrapper", args, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }
    assert(tc_variant_is_sframe(recommendation));
    tc_sframe *recommendation_sframe = tc_variant_sframe(
                                                         recommendation, &error_ptr);
    if (handleError(error_ptr, error)) {
        return nil;
    }

    assert(recommendation_sframe != NULL);
    assert(item_id_name != NULL);
    // Convert predictions to an id<MLFeatureProvider>
    return [[TCRecommenderOutput alloc] initWithSFrame:recommendation_sframe
                                          item_id_name:item_id_name
                                                 error:error];
}

@end

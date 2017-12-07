/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_categoricalMapping>(const Specification::Model& format) {
        auto interface = format.description();

        Result result;

        // Validate its a MLModel type.
        result = validateModelDescription(interface);
        if (!result.good()) {
            return result;
        }
        
        auto mapping_type = format.categoricalmapping().MappingType_case();
        auto defval_type = format.categoricalmapping().ValueOnUnknown_case();
        Specification::FeatureType::TypeCase requiredInputType;
        Specification::FeatureType::TypeCase requiredOutputType;
        
        switch(mapping_type) {
            case Specification::CategoricalMapping::MappingTypeCase::kStringToInt64Map:
                
                if(defval_type == Specification::CategoricalMapping::ValueOnUnknownCase::kStrValue) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "ValueOnUnknown set to string value while mapping produces int64.");
                }
                requiredInputType = Specification::FeatureType::kStringType;
                requiredOutputType = Specification::FeatureType::kInt64Type;
                
                break;
                
            case Specification::CategoricalMapping::MappingTypeCase::kInt64ToStringMap:
                if(defval_type == Specification::CategoricalMapping::ValueOnUnknownCase::kInt64Value) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "ValueOnUnknown set to Int64 value while mapping produces string.");
                }
                requiredOutputType = Specification::FeatureType::kStringType;
                requiredInputType = Specification::FeatureType::kInt64Type;
                
                break;
                
            case Specification::CategoricalMapping::MappingTypeCase::MAPPINGTYPE_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Mapping not set.");
        }
        
        // Validate the inputs
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1, {requiredInputType});
        if (!result.good()) {
            return result;
        }
        
        // Validate the outputs
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {requiredOutputType});
        if (!result.good()) {
            return result;
        }
        
        // Validate the parameters
        return result;
    }
}

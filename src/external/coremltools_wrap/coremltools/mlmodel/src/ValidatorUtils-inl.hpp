//
//  ValidatorUtils.hpp
//  libmlmodelspec
//
//  Created by Srikrishna Sridhar on 11/10/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#ifndef ValidatorUtils_h
#define ValidatorUtils_h

#include "Comparison.hpp"
#include "Format.hpp"
#include "Result.hpp"
#include "../build/format/FeatureTypes_enums.h"
#include <sstream>

namespace CoreML {

    enum WeightParamType {
        FLOAT32, // float32 weights
        FLOAT16, // float16 weights
        QUINT,   // smaller or equal to 8-bit unsigned integer
        UNSPECIFIED, // More then one type specified
        EMPTY // No populated fields
    };

    // Returns true if the weight params object has only a single type encoded in it
    inline bool checkSingleWeightType(const Specification::WeightParams &param) {
        int numFilledIn = 0;
        if (param.floatvalue_size() > 0)
            numFilledIn++;
        if (param.float16value().size() > 0)
            numFilledIn++;
        if (param.rawvalue().size() > 0)
            numFilledIn++;

        return (numFilledIn == 1);
    }

    inline int numberOfWeightType(const Specification::WeightParams &param) {
        int numFilledIn = 0;
        if (param.floatvalue_size() > 0)
            numFilledIn++;
        if (param.float16value().size() > 0)
            numFilledIn++;
        if (param.rawvalue().size() > 0)
            numFilledIn++;

        return numFilledIn;
    }

    inline WeightParamType valueType(const Specification::WeightParams &param) {
        int nw = numberOfWeightType(param);
        // Ensure only one field is set
        if (nw > 1){
            return UNSPECIFIED;
        }
        if (nw == 0){
            return EMPTY;
        }
        if (param.floatvalue_size() > 0) {
            return FLOAT32;
        } else if (param.float16value().size() > 0) {
            return FLOAT16;
        } else if (param.rawvalue().size() > 0 && param.has_quantization()){
            return QUINT;
        }
        return EMPTY;
    }

    /*
     * Utility that make sures the feature types are valid.
     *
     * @param  allowedFeatureTypes Allowed feature types.
     * @param featureType type of this operation.
     * @retun
     */
    inline Result validateSchemaTypes(const std::vector<Specification::FeatureType::TypeCase>& allowedFeatureTypes,
                 const Specification::FeatureDescription& featureDesc) {

        // Check the types
        auto type = featureDesc.type().Type_case();
        for (const auto& t : allowedFeatureTypes) {
            if (type == t) {
                // no invariant broken -- type matches one of the allowed types
                return Result();
            }
        }

        // Invalid type
        std::stringstream out;
        out << "Unsupported type \"" << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(featureDesc.type().Type_case()))
        << "\" for feature \"" << featureDesc.name() + "\". Should be one of: ";
        bool isFirst = true;
        for (const auto& t: allowedFeatureTypes) {
            if (!isFirst) {
                out << ", ";
            }
            out << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(t));
            isFirst = false;
        }
        out << "." << std::endl;
        return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE, out.str());
    }

    /*
     * Utility that checks all feature types are vectorizable
     */
    template <typename Descriptions>
    inline Result validateDescriptionsAreAllVectorizableTypes(const Descriptions &features) {
        Result result;
        for (int i = 0; i < features.size(); i++) {
            result = validateSchemaTypes({
                Specification::FeatureType::kDoubleType,
                Specification::FeatureType::kInt64Type,
                Specification::FeatureType::kMultiArrayType,
            }, features[i]);
            if (!result.good()) {
                return result;
            }
        }

        return result;
    }

    /*
     * Utility that checks a set of descriptions to validate
     * there is a feature with a specific name and type in an allowed set
     */
    template <typename Descriptions>
    inline Result validateDescriptionsContainFeatureWithTypes(const Descriptions &features,
                                                              int maxFeatureCount,
                                                              const std::vector<Specification::FeatureType::TypeCase>& allowedFeatureTypes) {
        Result result;

        // 0 means no maximum fixed feature count.
        if (maxFeatureCount != 0 && features.size() > maxFeatureCount) {
            return Result(ResultType::TOO_MANY_FEATURES_FOR_MODEL_TYPE, "Feature descriptions exceeded " + std::to_string(maxFeatureCount));
        }

        for (int i = 0; i < features.size(); i++) {
            result = validateSchemaTypes(allowedFeatureTypes, features[i]);
            if (!result.good()) {
                return result;
            }
        }

        return result;
    }

    /*
     * Utility that checks a set of descriptions to validate
     * there is a feature with a specific name and type in an allowed set
     */
    template <typename Descriptions>
    inline Result validateDescriptionsContainFeatureWithNameAndType(const Descriptions &features,
                                                                    const std::string &name,
                                                                    const std::vector<Specification::FeatureType::TypeCase>& allowedFeatureTypes) {
        Result result;
        for (int i = 0; i < features.size(); i++) {
            if (name.compare(features[i].name()) != 0) {
                continue;
            }
            return validateSchemaTypes(allowedFeatureTypes, features[i]);
        }

        return Result(ResultType::INTERFACE_FEATURE_NAME_MISMATCH, "Expected feature '" + name + "' to the model is not present in the model description.");
    }


    static inline int getWeightParamSize(const Specification::WeightParams& weights) {
        WeightParamType paramValueType = valueType(weights);
        switch (paramValueType) {
                case FLOAT32:
                return  weights.floatvalue_size();
                case FLOAT16:
                return (static_cast<int>(weights.float16value().size() / 2));
                case EMPTY:
                case UNSPECIFIED:
            default:
                break;
        }
        return 0;

    };

    static inline int getWeightParamSizeInBytes(const Specification::WeightParams& weights) {
        WeightParamType paramValueType = valueType(weights);
        switch (paramValueType) {
            case FLOAT32:
                return static_cast<int>(weights.floatvalue_size() * (int)sizeof(float));
            case FLOAT16:
                return (static_cast<int>(weights.float16value().size()));
            case QUINT:
                return static_cast<int>(weights.rawvalue().size());
            case EMPTY:
            case UNSPECIFIED:
            default:
                break;
        }
        return 0;
    };

    Result validateSizeRange(const Specification::SizeRange & range);

}
#endif /* ValidatorUtils_h */

//
//  BayesianProbitRegressionValidator.cpp
//  mlmodelspec
//
//  Created by Roman Holenstein on 7/18/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    // TODO: make enumgen generate this when compiling .proto files
    static const char * MLArrayDataType_Name(MLArrayDataType x) {
        switch (x) {
            case MLArrayDataTypeINVALID_ARRAY_DATA_TYPE:
                return "INVALID";
            case MLArrayDataTypeFLOAT32:
                return "MLArrayDataTypeFLOAT32";
            case MLArrayDataTypeDOUBLE:
                return "MLArrayDataTypeDOUBLE";
            case MLArrayDataTypeINT32:
                return "MLArrayDataTypeINT32";
        }
    }

    inline Result validateSchemaTypesIsMultiArray(const Specification::FeatureDescription& featureDesc,
                                           Specification::ArrayFeatureType_ArrayDataType allowedArrayFeatureDataType,
                                           int shapeSizeMin = 1,
                                           int shapeSizeMax = INT_MAX) {
        // Check the types
        auto& type = featureDesc.type();
        if (!type.has_multiarraytype() || type.Type_case() != Specification::FeatureType::kMultiArrayType) {
            // Invalid type
            std::stringstream out;
            out << "Unsupported type \"" << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(featureDesc.type().Type_case()))
            << "\" for feature \"" << featureDesc.name() + "\". "
            << "Should be of: " << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(Specification::FeatureType::kMultiArrayType))
            << " with data type of: " << MLArrayDataType_Name(static_cast<MLArrayDataType>(allowedArrayFeatureDataType))
            << "." << std::endl;
            return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE, out.str());
        }

        if (type.multiarraytype().datatype() != allowedArrayFeatureDataType) {
            std::stringstream out;
            out << "Unsupported array type \"" << MLArrayDataType_Name(static_cast<MLArrayDataType>(type.multiarraytype().datatype()))
            << "\" for feature \"" << featureDesc.name() + "\". "
            << "Should be of: " << MLArrayDataType_Name(static_cast<MLArrayDataType>(allowedArrayFeatureDataType))
            << "." << std::endl;
            return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE, out.str());
        }

        int shapeSize = type.multiarraytype().shape_size();
        if (shapeSize<shapeSizeMin || shapeSizeMax < shapeSize) {
            std::stringstream out;
            out << "Unsupported array rank " << shapeSize
            << " should be in range [" << shapeSizeMin << "," << shapeSizeMax << "]"
            << "." << std::endl;
            return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE, out.str());
        }

        // no invariant broken -- type matches one of the allowed types
        return Result();
    }

    template <>
    Result validate<MLModelType_bayesianProbitRegressor>(const Specification::Model& format) {
        Result result;
        const auto& description = format.description();
        const auto& inputFeatures = description.input();
        for (int i = 0; i < inputFeatures.size(); i++) {
            const auto& featureDesc = inputFeatures[i];
            result = validateSchemaTypesIsMultiArray(featureDesc, Specification::ArrayFeatureType_ArrayDataType_INT32, 1, 2);
            if (!result.good()) {
                return result;
            }
        }

        return result;
    }

}

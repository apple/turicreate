//
//  SoundAnalysisPreprocessingValidator.cpp
//  mlmodel
//
//  Created by Tao Jia on 4/3/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#include "../Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../../build/format/Model.pb.h"

namespace CoreML {
    
    constexpr int FRONTEND_PROCESSING_INPUT_LENGTH = 15600;
    constexpr int FRONTEND_PROCESSING_OUTPUT_NUM_FRAMES = 96;
    constexpr int FRONTEND_PROCESSING_OUTPUT_NUM_BANDS = 64;

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

    template <>
    Result validate<MLModelType_soundAnalysisPreprocessing>(const Specification::Model &format) {
        const auto &interface = format.description();
        
        // make sure model is a sound analysis preprocessing
        if (!format.has_soundanalysispreprocessing()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model not a sound analysis preprocessing.");
        }
        
        Result result;
        
        // validate the inputs: only one input with multiarray type is allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1, {Specification::FeatureType::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

        // validate the outputs: only one output with multiarray type is allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {Specification::FeatureType::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

        // other validate logics here
        const auto &soundAnalysisPreprocessing = format.soundanalysispreprocessing();
        switch (soundAnalysisPreprocessing.SoundAnalysisPreprocessingType_case()) {
            case Specification::CoreMLModels::SoundAnalysisPreprocessing::kVggish:
            {
                // validate input shape for VGGish preprocessing
                auto arrayInputShape = interface.input(0).type().multiarraytype().shape();
                if (arrayInputShape.size() != 1 || arrayInputShape[0] != FRONTEND_PROCESSING_INPUT_LENGTH) {
                    std::stringstream out;
                    out << "Incorrect input shape, should be 1-dimension, of length: "<<FRONTEND_PROCESSING_INPUT_LENGTH<<std::endl;
                    return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE, out.str());
                }

                // validate input data type
                auto inputType = interface.input(0).type();
                if (inputType.multiarraytype().datatype() != Specification::ArrayFeatureType_ArrayDataType_FLOAT32) {
                    std::stringstream out;
                    out << "Unsupported array type \"" << MLArrayDataType_Name(static_cast<MLArrayDataType>(inputType.multiarraytype().datatype()))
                    << "\" for feature \"" << interface.input(0).name() + "\". "
                    << "Should be of: " << MLArrayDataType_Name(static_cast<MLArrayDataType>(Specification::ArrayFeatureType_ArrayDataType_FLOAT32))
                    << "." << std::endl;
                    return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE, out.str());
                }

                // validate output shape
                auto arrayOutputShape = interface.output(0).type().multiarraytype().shape();
                if (arrayOutputShape.size() != 3 || arrayOutputShape[0] != 1
                    || arrayOutputShape[1] != FRONTEND_PROCESSING_OUTPUT_NUM_FRAMES
                    || arrayOutputShape[2] != FRONTEND_PROCESSING_OUTPUT_NUM_BANDS) {
                    std::stringstream out;
                    out << "Incorrect output shape, should be 3-dimension, of size: "<<1<<"x"<<FRONTEND_PROCESSING_OUTPUT_NUM_FRAMES<<"x"<<FRONTEND_PROCESSING_OUTPUT_NUM_BANDS<<std::endl;
                    return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE, out.str());
                }

                // validate output data type
                auto outputType = interface.output(0).type();
                if (outputType.multiarraytype().datatype() != Specification::ArrayFeatureType_ArrayDataType_FLOAT32) {
                    std::stringstream out;
                    out << "Unsupported array type \"" << MLArrayDataType_Name(static_cast<MLArrayDataType>(outputType.multiarraytype().datatype()))
                    << "\" for feature \"" << interface.input(0).name() + "\". "
                    << "Should be of: " << MLArrayDataType_Name(static_cast<MLArrayDataType>(Specification::ArrayFeatureType_ArrayDataType_FLOAT32))
                    << "." << std::endl;
                    return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE, out.str());
                }

                break;
            }
            case Specification::CoreMLModels::SoundAnalysisPreprocessing::SOUNDANALYSISPREPROCESSINGTYPE_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Type for sound analysis preprocessing not set");
        }
        
        return result;
    }
    
}

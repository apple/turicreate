//
//  VisionFeaturePrintValidator.cpp
//  mlmodel
//
//  Created by Tao Jia on 4/3/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_visionFeaturePrint>(const Specification::Model &format) {
        const auto &interface = format.description();

        // make sure model is a vision feature print
        if (!format.has_visionfeatureprint()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model not a vision feature print.");
        }

        Result result;

        // validate the inputs: only one input with image type is allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1, {Specification::FeatureType::kImageType});
        if (!result.good()) {
            return result;
        }

        // validate the outputs: only one output with multiarray type is allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {Specification::FeatureType::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

        // other validate logics here
        const auto &visionFeaturePrint = format.visionfeatureprint();
        switch (visionFeaturePrint.VisionFeaturePrintType_case()) {
            case Specification::CoreMLModels::VisionFeaturePrint::kScene:
                if (visionFeaturePrint.scene().version() == Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion_SCENE_VERSION_INVALID) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Version for scene is invalid");
                }
                break;
            case Specification::CoreMLModels::VisionFeaturePrint::VISIONFEATUREPRINTTYPE_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Type for vision feature print not set");
        }

        return result;
    }

}

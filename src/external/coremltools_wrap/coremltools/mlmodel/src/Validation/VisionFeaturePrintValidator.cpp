//
//  VisionFeaturePrintValidator.cpp
//  mlmodel
//
//  Created by Tao Jia on 4/3/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

#include "../Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../../build/format/Model.pb.h"

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

        // other validate logics here
        const auto &visionFeaturePrint = format.visionfeatureprint();
        switch (visionFeaturePrint.VisionFeaturePrintType_case()) {
            case Specification::CoreMLModels::VisionFeaturePrint::kScene:
                if (visionFeaturePrint.scene().version() == Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion_SCENE_VERSION_INVALID) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Version for scene is invalid");
                }

                if (visionFeaturePrint.scene().version() == Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion_SCENE_VERSION_1) {
                    // validate the outputs: only one output with multiarray type is allowed for version 1
                    result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {Specification::FeatureType::kMultiArrayType});
                    if (!result.good()) {
                        return result;
                    }
                }
                break;
            case Specification::CoreMLModels::VisionFeaturePrint::kObjects:
                if (visionFeaturePrint.objects().version() == Specification::CoreMLModels::VisionFeaturePrint_Objects_ObjectsVersion_OBJECTS_VERSION_INVALID) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Version for objects is invalid");
                }
                if (visionFeaturePrint.objects().version() == Specification::CoreMLModels::VisionFeaturePrint_Objects_ObjectsVersion_OBJECTS_VERSION_1) {

                    if (visionFeaturePrint.objects().output_size() != 2) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, "Two outputs for objects need to be provided");
                    }

                    // validate the outputs: only two outputs with multiarray type is allowed for version 1
                    result = validateDescriptionsContainFeatureWithTypes(interface.output(), 2, {Specification::FeatureType::kMultiArrayType});
                    if (!result.good()) {
                        return result;
                    }
                }
                for (auto modelOutputFeature : interface.output()) {
                    const std::string &modelOutputFeatureName = modelOutputFeature.name();
                    const auto &visionFeaturePrintOutputNames = visionFeaturePrint.objects().output();
                    if (find(visionFeaturePrintOutputNames.begin(), visionFeaturePrintOutputNames.end(), modelOutputFeatureName) == visionFeaturePrintOutputNames.end()) {
                        std::stringstream ss;
                        ss << "Model description declares an output: " << modelOutputFeatureName << " but it is not declared in Vision Feature Print output";
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, ss.str());
                    }
                }
                break;
            case Specification::CoreMLModels::VisionFeaturePrint::VISIONFEATUREPRINTTYPE_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Type for vision feature print not set");
        }

        return result;
    }

}

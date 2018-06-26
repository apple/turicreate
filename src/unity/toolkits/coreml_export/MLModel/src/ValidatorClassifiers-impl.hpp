/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef Validator_Classifier_Impl_h
#define Validator_Classifier_Impl_h

#include "Result.hpp"
#include "Format.hpp"

#include "unity/toolkits/coreml_export/protobuf_include_internal.hpp"

namespace CoreML {


    /*
     * Validate model interface describes a valid classifier
     *
     * @param  interface Model interface
     * @return Result type of this operation.
     */
    template<typename T, typename U>
    Result validateClassifierInterface(const T& model,
                                       const U& modelParameters) {
        // validate class labels
        switch (modelParameters.ClassLabels_case()) {
            case U::kInt64ClassLabels:
                if (modelParameters.int64classlabels().vector_size() == 0) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Classifier models must provide class labels.");
                }
                break;
            case U::kStringClassLabels:
                if (modelParameters.stringclasslabels().vector_size() == 0) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Classifier models must provide class labels.");
                }
                break;
            case U::CLASSLABELS_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Classifier models must provide class labels.");
        }
        
        const Specification::ModelDescription& interface = model.description();
        
        // Validate feature descriptions
        Result result = validateFeatureDescriptions(interface);
        if (!result.good()) {
            return result;
        }
        
        const auto& predictedFeatureName = interface.predictedfeaturename();
        const auto& probOutputName = interface.predictedprobabilitiesname();
        
        if (predictedFeatureName == "") {
            return Result(ResultType::INVALID_MODEL_INTERFACE,
                          "Specification is missing classifier predictedFeatureName");
        } else {
            result = validateDescriptionsContainFeatureWithNameAndType(interface.output(),
                                                                       predictedFeatureName,
                                                                       {Specification::FeatureType::TypeCase::kInt64Type , Specification::FeatureType::TypeCase::kStringType});
            if (!result.good()) {
                return result;
            }
        }
        
        if (probOutputName != "") {
            // TODO : validate array length below
            // and value type (must be double? different for different classifiers?)
            // TODO Probability outputs are always dictionaries!
            result = validateDescriptionsContainFeatureWithNameAndType(interface.output(),
                                                                       probOutputName,
                                                                       {Specification::FeatureType::TypeCase::kMultiArrayType, // TODO ARRAY TYPE IS INVALID, REMOVE
                                                                        Specification::FeatureType::TypeCase::kDictionaryType});
            if (!result.good()) {
                return result;
            }
        }
        
        return Result();
    }
  


}

#endif

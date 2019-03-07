//
//  WordTaggerValidator.cpp
//  mlmodel
//
//  Created by Tao Jia on 3/21/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_wordTagger>(const Specification::Model& format) {
        const auto& interface = format.description();
        const auto& inputs = interface.input();
        const auto& outputs = interface.output();

        // make sure model is a word tager
        if (!format.has_wordtagger()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model not a word tagger.");
        }

        const auto& wordTagger = format.wordtagger();

        int numNonEmptyOutputFeatures = 0;

        if (wordTagger.tokentagsoutputfeaturename().length() > 0) {
            numNonEmptyOutputFeatures++;
        }

        if (wordTagger.tokensoutputfeaturename().length() > 0) {
            numNonEmptyOutputFeatures++;
        }

        if (wordTagger.tokenlocationsoutputfeaturename().length() > 0) {
            numNonEmptyOutputFeatures++;
        }

        if (wordTagger.tokenlengthsoutputfeaturename().length() > 0) {
            numNonEmptyOutputFeatures++;
        }

        if (outputs.size() != numNonEmptyOutputFeatures) {
            return Result(ResultType::TOO_MANY_FEATURES_FOR_MODEL_TYPE,
                          "More model output features than the output features of the word tagger model.");
        }

        int tokensOutputIndex = -1;
        int tokenTagsOutputIndex = -1;
        int tokenLocationsOutputIndex = -1;
        int tokenLengthsOutputIndex = -1;

        // check that any interface output can be found from word tagger outputs
        for (int i = 0; i < outputs.size(); i++) {
            const std::string name = outputs[i].name();
            bool present = false;

            if (wordTagger.tokensoutputfeaturename().compare(name) == 0) {
                tokensOutputIndex = i;
                present = true;
            }

            if (wordTagger.tokentagsoutputfeaturename().compare(name) == 0) {
                tokenTagsOutputIndex = i;
                present = true;
            }

            if (wordTagger.tokenlocationsoutputfeaturename().compare(name) == 0) {
                tokenLocationsOutputIndex = i;
                present = true;
            }

            if (wordTagger.tokenlengthsoutputfeaturename().compare(name) == 0) {
                tokenLengthsOutputIndex = i;
                present = true;
            }

            if (!present) {
                return Result(ResultType::TOO_MANY_FEATURES_FOR_MODEL_TYPE,
                              "Output feature '" + name + "' was not required by the output features of the word tagger model.");
            }
        }

        // token tags is the required output feature name, while tokens/locations/lengths are optional
        if (tokenTagsOutputIndex == -1) {
            return Result(ResultType::INTERFACE_FEATURE_NAME_MISMATCH,
                          "Expected feature '" + wordTagger.tokentagsoutputfeaturename() + "' (defined by tokenTagsOutputFeatureName) to the model is not present in the model description.");
        }

        Result result;

        // Validate the inputs: only one input with string type is allowed
        result = validateDescriptionsContainFeatureWithTypes(inputs, 1, {Specification::FeatureType::kStringType});
        if (!result.good()) {
            return result;
        }

        // Validate the outputs: only sequence type is allowed for any output
        result = validateDescriptionsContainFeatureWithTypes(outputs, outputs.size(), {Specification::FeatureType::kSequenceType});
        if (!result.good()) {
            return result;
        }

        // validate the individual output type, token tags has to be a sequence of strings
        const auto& tokenTagsOutputSequenceType = outputs[tokenTagsOutputIndex].type().sequencetype();
        if (tokenTagsOutputSequenceType.Type_case() != Specification::SequenceFeatureType::kStringType) {
            std::stringstream out;
            out << "Unsupported type \"" << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(tokenTagsOutputSequenceType.Type_case()))
            << "\" for feature \"" << wordTagger.tokentagsoutputfeaturename() + "\". Should be: "
            << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(Specification::SequenceFeatureType::kStringType));

            return Result(ResultType::FEATURE_TYPE_INVARIANT_VIOLATION, out.str());
        }

        // validate the individual output type, tokens (if present) has to be a sequence of strings
        if (tokensOutputIndex != -1) {
            const auto& tokensOutputSequenceType = outputs[tokensOutputIndex].type().sequencetype();
            if (tokensOutputSequenceType.Type_case() != Specification::SequenceFeatureType::kStringType) {
                std::stringstream out;
                out << "Unsupported type \"" << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(tokensOutputSequenceType.Type_case()))
                << "\" for feature \"" << wordTagger.tokensoutputfeaturename() + "\". Should be: "
                << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(Specification::SequenceFeatureType::kStringType));

                return Result(ResultType::FEATURE_TYPE_INVARIANT_VIOLATION, out.str());
            }
        }

        // validate the individual output type, token locations (if present) has to be a sequence of integers
        if (tokenLocationsOutputIndex != -1) {
            const auto& tokenLocationsOutputSequenceType = outputs[tokenLocationsOutputIndex].type().sequencetype();
            if (tokenLocationsOutputSequenceType.Type_case() != Specification::SequenceFeatureType::kInt64Type) {
                std::stringstream out;
                out << "Unsupported type \"" << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(tokenLocationsOutputSequenceType.Type_case()))
                << "\" for feature \"" << wordTagger.tokenlocationsoutputfeaturename() + "\". Should be: "
                << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(Specification::SequenceFeatureType::kInt64Type));

                return Result(ResultType::FEATURE_TYPE_INVARIANT_VIOLATION, out.str());
            }
        }

        // validate the individual output type, token lengths (if present) has to be a sequence of integers
        if (tokenLengthsOutputIndex != -1) {
            const auto& tokenLengthsOutputSequenceType = outputs[tokenLengthsOutputIndex].type().sequencetype();
            if (tokenLengthsOutputSequenceType.Type_case() != Specification::SequenceFeatureType::kInt64Type) {
                std::stringstream out;
                out << "Unsupported type \"" << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(tokenLengthsOutputSequenceType.Type_case()))
                << "\" for feature \"" << wordTagger.tokenlengthsoutputfeaturename() + "\". Should be: "
                << MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(Specification::SequenceFeatureType::kInt64Type));

                return Result(ResultType::FEATURE_TYPE_INVARIANT_VIOLATION, out.str());
            }
        }

        // Validate the model parameters
        if (wordTagger.revision() == 0) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model revision number not set. Must be >= 1");
        }

        int numTags;
        switch (wordTagger.Tags_case()) {
            case Specification::CoreMLModels::WordTagger::kStringTags:
                numTags = wordTagger.stringtags().vector_size();
                break;
            case Specification::CoreMLModels::WordTagger::TAGS_NOT_SET:
                numTags = -1;
                break;
        }

        if (numTags <= 0) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model output tags not set. Must have at least one tag");
        }

        if (wordTagger.modelparameterdata().empty()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model parameter data not set");
        }

        return result;
    }
}

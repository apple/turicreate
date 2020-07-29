    //
    //  ItemSimilarityRecommenderValidator.hpp
    //  CoreML_framework
    //
    //  Created by Hoyt Koepke on 1/29/19.
    //  Copyright © 2019 Apple Inc. All rights reserved.
    //

#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../ItemSimilarityRecommenderCommon.hpp"

namespace CoreML {

    template <>
    Result validate<MLModelType_itemSimilarityRecommender>(const Specification::Model& format) {
        const auto& interface = format.description();

        Result result;

            // Validate its a MLModel type.
        result = validateModelDescription(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        try {
            Recommender::constructAndValidateItemSimilarityRecommenderFromSpec(format);

        } catch (const std::exception& error) {
            return Result(ResultType::INVALID_MODEL_INTERFACE, error.what());
        }

        return Result();
    }
}

//
//  TreeEnsembleValidator.cpp
//  libmlmodelspec
//
//  Created by Zach Nation on 12/21/16.
//  Copyright Â© 2016 Apple. All rights reserved.
//

#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "TreeEnsembleCommon.hpp"
#include "transforms/TreeEnsemble.hpp"

namespace CoreML {

    template <>
    Result validate<MLModelType_treeEnsembleClassifier>(const Specification::Model& format) {
        try {
            TreeEnsembles::constructAndValidateTreeEnsembleFromSpec(format);

        } catch (const std::logic_error& error) {
            return Result(ResultType::INVALID_MODEL_INTERFACE, error.what());
        }

        return Result();
    }

    template <>
    Result validate<MLModelType_treeEnsembleRegressor>(const Specification::Model& format) {
        try {
            TreeEnsembles::constructAndValidateTreeEnsembleFromSpec(format);

        } catch (const std::logic_error& error) {
            return Result(ResultType::INVALID_MODEL_INTERFACE, error.what());
        }

        return Result();
    }

}

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
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

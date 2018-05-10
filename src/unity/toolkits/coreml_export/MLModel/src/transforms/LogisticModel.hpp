/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef MLMODEL_LOGISTIC_MODEL_SPEC_HPP
#define MLMODEL_LOGISTIC_MODEL_SPEC_HPP

#include "../Export.hpp"
#include "../Result.hpp"
#include "../Model.hpp"

namespace CoreML {

/**
 * Reader/Writer interface for a GLM.
 *
 * A construction class that, in the end, outputs a properly constructed
 * specification that is gauranteed to load in an LinearModelSpec class.
 *
 */
class EXPORT LogisticModel : public Model {
  public:

    LogisticModel(const std::string& predictedClassOutputName,
                  const std::string& classProbabilityOutputName,
                  const std::string& description = "");

    LogisticModel(const Model &model);

    /**
     * Set the weights.
     *
     * @param weights Two dimensional vector of doulbes.
     * @return Result type with errors.
     */
    Result setWeights(const std::vector< std::vector<double> >& weights);

    /**
     * Set the offsets/intercepts.
     *
     * @param offsets The offsets or intercepts
     * @return Result type with errors.
     */
    Result setOffsets(const std::vector<double>& offsets);

    /**
     * Set the classes names
     *
     * @param classes Class values
     * @return Result type with errors.
     */
    Result setClassNames(const std::vector<std::string>& classes);

    /**
     * Set the classes names
     *
     * @param classes Class values
     * @return Result type with errors.
     */
    Result setClassNames(const std::vector<int64_t>& classes);
};
}

#endif

/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef MLMODEL_LINEAR_MODEL_SPEC_HPP
#define MLMODEL_LINEAR_MODEL_SPEC_HPP

#include "../Result.hpp"
#include "../Model.hpp"

namespace CoreML {

/**
 * Reader/Writer interface for a GLM.
 *
 * A construction class that, in the end, outputs a properly constructed
 * specification that is gauranteed to load in an TreeEnsemble class.
 *
 */
class EXPORT LinearModel : public Model {
  public:

    LinearModel(const std::string& predictedValueOutput,
                    const std::string& description);

    LinearModel(const Model &model);
    
    /**
     * Set the weights.
     *
     * @param weights Two dimensional vector of doulbes.
     * @return Result type with errors.
     */
    Result setWeights(std::vector< std::vector<double>> weights);

    /**
     * Set the offsets/intercepts.
     *
     * @param offsets The offsets or intercepts
     * @return Result type with errors.
     */
    Result setOffsets(std::vector<double> offsets);
    
    /**
     * Get offsets/intercepts.
     *
     * @return offsets.
     */
    std::vector<double> getOffsets();

    /**
     * Get weights.
     *
     * @return weights.
     */
    std::vector< std::vector<double>> getWeights();
    
};
}

#endif

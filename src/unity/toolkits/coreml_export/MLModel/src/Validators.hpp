/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef Validator_h
#define Validator_h

#include "Result.hpp"
#include "../build/format/Model_enums.h"

namespace CoreML {

    namespace Specification {
        class Model;
        class ModelDescription;
        class Metadata;
        class Kernel;
    }

    /*
     * Template specialization of validation of the protobuf.
     *
     * @param  format Model spec format.
     * @return Result type of this operation.
     */
    template <MLModelType T> Result validate(const Specification::Model& format);


    /*
     * Validate feature descriptions in interface have supported names and type info
     *
     * @param  interface Model interface
     * @return Result type of this operation.
     */
    Result validateFeatureDescriptions(const Specification::ModelDescription& interface);

    /*
     * Validate model interface describes a valid transform
     *
     * @param  interface Model interface
     * @return Result type of this operation.
     */
    Result validateModelDescription(const Specification::ModelDescription& interface);

    /*
     * Validate model interface describes a valid regressor
     *
     * @param  interface Model interface
     * @return Result type of this operation.
     */
    Result validateRegressorInterface(const Specification::ModelDescription& interface);
    
    
    
    /*
     * Validate model interface describes a valid classifier
     *
     * @param  interface Model interface
     * @return Result type of this operation.
     */
    template<typename T, typename U>
    Result validateClassifierInterface(const T& model,
                                       const U& modelParameters);
   
    /*
     * Validate optional inputs/outputs.
     * For most models, optional is not allowed (all inputs/outputs required).
     * Some models have different behavior.
     */
    Result validateOptional(const Specification::Model& format);

}
#endif /* Validator_h */

//
//  ResultType.hpp
//  CoreML
//
//  Created by Jeff Kilpatrick on 12/16/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#pragma once

namespace CoreML {

enum class ResultType {
    NO_ERROR,

    // Feature type of transform doesn't match target type of a prior
    // transform, i.e. one-hot encoding the output of a linear regressor.
    TYPE_MISMATCH,

    // Feature type of a transform violates invariant type conditions,
    // i.e. passing a dictionary to a linear regressor.
    FEATURE_TYPE_INVARIANT_VIOLATION,

    // File I/O errors
    UNABLE_TO_OPEN_FILE,
    FAILED_TO_SERIALIZE,
    FAILED_TO_DESERIALIZE,

    // Invalid protobuf file (internally inconsistent)
    INVALID_COMPATIBILITY_VERSION,
    UNSUPPORTED_COMPATIBILITY_VERSION,
    UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE,
    TOO_MANY_FEATURES_FOR_MODEL_TYPE,
    INTERFACE_FEATURE_NAME_MISMATCH,
    INTERFACE_MODEL_PARAMETER_MISMATCH,
    INVALID_MODEL_INTERFACE,

    // Invalid protobuf model parameters
    INVALID_MODEL_PARAMETERS,

    // Invalid updatable parameters
    INVALID_UPDATABLE_MODEL_PARAMETERS,
    INVALID_UPDATABLE_MODEL_CONFIGURATION,

    // NN shaper failure, not necessarily an error
    POTENTIALLY_INVALID_NEURAL_NETWORK_SHAPES
};

}

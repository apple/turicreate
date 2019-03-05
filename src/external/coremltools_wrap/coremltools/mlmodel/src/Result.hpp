#ifndef MLMODEL_RESULT_HPP
#define MLMODEL_RESULT_HPP

#include "DataType.hpp"

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

    // NN shaper failure, not necessarily an error
    POTENTIALLY_INVALID_NEURAL_NETWORK_SHAPES
};

class Result {

  private:
    ResultType m_type = ResultType::NO_ERROR;
    std::string m_message;

  public:
    Result();
    bool good() const;
    Result(ResultType type, const std::string& message);
    const std::string& message() const;
    const ResultType& type() const;

    static Result typeMismatchError(
        FeatureType expected,
        FeatureType actual,
        const std::string& parameterName);

    static Result featureTypeInvariantError(
        const std::vector<FeatureType>& allowed,
        FeatureType actual);


    bool operator==(const Result& other) const;
    bool operator!=(const Result& other) const;
};
}

// TODO -- Should we make this throw a C++ exception instead of result type?

/*
 * A convenience macro to pass results onto the caller. Useful when a function
 * both returns a Result and calls other functions that return a Result, and
 * the desired behavior is an early exit in the case of a failure.
*/
#define HANDLE_RESULT_AND_RETURN_ON_ERROR(EXPR)                                \
  {                                                                            \
    Result r = EXPR;                                                           \
    if (!r.good()) {                                                           \
      return r;                                                                \
    }                                                                          \
  }                                                                            \

#endif

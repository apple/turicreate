//
//  SVMValidator
//  libmlmodelspec
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    Result validateKernel(const Specification::Kernel& kernel);

    template <>
    Result validate<MLModelType_supportVectorClassifier>(const Specification::Model& format) {

        const Specification::ModelDescription& description = format.description();

        Result result;

        // Check that inputs are vectorizable
        result = validateDescriptionsAreAllVectorizableTypes(description.input());
        if (!result.good()) {
            return result;
        }

        // Check has a classifier interface
        result = validateClassifierInterface(format, format.supportvectorclassifier());
        if (!result.good()) {
            return result;
        }

        // Validate the params
        const Specification::SupportVectorClassifier& svmSpec = format.supportvectorclassifier();

        result = validateKernel(svmSpec.kernel());
        if (!result.good()) {
            return result;
        }

        // Validate number of coefficients and support vectors are consistent
        int K;
        switch (svmSpec.ClassLabels_case()) {
            case Specification::SupportVectorClassifier::kInt64ClassLabels:
                K = svmSpec.int64classlabels().vector_size();
                break;
            case Specification::SupportVectorClassifier::kStringClassLabels:
                K = svmSpec.stringclasslabels().vector_size();
                break;
            case Specification::SupportVectorClassifier::CLASSLABELS_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Int64 class labels must be supplied for SVM classifier.");
        }

        if (svmSpec.coefficients_size() != K-1) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "coefficient array must be size numberOfClasses - 1 (" +
                                              std::to_string(K-1) +
                                              "). Instead it is size " +
                                              std::to_string(svmSpec.coefficients_size()));
        }

        int totalSVs = 0;
        switch (svmSpec.supportVectors_case()) {
            case Specification::SupportVectorClassifier::kSparseSupportVectors:
                totalSVs = svmSpec.sparsesupportvectors().vectors_size();
                break;
            case Specification::SupportVectorClassifier::kDenseSupportVectors:
                totalSVs = svmSpec.densesupportvectors().vectors_size();
                break;
            default:
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Must specify sparse or dense support vectors");
        }

        if(svmSpec.numberofsupportvectorsperclass_size() != K) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "numberOfSupportVectoresPerClass array must be size numberOfClasses " + std::to_string(K) + " instead it is size "
                            +   std::to_string(svmSpec.numberofsupportvectorsperclass_size()));
        }

        int perClassSum = 0;
        for (int c=0; c<K; c++) {
            perClassSum += svmSpec.numberofsupportvectorsperclass(c);
        }

        if (totalSVs != perClassSum) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,

                        "sum of numberOfSupportVectorsPerClass "
                          + std::to_string(perClassSum)
                          + " must sum to total number of support vectors "
                          + std::to_string(totalSVs));
        }

        for (int c=0; c<K-1; c++) {
            if (svmSpec.coefficients(c).alpha_size() != totalSVs) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Incorrect number of coefficients: There should be " +
                                                  std::to_string(totalSVs) +
                                                  " not " +
                                                  std::to_string(svmSpec.coefficients(c).alpha_size()));
            }
        }

        // Validate probA and probB, if given
        if(svmSpec.proba_size() != 0 || svmSpec.probb_size() != 0) {
            int expected_length = K * (K-1) / 2;      // Number of class pairs
            if(svmSpec.proba_size() != svmSpec.probb_size()) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "probA and probB must be same size");
            } else if (svmSpec.proba_size() != expected_length) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Expected length of probA is number of class pairs: " + std::to_string(expected_length));
            }
        }

        return result;
    }

    template <>

    Result validate<MLModelType_supportVectorRegressor>(const Specification::Model& format) {

        Result result;
        const Specification::ModelDescription& description = format.description();

        // Check that inputs are vectorizable
        result = validateDescriptionsAreAllVectorizableTypes(description.input());
        if (!result.good()) {
            return result;
        }

        // Must have a regressor interface (since GLMRegressor is an MLRegressor)
        result = validateRegressorInterface(description, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        const Specification::SupportVectorRegressor& svmSpec = format.supportvectorregressor();

        result = validateKernel(svmSpec.kernel());
        if (!result.good()) {
            return result;
        }

        // Coefficient size must match the size of all support vectors
        int totalSVs = 0;
        switch (svmSpec.supportVectors_case()) {
            case Specification::SupportVectorRegressor::kSparseSupportVectors:
                totalSVs = svmSpec.sparsesupportvectors().vectors_size();
                break;
            case Specification::SupportVectorRegressor::kDenseSupportVectors:
                totalSVs = svmSpec.densesupportvectors().vectors_size();
                break;
            default:
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Must specify sparse or dense support vectors");
        }

        if(svmSpec.coefficients().alpha_size() != totalSVs) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "The number of coefficients must match the number of support vectors.");
        }

        return result;

    }

    Result validateKernel(const Specification::Kernel& kernel) {
        switch (kernel.kernel_case()) {
            case Specification::Kernel::kPolyKernel:
                if(kernel.polykernel().gamma() < 0)
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Gamma must be greater than or equal to zero");
                if(kernel.polykernel().degree() < 0)
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Degree must be greater than or equal to zero");
            case Specification::Kernel::kRbfKernel:
                if(kernel.rbfkernel().gamma() < 0)
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Gamma must be greater than or equal to zero");
                break;
            case Specification::Kernel::kSigmoidKernel:
                if(kernel.sigmoidkernel().gamma() < 0)
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Gamma must be greater than or equal to zero");
            case Specification::Kernel::kLinearKernel:
                break;
            default:
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "You must specify a supported kernel type");
        }
        return Result();
    }
}

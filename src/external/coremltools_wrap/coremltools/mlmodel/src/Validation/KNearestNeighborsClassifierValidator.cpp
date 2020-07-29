//
//  KNearestNeighborsClassifierValidator.cpp
//  CoreML_framework
//
//  Created by Bill March on 10/4/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

#include "../../build/format/NearestNeighbors_enums.h"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "ParameterValidator.hpp"

#include <algorithm>
#include <sstream>

namespace CoreML {

    static Result validateNearestNeighborsIndex(const Specification::Model& format, int expectedSampleCount) {

        const Specification::NearestNeighborsIndex& nnIndex = format.knearestneighborsclassifier().nearestneighborsindex();

        // A valid index should have some data points
        if (nnIndex.floatsamples_size() == 0 && expectedSampleCount != 0) {
            std::stringstream out;
            out << "KNearestNeighborsClassifier has no data points." << std::endl;
            return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
        }

        if (nnIndex.floatsamples_size() != expectedSampleCount) {
            std::stringstream out;
            out << "Unexpected number of labels \"" << expectedSampleCount << "\" for the given number of examples: \"" << nnIndex.floatsamples_size() << "." << std::endl;
            return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
        }

        // Only need to check that the length of the individual vectors are equivalent to the dimensionality (and thus eachother)
        for (int i = 0; i < nnIndex.floatsamples_size(); i++) {
            if (nnIndex.floatsamples(i).vector_size() != nnIndex.numberofdimensions()) {
                std::stringstream out;
                out << "Unexpected length \"" << nnIndex.floatsamples_size() << "\" given the provided number of dimensions \"" << nnIndex.numberofdimensions() << "." << std::endl;
                return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
            }
        }

        // Should we require the user to always specify an index type?
        bool hasLinearBackend = nnIndex.has_linearindex();
        bool hasKdTreeBackend = nnIndex.has_singlekdtreeindex();
        if (!hasLinearBackend && !hasKdTreeBackend) {
            std::stringstream out;
            out << "KNearestNeighborsClassifier has no index type specified." << std::endl;
            return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
        }

        if (hasKdTreeBackend) {
            if (nnIndex.singlekdtreeindex().leafsize() <= 0) {
                std::stringstream out;
                out << "KNearestNeighborsClassifier requires leaf size to be a positive integer." << std::endl;
                return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
            }
        }

        switch (nnIndex.DistanceFunction_case()) {
            case Specification::NearestNeighborsIndex::kSquaredEuclideanDistance:
                // Valid distance function
                break;

            case Specification::NearestNeighborsIndex::DISTANCEFUNCTION_NOT_SET:
                std::stringstream out;
                out << "KNearestNeighborsClassifier requires a distance function to be set." << std::endl;
                return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
        }

        return Result();

    }

    template <>
    Result validate<MLModelType_kNearestNeighborsClassifier>(const Specification::Model& format) {

        const Specification::KNearestNeighborsClassifier& knnClassifier = format.knearestneighborsclassifier();

        Result res = validateInt64Parameter("numberOfNeighbors", knnClassifier.numberofneighbors(), true);
        if (!res.good()) {
            return res;
        }

        switch (knnClassifier.WeightingScheme_case()) {
            case Specification::KNearestNeighborsClassifier::kUniformWeighting:
            case Specification::KNearestNeighborsClassifier::kInverseDistanceWeighting:
                // Valid weighting scheme
                break;

            case Specification::KNearestNeighborsClassifier::WEIGHTINGSCHEME_NOT_SET:
                std::stringstream out;
                out << "KNearestNeighborsClassifier requires a weighting scheme to be set." << std::endl;
                return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
        }
        
        int intLabelCount = knnClassifier.has_int64classlabels() ? knnClassifier.int64classlabels().vector_size() : 0;
        int stringLabelCount = knnClassifier.has_stringclasslabels() ? knnClassifier.stringclasslabels().vector_size() : 0;
        
        int labelCount = MAX(intLabelCount, stringLabelCount);
        
        auto classLabelCase = knnClassifier.ClassLabels_case();
        auto defaultClassLabelIsInt64 = false;
        
        switch (knnClassifier.DefaultClassLabel_case()) {
            case Specification::KNearestNeighborsClassifier::kDefaultStringLabel:
                if (classLabelCase != Specification::KNearestNeighborsClassifier::CLASSLABELS_NOT_SET &&
                    classLabelCase != Specification::KNearestNeighborsClassifier::kStringClassLabels) {
                    std::stringstream out;
                    out << "KNearestNeighborsClassifier's class label and default class label have different types." << std::endl;
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
                }
                defaultClassLabelIsInt64 = false;
                break;
                    
            case Specification::KNearestNeighborsClassifier::kDefaultInt64Label:
                if (classLabelCase != Specification::KNearestNeighborsClassifier::CLASSLABELS_NOT_SET &&
                    classLabelCase != Specification::KNearestNeighborsClassifier::kInt64ClassLabels) {
                    std::stringstream out;
                    out << "KNearestNeighborsClassifier's class label and default class label have different types." << std::endl;
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
                }
                defaultClassLabelIsInt64 = true;
                break;
                
            case Specification::KNearestNeighborsClassifier::DEFAULTCLASSLABEL_NOT_SET:
                if (labelCount == 0) {
                    std::stringstream out;
                    out << "KNearestNeighborsClassifier should specify default class labels when class labels are not specified." << std::endl;
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, out.str());
                }
        }
        
        res = validateClassifierInterface(format, format.knearestneighborsclassifier(), true, defaultClassLabelIsInt64);
        if (!res.good()) {
            return res;
        }

        
        return validateNearestNeighborsIndex(format, labelCount);

    }

}

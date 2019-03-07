//
//  Comparison.cpp
//  libmlmodelspec
//
//  Created by Zachary Nation on 3/22/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#include "Comparison.hpp"

#include <cmath>

namespace CoreML {

    namespace Specification {

#pragma mark Model container and metadata/interface

        bool operator==(const Model& a, const Model& b) {
            if (a.specificationversion() != b.specificationversion()) {
                return false;
            }
            if (a.description() != b.description()) {
                return false;
            }
            if (a.Type_case() != b.Type_case()) {
                return false;
            }

            // if everything else matches, check the model-specific parameters
            switch (a.Type_case()) {
                case Model::kPipelineClassifier:
                    return a.pipelineclassifier() == b.pipelineclassifier();
                case Model::kPipelineRegressor:
                    return a.pipelineregressor() == b.pipelineregressor();
                case Model::kPipeline:
                    return a.pipeline() == b.pipeline();
                case Model::kGlmRegressor:
                    return a.glmregressor() == b.glmregressor();
                case Model::kSupportVectorRegressor:
                    return a.supportvectorregressor() == b.supportvectorregressor();
                case Model::kTreeEnsembleRegressor:
                    return a.treeensembleregressor() == b.treeensembleregressor();
                case Model::kNeuralNetworkRegressor:
                    return a.neuralnetworkregressor() == b.neuralnetworkregressor();
                case Model::kGlmClassifier:
                    return a.glmclassifier() == b.glmclassifier();
                case Model::kSupportVectorClassifier:
                    return a.supportvectorclassifier() == b.supportvectorclassifier();
                case Model::kTreeEnsembleClassifier:
                    return a.treeensembleclassifier() == b.treeensembleclassifier();
                case Model::kNeuralNetworkClassifier:
                    return a.neuralnetworkclassifier() == b.neuralnetworkclassifier();
                case Model::kNeuralNetwork:
                    return a.neuralnetwork() == b.neuralnetwork();
                case Model::kBayesianProbitRegressor:
                    return a.bayesianprobitregressor() == b.bayesianprobitregressor();
                case Model::kOneHotEncoder:
                    return a.onehotencoder() == b.onehotencoder();
                case Model::kImputer:
                    return a.imputer() == b.imputer();
                case Model::kFeatureVectorizer:
                    return a.featurevectorizer() == b.featurevectorizer();
                case Model::kDictVectorizer:
                    return a.dictvectorizer() == b.dictvectorizer();
                case Model::kScaler:
                    return a.scaler() == b.scaler();
                case Model::kNonMaximumSuppression:
                    return a.nonmaximumsuppression() == b.nonmaximumsuppression();
                case Model::kCategoricalMapping:
                    return a.categoricalmapping() == b.categoricalmapping();
                case Model::kNormalizer:
                    return a.normalizer() == b.normalizer();
                case Model::kArrayFeatureExtractor:
                    return a.arrayfeatureextractor() == b.arrayfeatureextractor();
                case Model::kIdentity:
                    return true;
                case Model::kCustomModel:
                    return a.custommodel() == b.custommodel();
                case Model::kWordTagger:
                    return a.wordtagger() == b.wordtagger();
                case Model::kTextClassifier:
                    return a.textclassifier() == b.textclassifier();
                case Model::kVisionFeaturePrint:
                    return a.visionfeatureprint() == b.visionfeatureprint();
                case Model::TYPE_NOT_SET:
                    return true;
            }
        }

        bool operator==(const Metadata& a,
                        const Metadata& b) {
            if (a.shortdescription() != b.shortdescription()) {
                return false;
            }
            if (a.versionstring() != b.versionstring()) {
                return false;
            }
            if (a.author() != b.author()) {
                return false;
            }
            if (a.userdefined() != b.userdefined()) {
                return false;
            }
            return true;
        }

        bool operator==(const ModelDescription& a,
                        const ModelDescription& b) {
            if (a.input() != b.input()) {
                return false;
            }
            if (a.output() != b.output()) {
                return false;
            }
            if (a.predictedfeaturename() != b.predictedfeaturename()) {
                return false;
            }
            if (a.predictedprobabilitiesname() != b.predictedprobabilitiesname()) {
                return false;
            }
            if (a.metadata() != b.metadata()) {
                return false;
            }
            return true;
        }

        bool operator==(const FeatureDescription& a,
                        const FeatureDescription& b) {
            if (a.name() != b.name()) {
                return false;
            }
            if (a.shortdescription() != b.shortdescription()) {
                return false;
            }
            if (a.type() != b.type()) {
                return false;
            }
            return true;
        }

        bool isEquivalent(const FeatureDescription& a,
                          const FeatureDescription& b) {
            if (a.name() != b.name()) {
                return false;
            }
            if (a.type() != b.type()) {
                return false;
            }
            return true;
        }

        static inline bool hasFlexibleShape(const Specification::ArrayFeatureType &marray) {
            return marray.ShapeFlexibility_case() != Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET;
        }

        static inline int64_t rankOf(const Specification::ArrayFeatureType &marray) {
            switch (marray.ShapeFlexibility_case()) {
                case Specification::ArrayFeatureType::kEnumeratedShapes:
                    return marray.enumeratedshapes().shapes(0).shape_size();
                case  Specification::ArrayFeatureType::kShapeRange:
                    return marray.shaperange().sizeranges_size();
                case Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET:
                    return marray.shape_size();
            }
        }

        static inline bool compareArrayTypes(const Specification::FeatureType& x,
                                             const Specification::FeatureType& y) {
            const auto& xp = x.multiarraytype();
            const auto& yp = y.multiarraytype();
            if (xp.datatype() != yp.datatype()) {
                return false;
            }

            if (rankOf(xp) != rankOf(yp)) {
                return false;
            }

            if (!hasFlexibleShape(xp) && !hasFlexibleShape(yp)) {
                for (int i=0; i<xp.shape_size(); i++) {
                    if (xp.shape(i) != yp.shape(i)) {
                        return false;
                    }
                }
            }
            return true;
        }

        static inline bool compareDictionaryTypes(const Specification::FeatureType& x,
                                                  const Specification::FeatureType& y) {
            const auto& xp = x.dictionarytype();
            const auto& yp = y.dictionarytype();
            if (xp.KeyType_case() != yp.KeyType_case()) {
                return false;
            }
            return true;
        }

        static inline bool compareImageTypes(const Specification::FeatureType& x,
                                             const Specification::FeatureType& y) {
            const auto& xp = x.imagetype();
            const auto& yp = y.imagetype();
            if (xp.width() != yp.width()) {
                return false;
            }
            if (xp.height() != yp.height()) {
                return false;
            }
            if (xp.colorspace() != yp.colorspace()) {
                return false;
            }
            return true;
        }

        static inline bool compareSequenceTypes(const Specification::FeatureType& x,
                                                const Specification::FeatureType& y) {
            const auto& xp = x.sequencetype();
            const auto& yp = y.sequencetype();

            if (xp.Type_case() != yp.Type_case()) {
                return false;
            }

            // TODO: Compare sizes

            return true;
        }


        bool operator==(const FeatureType& a,
                        const FeatureType& b) {

            // TODO @znation: commenting out the below, because it breaks pipeline validator.
            // Pipeline validator assumes that T -> optional<T> should be allowed, but
            // it's using this operator== to test that, and failing.
            // We should eventually fix that by making a notion of "is valid as type" method
            // for FeatureType, such that T is always a valid optional<T> but not the other
            // way around.

            /*
            if (a.isoptional() != b.isoptional()) {
                return false;
            }
            */


            if (a.Type_case() != b.Type_case()) {
                return false;
            }
            switch (a.Type_case()) {
                    // non-parametric types are always equal to each other
                case Specification::FeatureType::kDoubleType:
                case Specification::FeatureType::kInt64Type:
                case Specification::FeatureType::kStringType:
                    return true;
                case Specification::FeatureType::kMultiArrayType:
                    return compareArrayTypes(a, b);
                case Specification::FeatureType::kDictionaryType:
                    return compareDictionaryTypes(a, b);
                case Specification::FeatureType::kImageType:
                    return compareImageTypes(a, b);
                case Specification::FeatureType::kSequenceType:
                    return compareSequenceTypes(a,b);
                case Specification::FeatureType::TYPE_NOT_SET:
                    return true;
            }
        }

#pragma mark Pipelines

        bool operator==(const Pipeline& a,
                        const Pipeline& b) {
            if (a.models_size() != b.models_size()) {
                return false;
            }
            for (int i=0; i<a.models_size(); i++) {
                if (a.models(i) != b.models(i)) {
                    return false;
                }
            }
            return true;
        }

        bool operator==(const PipelineClassifier& a,
                        const PipelineClassifier& b) {
            return a.pipeline() == b.pipeline();
        }

        bool operator==(const PipelineRegressor& a,
                        const PipelineRegressor& b) {
            return a.pipeline() == b.pipeline();
        }

#pragma mark Regressors

        bool operator==(const GLMRegressor& a,
                        const GLMRegressor& b) {
            if (a.weights() != b.weights()) {
                return false;
            }
            if (a.offset() != b.offset()) {
                return false;
            }
            if (a.postevaluationtransform() != b.postevaluationtransform()) {
                return false;
            }
            return true;
        }

        bool operator==(const GLMRegressor_DoubleArray& a,
                        const GLMRegressor_DoubleArray& b) {
            return a.value() == b.value();
        }

        bool operator==(const SupportVectorRegressor& a,
                        const SupportVectorRegressor& b) {
            if (a.kernel() != b.kernel()) {
                return false;
            }
            if (a.supportVectors_case() != b.supportVectors_case()) {
                return false;
            }
            if (a.coefficients() != b.coefficients()) {
                return false;
            }
            if (a.rho() != b.rho()) {
                return false;
            }
            switch (a.supportVectors_case()) {
                case SupportVectorRegressor::kSparseSupportVectors:
                    return a.sparsesupportvectors().vectors() == b.sparsesupportvectors().vectors();
                case SupportVectorRegressor::kDenseSupportVectors:
                    return a.densesupportvectors().vectors() == b.densesupportvectors().vectors();
                case SupportVectorRegressor::SUPPORTVECTORS_NOT_SET:
                    return false;
            }
        }

        bool operator==(const Kernel& a,
                        const Kernel& b) {
            if (a.kernel_case() != b.kernel_case()) {
                return false;
            }
            switch (a.kernel_case()) {
                case Kernel::kLinearKernel:
                    return true; // no parameters
                case Kernel::kRbfKernel:
                    return a.rbfkernel() == b.rbfkernel();
                case Kernel::kPolyKernel:
                    return a.polykernel() == b.polykernel();
                case Kernel::kSigmoidKernel:
                    return a.sigmoidkernel() == b.sigmoidkernel();
                case Kernel::KERNEL_NOT_SET:
                    return false;
            }
        }

        bool operator==(const RBFKernel& a,
                        const RBFKernel& b) {
            return a.gamma() == b.gamma();
        }

        bool operator==(const PolyKernel& a,
                        const PolyKernel& b) {
            if (a.degree() != b.degree()) {
                return false;
            }
            if (a.c() != b.c()) {
                return false;
            }
            if (a.gamma() != b.gamma()) {
                return false;
            }
            return true;
        }

        bool operator==(const SigmoidKernel& a,
                        const SigmoidKernel& b) {
            if (a.gamma() != b.gamma()) {
                return false;
            }
            if (a.c() != b.c()) {
                return false;
            }
            return true;
        }

        bool operator==(const Coefficients& a,
                        const Coefficients& b) {
            return a.alpha() == b.alpha();
        }

        bool operator==(const SparseVector& a,
                        const SparseVector& b) {
            return a.nodes() == b.nodes();
        }

        bool operator==(const SparseNode& a,
                        const SparseNode& b) {
            if (a.index() != b.index()) {
                return false;
            }
            if (a.value() != b.value()) {
                return false;
            }
            return true;
        }

        bool operator==(const DenseVector& a,
                        const DenseVector& b) {
            return a.values() == b.values();
        }

        bool operator==(const TreeEnsembleRegressor& a,
                        const TreeEnsembleRegressor& b) {
            if (a.postevaluationtransform() != b.postevaluationtransform()) {
                return false;
            }
            return a.treeensemble() == b.treeensemble();
        }

        bool operator==(const TreeEnsembleParameters& a,
                        const TreeEnsembleParameters& b) {
            if (a.nodes() != b.nodes()) {
                return false;
            }
            if (a.numpredictiondimensions() != b.numpredictiondimensions()) {
                return false;
            }
            if (a.basepredictionvalue() != b.basepredictionvalue()) {
                return false;
            }
            return true;
        }

        bool operator==(const TreeEnsembleParameters_TreeNode& a,
                        const TreeEnsembleParameters_TreeNode& b) {
            if (a.treeid() != b.treeid()) {
                return false;
            }
            if (a.nodeid() != b.nodeid()) {
                return false;
            }
            if (a.nodebehavior() != b.nodebehavior()) {
                return false;
            }
            if (a.branchfeatureindex() != b.branchfeatureindex()) {
                return false;
            }
            if (a.branchfeaturevalue() != b.branchfeaturevalue()) {
                return false;
            }
            if (a.truechildnodeid() != b.truechildnodeid()) {
                return false;
            }
            if (a.falsechildnodeid() != b.falsechildnodeid()) {
                return false;
            }
            if (a.missingvaluetrackstruechild() != b.missingvaluetrackstruechild()) {
                return false;
            }
            if (a.evaluationinfo() != b.evaluationinfo()) {
                return false;
            }
            if (a.relativehitrate() != b.relativehitrate()) {
                return false;
            }
            return true;
        }

        bool operator==(const TreeEnsembleParameters_TreeNode_EvaluationInfo& a,
                        const TreeEnsembleParameters_TreeNode_EvaluationInfo& b) {
            if (a.evaluationindex() != b.evaluationindex()) {
                return false;
            }
            if (a.evaluationvalue() != b.evaluationvalue()) {
                return false;
            }
            return true;
        }

        bool operator==(const NeuralNetworkRegressor& a,
                        const NeuralNetworkRegressor& b) {
            if (a.layers() != b.layers()) {
                return false;
            }
            if (a.preprocessing() != b.preprocessing()) {
                return false;
            }
            return true;
        }

        bool operator==(const NeuralNetworkLayer& a,
                        const NeuralNetworkLayer& b) {
            if (a.layer_case() != b.layer_case()) {
                return false;
            }

            // TODO -- not implemented!
            // for now, all neural network layers are not equal.
            assert(false);
            return false;
        }

        bool operator==(const NeuralNetworkPreprocessing& a,
                        const NeuralNetworkPreprocessing& b) {
            if (a.featurename() != b.featurename()) {
                return false;
            }
            if (a.preprocessor_case() != b.preprocessor_case()) {
                return false;
            }
            switch (a.preprocessor_case()) {
                case NeuralNetworkPreprocessing::kScaler:
                    return a.scaler() == b.scaler();
                case NeuralNetworkPreprocessing::kMeanImage:
                    return a.meanimage() == b.meanimage();
                case NeuralNetworkPreprocessing::PREPROCESSOR_NOT_SET:
                    return false;
            }
        }

        bool operator==(const NeuralNetworkImageScaler& a,
                        const NeuralNetworkImageScaler& b) {
            if (a.redbias() != b.redbias()) {
                return false;
            }
            if (a.bluebias() != b.bluebias()) {
                return false;
            }
            if (a.greenbias() != b.greenbias()) {
                return false;
            }
            if (a.graybias() != b.graybias()) {
                return false;
            }
            if (a.channelscale() != b.channelscale()) {
                return false;
            }
            return true;
        }

        bool operator==(const NeuralNetworkMeanImage& a,
                        const NeuralNetworkMeanImage& b) {
            if (a.meanimage() != b.meanimage()) {
                return false;
            }
            return true;
        }

        bool operator==(const BayesianProbitRegressor& a,
                        const BayesianProbitRegressor& b) {
#pragma unused(a)
#pragma unused(b)
#pragma mark -- equality operator for BOPR not yet implemented
            throw std::logic_error("operator not implemented");
        }

#pragma mark Classifiers

        bool operator==(const GLMClassifier& a,
                        const GLMClassifier& b) {
            if (a.weights() != b.weights()) {
                return false;
            }
            if (a.offset() != b.offset()) {
                return false;
            }
            if (a.postevaluationtransform() != b.postevaluationtransform()) {
                return false;
            }
            if (a.classencoding() != b.classencoding()) {
                return false;
            }
            if (a.ClassLabels_case() != b.ClassLabels_case()) {
                return false;
            }
            switch (a.ClassLabels_case()) {
                case GLMClassifier::kInt64ClassLabels:
                    return a.int64classlabels() == b.int64classlabels();
                case GLMClassifier::kStringClassLabels:
                    return a.stringclasslabels() == b.stringclasslabels();
                case GLMClassifier::CLASSLABELS_NOT_SET:
                    return true;
            }
        }

        bool operator==(const GLMClassifier_DoubleArray& a,
                        const GLMClassifier_DoubleArray& b) {
            return a.value() == b.value();
        }

        bool operator==(const SupportVectorClassifier& a,
                        const SupportVectorClassifier& b) {
            if (a.kernel() != b.kernel()) {
                return false;
            }
            if (a.numberofsupportvectorsperclass() != b.numberofsupportvectorsperclass()) {
                return false;
            }
            if (a.coefficients() != b.coefficients()) {
                return false;
            }
            if (a.rho() != b.rho()) {
                return false;
            }
            if (a.proba() != b.proba()) {
                return false;
            }
            if (a.probb() != b.probb()) {
                return false;
            }
            if (a.supportVectors_case() != b.supportVectors_case()) {
                return false;
            }
            switch (a.supportVectors_case()) {
                case SupportVectorClassifier::kSparseSupportVectors:
                    if (a.sparsesupportvectors().vectors() != b.sparsesupportvectors().vectors()) {
                        return false;
                    }
                    break;
                case SupportVectorClassifier::kDenseSupportVectors:
                    if (a.densesupportvectors().vectors() != b.densesupportvectors().vectors()) {
                        return false;
                    }
                    break;
                case SupportVectorClassifier::SUPPORTVECTORS_NOT_SET:
                    break;
            }
            if (a.ClassLabels_case() != b.ClassLabels_case()) {
                return false;
            }
            switch (a.ClassLabels_case()) {
                case SupportVectorClassifier::kInt64ClassLabels:
                    return a.int64classlabels() == b.int64classlabels();
                case SupportVectorClassifier::kStringClassLabels:
                    return a.stringclasslabels() == b.stringclasslabels();
                case SupportVectorClassifier::CLASSLABELS_NOT_SET:
                    return true;
            }
        }

        bool operator==(const TreeEnsembleClassifier& a,
                        const TreeEnsembleClassifier& b) {
            if (a.treeensemble() != b.treeensemble()) {
                return false;
            }
            if (a.postevaluationtransform() != b.postevaluationtransform()) {
                return false;
            }
            if (a.ClassLabels_case() != b.ClassLabels_case()) {
                return false;
            }
            switch (a.ClassLabels_case()) {
                case TreeEnsembleClassifier::kInt64ClassLabels:
                    return a.int64classlabels() == b.int64classlabels();
                case TreeEnsembleClassifier::kStringClassLabels:
                    return a.stringclasslabels() == b.stringclasslabels();
                case TreeEnsembleClassifier::CLASSLABELS_NOT_SET:
                    return true;
            }
            return true;
        }

        bool operator==(const NeuralNetworkClassifier& a,
                        const NeuralNetworkClassifier& b) {
            if (a.layers() != b.layers()) {
                return false;
            }
            if (a.preprocessing() != b.preprocessing()) {
                return false;
            }
            if (a.ClassLabels_case() != b.ClassLabels_case()) {
                return false;
            }
            switch (a.ClassLabels_case()) {
                case NeuralNetworkClassifier::kInt64ClassLabels:
                    return a.int64classlabels() == b.int64classlabels();
                case NeuralNetworkClassifier::kStringClassLabels:
                    return a.stringclasslabels() == b.stringclasslabels();
                case NeuralNetworkClassifier::CLASSLABELS_NOT_SET:
                    return true;
            }
        }

#pragma mark Generic models

        bool operator==(const NeuralNetwork& a,
                        const NeuralNetwork& b) {
            if (a.layers() != b.layers()) {
                return false;
            }
            if (a.preprocessing() != b.preprocessing()) {
                return false;
            }
            return true;
        }

        bool operator==(const CustomModel& a,
                        const CustomModel& b) {
            if (a.classname() != b.classname()) {
                return false;
            }
            // TODO: Check parameters match
            return true;
        }

        bool operator==(const CoreMLModels::WordTagger& a,
                        const CoreMLModels::WordTagger& b) {

            if (a.revision()!= b.revision()) {
                return false;
            }

            if (a.language()!= b.language()) {
                return false;
            }

            if (a.tokensoutputfeaturename() != b.tokensoutputfeaturename()) {
                return false;
            }

            if (a.tokentagsoutputfeaturename() != b.tokentagsoutputfeaturename()) {
                return false;
            }

            if (a.tokenlocationsoutputfeaturename() != b.tokenlocationsoutputfeaturename()) {
                return false;
            }

            if (a.tokenlengthsoutputfeaturename() != b.tokenlengthsoutputfeaturename()) {
                return false;
            }

            if (a.Tags_case()!= b.Tags_case()) {
                return false;
            }

            switch (a.Tags_case()) {
                case CoreMLModels::WordTagger::kStringTags:
                    if (a.stringtags() != b.stringtags()) {
                        return false;
                    }
                    break;
                case CoreMLModels::WordTagger::TAGS_NOT_SET:
                    break;
            }

            if (a.modelparameterdata().size() != b.modelparameterdata().size()) {
                return false;
            }

            size_t s = a.modelparameterdata().size();
            if (s > 0) {
                if (memcmp(&a.modelparameterdata()[0], &b.modelparameterdata()[0], s)) {
                    return false;
                }
            }

            return true;
        }

        bool operator==(const CoreMLModels::TextClassifier& a,
                        const CoreMLModels::TextClassifier& b) {

            if (a.revision()!= b.revision()) {
                return false;
            }

            if (a.language()!= b.language()) {
                return false;
            }

            if (a.ClassLabels_case()!= b.ClassLabels_case()) {
                return false;
            }

            switch (a.ClassLabels_case()) {
                case CoreMLModels::TextClassifier::kStringClassLabels:
                    if (a.stringclasslabels()!= b.stringclasslabels()) {
                        return false;
                    }
                    break;
                case CoreMLModels::TextClassifier::CLASSLABELS_NOT_SET:
                    break;
            }

            if (a.modelparameterdata().size() != b.modelparameterdata().size()) {
                return false;
            }

            size_t s = a.modelparameterdata().size();
            if (s > 0) {
                if (memcmp(&a.modelparameterdata()[0], &b.modelparameterdata()[0], s)) {
                    return false;
                }
            }

            return true;
        }

        bool operator==(const CoreMLModels::VisionFeaturePrint& a,
                        const CoreMLModels::VisionFeaturePrint& b) {

            if (a.VisionFeaturePrintType_case() != b.VisionFeaturePrintType_case()) {
                return false;
            }

            switch (a.VisionFeaturePrintType_case()) {
                case CoreMLModels::VisionFeaturePrint::kScene:
                    if (a.scene().version() != b.scene().version()) {
                        return false;
                    }
                    break;
                case CoreMLModels::VisionFeaturePrint::VISIONFEATUREPRINTTYPE_NOT_SET:
                    break;
            }

            return true;
        }

#pragma mark Feature engineering

        bool operator==(const OneHotEncoder& a,
                        const OneHotEncoder& b) {
            if (a.CategoryType_case() != b.CategoryType_case()) {
                return false;
            }
            switch (a.CategoryType_case()) {
                case OneHotEncoder::kInt64Categories:
                    if (a.int64categories() != b.int64categories()) {
                        return false;
                    }
                    break;
                case OneHotEncoder::kStringCategories:
                    if (a.stringcategories() != b.stringcategories()) {
                        return false;
                    }
                    break;
                case OneHotEncoder::CATEGORYTYPE_NOT_SET:
                    break;
            }
            if (a.outputsparse() != b.outputsparse()) {
                return false;
            }
            if (a.handleunknown() != b.handleunknown()) {
                return false;
            }
            return true;
        }

        bool operator==(const Imputer& a, const Imputer& b) {
            if (a.ImputedValue_case() != b.ImputedValue_case()) {
                return false;
            }

            switch (a.ImputedValue_case()) {
                case Imputer::kImputedDoubleValue:
                    if( !(a.imputeddoublevalue() == b.imputeddoublevalue())) {
                        return false;
                    }
                    break;
                case Imputer::kImputedInt64Value:
                    if(!(a.imputedint64value() == b.imputedint64value())) {
                        return false;
                    }
                    break;
                case Imputer::kImputedStringValue:
                    if(!(a.imputedstringvalue() == b.imputedstringvalue())) {
                        return false;
                    }
                    break;
                case Imputer::kImputedDoubleArray:
                    if(!(a.imputeddoublearray() == b.imputeddoublearray())) {
                        return false;
                    }
                    break;
                case Imputer::kImputedInt64Array:
                    if(!(a.imputedint64array() == b.imputedint64array())) {
                        return false;
                    }
                    break;
                case Imputer::kImputedInt64Dictionary:
                    if(!(a.imputedint64dictionary() == b.imputedint64dictionary())) {
                        return false;
                    }
                    break;
                case Imputer::kImputedStringDictionary:
                    if(!(a.imputedstringdictionary() == b.imputedstringdictionary())) {
                        return false;
                    }
                case Imputer::IMPUTEDVALUE_NOT_SET:
                    // OK to return here, as this just means it's uninitialized.
                    return true;
            }

            // Now test the replacement value.
            if (a.ReplaceValue_case() != b.ReplaceValue_case()) {
                return false;
            }

            switch(a.ReplaceValue_case()) {
                case Imputer::kReplaceDoubleValue: {
                    if( ! ( (std::isnan(a.replacedoublevalue()) && std::isnan(b.replacedoublevalue()))
                           || (a.replacedoublevalue() == b.replacedoublevalue())) ) {
                        return false;
                    }
                    break;
                }
                case Imputer::kReplaceInt64Value: {
                    if(a.replaceint64value() != b.replaceint64value()) {
                        return false;
                    }
                }
                case Imputer::kReplaceStringValue: {
                    if(a.replacestringvalue() != b.replacestringvalue()) {
                        return false;
                    }
                }
                case Imputer::REPLACEVALUE_NOT_SET: {
                    break;
                }
            }

            // Done testing all of this.
            return true;
        }

        bool operator==(const FeatureVectorizer& a,
                        const FeatureVectorizer& b) {
            return a.inputlist() == b.inputlist();
        }

        bool operator==(const FeatureVectorizer_InputColumn& a,
                        const FeatureVectorizer_InputColumn& b) {
            if (a.inputcolumn() != b.inputcolumn()) {
                return false;
            }
            if (a.inputdimensions() != b.inputdimensions()) {
                return false;
            }
            return true;
        }

        bool operator==(const DictVectorizer& a,
                        const DictVectorizer& b) {
            if (a.Map_case() != b.Map_case()) {
                return false;
            }
            switch (a.Map_case()) {
                case DictVectorizer::kInt64ToIndex:
                    return a.int64toindex() == b.int64toindex();
                case DictVectorizer::kStringToIndex:
                    return a.stringtoindex() == b.stringtoindex();
                case DictVectorizer::MAP_NOT_SET:
                    return true;
            }
        }

        bool operator==(const Scaler& a,
                        const Scaler& b) {
            if (a.shiftvalue() != b.shiftvalue()) {
                return false;
            }
            if (a.scalevalue() != b.scalevalue()) {
                return false;
            }
            return true;
        }

        bool operator==(const NonMaximumSuppression& a,
                        const NonMaximumSuppression& b) {
            // Parameters
            if (a.iouthreshold() != b.iouthreshold()) {
                return false;
            }
            if (a.confidencethreshold() != b.confidencethreshold()) {
                return false;
            }

            // Input and outputs feature names
            if (a.confidenceinputfeaturename() != b.confidenceinputfeaturename()) {
                return false;
            }
            if (a.coordinatesinputfeaturename() != b.coordinatesinputfeaturename()) {
                return false;
            }
            if (a.iouthresholdinputfeaturename() != b.iouthresholdinputfeaturename()) {
                return false;
            }
            if (a.confidencethresholdinputfeaturename() != b.confidencethresholdinputfeaturename()) {
                return false;
            }
            if (a.confidenceoutputfeaturename() != b.confidenceoutputfeaturename()) {
                return false;
            }
            if (a.coordinatesoutputfeaturename() != b.coordinatesoutputfeaturename()) {
                return false;
            }

            // Same suppression method
            if (a.SuppressionMethod_case() != b.SuppressionMethod_case()) {
                return false;
            }

            // Method-specific parameters
            if (a.SuppressionMethod_case() == NonMaximumSuppression::SuppressionMethodCase::kPickTop) {
                if (a.picktop().perclass() != b.picktop().perclass()) {
                    return false;
                }
            }
            return true;
        }

        bool operator==(const CategoricalMapping& a,
                        const CategoricalMapping& b) {
            if (a.MappingType_case() != b.MappingType_case()) {
                return false;
            }
            switch (a.MappingType_case()) {
                case CategoricalMapping::kInt64ToStringMap:
                    return a.int64tostringmap().map() == b.int64tostringmap().map();
                case CategoricalMapping::kStringToInt64Map:
                    return a.stringtoint64map().map() == b.stringtoint64map().map();
                case CategoricalMapping::MAPPINGTYPE_NOT_SET:
                    return true;
            }
        }

        bool operator==(const Normalizer& a,
                        const Normalizer& b) {
            return a.normtype() == b.normtype();
        }

        bool operator==(const ArrayFeatureExtractor& a,
                        const ArrayFeatureExtractor& b) {
            return a.extractindex() == b.extractindex();
        }

#pragma mark Data structures

        template<typename T>
        bool vectorsEqual(const T& a, const T& b) {
            if (a.vector_size() != b.vector_size()) {
                return false;
            }
            for (int i=0; i<a.vector_size(); i++) {
                if (a.vector(i) != b.vector(i)) {
                    return false;
                }
            }
            return true;
        }

        bool operator==(const Int64Vector& a,
                        const Int64Vector& b) {
            return vectorsEqual(a, b);
        }

        bool operator==(const StringVector& a,
                        const StringVector& b) {
            return vectorsEqual(a, b);
        }

        bool operator==(const DoubleVector& a,
                        const DoubleVector& b) {
            return vectorsEqual(a, b);
        }

        template<typename T>
        bool mapsEqual(const T& a, const T& b) {
            if (a.map_size() != b.map_size()) {
                return false;
            }
            for (const auto& p : a.map()) {
                if (p.second != b.map().at(p.first)) {
                    return false;
                }
            }
            return true;
        }

        bool operator==(const StringToInt64Map& a,
                        const StringToInt64Map& b) {
            return mapsEqual(a, b);
        }

        bool operator==(const Int64ToStringMap& a,
                        const Int64ToStringMap& b) {
            return mapsEqual(a, b);
        }
        bool operator==(const StringToDoubleMap& a,
                        const StringToDoubleMap& b) {
            return mapsEqual(a, b);
        }

        bool operator==(const Int64ToDoubleMap& a,
                        const Int64ToDoubleMap& b){
            return mapsEqual(a, b);
        }

    }}

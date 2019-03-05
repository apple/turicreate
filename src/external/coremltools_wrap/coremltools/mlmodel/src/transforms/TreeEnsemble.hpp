#ifndef MLMODEL_TREE_ENSEMBLE_SPEC_HPP
#define MLMODEL_TREE_ENSEMBLE_SPEC_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "../Model.hpp"


namespace CoreML {

    // Forward declare structs to abstract way storage layouts.
    namespace Specification {
        class TreeEnsembleParameters;
        class TreeEnsembleParameters_TreeNode;
        class TreeEnsembleClassifier;
        class TreeEnsembleRegressor;
    }

    class TreeEnsembleBase : public Model {
    protected:

        // Init from one of the parent classes.
        TreeEnsembleBase(Model&& model_spec, bool isClassifier);

    public:

        enum class BranchMode {
            BranchOnValueLessThanEqual = 0,
            BranchOnValueLessThan = 1,
            BranchOnValueGreaterThanEqual = 2,
            BranchOnValueGreaterThan = 3,
            BranchOnValueEqual = 4,
            BranchOnValueNotEqual = 5
        };

        enum class PostEvaluationTransform {
            NoTransform = 0,
            Classification_SoftMax = 1,
            Regression_Logistic = 2,
            Classification_SoftMaxWithZeroClassReference = 3
        };

        ////////////////////////////////////////////////////////////////////////////////

        virtual ~TreeEnsembleBase();

        /**
         *  All of the leaf values are added to this value to form the base prediction
         *  value.
         */
        void setDefaultPredictionValue(double v);

        /**
         *  All of the leaf values are added to this value to form the base prediction
         *  value.
         */
        void setDefaultPredictionValue(const std::vector<double>& v);


        /**
         * This function creates a branching node tagged using nodeId with the
         * specified branch criteria.  The methods below -- setBranchNodeTrueChild
         * and setBranchNodeFalseChild must both be called on this node, or an
         * error will be raised during final construction.  The nodeId is
         * user-determined and must simply be a unique identifier of
         * the node.
         *
         * branchMode specifies what true criteria must be met in order to have
         * evaluation proceed down the "true" branch.  Possible values are given above
         * using the branch_type enum class. For a prediction row x and the
         * BRANCH_ON_VALUE_LESS_THAN_EQUAL comparison, these evaluated as:
         *
         *   if(x[feature_index] <= feature_value) {
         *       // go down true branch
         *    } else {
         *      // go down false branch
         *    }
         */
        void setupBranchNode(size_t treeId,
                             size_t nodeId,
                             size_t feature_index,
                             BranchMode branchMode,
                             double feature_value,
                             size_t trueChildNodeId,
                             size_t falseChildNodeId);

        /**
         * Some of the optimizations require an estimate of the relative hit rates as
         * given by the training data.  Scikit learn models include this in the built
         * tree models.
         */
        void setRelativeNodeHitRate(size_t treeId, size_t nodeId, double v);

        /**
         * Missing values can either track the path of the "true" child or the "false" child.  By
         * default, they always travel down the false path.  Set this to alter this behavior for a
         * given node.
         */
        void setMissingValueBehavior(size_t treeId, size_t nodeId, bool missing_value_tracks_true_child);

        /**
         * If this is called, a node is created that is marked as a leaf evaluation node,
         * which means that when this node is triggered, a value is added to the
         * total prediction value.  Currently, the sum total of all activated leaves
         * is returned as the prediction value.
         */
        void setupLeafNode(size_t treeId, size_t nodeId, double value);

        /** If this is called, then the evaluation node is set to be a
         *  multidimensional prediction with one or more values added to the
         *  final outcome.  Note that the multidimb and single dim versions of
         *  these cannot be mixed.
         *
         */
        void setupLeafNode(size_t treeId, size_t nodeId,
                           const std::vector<std::pair<size_t, double> >& values);

        /**
         *  This function sets the post evaluation transform that will be used
         *  following the prediction.  Possible values are "none", "sigmoid".  More
         *  added later.
         */
        void setPostEvaluationTransform(PostEvaluationTransform transform);

        /** Sets the value to traverse to the end of the range.
         *
         */

        /** Saves and validates the current model.
         */
        void finish();


    protected:

        // Intermediate structures for building up the nodes.
        Specification::TreeEnsembleParameters_TreeNode& _getNode(size_t treeId, size_t nodeId, bool from_setup_routine);
        std::map<std::pair<size_t, size_t>, int> _nodeId_map;

        Specification::TreeEnsembleParameters* tree_parameters;

    };


    /**
     * Reader/Writer interface for a tree ensemble spec.
     *
     * A construction class that, in the end, outputs a properly constructed
     * specification that is gauranteed to load in an TreeEnsembleSpec class.
     *
     */
    class TreeEnsembleClassifier : public TreeEnsembleBase {
    public:
        TreeEnsembleClassifier(const std::string& predictedClassOutputName,
                               const std::string& classProbabilityOutputName,
                               const std::string& description = "");

        virtual ~TreeEnsembleClassifier();

        /**  Set up the class list.
         */
        void setOutputClassList(const std::vector<std::string>& classes);

        void setOutputClassList(const std::vector<int64_t>& classes);

        void setBinaryOutputClasses(const std::string& negativeClass, const std::string& positiveClass);

        void setPostEvaluationTransform(PostEvaluationTransform transform);

    private:
        Specification::TreeEnsembleClassifier* tree_classifier_parameters;
    };


    class TreeEnsembleRegressor : public TreeEnsembleBase {
    public:
        /** Initialize the TreeEnsembleSpec in regression mode.
         *
         *  The dimension
         */
        TreeEnsembleRegressor(const std::string& predictedValueOutput,
                              const std::string& description = "");

        void setPostEvaluationTransform(PostEvaluationTransform transform);

        virtual ~TreeEnsembleRegressor();

    private:
        Specification::TreeEnsembleRegressor* tree_regressor_parameters;
    };

    typedef TreeEnsembleBase::BranchMode BranchMode;
    typedef TreeEnsembleBase::PostEvaluationTransform PostEvaluationTransform;
}

#endif

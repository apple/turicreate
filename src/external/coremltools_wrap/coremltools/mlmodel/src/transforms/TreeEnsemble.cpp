//
//  MLTreeEnsembleSpecification.cpp
//  CoreML
//
//  Created by Hoyt Koepke on 10/21/16.
//  Copyright Â© 2016 Apple Inc. All rights reserved.
//

#include "TreeEnsemble.hpp"
#include "../Format.hpp"
#include "../TreeEnsembleCommon.hpp"
#include <memory>

namespace CoreML {

    TreeEnsembleBase::TreeEnsembleBase(Model&& model_spec, bool isClassifier)
    : Model(model_spec)
    , tree_parameters(  isClassifier
                      ? m_spec->mutable_treeensembleclassifier()->mutable_treeensemble()
                      : m_spec->mutable_treeensembleregressor()->mutable_treeensemble())
    {
    }

    TreeEnsembleClassifier::TreeEnsembleClassifier
    (const std::string& predictedClassOutputName,
     const std::string& classProbabilityOutputName,
     const std::string& description)
    : TreeEnsembleBase(Model(description), true /* isClassifier */),
      tree_classifier_parameters(m_spec->mutable_treeensembleclassifier())
    {
        m_spec->mutable_description()->set_predictedfeaturename(predictedClassOutputName);
        m_spec->mutable_description()->set_predictedprobabilitiesname(classProbabilityOutputName);
    }

    TreeEnsembleRegressor::TreeEnsembleRegressor
    (const std::string& predictedValueOutput,
     const std::string& description)
    : TreeEnsembleBase(Model(description), false /* isClassifier */)
    , tree_regressor_parameters(m_spec->mutable_treeensembleregressor())
    {
        m_spec->mutable_description()->set_predictedfeaturename(predictedValueOutput);
    }


    TreeEnsembleBase::~TreeEnsembleBase() = default;
    TreeEnsembleRegressor::~TreeEnsembleRegressor() = default;
    TreeEnsembleClassifier::~TreeEnsembleClassifier() = default;

    /**
     *  All of the leaf values are added to this value to form the base prediction
     *  value.
     */
    void TreeEnsembleBase::setDefaultPredictionValue(double v) {
        tree_parameters->clear_basepredictionvalue();
        tree_parameters->add_basepredictionvalue(v);
        tree_parameters->set_numpredictiondimensions(1);
    }


    /**
     *  All of the leaf values are added to this value to form the base prediction
     *  value.
     */
    void TreeEnsembleBase::setDefaultPredictionValue(const std::vector<double>& v) {
        tree_parameters->clear_basepredictionvalue();
        for(double e : v) {
            tree_parameters->add_basepredictionvalue(e);
        }

        tree_parameters->set_numpredictiondimensions(v.size());
    }



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
    void TreeEnsembleBase::setupBranchNode
    (size_t treeId, size_t nodeId, size_t feature_index,
     BranchMode branchMode,
     double feature_value,
     size_t trueChildNodeId, size_t falseChildNodeId) {

        auto& node = _getNode(treeId, nodeId, true);

        // TODO: Error checking routines -- make sure this node hasn't been requested before.
        node.set_branchfeatureindex(feature_index);
        node.set_nodebehavior(static_cast<Specification::TreeEnsembleParameters::TreeNode::TreeNodeBehavior>(branchMode));
        node.set_branchfeaturevalue(feature_value);
        node.set_truechildnodeid(trueChildNodeId);
        node.set_falsechildnodeid(falseChildNodeId);
    }

    /**
     * Some of the optimizations require an estimate of the relative hit rates as
     * given by the training data.  Scikit learn models include this in the built
     * tree models.
     */
    void TreeEnsembleBase::setRelativeNodeHitRate
    (size_t treeId, size_t nodeId, double v) {

        _getNode(treeId, nodeId, false).set_relativehitrate(v);
    }

    /**
     * Missing values can either track the path of the "true" child or the "false" child.  By
     * default, they always travel down the false path.  Set this to alter this behavior for a
     * given node.
     */
    void TreeEnsembleBase::setMissingValueBehavior(size_t treeId, size_t nodeId, bool missing_value_tracks_true_child) {
        _getNode(treeId, nodeId, false).set_missingvaluetrackstruechild(missing_value_tracks_true_child);
    }


    /**
     * If this is called, a node is created that is marked as a leaf evaluation node,
     * which means that when this node is triggered, a value is added to the
     * total prediction value.  Currently, the sum total of all activated leaves
     * is returned as the prediction value.
     */
    void TreeEnsembleBase::setupLeafNode(size_t treeId, size_t nodeId, double value) {
        Specification::TreeEnsembleParameters::TreeNode& node = _getNode(treeId, nodeId, true);

        // TODO: Error checking routines -- make sure this node hasn't been requested before.
        node.set_nodebehavior(Specification::TreeEnsembleParameters::TreeNode::LeafNode);

        node.clear_evaluationinfo();
        auto ei = node.add_evaluationinfo();
        ei->set_evaluationindex(0);
        ei->set_evaluationvalue(value);
    }

    /**
     * If this is called, a node is created that is marked as a leaf evaluation node,
     * which means that when this node is triggered, a value is added to the
     * total prediction value.  Currently, the sum total of all activated leaves
     * is returned as the prediction value.
     */
    void TreeEnsembleBase::setupLeafNode(size_t treeId, size_t nodeId,
                                         const std::vector<std::pair<size_t, double> >& values) {
        Specification::TreeEnsembleParameters::TreeNode& node = _getNode(treeId, nodeId, true);

        // TODO: Error checking routines -- make sure this node hasn't been requested before.
        node.set_nodebehavior(Specification::TreeEnsembleParameters::TreeNode::LeafNode);

        node.clear_evaluationinfo();
        for(const auto& p : values) {
            auto ei = node.add_evaluationinfo();
            ei->set_evaluationindex(p.first);
            ei->set_evaluationvalue(p.second);
        }
    }

    Specification::TreeEnsembleParameters_TreeNode& TreeEnsembleBase::_getNode(size_t treeId, size_t nodeId, bool is_setup_routine) {
        auto it = _nodeId_map.lower_bound({treeId, nodeId});

        if(it == _nodeId_map.end() || it->first != std::make_pair(treeId, nodeId)) {
            if(!is_setup_routine) {
                std::ostringstream ss;
                ss << "Setup routine not called yet for node with treeId=" << treeId << ", nodeID=" << nodeId << ".";
                throw std::logic_error(ss.str());
            }

            int new_node_index = tree_parameters->nodes_size();
            Specification::TreeEnsembleParameters_TreeNode* newNode = tree_parameters->add_nodes();
            newNode->set_treeid(treeId);
            newNode->set_nodeid(nodeId);
            _nodeId_map.insert(it, {{treeId, nodeId}, new_node_index});
            return *newNode;
        } else {
            if(is_setup_routine) {
                std::ostringstream ss;
                ss << "Setup routine called multiple times for treeId=" << treeId << ", nodeID=" << nodeId << ".";
                throw std::logic_error(ss.str());
            }
            return *(tree_parameters->mutable_nodes(it->second));
        }
    }

    void TreeEnsembleBase::finish() {
        TreeEnsembles::constructAndValidateTreeEnsembleFromSpec(*m_spec);
    }

    /////////////////////////////////////////////////////////////////////
    // Classifier specific methods

    /**  Set up the class list -- two versions for string and integer values.
     */
    void TreeEnsembleClassifier::setOutputClassList(const std::vector<std::string>& classes) {
        m_spec->mutable_treeensembleclassifier()->mutable_stringclasslabels()->clear_vector();
        for(size_t i = 0; i < classes.size(); ++i) {
            std::string *category = m_spec->mutable_treeensembleclassifier()->mutable_stringclasslabels()->add_vector();
            *category = classes[i];
        }
    }

    void TreeEnsembleClassifier::setOutputClassList(const std::vector<int64_t>& classes) {
        m_spec->mutable_treeensembleclassifier()->mutable_int64classlabels()->clear_vector();
        for(size_t i = 0; i < classes.size(); ++i) {
            m_spec->mutable_treeensembleclassifier()->mutable_int64classlabels()->add_vector(classes[i]);
        }
    }

    void TreeEnsembleClassifier::setBinaryOutputClasses(const std::string& negativeClass, const std::string& positiveClass) {

        setOutputClassList({negativeClass, positiveClass});
    }


    /**
     * This function sets the post evaluation transform that will be used
     * following the prediction.  Possible values are "none", "sigmoid".  More
     * added later.
     */
    void TreeEnsembleClassifier::setPostEvaluationTransform
    (PostEvaluationTransform transform) {

        // TODO: Check to make sure the particular option is compatible with a classifier
        tree_classifier_parameters->set_postevaluationtransform(
                                                                static_cast<Specification::TreeEnsemblePostEvaluationTransform>(transform));
    }


    ///////////////////////////////////////////////////////////////////////
    // Regressor parameters

    /**
     * This function sets the post evaluation transform that will be used
     * following the prediction.  Possible values are "none", "sigmoid".  More
     * added later.
     */
    void TreeEnsembleRegressor::setPostEvaluationTransform
    (PostEvaluationTransform transform) {
        // TODO: this should do something.
        tree_regressor_parameters->set_postevaluationtransform(static_cast<Specification::TreeEnsemblePostEvaluationTransform>(transform));
    }

}

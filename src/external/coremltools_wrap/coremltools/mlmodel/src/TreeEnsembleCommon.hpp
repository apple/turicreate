#ifndef _MLTREEENSEMBLECOMMONSTRUCTURES_H_
#define _MLTREEENSEMBLECOMMONSTRUCTURES_H_

#include <stdexcept>
#include <cassert>
#include <sstream>
#include <cmath>
#include <memory>
#include <vector>
#include "transforms/TreeEnsemble.hpp"

namespace CoreML {

namespace Specification {
    class Model;
}

namespace TreeEnsembles {

    typedef TreeEnsembleBase::BranchMode BranchMode;
    typedef TreeEnsembleBase::PostEvaluationTransform PostEvaluationTransform;


    /** An intermediate tree node structure that is used during the
     *  construction, building, and validation.  It allows us to perform
     */
    struct _TreeComputationNode {

        // Is this a leaf node?
        bool is_leaf_node = false;

        // What is the branching mode, "<=", "==", etc.
        BranchMode branch_mode;

        // If it is a branch, what is the
        uint64_t branch_feature_index = 0;
        double branch_feature_value = 0;

        // If is_leaf_node is true, then these are filled in.
        double relative_hit_rate = 0;

        // Some of the original information in the nodes that is now
        // replaced by pointers.
        uint64_t tree_id = 0;
        uint64_t node_id = 0;
        uint64_t true_child_node_id = 0;
        uint64_t false_child_node_id = 0;

        // Missing value behavior.
        bool missing_value_tracks_true_child = false;

        // Optional -- the relative hit rate of the node.
        double relative_node_hit_rate = 0;

        // If it's a leaf node, then this will have 1 or more values.
        std::vector<std::pair<size_t, double> > evaluation_values;

        // Pointers to the parent and children that will enable efficient
        // tree traversal, validation, and statistics gathering.
        std::weak_ptr<_TreeComputationNode> parent_node;
        std::shared_ptr<_TreeComputationNode> true_child_node;
        std::shared_ptr<_TreeComputationNode> false_child_node;
    };


    struct _TreeEnsemble {

        // The header information; contains everything needed for this stuff.
        uint64_t num_nodes = 0;
        uint64_t num_roots = 0;
        uint64_t num_dimensions = 0;

        // The default values.  The size of this determines the dimension.
        std::vector<double> default_values;

        // The root nodes.  These contain the entire tree structure.
        std::vector<std::shared_ptr<_TreeComputationNode> > root_nodes;

        // The mode used for operating.  Binary Classification is basically
        // just logistic regression.
        enum class OperationMode {
            REGRESSION_MODE,
            BINARY_CLASSIFICATION_MODE,
            MULTICLASS_CLASSIFICATION_MODE};

        OperationMode operation_mode;

        // The final output vector for the classes.  Exactly one of these
        // must be filled in classification mode. If the output classes are
        // not specified, they default as {0,1,...}.
        std::vector<std::string> output_classes_string;
        std::vector<int64_t> output_classes_integer;


        /** The post-processing transform.
         */
        PostEvaluationTransform post_processing_transform;
    };

    /** If the model is a tree, then it will have tree ensemble
     *  tendencies.
     */
    std::shared_ptr<_TreeEnsemble>
    constructAndValidateTreeEnsembleFromSpec(const Specification::Model& tes);

}}

#endif /* _MLTREEENSEMBLECOMMONSTRUCTURES_H_ */

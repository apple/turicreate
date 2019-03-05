#include "TreeEnsembleCommon.hpp"
#include <algorithm>
#include <sstream>
#include <string>
#include <list>
#include <limits>
#include "transforms/TreeEnsemble.hpp"
#include "Format.hpp"


namespace CoreML { namespace TreeEnsembles {

    std::shared_ptr<_TreeEnsemble>
    constructAndValidateTreeEnsembleFromSpec(const Specification::Model& m_spec) {

        ////////////////////////////////////////////////////////////////////////////////
        // Error management.

        size_t error_count = 0;
        std::ostringstream current_error_msg;

        // This adds in the error message to the string of potential error
        // messages on the list.
        auto add_error_message = [&](const std::string& error, bool is_fatal = false) {

            // Adds to the general error message string.
            ++error_count;
            current_error_msg << "\n  " << (is_fatal ? "FATAL: " : "") << error << ";";

            if(error_count >= 50) {
                current_error_msg << "\n  FATAL: maximum number of errors reached; aborting processing.";
                is_fatal = true;
            }

            if(is_fatal) {
                throw std::logic_error("Errors encountered during processing tree model:\n" + current_error_msg.str());
            }
        };

        /** Preliminary. -- get the right tree parameters out to get all the nodes.
         */
        if(!m_spec.has_treeensembleclassifier() && !m_spec.has_treeensembleregressor()) {
            throw std::invalid_argument("Model is not a tree ensemble.");
        }

        const Specification::TreeEnsembleParameters& tes =
        (m_spec.has_treeensembleregressor()
         ? m_spec.treeensembleregressor().treeensemble()
         : m_spec.treeensembleclassifier().treeensemble());


        typedef std::shared_ptr<_TreeComputationNode> node_ptr;

        std::vector<node_ptr> construction_nodes;

        // This is used for the error checking with the various values.
        uint64_t output_dimension = tes.numpredictiondimensions();

        if(output_dimension == 0) {
            add_error_message("Given output dimension equals 0.", true);
        }


        ////////////////////////////////////////////////////////////////////////////////
        // Utility routines

        // This provides the interface from a node of the  TreeEnsembleSpec to the
        auto import_data_from_spec = [&](_TreeComputationNode& n,
                                         const Specification::TreeEnsembleParameters::TreeNode& ns) {

            n.node_id = ns.nodeid();
            n.tree_id = ns.treeid();

            typedef Specification::TreeEnsembleParameters::TreeNode TreeNode;

            n.is_leaf_node = (ns.nodebehavior() == TreeNode::LeafNode);
            n.relative_node_hit_rate = ns.relativehitrate();

            if(!n.is_leaf_node) {

                switch(ns.nodebehavior()) {
                    case TreeNode::BranchOnValueLessThanEqual:
                        n.branch_mode = BranchMode::BranchOnValueLessThanEqual;
                        break;
                    case TreeNode::BranchOnValueLessThan:
                        n.branch_mode = BranchMode::BranchOnValueLessThan;
                        break;
                    case TreeNode::BranchOnValueGreaterThanEqual:
                        n.branch_mode = BranchMode::BranchOnValueGreaterThanEqual;
                        break;
                    case TreeNode::BranchOnValueGreaterThan:
                        n.branch_mode = BranchMode::BranchOnValueGreaterThan;
                        break;
                    case TreeNode::BranchOnValueEqual:
                        n.branch_mode = BranchMode::BranchOnValueEqual;
                        break;
                    case TreeNode::BranchOnValueNotEqual:
                        n.branch_mode = BranchMode::BranchOnValueNotEqual;
                        break;
                    default:
                    case TreeNode::LeafNode:
                        throw std::logic_error("Branch mode hit bad value -- this is confusing; error in validator?");
                }

                n.branch_feature_index = ns.branchfeatureindex();
                n.branch_feature_value = ns.branchfeaturevalue();
                n.true_child_node_id = ns.truechildnodeid();
                n.false_child_node_id = ns.falsechildnodeid();
                n.missing_value_tracks_true_child = ns.missingvaluetrackstruechild();
            } else {
                n.evaluation_values.resize(static_cast<size_t>(ns.evaluationinfo_size()));

                for(int i = 0; i < ns.evaluationinfo_size(); ++i) {
                    uint64_t index = ns.evaluationinfo(i).evaluationindex();
                    double value = ns.evaluationinfo(i).evaluationvalue();
                    n.evaluation_values[static_cast<size_t>(i)] = {index, value};
                }

                // Check the evaluation values.
                if(n.evaluation_values.size() == 0) {
                    std::ostringstream ss;
                    ss << "Leaf Node (TreeID=" << n.tree_id << ", NodeID="
                    << n.node_id << ") has no evaluation value(s) specified.";
                    add_error_message(ss.str());
                    return;
                }

                // Sort these so that we can easily apply optimizations, detect
                // duplications, etc.
                if(n.evaluation_values.size() > 1) {
                    std::sort(n.evaluation_values.begin(), n.evaluation_values.end());
                }

                // Make sure all values are in range.
                for(const std::pair<size_t, double>& p : n.evaluation_values) {
                    if(p.first >= output_dimension) {
                        std::ostringstream ss;
                        ss << "Leaf Node (TreeID=" << n.tree_id << ", NodeID="
                        << n.node_id << ") specifies evaluation value applied to dimension "
                        << p.first << "; which is out of range. Dimension must be less than "
                        << output_dimension << ".";
                        add_error_message(ss.str());
                    }
                }

                // Detect duplicated values.
                for(size_t i = 0; i < n.evaluation_values.size() - 1; ++i) {
                    const std::pair<size_t, double>& p1 = n.evaluation_values[i];
                    const std::pair<size_t, double>& p2 = n.evaluation_values[i+1];

                    if(p1.first == p2.first) {
                        std::ostringstream ss;
                        ss << "Leaf Node (TreeID=" << n.tree_id << ", NodeID="
                        << n.node_id << ") specifies multipule evaluation values applied to dimension "
                        << p1.first << ".";
                        add_error_message(ss.str());
                    }
                }
            }
        };

        ////////////////////////////////////////////////////////////////////////////////

        // Get the full list of nodes from the spec.
        auto nodes = tes.nodes();

        /**  Stage 1: Translation.
         *
         *   In this first stage, we simply translate all the nodes in the
         *   specification into more usable intermediate structure that has
         *   pointer references between nodes, allowing for easy traversal.  All
         *   error checking is done at this stage as well.
         */
        {
            // Go through and build the lookup table.
            std::map<std::pair<size_t, size_t>, node_ptr> construction_node_id_map;

            for(const auto& node : nodes) {
                std::pair<size_t, size_t> key = {size_t(node.treeid()), size_t(node.nodeid())};

                auto it = construction_node_id_map.lower_bound(key);

                // Make sure that it's not already in the node map.  If that's the case, we have duplicate nodes.
                if(it != construction_node_id_map.end()
                   && it->second->tree_id == node.treeid()
                   && it->second->node_id == node.nodeid()) {

                    std::ostringstream ss;
                    ss << "Tree Node with TreeID=" << node.treeid()
                    << "and NodeID=" << node.nodeid()
                    << " duplicated in specification.";
                    add_error_message(ss.str());
                    continue;
                }

                // Create a new tree node.
                auto n = std::make_shared<_TreeComputationNode>();

                // Copy over the relevant information.
                import_data_from_spec(*n, node);

                // Add it to the map.
                construction_node_id_map.insert(it, {key, n});
            }

            /**  Stage 2: Connecting nodes.
             *
             *   In this second stage, we traverse the list, replacing the
             *   node_ids by pointers that can be used to traverse the tree
             *   easily during the building and optimization stage.
             */

            // A common function to access the nodes.
            auto _get_node = [&](std::pair<size_t, size_t> node_key) -> node_ptr {

                auto it = construction_node_id_map.find(node_key);

                // It should be added as part of the above construction routines.
                if(it == construction_node_id_map.end()) {
                    std::ostringstream ss;
                    ss << "Tree Node with TreeID=" << node_key.first
                    << " and NodeID=" << node_key.second
                    << " referenced but not declared in specification.";

                    add_error_message(ss.str());
                    return nullptr;
                } else {
                    return it->second;
                }
            };

            // Go through all the nodes in the specification, filling the corresponding data
            for(const auto& p : construction_node_id_map) {
                const node_ptr& n = p.second;

                if(!n->is_leaf_node) {

                    // Set up the false child node.
                    {
                        auto false_child_node = _get_node( {n->tree_id, n->false_child_node_id} );

                        if(false_child_node == n) {
                            std::ostringstream ss;
                            ss << "False child and parent have same ID (TreeID=" << n->tree_id
                            << ", NodeID=" << n->node_id << ")";
                            add_error_message(ss.str());
                            continue;
                        }

                        if(false_child_node->parent_node.lock() != nullptr) {
                            std::ostringstream ss;
                            ss << "In TreeID=" << n->tree_id << ", false child of NodeID=" << n->node_id
                            << " is already the child of node NodeID="
                            << false_child_node->parent_node.lock()->node_id;
                            add_error_message(ss.str());
                            continue;
                        }

                        n->false_child_node = false_child_node;
                        false_child_node->parent_node = n;
                    }

                    // Set up the true child node.
                    {
                        auto true_child_node = _get_node( {n->tree_id, n->true_child_node_id} );

                        if(true_child_node == n) {
                            std::ostringstream ss;
                            ss << "True child and parent have same ID (TreeID=" << n->tree_id
                            << ", NodeID=" << n->node_id << ")";
                            add_error_message(ss.str());
                            continue;
                        }

                        if(true_child_node->parent_node.lock() != nullptr) {
                            std::ostringstream ss;
                            ss << "In TreeID=" << n->tree_id << ", true child of NodeID=" << n->node_id
                            << " is already the child of node NodeID="
                            << true_child_node->parent_node.lock()->node_id;
                            add_error_message(ss.str());
                            continue;
                        }

                        n->true_child_node = true_child_node;
                        true_child_node->parent_node = n;
                    }
                }
            }

            construction_nodes.reserve(construction_node_id_map.size());
            for(const auto& p : construction_node_id_map) {
                construction_nodes.push_back(p.second);
            }
        }

        std::vector<node_ptr> root_nodes;

        /** Stage 3: Validation of independent tree structures.
         *
         *  In this stage, we go through and validate that each tree
         *  structure is, in fact, a valid tree.  In addition, we accumulate
         *  a list of root nodes of the trees in order to return to the user at the end.
         */
        {
            std::vector<std::pair<size_t, size_t> > root_node_ids;

            // Now, go through and record all the root nodes in the map, making
            // sure there is exactly one root node per tree.

            for(const node_ptr& n : construction_nodes) {

                if(n->parent_node.lock() == nullptr) {
                    root_node_ids.push_back({n->tree_id, n->node_id});
                    root_nodes.push_back(n);
                }
            }

            // Check to make sure that each tree has exactly one root.
            {
                // Sort by tree ID to detect non-unique elements.
                std::sort(root_node_ids.begin(), root_node_ids.end());

                for(size_t i = 0; i < root_node_ids.size() - 1;) {
                    size_t tree_id = root_node_ids[i].first;

                    if(root_node_ids[i+1].first == tree_id) {
                        // Find how many there are.
                        size_t j = i + 1;

                        // Advance j to first element with tree_id not equal to this tree_id.
                        for(; j < root_node_ids.size() && root_node_ids[j].first == tree_id; ++j) /*pass*/;

                        // Now record these nodes.
                        std::ostringstream ss;

                        ss << "Tree TreeID=" << tree_id << " has multiple root nodes: ";
                        for(size_t k = i; k < j; ++k) {
                            ss << "NodeID=" << root_node_ids[k].second;
                            if(k != j-1) { ss << ", "; }
                        }
                        ss << ".";

                        add_error_message(ss.str());

                        // Move position forward
                        i = j;
                    } else {
                        ++i;
                    }
                }
            }

            // Check to make sure there are no cycles, unaccounted for blocks,
            // etc.  In this case, since we have made sure that the above
            {
                // Maintain a list of nodes not connected to roots.
                std::set<void*> nodes_connected_to_roots;

                std::vector<std::pair<node_ptr, size_t> > node_stack;

                for(node_ptr root : root_nodes) {
                    node_stack = { {root, 0} };
                    while(!node_stack.empty()) {
                        auto& top = node_stack.back();
                        node_ptr n = top.first;

                        if(n == nullptr) {
                            // This indicates there are logic errors above fooling us up; abort.
                            add_error_message("Internal error: null child node; likely specification error.", true);
                        }

                        if(n->is_leaf_node) {
                            node_stack.pop_back();
                            continue;
                        }

                        switch(top.second) {

                            case 0:
                                // Process false_child_node; jump to that in stack.
                                top.second = 1;
                                nodes_connected_to_roots.insert(n->false_child_node.get());
                                node_stack.push_back({n->false_child_node, 0});
                                continue;

                            case 1:
                                // Process false_child_node; jump to that in stack.
                                top.second = 2;
                                nodes_connected_to_roots.insert(n->true_child_node.get());
                                node_stack.push_back({n->true_child_node, 0});
                                continue;

                            case 2:
                            default:
                                // Done with processing.
                                node_stack.pop_back();
                                continue;
                        }
                    }
                }

                // Now, are there nodes in not connected to any root nodes?
                // Because no node can have more than one parent, and that root
                // nodes are definied by a node having no parent, then there
                // exists nodes not connected to any root node if and only if
                // there is a cycle.  Thus we can easily test for this.

                // Actually add in the root nodes since they are connected to themselves.
                for(const auto& r : root_nodes) {
                    nodes_connected_to_roots.insert(r.get());
                }

                if(nodes_connected_to_roots.size() != construction_nodes.size()) {
                    std::ostringstream ss;

                    ss << "Node detected that are not connected to any single root node. Note: ";

                    for(const node_ptr& n : construction_nodes) {
                        if(nodes_connected_to_roots.find(n.get()) == nodes_connected_to_roots.end()) {
                            ss << "(TreeID=" << n->tree_id << ", NodeID=" << n->node_id << ") ";
                        }
                    }

                    add_error_message(ss.str());
                }
            }
        }


        /** Stage 4: Construct the rest of the ensemble class and verify the
         *  correctness of the dimension and multiclass options.
         */
        auto tree_ensemble = std::make_shared<_TreeEnsemble>();


        /** Add in the default values.
         */
        tree_ensemble->root_nodes = root_nodes;

        // Set the stats.
        tree_ensemble->num_nodes = construction_nodes.size();
        tree_ensemble->num_roots = root_nodes.size();
        tree_ensemble->num_dimensions = output_dimension;

        assert(output_dimension <= std::numeric_limits<std::size_t>::max());
        tree_ensemble->default_values.resize(static_cast<size_t>(output_dimension));

        if(tes.basepredictionvalue_size() == 0) {
            tree_ensemble->default_values.assign(static_cast<size_t>(output_dimension), 0);
        } else if(static_cast<size_t>(tes.basepredictionvalue_size()) == output_dimension) {
            for(int i = 0; i < tes.basepredictionvalue_size(); ++i) {
                tree_ensemble->default_values[static_cast<size_t>(i)] = tes.basepredictionvalue(i);
            }
        } else {
            std::ostringstream ss;
            ss << "Dimension of default value array (" << tes.basepredictionvalue_size()
            << ") does not match specified output dimension (" << output_dimension << ").";
            add_error_message(ss.str());
        }

        /** Stage 5: pull out and verify the class-type specific parameters.
         */
        if(m_spec.has_treeensembleregressor()) {
            tree_ensemble->operation_mode = _TreeEnsemble::OperationMode::REGRESSION_MODE;
            tree_ensemble->post_processing_transform = static_cast<PostEvaluationTransform>(
                                        m_spec.treeensembleregressor().postevaluationtransform());

        } else if(m_spec.has_treeensembleclassifier()) {

            //auto tes_cl = m_spec.description().classifiertargets();

            const auto& classifier = m_spec.treeensembleclassifier();
            Specification::Int64Vector int64ClassLabels;
            Specification::StringVector stringClassLabels;

            switch (classifier.ClassLabels_case()) {
                case Specification::TreeEnsembleClassifier::kInt64ClassLabels:
                    int64ClassLabels = classifier.int64classlabels();
                    break;
                case Specification::TreeEnsembleClassifier::kStringClassLabels:
                    stringClassLabels = classifier.stringclasslabels();
                    break;
                case Specification::TreeEnsembleClassifier::CLASSLABELS_NOT_SET:
                    // no labels are set.
                    // this model will assume that means binary classification...
                    // not sure if that's the desired outcome.
                    break;
            }

            tree_ensemble->post_processing_transform = static_cast<PostEvaluationTransform>(
                                    m_spec.treeensembleclassifier().postevaluationtransform());

            size_t n_classes = static_cast<size_t>(std::max(int64ClassLabels.vector_size(), stringClassLabels.vector_size()));
            if(n_classes == 0) {

                /** Handle the binary classification mode.
                 */
                if(output_dimension == 1) {
                    tree_ensemble->output_classes_string.clear();
                    tree_ensemble->output_classes_integer = {0, 1};

                    tree_ensemble->operation_mode = _TreeEnsemble::OperationMode::BINARY_CLASSIFICATION_MODE;

                } else {

                    tree_ensemble->output_classes_string.clear();
                    tree_ensemble->output_classes_integer.resize(static_cast<size_t>(output_dimension));

                    for(size_t i = 0; i < output_dimension; ++i) {
                        tree_ensemble->output_classes_integer[i] = int64_t(i);
                    }

                    tree_ensemble->operation_mode = _TreeEnsemble::OperationMode::MULTICLASS_CLASSIFICATION_MODE;
                }

            } else if(/* Binary classification. */
                      (output_dimension == 1
                       && n_classes == 2)

                      /* Multiclass classification. */
                      || (output_dimension >= 2
                          && n_classes == output_dimension) ) {

                          bool binary_classification = (output_dimension == 1);

                          if(binary_classification) {
                              tree_ensemble->operation_mode = _TreeEnsemble::OperationMode::BINARY_CLASSIFICATION_MODE;
                          } else {
                              tree_ensemble->operation_mode = _TreeEnsemble::OperationMode::MULTICLASS_CLASSIFICATION_MODE;
                          }


                          bool integer_classes = int64ClassLabels.vector_size() > stringClassLabels.vector_size();

                          if(integer_classes) {
                              tree_ensemble->output_classes_integer.resize(n_classes);
                          } else {
                              tree_ensemble->output_classes_string.resize(n_classes);
                          }

                          assert(n_classes >= 0);
                          assert(n_classes < std::numeric_limits<int>::max());
                          for(size_t i = 0; i < n_classes; ++i) {
                              if(integer_classes) {
                                  tree_ensemble->output_classes_integer[i] = int64ClassLabels.vector(static_cast<int>(i));
                              } else {
                                  tree_ensemble->output_classes_string[i] = stringClassLabels.vector(static_cast<int>(i));
                              }
                          }
                      } else {
                          // Okay, this doesn't match up.
                          std::ostringstream ss;
                          ss << "Specified output dimension (" << output_dimension
                          << ") does not match the given number of classes (" << n_classes
                          << ").";
                          add_error_message(ss.str());
                      }
        }

        /** Stage 6: If there have been any errors, raise them.
         *
         */
        if(error_count != 0) {
            throw std::logic_error("Error(s) in tree structure: \n" + current_error_msg.str());
        }

        // And we're done.
        return tree_ensemble;
    }
}}

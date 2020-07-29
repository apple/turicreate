# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ...models.tree_ensemble import TreeEnsembleRegressor, TreeEnsembleClassifier
from ...models._feature_management import process_or_validate_features

from ..._deps import _HAS_SKLEARN

if _HAS_SKLEARN:
    from sklearn.tree import _tree

import numpy as _np


def _get_value(scikit_value, mode="regressor", scaling=1.0, n_classes=2, tree_index=0):
    """ Get the right value from the scikit-tree
    """
    # Regression
    if mode == "regressor":
        return scikit_value[0] * scaling

    # Binary classification
    if n_classes == 2:
        # Decision tree
        if len(scikit_value[0]) != 1:
            value = scikit_value[0][1] * scaling / scikit_value[0].sum()
        # boosted tree
        else:
            value = scikit_value[0][0] * scaling
        if value == 0.5:
            value = value - 1e-7

    # Multiclass classification
    else:
        # Decision tree
        if len(scikit_value[0]) != 1:
            value = scikit_value[0] / scikit_value[0].sum()
        # boosted tree
        else:
            value = {tree_index: scikit_value[0] * scaling}
    return value


def _recurse(
    coreml_tree,
    scikit_tree,
    tree_id,
    node_id,
    scaling=1.0,
    mode="regressor",
    n_classes=2,
    tree_index=0,
):
    """Traverse through the tree and append to the tree spec.
    """
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )

    ## Recursion should not be called on the leaf node.
    if node_id == _tree.TREE_LEAF:
        raise ValueError("Invalid node_id %s" % _tree.TREE_LEAF)

    # Add a branch node to the tree
    if scikit_tree.children_left[node_id] != _tree.TREE_LEAF:
        branch_mode = "BranchOnValueLessThanEqual"
        feature_index = scikit_tree.feature[node_id]
        feature_value = scikit_tree.threshold[node_id]
        left_child_id = scikit_tree.children_left[node_id]
        right_child_id = scikit_tree.children_right[node_id]

        # Add a branch node
        coreml_tree.add_branch_node(
            tree_id,
            node_id,
            feature_index,
            feature_value,
            branch_mode,
            left_child_id,
            right_child_id,
        )

        # Now recurse
        _recurse(
            coreml_tree,
            scikit_tree,
            tree_id,
            left_child_id,
            scaling,
            mode,
            n_classes,
            tree_index,
        )
        _recurse(
            coreml_tree,
            scikit_tree,
            tree_id,
            right_child_id,
            scaling,
            mode,
            n_classes,
            tree_index,
        )

    # Add a leaf node to the tree
    else:
        # Get the scikit-learn value
        if scikit_tree.n_outputs != 1:
            raise ValueError("Expected only 1 output in the scikit-learn tree.")
        value = _get_value(
            scikit_tree.value[node_id], mode, scaling, n_classes, tree_index
        )
        coreml_tree.add_leaf_node(tree_id, node_id, value)


def get_input_dimension(model):
    if hasattr(model, "n_features_"):
        return model.n_features_

    elif hasattr(model, "n_estimators"):
        if model.n_estimators == 0:
            raise ValueError("model not trained.")

        try:
            return model.estimators_[0, 0].n_features_
        except IndexError:
            raise ValueError("Model not trained or invalid model.")
    else:
        raise ValueError("Unable to obtain input dimension from model.")


def convert_tree_ensemble(
    model,
    input_features,
    output_features=("predicted_class", float),
    mode="regressor",
    base_prediction=None,
    class_labels=None,
    post_evaluation_transform=None,
):
    """
    Convert a generic tree regressor model to the protobuf spec.

    This currently supports:
      * Decision tree regression
      * Gradient boosted tree regression
      * Random forest regression
      * Decision tree classifier.
      * Gradient boosted tree classifier.
      * Random forest classifier.

    ----------
    Parameters
    model: [DecisionTreeRegressor | GradientBoostingRegression | RandomForestRegressor]
        A scikit learn tree model.

    feature_names : list of strings, optional (default=None)
        Names of each of the features.

    target: str
        Name of the output column.

    base_prediction: double
        Base prediction value.

    mode: str in ['regressor', 'classifier']
        Mode of the tree model.

    class_labels: list[int]
        List of classes
        
    post_evaluation_transform: list[int]
        Post evaluation transform
        
    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """

    num_dimensions = get_input_dimension(model)
    features = process_or_validate_features(input_features, num_dimensions)

    n_classes = None
    if mode == "classifier":
        n_classes = model.n_classes_
        if class_labels is None:
            class_labels = range(n_classes)
        else:
            if len(class_labels) != n_classes:
                raise ValueError(
                    "Number of classes in model (%d) does not match "
                    "length of supplied class list (%d)."
                    % (n_classes, len(class_labels))
                )

        coreml_tree = TreeEnsembleClassifier(
            input_features, class_labels, output_features
        )
        if post_evaluation_transform is not None:
            coreml_tree.set_post_evaluation_transform(post_evaluation_transform)

        # Base prediction not provided
        if base_prediction is None:
            if n_classes == 2:
                base_prediction = [0.0]
            else:
                base_prediction = [0.0 for c in range(n_classes)]
        coreml_tree.set_default_prediction_value(base_prediction)
    else:
        if base_prediction is None:
            base_prediction = 0.0
        coreml_tree = TreeEnsembleRegressor(input_features, output_features)
        coreml_tree.set_default_prediction_value(base_prediction)

    # Single tree
    if hasattr(model, "tree_"):
        _recurse(
            coreml_tree,
            model.tree_,
            tree_id=0,
            node_id=0,
            mode=mode,
            n_classes=n_classes,
        )

    # Multiple trees
    elif hasattr(model, "estimators_"):
        is_ensembling_in_separate_trees = False
        if type(model.estimators_) != list:
            is_ensembling_in_separate_trees = (
                len(model.estimators_.shape) > 0 and model.estimators_.shape[1] > 1
            )
            estimators = model.estimators_.flatten()
        else:
            estimators = model.estimators_

        scaling = (
            model.learning_rate
            if hasattr(model, "learning_rate")
            else 1.0 / len(estimators)
        )
        for tree_id, base_model in enumerate(estimators):
            if is_ensembling_in_separate_trees:
                tree_index = tree_id % n_classes
            else:
                tree_index = 0
            _recurse(
                coreml_tree,
                base_model.tree_,
                tree_id,
                node_id=0,
                scaling=scaling,
                mode=mode,
                n_classes=n_classes,
                tree_index=tree_index,
            )
    else:
        raise TypeError("Unknown scikit-learn tree model type.")

    return coreml_tree.spec

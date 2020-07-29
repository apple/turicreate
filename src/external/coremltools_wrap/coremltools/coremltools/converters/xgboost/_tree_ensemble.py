# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ...models.tree_ensemble import (
    TreeEnsembleRegressor as _TreeEnsembleRegressor,
    TreeEnsembleClassifier,
)
from ..._deps import _HAS_XGBOOST

import numpy as _np
from six import string_types as _string_types

if _HAS_XGBOOST:
    import xgboost as _xgboost


def recurse_json(
    mlkit_tree,
    xgb_tree_json,
    tree_id,
    node_id,
    feature_map,
    force_32bit_float,
    mode="regressor",
    tree_index=0,
    n_classes=2,
):
    """Traverse through the tree and append to the tree spec.
    """
    relative_hit_rate = None

    try:
        relative_hit_rate = xgb_tree_json["cover"]
    except KeyError:
        pass

    # Fill node attributes
    if "leaf" not in xgb_tree_json:
        branch_mode = "BranchOnValueLessThan"
        split_name = xgb_tree_json["split"]
        feature_index = split_name if not feature_map else feature_map[split_name]

        # xgboost internally uses float32, but the parsing from json pulls it out
        # as a 64bit double.  To trigger the internal float32 detection in the
        # tree ensemble compiler, we need to explicitly cast it to a float 32
        # value, then back to the 64 bit float that protobuf expects.  This is
        # controlled with the force_32bit_float flag.
        feature_value = xgb_tree_json["split_condition"]

        if force_32bit_float:
            feature_value = float(_np.float32(feature_value))

        true_child_id = xgb_tree_json["yes"]
        false_child_id = xgb_tree_json["no"]

        # Get the missing value behavior correct
        missing_value_tracks_true_child = False

        try:
            if xgb_tree_json["missing"] == true_child_id:
                missing_value_tracks_true_child = True
        except KeyError:
            pass

        mlkit_tree.add_branch_node(
            tree_id,
            node_id,
            feature_index,
            feature_value,
            branch_mode,
            true_child_id,
            false_child_id,
            relative_hit_rate=relative_hit_rate,
            missing_value_tracks_true_child=missing_value_tracks_true_child,
        )

    else:
        value = xgb_tree_json["leaf"]
        if force_32bit_float:
            value = float(_np.float32(value))

        if mode == "classifier" and n_classes > 2:
            value = {tree_index: value}

        mlkit_tree.add_leaf_node(
            tree_id, node_id, value, relative_hit_rate=relative_hit_rate
        )

    # Now recurse
    if "children" in xgb_tree_json:
        for child in xgb_tree_json["children"]:
            recurse_json(
                mlkit_tree,
                child,
                tree_id,
                child["nodeid"],
                feature_map,
                force_32bit_float,
                mode=mode,
                tree_index=tree_index,
                n_classes=n_classes,
            )


def convert_tree_ensemble(
    model,
    feature_names,
    target,
    force_32bit_float,
    mode="regressor",
    class_labels=None,
    n_classes=None,
):
    """Convert a generic tree model to the protobuf spec.

    This currently supports:
      * Decision tree regression

    Parameters
    ----------
    model: str | Booster
        Path on disk where the XGboost JSON representation of the model is or
        a handle to the XGboost model.

    feature_names : list of strings or None
        Names of each of the features. When set to None, the feature names are
        extracted from the model.

    target: str,
        Name of the output column.

    force_32bit_float: bool
        If True, then the resulting CoreML model will use 32 bit floats internally.

    mode: str in ['regressor', 'classifier']
        Mode of the tree model.

    class_labels: list[int] or None
        List of classes. When set to None, the class labels are just the range from
        0 to n_classes - 1.

    n_classes: int or None
        Number of classes in classification. When set to None, the number of
        classes is expected from the model or class_labels should be provided.

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """
    if not (_HAS_XGBOOST):
        raise RuntimeError("xgboost not found. xgboost conversion API is disabled.")
    accepted_modes = ["regressor", "classifier"]
    if mode not in accepted_modes:
        raise ValueError("mode should be in %s" % accepted_modes)
    import json
    import os

    feature_map = None
    if isinstance(
        model, (_xgboost.core.Booster, _xgboost.XGBRegressor, _xgboost.XGBClassifier)
    ):

        # Testing a few corner cases that we don't support
        if isinstance(model, _xgboost.XGBRegressor):
            if mode == "classifier":
                raise ValueError("mode is classifier but provided a regressor")
            try:
                objective = model.get_xgb_params()["objective"]
            except:
                objective = None
            if objective in ["reg:gamma", "reg:tweedie"]:
                raise ValueError(
                    "Regression objective '%s' not supported for export." % objective
                )

        if isinstance(model, _xgboost.XGBClassifier):
            if mode == "regressor":
                raise ValueError("mode is regressor but provided a classifier")
            n_classes = model.n_classes_
            if class_labels is not None:
                if len(class_labels) != n_classes:
                    raise ValueError(
                        "Number of classes in model (%d) does not match "
                        "length of supplied class list (%d)."
                        % (n_classes, len(class_labels))
                    )
            else:
                class_labels = list(range(n_classes))

        # Now use the booster API.
        if isinstance(model, (_xgboost.XGBRegressor, _xgboost.XGBClassifier)):
            # Name change in 0.7
            if hasattr(model, "get_booster"):
                model = model.get_booster()
            else:
                model = model.booster()

        # Xgboost sometimes has feature names in there. Sometimes does not.
        if (feature_names is None) and (model.feature_names is None):
            raise ValueError(
                "The XGBoost model does not have feature names. They must be provided in convert method."
            )
            feature_names = model.feature_names
        if feature_names is None:
            feature_names = model.feature_names

        xgb_model_str = model.get_dump(with_stats=True, dump_format="json")

        if model.feature_names:
            feature_map = {f: i for i, f in enumerate(model.feature_names)}

    # Path on the file system where the XGboost model exists.
    elif isinstance(model, _string_types):
        if not os.path.exists(model):
            raise TypeError("Invalid path %s." % model)
        with open(model) as f:
            xgb_model_str = json.load(f)

        if feature_names is None:
            raise ValueError(
                "feature names must be provided in convert method if the model is a path on file system."
            )
        else:
            feature_map = {f: i for i, f in enumerate(feature_names)}

    else:
        raise TypeError("Unexpected type. Expecting XGBoost model.")

    if mode == "classifier":
        if n_classes is None and class_labels is None:
            raise ValueError(
                "You must provide class_labels or n_classes when not providing the XGBClassifier"
            )
        elif n_classes is None:
            n_classes = len(class_labels)
        elif class_labels is None:
            class_labels = range(n_classes)
        if n_classes == 2:
            # if we have only 2 classes we only have one sequence of estimators
            base_prediction = [0.0]
        else:
            base_prediction = [0.0 for c in range(n_classes)]
        # target here is the equivalent of output_features in scikit learn
        mlkit_tree = TreeEnsembleClassifier(feature_names, class_labels, target)
        mlkit_tree.set_default_prediction_value(base_prediction)
        if n_classes == 2:
            mlkit_tree.set_post_evaluation_transform("Regression_Logistic")
        else:
            mlkit_tree.set_post_evaluation_transform("Classification_SoftMax")
    else:
        mlkit_tree = _TreeEnsembleRegressor(feature_names, target)
        mlkit_tree.set_default_prediction_value(0.5)

    for xgb_tree_id, xgb_tree_str in enumerate(xgb_model_str):
        if mode == "classifier" and n_classes > 2:
            tree_index = xgb_tree_id % n_classes
        else:
            tree_index = 0

        try:
            # this means that the xgb_tree_str is a json dump and needs to be loaded
            xgb_tree_json = json.loads(xgb_tree_str)
        except:
            # this means that the xgb_tree_str is loaded from a path in file system already and does not need to be reloaded
            xgb_tree_json = xgb_tree_str

        recurse_json(
            mlkit_tree,
            xgb_tree_json,
            xgb_tree_id,
            node_id=0,
            feature_map=feature_map,
            force_32bit_float=force_32bit_float,
            mode=mode,
            tree_index=tree_index,
            n_classes=n_classes,
        )

    return mlkit_tree.spec

# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ...models.tree_ensemble import TreeEnsembleRegressor as _TreeEnsembleRegressor

from ..._deps import HAS_XGBOOST as _HAS_XGBOOST

import numpy as _np

if _HAS_XGBOOST:
    import xgboost as _xgboost

def recurse_json(mlkit_tree, xgb_tree_json, tree_id, node_id, feature_map,
        force_32bit_float):
    """Traverse through the tree and append to the tree spec.
    """
    relative_hit_rate = None

    try:
        relative_hit_rate = xgb_tree_json['cover']
    except KeyError:
        pass


    # Fill node attributes
    if 'leaf' not in xgb_tree_json:
        branch_mode = 'BranchOnValueLessThan'
        split_name = xgb_tree_json['split']
        feature_index = split_name if not feature_map else feature_map[split_name]

        # xgboost internally uses float32, but the parsing from json pulls it out
        # as a 64bit double.  To trigger the internal float32 detection in the
        # tree ensemble compiler, we need to explicitly cast it to a float 32
        # value, then back to the 64 bit float that protobuf expects.  This is
        # controlled with the force_32bit_float flag.
        feature_value = xgb_tree_json['split_condition']

        if force_32bit_float:
            feature_value = float(_np.float32(feature_value))


        true_child_id = xgb_tree_json['yes']
        false_child_id = xgb_tree_json['no']

        # Get the missing value behavior correct
        missing_value_tracks_true_child = False

        try:
            if xgb_tree_json['missing'] == true_child_id:
                missing_value_tracks_true_child = True
        except KeyError:
            pass

        mlkit_tree.add_branch_node(tree_id, node_id, feature_index,
                feature_value, branch_mode, true_child_id, false_child_id,
                relative_hit_rate = relative_hit_rate,
                missing_value_tracks_true_child = missing_value_tracks_true_child)

    else:
        value = xgb_tree_json["leaf"]

        if force_32bit_float:
            value = float(_np.float32(value))

        mlkit_tree.add_leaf_node(tree_id, node_id, value,
                relative_hit_rate = relative_hit_rate)

    # Now recurse
    if "children" in xgb_tree_json:
        for child in xgb_tree_json["children"]:
            recurse_json(mlkit_tree, child, tree_id, child['nodeid'], feature_map, force_32bit_float)

def convert_tree_ensemble(model, feature_names, target, force_32bit_float):
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

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """
    if not(_HAS_XGBOOST):
        raise RuntimeError('xgboost not found. xgboost conversion API is disabled.')

    import json
    import os
    feature_map = None
    if isinstance(model,  (_xgboost.core.Booster, _xgboost.XGBRegressor)):

        # Testing a few corner cases that we don't support
        if isinstance(model, _xgboost.XGBRegressor):
            try:
                objective = model.get_xgb_params()["objective"]
            except:
                objective = None
            if objective in ["reg:gamma", "reg:tweedie"]:
                raise ValueError("Regression objective '%s' not supported for export." % objective)

        # Now use the booster API.
        if isinstance(model, _xgboost.XGBRegressor):
            # Name change in 0.7
            if hasattr(model, 'get_booster'):
                model = model.get_booster()
            else:
                model = model.booster()

        # Xgboost sometimes has feature names in there. Sometimes does not.
        if (feature_names is None) and (model.feature_names is None):
            raise ValueError("Feature names not present in the model. Must be provided during conversion.")
            feature_names = model.feature_names
        if feature_names is None:
            feature_names = model.feature_names

        xgb_model_str = model.get_dump(with_stats=True, dump_format = 'json')

        if model.feature_names:
            feature_map = {f:i for i,f in enumerate(model.feature_names)}

    # Path on the file system where the XGboost model exists.
    elif isinstance(model, str):
        if not os.path.exists(model):
            raise TypeError("Invalid path %s." % model)
        with open(model) as f:
            xgb_model_str = json.load(f)
        feature_map = {f:i for i,f in enumerate(feature_names)}
    else:
        raise TypeError("Unexpected type. Expecting XGBoost model.")

    mlkit_tree = _TreeEnsembleRegressor(feature_names, target)
    mlkit_tree.set_default_prediction_value(0.5)
    for xgb_tree_id, xgb_tree_str in enumerate(xgb_model_str):
        xgb_tree_json = json.loads(xgb_tree_str)
        recurse_json(mlkit_tree, xgb_tree_json, xgb_tree_id, node_id = 0,
                feature_map = feature_map, force_32bit_float = force_32bit_float)

    return mlkit_tree.spec

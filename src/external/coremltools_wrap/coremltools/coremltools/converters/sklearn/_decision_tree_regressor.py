# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ._tree_ensemble import convert_tree_ensemble as _convert_tree_ensemble

from ..._deps import _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    import sklearn.tree as _tree
    from . import _sklearn_util

model_type = "regressor"
sklearn_class = _tree.DecisionTreeRegressor


def convert(model, feature_names, target):
    """Convert a decision tree model to protobuf format.

    Parameters
    ----------
    decision_tree : DecisionTreeRegressor
        A trained scikit-learn tree model.

    feature_names: [str]
        Name of the input columns.

    target: str
        Name of the output column.

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )

    _sklearn_util.check_expected_type(model, _tree.DecisionTreeRegressor)
    _sklearn_util.check_fitted(
        model, lambda m: hasattr(m, "tree_") and model.tree_ is not None
    )
    return _MLModel(_convert_tree_ensemble(model, feature_names, target))


def get_input_dimension(model):
    return model.n_features_

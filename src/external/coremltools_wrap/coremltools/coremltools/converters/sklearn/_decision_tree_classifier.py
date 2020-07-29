# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ._tree_ensemble import convert_tree_ensemble

from ..._deps import _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    import sklearn.tree as _tree
    from . import _sklearn_util

model_type = "classifier"
sklearn_class = _tree.DecisionTreeClassifier


def convert(model, input_name, output_features):
    """Convert a decision tree model to protobuf format.

    Parameters
    ----------
    decision_tree : DecisionTreeClassifier
        A trained scikit-learn tree model.

    input_name: str
        Name of the input columns.

    output_name: str
        Name of the output columns.

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )

    _sklearn_util.check_expected_type(model, _tree.DecisionTreeClassifier)
    _sklearn_util.check_fitted(
        model, lambda m: hasattr(m, "tree_") and model.tree_ is not None
    )

    return _MLModel(
        convert_tree_ensemble(
            model,
            input_name,
            output_features,
            mode="classifier",
            class_labels=model.classes_,
        )
    )


def supports_output_scores(model):
    return True


def get_output_classes(model):
    return list(model.classes_)


def get_input_dimension(model):
    return model.n_features_

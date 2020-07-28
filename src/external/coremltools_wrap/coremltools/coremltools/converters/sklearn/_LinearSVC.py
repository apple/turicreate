# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ..._deps import _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    from sklearn.svm import LinearSVC as _LinearSVC

    sklearn_class = _LinearSVC
    from . import _sklearn_util

from . import _logistic_regression

model_type = "classifier"


def convert(model, feature_names, target):
    """Convert a LinearSVC model to the protobuf spec.
    Parameters
    ----------
    model: LinearSVC
        A trained LinearSVC model.

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

    _sklearn_util.check_expected_type(model, _LinearSVC)
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "coef_"))

    return _MLModel(_logistic_regression._convert(model, feature_names, target))


def supports_output_scores(model):
    return True


def get_output_classes(model):
    return _logistic_regression.get_output_classes(model)


def get_input_dimension(model):
    return _logistic_regression.get_input_dimension(model)

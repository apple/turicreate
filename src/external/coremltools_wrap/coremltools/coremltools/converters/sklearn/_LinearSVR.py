# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ..._deps import _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    from sklearn.svm import LinearSVR as _LinearSVR
    import sklearn
    from . import _sklearn_util

    sklearn_class = sklearn.svm.LinearSVR

from . import _linear_regression

model_type = "regressor"


def convert(model, features, target):
    """Convert a LinearSVR model to the protobuf spec.
    Parameters
    ----------
    model: LinearSVR
        A trained LinearSVR model.

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

    # Check the scikit learn model
    _sklearn_util.check_expected_type(model, _LinearSVR)
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "coef_"))

    return _MLModel(_linear_regression._convert(model, features, target))


def get_input_dimension(model):
    return _linear_regression.get_input_dimension(model)

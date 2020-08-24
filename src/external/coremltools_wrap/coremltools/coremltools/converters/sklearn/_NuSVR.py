# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from . import _SVR as _SVR

from ..._deps import _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    from ._sklearn_util import check_fitted
    from sklearn.svm import NuSVR as _NuSVR
    from . import _sklearn_util

    sklearn_class = _NuSVR

model_type = "regressor"


def convert(model, feature_names, target):
    """Convert a Nu Support Vector Regression (NuSVR) model to the protobuf spec.
    Parameters
    ----------
    model: NuSVR
        A trained NuSVR encoder model.

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

    _sklearn_util.check_expected_type(model, _NuSVR)
    return _SVR.convert(model, feature_names, target)


def get_input_dimension(model):
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )

    check_fitted(model, lambda m: hasattr(m, "support_vectors_"))
    return _SVR.get_input_dimension(model)

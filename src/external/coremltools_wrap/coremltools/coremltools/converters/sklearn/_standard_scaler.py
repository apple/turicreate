# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


from ... import SPECIFICATION_VERSION
from ...models._interface_management import (
    set_transform_interface_params as _set_transform_interface_params,
)
from ...proto import Model_pb2 as _Model_pb2
from ...proto import FeatureTypes_pb2 as _FeatureTypes_pb2

from ..._deps import _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    from . import _sklearn_util
    import sklearn
    from sklearn.preprocessing import StandardScaler

    sklearn_class = StandardScaler

model_type = "transformer"


def convert(model, input_features, output_features):
    """Convert a _imputer model to the protobuf spec.

    Parameters
    ----------
    model: Imputer
        A trained Imputer model.

    input_features: str
        Name of the input column.

    output_features: str
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

    # Test the scikit-learn model
    _sklearn_util.check_expected_type(model, StandardScaler)
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "mean_"))
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "scale_"))

    # Set the interface params.
    spec = _Model_pb2.Model()
    spec.specificationVersion = SPECIFICATION_VERSION
    spec = _set_transform_interface_params(spec, input_features, output_features)

    # Set the parameters
    tr_spec = spec.scaler
    for x in model.mean_:
        tr_spec.shiftValue.append(-x)

    for x in model.scale_:
        tr_spec.scaleValue.append(1.0 / x)

    return _MLModel(spec)


def update_dimension(model, input_dimension):
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )

    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "mean_"))
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "scale_"))
    # Nothing to do for this model
    return input_dimension


def get_input_dimension(model):
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )

    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "mean_"))
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "scale_"))
    return len(model.mean_)

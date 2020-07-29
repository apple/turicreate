# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


from ... import SPECIFICATION_VERSION
from ...models._interface_management import (
    set_transform_interface_params as _set_transform_interface_params,
)
from ...proto import Model_pb2 as _Model_pb2
from ...proto.Normalizer_pb2 import Normalizer as _proto__normalizer

from ..._deps import _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    from . import _sklearn_util
    from sklearn.preprocessing import Normalizer

    sklearn_class = Normalizer

model_type = "transformer"


def convert(model, input_features, output_features):
    """Convert a normalizer model to the protobuf spec.

    Parameters
    ----------
    model: Normalizer
        A Normalizer.

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
    _sklearn_util.check_expected_type(model, Normalizer)
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "norm"))

    # Set the interface params.
    spec = _Model_pb2.Model()
    spec.specificationVersion = SPECIFICATION_VERSION
    spec = _set_transform_interface_params(spec, input_features, output_features)

    # Set the one hot encoder parameters
    _normalizer_spec = spec.normalizer
    if model.norm == "l1":
        _normalizer_spec.normType = _proto__normalizer.L1
    elif model.norm == "l2":
        _normalizer_spec.normType = _proto__normalizer.L2
    elif model.norm == "max":
        _normalizer_spec.normType = _proto__normalizer.LMax
    return _MLModel(spec)


def update_dimension(model, input_dimension):
    """
    Given a model that takes an array of dimension input_dimension, returns
    the output dimension.
    """

    # No change
    return input_dimension


def get_input_dimension(model):
    # Cannot determine this now.
    return None

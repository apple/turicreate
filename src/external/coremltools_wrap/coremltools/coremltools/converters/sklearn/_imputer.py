# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from . import _sklearn_util
from ... import SPECIFICATION_VERSION
from ...models._interface_management import set_transform_interface_params
from ...proto import Model_pb2 as _Model_pb2
from ...models import datatypes
from ...models import MLModel as _MLModel

from ..._deps import HAS_SKLEARN as _HAS_SKLEARN

if _HAS_SKLEARN:
    import sklearn
    from sklearn.preprocessing import Imputer
    model_type = 'transformer'
    sklearn_class = sklearn.preprocessing.Imputer

def convert(model, input_features, output_features):
    """Convert a DictVectorizer model to the protobuf spec.

    Parameters
    ----------
    model: DictVectorizer
        A fitted DictVectorizer model.

    input_features: str
        Name of the input column.

    output_features: str
        Name of the output column.

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """
    if not(_HAS_SKLEARN):
        raise RuntimeError('scikit-learn not found. scikit-learn conversion API is disabled.')
    
    # Set the interface params.
    spec = _Model_pb2.Model()
    spec.specificationVersion = SPECIFICATION_VERSION

    assert len(input_features) == 1
    assert isinstance(input_features[0][1], datatypes.Array)

    # feature name in and out are the same here
    spec = set_transform_interface_params(spec, input_features, output_features)

    # Test the scikit-learn model
    _sklearn_util.check_expected_type(model, Imputer)
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, 'statistics_'))
    
    if model.axis != 0:
        raise ValueError("Imputation is only supported along axis = 0.")


    # The imputer in our framework only works on single columns, so
    # we need to translate that over.  The easiest way to do that is to 
    # put it in a nested pipeline with a feature extractor and a 

    tr_spec = spec.imputer

    for v in model.statistics_:
        tr_spec.imputedDoubleArray.vector.append(v)

    try:
        tr_spec.replaceDoubleValue = float(model.missing_values)
    except ValueError:
        raise ValueError("Only scalar values or NAN as missing_values "
                "in _imputer are supported.")

    return _MLModel(spec)


def update_dimension(model, input_dimension):
    """
    Given a model that takes an array of dimension input_dimension, returns
    the output dimension.
    """

    # This doesn't expand anything.
    return input_dimension

def get_input_dimension(model):
    if not(_HAS_SKLEARN):
        raise RuntimeError('scikit-learn not found. scikit-learn conversion API is disabled.')
    
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, 'statistics_'))
    return len(model.statistics_)



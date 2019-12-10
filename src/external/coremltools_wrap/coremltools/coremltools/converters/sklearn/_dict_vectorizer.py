# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import six as _six

from . import _sklearn_util

from ... import SPECIFICATION_VERSION
from ...models._interface_management import set_transform_interface_params
from ...proto import Model_pb2 as _Model_pb2
from ...proto import FeatureTypes_pb2 as _FeatureTypes_pb2
from ...models._feature_management import process_or_validate_features
from ...models.feature_vectorizer import create_feature_vectorizer
from ...models import MLModel as _MLModel

from ..._deps import HAS_SKLEARN as _HAS_SKLEARN
if _HAS_SKLEARN:
    from sklearn.feature_extraction import DictVectorizer
    sklearn_class = DictVectorizer

from ...models import datatypes
from ...models.pipeline import Pipeline

model_type = 'transformer'


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

    _INTERMEDIATE_FEATURE_NAME = "__sparse_vector_features__"

    n_dimensions = len(model.feature_names_)
    input_features = process_or_validate_features(input_features)

    # Ensure that the output_features are also solid.
    output_features = process_or_validate_features(output_features, n_dimensions)

    # The DictVectorizer in the framework outputs a sparse dictionary 
    # of index to value due to other considerations, but we are expecting 
    # the output of this to be a dense feature vector.  To make that happen, 
    # put a feature_vectorizer immediately after the dict vectorizer.
    pline = Pipeline(input_features, output_features)

    # Set the basic model parameters of the dict vectorizer component.
    dv_spec = _Model_pb2.Model()
    dv_spec.specificationVersion = SPECIFICATION_VERSION

    # Set up the dict vectorizer parameters
    tr_spec = dv_spec.dictVectorizer
    is_str = None
    for feature_name in model.feature_names_:
        if isinstance(feature_name, _six.string_types):
            if is_str == False:
                raise ValueError("Mapping of DictVectorizer mixes int and str types.")
            
            tr_spec.stringToIndex.vector.append(feature_name)
            is_str == True

        if isinstance(feature_name, _six.integer_types):
            if is_str == True:
                raise ValueError("Mapping of DictVectorizer mixes int and str types.")
            
            tr_spec.int64ToIndex.vector.append(feature_name)
            is_str == False

    intermediate_features = [(_INTERMEDIATE_FEATURE_NAME, 
        datatypes.Dictionary(key_type = int))]

    # Set the interface for the dict vectorizer with the input and the 
    # intermediate output
    set_transform_interface_params(
            dv_spec, input_features, intermediate_features)

    pline.add_model(dv_spec)

    # Follow the dict vectorizer by a feature_vectorizer to change the sparse 
    # output layer into a dense vector as expected.
    fvec, _num_out_dim = create_feature_vectorizer(intermediate_features,
            output_features[0][0], {"__sparse_vector_features__" : n_dimensions})

    pline.add_model(fvec) 

    return _MLModel(pline.spec)

def update_dimension(m, current_num_dimensions): 
    return len(m.feature_names_)

def get_input_dimension(m):
    return None

def get_input_feature_names(m): 
    return m.feature_names_

# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from six import integer_types as _integer_types

from . import datatypes
from .. import SPECIFICATION_VERSION
from ..proto import Model_pb2 as _Model_pb2
from ..proto import FeatureTypes_pb2 as _FeatureTypes_pb2
from ._interface_management import set_transform_interface_params



def create_array_feature_extractor(input_features, output_name, extract_indices, 
                                   output_type = None): 
    """
    Creates a feature extractor from an input array feature, return

    input_features is a list of one (name, array) tuple. 

    extract_indices is either an integer or a list.  If it's an integer, 
    the output type is by default a double (but may also be an integer).
    If a list, the output type is an array. 
    """

    # Make sure that our starting stuff is in the proper form.
    assert len(input_features) == 1
    assert isinstance(input_features[0][1], datatypes.Array)

    # Create the model. 
    spec = _Model_pb2.Model()
    spec.specificationVersion = SPECIFICATION_VERSION

    if isinstance(extract_indices, _integer_types):
        extract_indices = [extract_indices]
        if output_type is None:
            output_type = datatypes.Double()

    elif isinstance(extract_indices, (list, tuple)):
        if not all(isinstance(x, _integer_types) for x in extract_indices):
            raise TypeError("extract_indices must be an integer or a list of integers.")

        if output_type is None:
            output_type = datatypes.Array(len(extract_indices))

    else:
        raise TypeError("extract_indices must be an integer or a list of integers.")

    output_features = [(output_name, output_type)]

    for idx in extract_indices:
        assert idx < input_features[0][1].num_elements
        spec.arrayFeatureExtractor.extractIndex.append(idx)

    set_transform_interface_params(spec, input_features, output_features)

    return spec    



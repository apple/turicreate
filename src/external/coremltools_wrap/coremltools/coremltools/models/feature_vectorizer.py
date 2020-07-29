# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from six import string_types as _string_types

from . import datatypes
from .. import SPECIFICATION_VERSION
from ..proto import Model_pb2 as _Model_pb2
from ..proto import FeatureTypes_pb2 as _FeatureTypes_pb2
from ._interface_management import set_transform_interface_params
from ._feature_management import is_valid_feature_list, process_or_validate_features


def create_feature_vectorizer(input_features, output_feature_name, known_size_map={}):
    """
    Creates a feature vectorizer from input features, return the spec for
    a feature vectorizer that puts everything into a single array of length
    equal to the total size of all the input features.  Returns a 2-tuple
    `(spec, num_dimension)`

    Parameters
    ----------
    input_features: [list of 2-tuples]
        Name(s) of the input features, given as a list of `('name', datatype)`
        tuples.  The datatypes entry is one of the data types defined in the
        :ref:`datatypes` module.  Allowed datatypes are :ref:`datatype.Int64`,
        :ref:`datatype.Double`, :ref:`datatypes.Dictionary`,
        or :ref:`datatype.Array`.

        If the feature is a dictionary type, then the dictionary must have integer
        keys, and the number of dimensions to expand it into must be given by
        `known_size_map`.

        Feature indices in the final array are counted sequentially from the
        from 0 through the total number of features.


    output_feature_name: str
        The name of the output feature.  The type is an Array
        List of output feature of the network.

    known_size_map:
        A dictionary mapping the feature name to the expanded size in the final
        array.  This is most useful for specifying the size of sparse vectors
        given as dictionaries of index to value.

    """

    spec = _Model_pb2.Model()
    spec.specificationVersion = SPECIFICATION_VERSION

    input_features = process_or_validate_features(input_features)

    feature_vectorizer = spec.featureVectorizer

    num_output_dimensions = 0

    for n, ft in input_features:
        if n in known_size_map:
            dim = known_size_map[n]

            if ft.num_elements is not None:
                if dim != ft.num_elements:
                    raise ValueError(
                        "In feature {}, override size {} not compatible with inherent "
                        "value size {}.".format(n, dim, ft.num_elements)
                    )
        else:
            if ft.num_elements is None:
                raise ValueError(
                    "In feature {}, inherent size unknown so must be manually supplied.".format(
                        n
                    )
                )
            dim = ft.num_elements

        num_output_dimensions += dim

        new_feature = feature_vectorizer.inputList.add()
        new_feature.inputColumn = n
        new_feature.inputDimensions = dim

    if not isinstance(output_feature_name, _string_types):
        if (
            is_valid_feature_list(output_feature_name)
            and len(output_feature_name) == 1
            and output_feature_name[0][1] == datatypes.Array(num_output_dimensions)
        ):

            output_feature_name = output_feature_name[0][0]

        else:
            raise TypeError(
                "Output feature must be specified as a feature name or correct output feature list."
            )

    output_features = [(output_feature_name, datatypes.Array(num_output_dimensions))]
    set_transform_interface_params(spec, input_features, output_features)

    return spec, num_output_dimensions

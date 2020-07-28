# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from collections import defaultdict
from copy import copy
from functools import reduce
import numpy as _np
import operator as op
from six import (
    integer_types as _integer_types,
    string_types as _string_types,
)

from . import datatypes
from .. import SPECIFICATION_VERSION
from ..proto import Model_pb2 as _Model_pb2
from ..proto import FeatureTypes_pb2 as _FeatureTypes_pb2


def process_or_validate_classifier_output_features(
    output_features, class_labels, supports_class_scores=True
):
    """
    Given a list of class labels and a list of output_features, validate the
    list and return a valid version of output_features with all the correct
    data type information included.
    """

    def raise_error(msg):

        raise ValueError("Classifier error: %s" % msg)

    class_labels = list(class_labels)

    # First, we need to determine the type of the classes.
    _int_types = _integer_types + (bool, _np.bool_, _np.int32, _np.int64)

    if all(isinstance(cl, _int_types) for cl in class_labels):
        output_class_type = datatypes.Int64()

    elif all(isinstance(cl, _string_types) for cl in class_labels):
        output_class_type = datatypes.String()

    else:
        raise ValueError("Class labels must be all of type int or all of type string.")

    if output_features is None:

        out = [("classLabel", output_class_type)]

        if supports_class_scores:
            out += [("classProbability", datatypes.Dictionary(output_class_type))]

    elif isinstance(output_features, _string_types):

        out = [(output_features, output_class_type)]

        if supports_class_scores:
            out += [("classProbability", datatypes.Dictionary(output_class_type))]

    elif (
        isinstance(output_features, (list, tuple))
        and all(isinstance(fn, _string_types) for fn in output_features)
        and len(output_features) == 2
    ):

        if supports_class_scores:
            out = [
                (output_features[0], output_class_type),
                (output_features[1], datatypes.Dictionary(output_class_type)),
            ]
        else:
            raise ValueError(
                "Classifier model (as trained) does not support output scores for classes."
            )

    elif is_valid_feature_list(output_features):

        output_features = [
            (k, datatypes._normalize_datatype(dt)) for k, dt in output_features
        ]

        if len(output_features) == 1 or not supports_class_scores:
            if not output_features[0][1] == output_class_type:
                raise ValueError(
                    "Type of output class feature does not match type of class labels."
                )

        else:
            # Make sure the first two output features specified give the output
            # class field and the output class scores dictionary field
            if isinstance(output_features[0][1], datatypes.Dictionary) and isinstance(
                output_features[1][1], output_class_type
            ):
                output_features[0], output_features[1] = (
                    output_features[1],
                    output_features[0],
                )

            if not isinstance(output_features[1][1], datatypes.Dictionary):
                raise_error("Output features class scores should be dictionary type.")

            if output_features[1][1].key_type != output_class_type:
                raise_error(
                    "Class scores dictionary key type does not match type of class labels."
                )

            if output_features[0][1] != output_class_type:
                raise_error(
                    "Specified type of output class does not match type of class labels."
                )

        # NOTE: We are intentionally allowing the case where additional fields are allowed
        # beyond the original two features.

        out = output_features

    else:
        raise_error("Form of output features not recognized")

    return out


def is_valid_feature_list(features):
    # Just test all the ways this could be
    return (
        type(features) is list
        and len(features) >= 1
        and all(type(t) is tuple and len(t) == 2 for t in features)
        and all(isinstance(n, _string_types) for n, td in features)
        and all(datatypes._is_valid_datatype(td) for n, td in features)
    )


def dimension_of_array_features(features):
    if not is_valid_feature_list(features):
        raise ValueError("Expected feature list in valid form.")

    dim = 0
    for n, td in features:
        if isinstance(td, (datatypes.Int64, datatypes.Double)):
            dim += 1
        elif isinstance(td, datatypes.Array):
            dim += reduce(op.mul, td.dimensions, 1)
        else:
            raise ValueError(
                "Unable to determine number of dimensions from feature list."
            )

    return dim


def process_or_validate_features(features, num_dimensions=None, feature_type_map={}):
    """
    Puts features into a standard form from a number of different possible forms.

    The standard form is a list of 2-tuples of (name, datatype) pairs.  The name
    is a string and the datatype is an object as defined in the _datatype module.

    The possible input forms are as follows:

    *   A list of strings. in this case, the overall dimension is assumed to be
        the length of the list.  If neighboring names are identical, they are
        assumed to be an input array of that length.  For example:

           ["a", "b", "c"]

        resolves to

            [("a", Double), ("b", Double), ("c", Double)].

        And:

            ["a", "a", "b"]

        resolves to

            [("a", Array(2)), ("b", Double)].

    *   A dictionary of keys to indices or ranges of feature indices.

        In this case, it's presented as a mapping from keys to indices or
        ranges of contiguous indices.  For example,

            {"a" : 0, "b" : [2,3], "c" : 1}

        Resolves to

            [("a", Double), ("c", Double), ("b", Array(2))].

        Note that the ordering is determined by the indices.

    *   A single string.  In this case, the input is assumed to be a single array,
        with the number of dimensions set using num_dimensions.


    Notes:

    If the features variable is in the standard form, it is simply checked and
    returned.

    If num_dimensions is given, it is used to check against the existing features,
    or fill in missing information in the case when features is a single string.
    """

    original_features = copy(features)

    if num_dimensions is not None and not isinstance(num_dimensions, _integer_types):
        raise TypeError(
            "num_dimensions must be None, an integer or a long, not '%s'"
            % str(type(num_dimensions))
        )

    def raise_type_error(additional_msg):
        raise TypeError(
            "Error processing feature list: %s\nfeatures = %s"
            % (additional_msg, str(original_features))
        )

    if type(features) is dict and is_valid_feature_list(features.items()):
        features = features.items()

    # First, see if the features are already in the correct form.  If they are,
    # then we
    if is_valid_feature_list(features):
        if num_dimensions is not None:
            try:
                feature_dims = dimension_of_array_features(features)
            except ValueError:
                feature_dims = None

            if feature_dims is not None and feature_dims != num_dimensions:
                raise_type_error("Dimension mismatch.")

        # We may need to translate some parts of this back to the actual
        # datatype class -- e.g. translate str to datatypes.String().
        return [(k, datatypes._normalize_datatype(dt)) for k, dt in features]

    if isinstance(features, _string_types):
        if num_dimensions is None:
            raise_type_error(
                "If a single feature name is given, then "
                "num_dimensions must be provided."
            )
        features = {features: range(num_dimensions)}

    if isinstance(features, (list, tuple, _np.ndarray)):
        # Change this into a dictionary

        mapping = defaultdict(lambda: [])

        for i, k in enumerate(features):
            if not isinstance(k, _string_types):
                raise_type_error(
                    "List of feature names must either be a list of strings, or a list of (name, datatypes.Array instance) tuples."
                )

        if num_dimensions is not None and len(features) != num_dimensions:
            raise_type_error(
                ("List of feature names has wrong length; " "%d required, %d provided.")
                % (num_dimensions, len(features))
            )

        for i, k in enumerate(features):
            mapping[k].append(i)

        # Replace the features
        features = mapping

    if not isinstance(features, dict):
        raise_type_error(
            "features must be either a list of feature names "
            "or a dictionary of feature names to ranges."
        )

    # We'll be invasive here so make a copy.
    features = copy(features)

    for k, v in list(features.items()):

        if not isinstance(k, _string_types):
            raise_type_error("Feature names must be strings.")

        def test_index(val):
            error = False
            try:
                if val != int(val):
                    error = True
            except:
                error = True

            if error:
                raise_type_error(
                    "Specified indices for feature %s must be integers." % k
                )

            if val < 0 or (num_dimensions is not None and val >= num_dimensions):
                raise_type_error("Index in feature %s out of range." % k)

        iterable_types = [tuple, list, set]
        iterable_types.append(range)
        if isinstance(v, tuple(iterable_types)):
            for idx in v:
                test_index(idx)

            # Replace and update
            features[k] = v = list(sorted(v))

        elif isinstance(v, _integer_types):
            test_index(v)
            features[k] = v = [v]
        else:
            raise_type_error(
                (
                    "Value type for feature %s not recognized; "
                    "values must be either integers, lists or range objects."
                )
                % k
            )

        # check to make sure things are contiguous
        if v != list(range(v[0], v[-1] + 1)):
            raise_type_error(
                "Index list for feature %s must consist of "
                "a contiguous range of indices." % k
            )

        if len(set(v)) != len(v):
            raise_type_error("Index list for feature %s contains duplicates." % k)

    # Now, set num dimensions from the list if it's actually None
    if num_dimensions is None:
        from itertools import chain
        num_dimensions = 1 + max(chain.from_iterable(features.values()))

    if (
        set().union(*features.values()) != set(range(num_dimensions))
        or sum(len(v) for v in features.values()) != num_dimensions
    ):
        raise_type_error(
            "Supplied indices must cover entire range of 0, ..., num_dimensions-1."
        )

    # Define the output feature types
    output_features = [None] * len(features)

    # Finally, go through and map all these things out as types.
    # Sort by first value of the index range.
    for i, (k, v) in enumerate(sorted(features.items(), key=lambda t: t[1][0])):
        if k in feature_type_map:
            output_features[i] = (k, feature_type_map[k])

        elif len(v) == 1:
            output_features[i] = (k, datatypes.Double())
        else:
            output_features[i] = (k, datatypes.Array(len(v)))

    return output_features

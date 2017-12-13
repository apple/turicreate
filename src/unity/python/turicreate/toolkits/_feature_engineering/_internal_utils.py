# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import copy as _copy
from turicreate.util import _raise_error_if_not_of_type
import logging as _logging
import os as _os
from distutils.version import LooseVersion
from six.moves.urllib.request import urlretrieve
import zipfile

NoneType = type(None)

def select_valid_features(data, feature_columns, valid_types):
    valid_features = []

    for f in feature_columns:
        if data[f].dtype in valid_types:
            valid_features.append(f)
        else:
            _logging.warning("Warning: Column " + f + " is excluded due to" +
                            " invalid column type " + str(data[f].dtype))
    return valid_features

def select_feature_subset(data, feature_columns):
    total_set = set(data.column_names())
    feature_set = set(feature_columns)

    result = total_set.intersection(feature_set)

    if len(result) != len(feature_set):
        _logging.warning("Warning: The model was fit with " + str(len(feature_columns)) + " feature columns but only " + str(len(result)) + " were present during transform()." +" Proceeding with transform by ignoring the missing columns.")

    return [f for f in feature_columns if f in result]

def get_column_names(data, interpret_as_excluded, column_names):

    assert interpret_as_excluded in [True, False]
    assert type(column_names) in [list, type(None)]

    all_columns = data.column_names()

    if column_names is None:
        selected_columns = all_columns
    else:
        selected_columns = column_names

    if interpret_as_excluded:
        exclude_set = set(selected_columns)
        return [c for c in all_columns if c not in exclude_set]
    else:
        return selected_columns



def validate_feature_columns(data_column_names, feature_column_names):

    if len(feature_column_names) == 0:
        raise RuntimeError("None of the selected features have valid type.")

    set_difference = set(feature_column_names) - set(data_column_names)

    if len(set_difference) > 0:
        err = 'Feature(s) '
        for s in range(len(set_difference) - 1):
            err = err + str(list(set_difference)[s]) + ", "
        err = err + str(list(set_difference).pop()) + " are missing from the dataset."
        raise ValueError(err)


def validate_feature_types(feature_names, feature_types, data):
    for col_name in feature_names:
        if data[col_name].dtype != feature_types[col_name]:
            err = "Column '" + col_name + "' was of type " + \
                    str(feature_types[col_name]) + " when fitted using .fit() but is of type " +\
                    str(data[col_name].dtype) + "during .transform()"
            raise ValueError(err)

def process_features(features, exclude):
    """
    Parameters
    ----------
    features : list[str] | str | None, optional
        Column names of features to be transformed. If None, all columns
        are selected. If string, that column is transformed. If list of strings,
        this list of column names is selected.

    exclude : list[str] | str | None, optional
        Column names of features to be ignored in transformation. Can be string
        or list of strings. Either 'exclude' or 'features' can be passed, but
        not both.

    Returns
    -------
    (features, exclude) that are processed.

    """

    # Check types
    _raise_error_if_not_of_type(features, [NoneType, str, list], 'features')
    _raise_error_if_not_of_type(exclude, [NoneType, str, list], 'exclude')

    # Make a copy of the parameters.
    _features = _copy.copy(features)
    _exclude = _copy.copy(exclude)

    # Check of both are None or empty.
    if _features and _exclude:
        raise ValueError("The parameters 'features' and 'exclude' cannot both be set."
                " Please set one or the other.")
    if _features == [] and not _exclude:
        raise ValueError("Features cannot be an empty list.")


    # Allow a single list
    _features = [_features] if type(_features) == str else _features
    _exclude = [_exclude] if type(_exclude) == str else _exclude

    # Type check each feature/exclude
    if _features:
        for f in _features:
            _raise_error_if_not_of_type(f, str, "Feature names")
    if _exclude:
        for e in _exclude:
            _raise_error_if_not_of_type(e, str, "Excluded feature names")

    if _exclude is not None and _features is not None:
        feature_set = set(_features)
        for col_name in _exclude:
            if col_name in feature_set:
                raise ValueError("'%s' appears in both features and excluded_features." % col_name)

    return _features, _exclude


def pretty_print_list(lst, name = 'features', repr_format=True):
    """ Pretty print a list to be readable.
    """
    if not lst or len(lst) < 8:
        if repr_format:
            return lst.__repr__()
        else:
            return ', '.join(map(str, lst))
    else:
        topk = ', '.join(map(str, lst[:3]))
        if repr_format:
            lst_separator = "["
            lst_end_separator = "]"
        else:
            lst_separator = ""
            lst_end_separator = ""

        return "{start}{topk}, ... {last}{end} (total {size} {name})".format(\
                topk = topk, last = lst[-1], name = name, size = len(lst),
                start = lst_separator, end = lst_end_separator)


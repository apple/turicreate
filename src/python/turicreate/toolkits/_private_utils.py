# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from turicreate.toolkits._main import ToolkitError
import logging as _logging


def _validate_row_label(label, column_type_map):
    """
    Validate a row label column.

    Parameters
    ----------
    label : str
        Name of the row label column.

    column_type_map : dict[str, type]
        Dictionary mapping the name of each column in an SFrame to the type of
        the values in the column.
    """
    if not isinstance(label, str):
        raise TypeError("The row label column name must be a string.")

    if not label in column_type_map.keys():
        raise ToolkitError("Row label column not found in the dataset.")

    if not column_type_map[label] in (str, int):
        raise TypeError("Row labels must be integers or strings.")


def _robust_column_name(base_name, column_names):
    """
    Generate a new column name that is guaranteed not to conflict with an
    existing set of column names.

    Parameters
    ----------
    base_name : str
        The base of the new column name. Usually this does not conflict with
        the existing column names, in which case this function simply returns
        `base_name`.

    column_names : list[str]
        List of existing column names.

    Returns
    -------
    robust_name : str
        The new column name. If `base_name` isn't in `column_names`, then
        `robust_name` is the same as `base_name`. If there are conflicts, a
        numeric suffix is added to `base_name` until it no longer conflicts
        with the column names.
    """
    robust_name = base_name
    i = 1

    while robust_name in column_names:
        robust_name = base_name + '.{}'.format(i)
        i += 1

    return robust_name

def _select_valid_features(dataset, features, valid_feature_types,
                           target_column=None):
    """
    Utility function for selecting columns of only valid feature types.

    Parameters
    ----------
    dataset: SFrame
        The input SFrame containing columns of potential features.

    features: list[str]
        List of feature column names.  If None, the candidate feature set is
        taken to be all the columns in the dataset.

    valid_feature_types: list[type]
        List of Python types that represent valid features.  If type is array.array,
        then an extra check is done to ensure that the individual elements of the array
        are of numeric type.  If type is dict, then an extra check is done to ensure
        that dictionary values are numeric.

    target_column: str
        Name of the target column.  If not None, the target column is excluded
        from the list of valid feature columns.

    Returns
    -------
    out: list[str]
        List of valid feature column names.  Warnings are given for each candidate
        feature column that is excluded.

    Examples
    --------
    # Select all the columns of type `str` in sf, excluding the target column named
    # 'rating'
    >>> valid_columns = _select_valid_features(sf, None, [str], target_column='rating')

    # Select the subset of columns 'X1', 'X2', 'X3' that has dictionary type or defines
    # numeric array type
    >>> valid_columns = _select_valid_features(sf, ['X1', 'X2', 'X3'], [dict, array.array])
    """
    if features is not None:
        if not hasattr(features, '__iter__'):
            raise TypeError("Input 'features' must be an iterable type.")

        if not all([isinstance(x, str) for x in features]):
            raise TypeError("Input 'features' must contain only strings.")

    ## Extract the features and labels
    if features is None:
        features = dataset.column_names()

    col_type_map = {
        col_name: col_type for (col_name, col_type) in
        zip(dataset.column_names(), dataset.column_types())}

    valid_features = []
    for col_name in features:

        if col_name not in dataset.column_names():
            _logging.warning("Column '{}' is not in the input dataset.".format(col_name))

        elif col_name == target_column:
            _logging.warning("Excluding target column " + target_column + " as a feature.")

        elif col_type_map[col_name] not in valid_feature_types:
            _logging.warning("Column '{}' is excluded as a ".format(col_name) +
                             "feature due to invalid column type.")

        else:
            valid_features.append(col_name)

    if len(valid_features) == 0:
        raise ValueError("The dataset does not contain any valid feature columns. " +
                         "Accepted feature types are " + str(valid_feature_types) + ".")

    return valid_features

def _check_elements_equal(lst):
    """
    Returns true if all of the elements in the list are equal.
    """
    assert isinstance(lst, list), "Input value must be a list."
    return not lst or lst.count(lst[0]) == len(lst)

def _validate_lists(sa, allowed_types=[str], require_same_type=True,
                    require_equal_length=False, num_to_check=10):
    """
    For a list-typed SArray, check whether the first elements are lists that
    - contain only the provided types
    - all have the same lengths (optionally)

    Parameters
    ----------
    sa : SArray
        An SArray containing lists.

    allowed_types : list
        A list of types that are allowed in each list.

    require_same_type : bool
        If true, the function returns false if more than one type of object
        exists in the examined lists.

    require_equal_length : bool
        If true, the function requires false when the list lengths differ.

    Returns
    -------
    out : bool
        Returns true if all elements are lists of equal length and containing
        only ints or floats. Otherwise returns false.
    """
    if len(sa) == 0:
        return True

    first_elements = sa.head(num_to_check)
    if first_elements.dtype != list:
        raise ValueError("Expected an SArray of lists when type-checking lists.")

    # Check list lengths
    list_lengths = list(first_elements.item_length())
    same_length = _check_elements_equal(list_lengths)
    if require_equal_length and not same_length:
        return False

    # If list lengths are all zero, return True.
    if len(first_elements[0]) == 0:
        return True

    # Check for matching types within each list
    types = first_elements.apply(lambda xs: [str(type(x)) for x in xs])
    same_type = [_check_elements_equal(x) for x in types]
    all_same_type = _check_elements_equal(same_type)
    if require_same_type and not all_same_type:
        return False

    # Check for matching types across lists
    first_types = [t[0] for t in types if t]
    all_same_type = _check_elements_equal(first_types)
    if require_same_type and not all_same_type:
        return False

    # Check to make sure all elements have types that are allowed
    allowed_type_strs = [str(x) for x in allowed_types]
    for list_element_types in types:
        for t in list_element_types:
            if t not in allowed_type_strs:
                return False

    return True

def _summarize_accessible_fields(field_descriptions, width=40,
                                 section_title='Accessible fields'):
    """
    Create a summary string for the accessible fields in a model. Unlike
    `_toolkit_repr_print`, this function does not look up the values of the
    fields, it just formats the names and descriptions.

    Parameters
    ----------
    field_descriptions : dict{str: str}
        Name of each field and its description, in a dictionary. Keys and
        values should be strings.

    width : int, optional
        Width of the names. This is usually determined and passed by the
        calling `__repr__` method.

    section_title : str, optional
        Name of the accessible fields section in the summary string.

    Returns
    -------
    out : str
    """
    key_str = "{:<{}}: {}"

    items = []
    items.append(section_title)
    items.append("-" * len(section_title))

    for field_name, field_desc in field_descriptions.items():
        items.append(key_str.format(field_name, width, field_desc))

    return "\n".join(items)

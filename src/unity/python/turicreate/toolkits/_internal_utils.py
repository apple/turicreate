# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
##\internal
"""@package turicreate.toolkits

This module defines the (internal) utility functions used by the toolkits.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ..data_structures.sframe import SArray as _SArray
from ..data_structures.sframe import SFrame as _SFrame
from ..data_structures.sgraph import SGraph as _SGraph
from ..data_structures.sgraph import Vertex as _Vertex
from ..data_structures.sgraph import Edge as _Edge
from ..cython.cy_sarray import UnitySArrayProxy
from ..cython.cy_sframe import UnitySFrameProxy
from ..cython.cy_graph import UnityGraphProxy
from ..toolkits._main import ToolkitError

import json
import logging as _logging
import six as _six


_proxy_map = {UnitySFrameProxy: (lambda x: _SFrame(_proxy=x)),
              UnitySArrayProxy: (lambda x: _SArray(_proxy=x)),
              UnityGraphProxy: (lambda x: _SGraph(_proxy=x))}

def _toolkit_serialize_summary_struct(model, sections, section_titles):
    """
      Serialize model summary into a dict with ordered lists of sections and section titles

    Parameters
    ----------
    model : Model object
    sections : Ordered list of lists (sections) of tuples (field,value)
      [
        [(field1, value1), (field2, value2)],
        [(field3, value3), (field4, value4)],

      ]
    section_titles : Ordered list of section titles


    Returns
    -------
    output_dict : A dict with two entries:
                    'sections' : ordered list with tuples of the form ('label',value)
                    'section_titles' : ordered list of section labels
    """
    output_dict = dict()
    output_dict['sections'] = [ [ ( field[0], __extract_model_summary_value(model, field[1]) ) \
                                                                            for field in section ]
                                                                            for section in sections ]
    output_dict['section_titles'] = section_titles
    return output_dict


def _add_docstring(format_dict):
    """
    Format a doc-string on the fly.
    @arg format_dict: A dictionary to format the doc-strings
    Example:

        @add_docstring({'context': __doc_string_context})
        def predict(x):
            '''
            {context}
            >> model.predict(data)
            '''
            return x
    """
    def add_docstring_context(func):
        def wrapper(*args, **kwargs):
            return func(*args, **kwargs)
        wrapper.__doc__ = func.__doc__.format(**format_dict)
        return wrapper
    return add_docstring_context


def _find_only_column_of_type(sframe, target_type, type_name, col_name):
    """
    Finds the only column in `SFrame` with a type specified by `target_type`.
    If there are zero or more than one such columns, an exception will be
    raised. The name and type of the target column should be provided as
    strings for the purpose of error feedback.
    """
    image_column_name = None
    if type(target_type) != list:
        target_type = [target_type]
    for name, ctype in zip(sframe.column_names(), sframe.column_types()):
        if ctype in target_type:
            if image_column_name is not None:
                raise ToolkitError('No "{col_name}" column specified and more than one {type_name} column in "dataset". Can not infer correct {col_name} column.'.format(col_name=col_name, type_name=type_name))
            image_column_name = name
    if image_column_name is None:
        raise ToolkitError('No %s column in "dataset".' % type_name)
    return image_column_name


def _find_only_image_column(sframe):
    """
    Finds the only column in `sframe` with a type of turicreate.Image. If there are zero or
    more than one image columns, an exception will be raised.
    """
    from turicreate import Image
    return _find_only_column_of_type(sframe, target_type=Image,
                                     type_name='image', col_name='feature')


def _SGraphFromJsonTree(json_str):
    """
    Convert the Json Tree to SGraph
    """
    g = json.loads(json_str)
    vertices = [_Vertex(x['id'],
                dict([(str(k), v) for k, v in _six.iteritems(x) if k != 'id']))
                                                      for x in g['vertices']]
    edges = [_Edge(x['src'], x['dst'],
             dict([(str(k), v) for k, v in _six.iteritems(x) if k != 'src' and k != 'dst']))
                                                      for x in g['edges']]
    sg = _SGraph().add_vertices(vertices)
    if len(edges) > 0:
        sg = sg.add_edges(edges)
    return sg

class _precomputed_field(object):
    def __init__(self, field):
        self.field = field

def _summarize_coefficients(top_coefs, bottom_coefs):
    """
    Return a tuple of sections and section titles.
    Sections are pretty print of model coefficients

    Parameters
    ----------
    top_coefs : SFrame of top k coefficients

    bottom_coefs : SFrame of bottom k coefficients

    Returns
    -------
    (sections, section_titles) : tuple
            sections : list
                summary sections for top/bottom k coefficients
            section_titles : list
                summary section titles
    """

    def get_row_name(row):
        if row['index'] is None:
            return row['name']
        else:
            return "%s[%s]" % (row['name'], row['index'])

    if len(top_coefs) == 0:
        top_coefs_list = [('No Positive Coefficients', _precomputed_field('') )]
    else:
        top_coefs_list = [ (get_row_name(row),
                            _precomputed_field(row['value'])) \
                            for row in top_coefs ]

    if len(bottom_coefs) == 0:
        bottom_coefs_list = [('No Negative Coefficients', _precomputed_field(''))]
    else:
        bottom_coefs_list = [ (get_row_name(row),
                            _precomputed_field(row['value'])) \
                            for row in bottom_coefs ]

    return ([top_coefs_list, bottom_coefs_list], \
                    [ 'Highest Positive Coefficients', 'Lowest Negative Coefficients'] )

def _toolkit_get_topk_bottomk(values, k=5):
    """
    Returns a tuple of the top k values from the positive and
    negative values in a SArray

    Parameters
    ----------
    values : SFrame of model coefficients

    k: Maximum number of largest positive and k lowest negative numbers to return

    Returns
    -------
    (topk_positive, bottomk_positive) : tuple
            topk_positive : list
                floats that represent the top 'k' ( or less ) positive
                values
            bottomk_positive : list
                floats that represent the top 'k' ( or less ) negative
                values
    """

    top_values = values.topk('value', k=k)
    top_values = top_values[top_values['value'] > 0]

    bottom_values = values.topk('value', k=k, reverse=True)
    bottom_values = bottom_values[bottom_values['value'] < 0]

    return (top_values, bottom_values)

def _toolkit_summary_dict_to_json(summary_dict):
    return json.dumps(summary_dict, allow_nan=False, ensure_ascii=False)

def _toolkit_summary_to_json(model, sections, section_titles):
    """
    Serialize model summary to JSON string.  JSON is an object with ordered arrays of
    section_titles and sections

    Parameters
    ----------
    model : Model object
    sections : Ordered list of lists (sections) of tuples (field,value)
      [
        [(field1, value1), (field2, value2)],
        [(field3, value3), (field4, value4)],

      ]
    section_titles : Ordered list of section titles

    """
    return _toolkit_summary_dict_to_json( \
                    _toolkit_serialize_summary_struct(  \
                        model, sections, section_titles) )

def __extract_model_summary_value(model, value):
    """
    Extract a model summary field value
    """
    field_value = None
    if isinstance(value, _precomputed_field):
        field_value = value.field
    else:
        field_value = model._get(value)
    if isinstance(field_value, float):
        try:
            field_value = round(field_value, 4)
        except:
            pass
    return field_value

def _make_repr_table_from_sframe(X):
    """
    Serializes an SFrame to a list of strings, that, when printed, creates a well-formatted table.
    """

    assert isinstance(X, _SFrame)

    column_names = X.column_names()

    out_data = [ [None]*len(column_names) for i in range(X.num_rows())]

    column_sizes = [len(s) for s in column_names]

    for i, c in enumerate(column_names):
        for j, e in enumerate(X[c]):
            out_data[j][i] = str(e)
            column_sizes[i] = max(column_sizes[i], len(e))

    # now, go through and pad everything.
    out_data = ([ [cn.ljust(k, ' ') for cn, k in zip(column_names, column_sizes)],
                  ["-"*k for k in column_sizes] ]
                + [ [e.ljust(k, ' ') for e, k in zip(row, column_sizes)] for row in out_data] )

    return ['  '.join(row) for row in out_data]


def _toolkit_repr_print(model, fields, section_titles, width = None):
    """
    Display a toolkit repr according to some simple rules.

    Parameters
    ----------
    model : Turi Create model

    fields: List of lists of tuples
        Each tuple should be (display_name, field_name), where field_name can
        be a string or a _precomputed_field object.


    section_titles: List of section titles, one per list in the fields arg.

    Example
    -------

        model_fields = [
            ("L1 penalty", 'l1_penalty'),
            ("L2 penalty", 'l2_penalty'),
            ("Examples", 'num_examples'),
            ("Features", 'num_features'),
            ("Coefficients", 'num_coefficients')]

        solver_fields = [
            ("Solver", 'solver'),
            ("Solver iterations", 'training_iterations'),
            ("Solver status", 'training_solver_status'),
            ("Training time (sec)", 'training_time')]

        training_fields = [
            ("Log-likelihood", 'training_loss')]

        fields = [model_fields, solver_fields, training_fields]:

        section_titles = ['Model description',
                          'Solver description',
                          'Training information']

        _toolkit_repr_print(model, fields, section_titles)
    """

    assert len(section_titles) == len(fields), \
        "The number of section titles ({0}) ".format(len(section_titles)) +\
        "doesn't match the number of groups of fields, {0}.".format(len(fields))

    out_fields = [ ("Class", model.__class__.__name__), ""]

    # Record the max_width so that if width is not provided, we calculate it.
    max_width = len("Class")

    for index, (section_title, field_list) in enumerate(zip(section_titles, fields)):

        # Add in the section header.
        out_fields += [section_title, "-"*len(section_title)]

        # Add in all the key-value pairs
        for f in field_list:
            if isinstance(f, tuple):
                f = (str(f[0]), f[1])
                out_fields.append( (f[0], __extract_model_summary_value(model, f[1])) )
                max_width = max(max_width, len(f[0]))
            elif isinstance(f, _SFrame):
                out_fields.append("")
                out_fields += _make_repr_table_from_sframe(f)
                out_fields.append("")
            else:
                raise TypeError("Type of field %s not recognized." % str(f))

        # Add in the empty footer.
        out_fields.append("")

    if width is None:
        width = max_width

    # Now, go through and format the key_value pairs nicely.
    def format_key_pair(key, value):
        if type(key) is list:
            key = ','.join(str(k) for k in key)

        return key.ljust(width, ' ') + ' : ' + str(value)
    out_fields = [s if type(s) is str else format_key_pair(*s) for s in out_fields]

    return '\n'.join(out_fields)

def _map_unity_proxy_to_object(value):
    """
    Map returning value, if it is unity SFrame, SArray, map it
    """
    vtype = type(value)
    if vtype in _proxy_map:
        return _proxy_map[vtype](value)
    elif vtype == list:
        return [_map_unity_proxy_to_object(v) for v in value]
    elif vtype == dict:
        return {k:_map_unity_proxy_to_object(v) for k,v in value.items()}
    else:
        return value

def _toolkits_select_columns(dataset, columns):
    """
    Same as select columns but redirect runtime error to ToolkitError.
    """
    try:
        return dataset.select_columns(columns)
    except RuntimeError:
        missing_features = list(set(columns).difference(set(dataset.column_names())))
        raise ToolkitError("Input data does not contain the following columns: "  +
                "{}".format(missing_features))

def _raise_error_if_column_exists(dataset, column_name = 'dataset',
                            dataset_variable_name = 'dataset',
                            column_name_error_message_name = 'column_name'):
    """
    Check if a column exists in an SFrame with error message.
    """
    err_msg = 'The SFrame {0} must contain the column {1}.'.format(
                                                dataset_variable_name,
                                             column_name_error_message_name)
    if column_name not in dataset.column_names():
      raise ToolkitError(str(err_msg))

def _check_categorical_option_type(option_name, option_value, possible_values):
    """
    Check whether or not the requested option is one of the allowed values.
    """
    err_msg = '{0} is not a valid option for {1}. '.format(option_value, option_name)
    err_msg += ' Expected one of: '.format(possible_values)

    err_msg += ', '.join(map(str, possible_values))
    if option_value not in possible_values:
        raise ToolkitError(err_msg)

def _raise_error_if_not_sarray(dataset, variable_name="SArray"):
    """
    Check if the input is an SArray. Provide a proper error
    message otherwise.
    """
    err_msg = "Input %s is not an SArray."
    if not isinstance(dataset, _SArray):
      raise ToolkitError(err_msg % variable_name)

def _raise_error_if_sarray_not_expected_dtype(sa, name, types):
    err_msg = "Column '%s' cannot be of type %s. Expecting a column of type in [%s]." % \
            (name, sa.dtype.__name__, ', '.join([x.__name__ for x in types]))
    if sa.dtype not in types:
        raise ToolkitError(err_msg)

def _raise_error_if_not_sframe(dataset, variable_name="SFrame"):
    """
    Check if the input is an SFrame. Provide a proper error
    message otherwise.
    """
    err_msg = "Input %s is not an SFrame. If it is a Pandas DataFrame,"
    err_msg += " you may use the to_sframe() function to convert it to an SFrame."

    if not isinstance(dataset, _SFrame):
      raise ToolkitError(err_msg % variable_name)

def _raise_error_if_sframe_empty(dataset, variable_name="SFrame"):
    """
    Check if the input is empty.
    """
    err_msg = "Input %s either has no rows or no columns. A non-empty SFrame "
    err_msg += "is required."

    if dataset.num_rows() == 0 or dataset.num_columns() == 0:
        raise ToolkitError(err_msg % variable_name)

def _raise_error_if_not_iterable(dataset, variable_name="SFrame"):
    """
    Check if the input is iterable.
    """
    err_msg = "Input %s is not iterable: hasattr(%s, '__iter__') must be true."

    if not hasattr(dataset, '__iter__'):
        raise ToolkitError(err_msg % variable_name)

def _raise_error_evaluation_metric_is_valid(metric, allowed_metrics):
    """
    Check if the input is an SFrame. Provide a proper error
    message otherwise.
    """

    err_msg = "Evaluation metric '%s' not recognized. The supported evaluation"
    err_msg += " metrics are (%s)."

    if metric not in allowed_metrics:
      raise ToolkitError(err_msg % (metric,
                          ', '.join(map(lambda x: "'%s'" % x, allowed_metrics))))

def _numeric_param_check_range(variable_name, variable_value, range_bottom, range_top):
    """
    Checks if numeric parameter is within given range
    """
    err_msg = "%s must be between %i and %i"

    if variable_value < range_bottom or variable_value > range_top:
        raise ToolkitError(err_msg % (variable_name, range_bottom, range_top))

def _validate_row_label(dataset, label=None, default_label='__id'):
    """
    Validate a row label column. If the row label is not specified, a column is
    created with row numbers, named with the string in the `default_label`
    parameter.

    Parameters
    ----------
    dataset : SFrame
        Input dataset.

    label : str, optional
        Name of the column containing row labels.

    default_label : str, optional
        The default column name if `label` is not specified. A column with row
        numbers is added to the output SFrame in this case.

    Returns
    -------
    dataset : SFrame
        The input dataset, but with an additional row label column, *if* there
        was no input label.

    label : str
        The final label column name.
    """
    ## If no label is provided, set it to be a default and add a row number to
    #  dataset. Check that this new name does not conflict with an existing
    #  name.
    if not label:

        ## Try a bunch of variations of the default label to find one that's not
        #  already a column name.
        label_name_base = default_label
        label = default_label
        i = 1

        while label in dataset.column_names():
            label = label_name_base + '.{}'.format(i)
            i += 1

        dataset = dataset.add_row_number(column_name=label)

    ## Validate the label name and types.
    if not isinstance(label, str):
        raise TypeError("The row label column name '{}' must be a string.".format(label))

    if not label in dataset.column_names():
        raise ToolkitError("Row label column '{}' not found in the dataset.".format(label))

    if not dataset[label].dtype in (str, int):
        raise TypeError("Row labels must be integers or strings.")

    ## Return the modified dataset and label
    return dataset, label

def _model_version_check(file_version, code_version):
    """
    Checks if a saved model file with version (file_version)
    is compatible with the current code version (code_version).
    Throws an exception telling the user to upgrade.
    """
    if (file_version > code_version):
        raise RuntimeError("Failed to load model file.\n\n"
           "The model that you are trying to load was saved with a newer version of\n"
           "Turi Create than what you have. Please upgrade before attempting to load\n"
           "the file again:\n"
           "\n"
           "    pip install -U turicreate\n")

def _mac_ver():
    """
    Returns Mac version as a tuple of integers, making it easy to do proper
    version comparisons. On non-Macs, it returns an empty tuple.
    """
    import platform
    import sys
    if sys.platform == 'darwin':
        ver_str = platform.mac_ver()[0]
        return tuple([int(v) for v in ver_str.split('.')])
    else:
        return ()

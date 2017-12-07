# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import sys as _sys

def _raise_error_if_not_of_type(arg, expected_type, arg_name=None):
    """
    Check if the input is of expected type.

    Parameters
    ----------
    arg            : Input argument.

    expected_type  : A type OR a list of types that the argument is expected
                     to be.

    arg_name      : The name of the variable in the function being used. No
                    name is assumed if set to None.

    Examples
    --------
    _raise_error_if_not_of_type(sf, str, 'sf')
    _raise_error_if_not_of_type(sf, [str, int], 'sf')
    """

    display_name = "%s " % arg_name if arg_name is not None else "Argument "
    lst_expected_type = [expected_type] if \
                        type(expected_type) == type else expected_type

    err_msg = "%smust be of type %s " % (display_name,
                        ' or '.join([x.__name__ for x in lst_expected_type]))
    err_msg += "(not %s)." % type(arg).__name__
    if not any(map(lambda x: isinstance(arg, x), lst_expected_type)):
        raise TypeError(err_msg)

def _raise_error_if_not_function(arg, arg_name=None):
    """
    Check if the input is of expected type.

    Parameters
    ----------
    arg            : Input argument.

    arg_name      : The name of the variable in the function being used. No
                    name is assumed if set to None.

    Examples
    --------
    _raise_error_if_not_function(func, 'func')

    """
    display_name = '%s ' % arg_name if arg_name is not None else ""
    err_msg = "Argument %smust be a function." % display_name
    if not hasattr(arg, '__call__'):
        raise TypeError(err_msg)

def _is_non_string_iterable(obj):
    # In Python 3, str implements '__iter__'.
    return (hasattr(obj, '__iter__') and not isinstance(obj, str))

def _is_string(obj):
    return (( _sys.version_info.major == 3 and isinstance(obj, str))
            or (_sys.version_info.major == 2 and isinstance(obj, basestring)))



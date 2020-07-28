#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import sympy as sm
import numpy as np
import six

k_used_symbols = set()
k_num_internal_syms = 0


def is_compatible_symbolic_vector(val_a, val_b):
    """
    compare two vector and check if they are compatible.
    ([is0, 4], [9, 4]), ([is0, 1],[is1, is2]) are twp compatible examples.
    """
    val_a = tuple(val_a)
    val_b = tuple(val_b)

    if len(val_a) != len(val_b):
        return False

    for a, b in zip(val_a, val_b):
        if not is_symbolic(a) and not is_symbolic(b):
            if a != b:
                return False
    return True


def is_symbolic(val):
    return issubclass(type(val), sm.Basic)  # pylint: disable=consider-using-ternary


def is_variadic(val):
    return (
        issubclass(type(val), sm.Symbol) and val.name[0] == "*"
    )  # pylint: disable=consider-using-ternary


def num_symbolic(val):
    """
    Return the number of symbols in val
    """
    if is_symbolic(val):
        return 1
    elif isinstance(val, np.ndarray) and np.issctype(val.dtype):
        return 0
    elif hasattr(val, "__iter__"):
        return sum(any_symbolic(i) for i in val)
    return 0


def any_symbolic(val):
    if is_symbolic(val):
        return True
    if isinstance(val, np.ndarray) and val.ndim == 0:
        return is_symbolic(val[()])
    elif isinstance(val, np.ndarray) and np.issctype(val.dtype):
        return False
    elif isinstance(val, six.string_types):  # string is iterable
        return False
    elif hasattr(val, "__iter__"):
        return any(any_symbolic(i) for i in val)
    return False


def any_variadic(val):
    if is_variadic(val):
        return True
    elif isinstance(val, np.ndarray) and np.issctype(val.dtype):
        return False
    elif isinstance(val, six.string_types):  # string is iterable
        return False
    elif hasattr(val, "__iter__"):
        return any(any_variadic(i) for i in val)
    return False


def isscalar(val):
    return np.isscalar(val) or issubclass(type(val), sm.Basic)

# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as np
import sympy as sm
import logging

from .type_spec import Type
from .get_type_info import get_type_info
from .type_mapping import (
    promote_types,
    is_tensor,
    nptype_from_builtin,
    builtin_to_string,
    numpy_type_to_builtin_type,
)


def memoize(f):
    memo = {}

    def helper(x, y):
        y = tuple(y)
        if (x, y,) not in memo:
            memo[(x, y,)] = f(x, y,)
        return memo[(x, y,)]

    return helper


def canonical_shape(shape):
    """ Return shape as tuple of int or Symbol.

    This utility function ensures the shape tuple
    using a single integer type (to its best effort).

    Args:
        shape: tuple(int|long|np.int*|Symbol|SymbolExpr...)
    """

    def try_cast(x):
        try:
            # In python2.7, long and int are different types.
            # If we cast a long int whose value is out of the range of int,
            # the result is still long, avoiding overflow:
            #
            #     `type(2<<64) == long        # true`
            #     `type(int(2<<64)) == long   # true`
            x = int(x)
        except TypeError:
            # ignore symbolic value (sm.Symbol or sm.Expr)
            pass
        return x

    return tuple(try_cast(x) for x in shape)


@memoize
def tensor(primitive, shape):
    shape = canonical_shape(shape)

    class tensor:
        T = [primitive, shape]

        def __init__(self):
            self._val = []

        @classmethod
        def __type_info__(cls):
            return Type(
                "tensor", [get_type_info(primitive)] + list(shape), python_class=cls
            )

        @classmethod
        def get_primitive(cls):
            return primitive

        @classmethod
        def get_shape(cls):
            return shape

        @property
        def val(self):
            return self._val

        @val.setter
        def val(self, v):
            if not isinstance(v, np.ndarray):
                raise ValueError(
                    "tensor should have value of type ndarray, got {} instead".format(
                        type(v)
                    )
                )

            v_type = numpy_type_to_builtin_type(v.dtype)
            promoted_type = promote_types(v_type, primitive)
            if v_type == primitive or v.dtype == np.dtype("O"):
                # np.array of symbolic has object type. Don't cast type.
                self._val = v
            elif promoted_type == primitive:
                self._val = v.astype(nptype_from_builtin(primitive))
            else:
                logging.warning(
                    "Saving value type of {} into a builtin type of {}, might lose precision!".format(
                        v.dtype, builtin_to_string(primitive)
                    )
                )
                self._val = v.astype(nptype_from_builtin(primitive))

    tensor.__template_name__ = (
        "tensor[" + primitive.__name__ + "," + ",".join(str(s) for s in shape) + "]"
    )
    tensor.__name__ = (
        "tensor[" + ",".join(str(s) for s in shape) + "," + primitive.__name__ + "]"
    )
    return tensor


def is_tensor_and_is_compatible(tensor_type1, tensor_type2, allow_promotion=False):
    """
    Try to find a tensor type compatible with both input types.

    Compatible means that the tensors have the same rank and matching or unspecified
    dimensions. For example, (10, -1) is compatible with (-1, 20) with the compatible
    shape (10, 20).

    Args:
        tensor_type1 (types.tensor)
        tensor_type2 (types.tensor)
        allow_promotion (bool): If True, allow primitive types to be promoted.

    Returns:
        A pair of (bool, type). If the given types are not tensor types with
        (1) compatible shapes and (2) either identical primitive types or
        allow_promition=True, return is False, None. Otherwise, return True
        and the compatible shape. Note that the returned shape may
        not be the same as either input. For example,

        is_tensor_and_is_compatible(
            tensor[fp32,[10,-1]],
            tensor[fp32,[-1,20]]) --> tensor[fp32, [10,20]]
    """

    if not is_tensor(tensor_type1) or not is_tensor(tensor_type2):
        return False, None
    shape1 = tensor_type1.get_shape()
    shape2 = tensor_type2.get_shape()

    primitive_type = tensor_type1.get_primitive()
    if primitive_type != tensor_type2.get_primitive():
        promoted_type = promote_types(primitive_type, tensor_type2.get_primitive())
        if allow_promotion:
            primitive_type = promoted_type
        else:
            return False, promoted_type

    if len(shape1) == 0:
        return True, tensor_type2
    if len(shape2) == 0:
        return True, tensor_type1

    if len(shape1) != len(shape2):
        return False, None

    most_specific_shape = []
    for i in range(len(shape1)):
        if shape1[i] == -1 or issubclass(type(shape1[i]), sm.Basic):
            most_specific_shape.append(shape2[i])
        elif shape2[i] == -1 or issubclass(type(shape2[i]), sm.Basic):
            most_specific_shape.append(shape1[i])
        elif shape1[i] == shape2[i]:
            most_specific_shape.append(shape1[i])
        elif shape1[i] != shape2[i]:
            return False, None

    return True, tensor(primitive_type, most_specific_shape)


def is_tensor_and_is_compatible_general_shape(tensor_type1, tensor_type2):
    # returns a pair of (bool, type)
    # If Both are tensors, and have compatible shape, the first return is true
    # The return will be the most general version of the tensor type.
    # Note that this may not be either tensor types. i.e.
    #
    # is_tensor_and_is_compatible(tensor[fp32,[10,-1]] ,tensor[fp32,[-1,20]])
    # will return True, tensor[fp32, [-1,-1]]

    if not is_tensor(tensor_type1) or not is_tensor(tensor_type2):
        return False, None
    shape1 = tensor_type1.get_shape()
    shape2 = tensor_type2.get_shape()

    if tensor_type1.get_primitive() != tensor_type2.get_primitive():
        return False, None

    if len(shape1) == 0:
        return True, tensor_type2
    if len(shape2) == 0:
        return True, tensor_type1

    if len(shape1) != len(shape2):
        return False, None

    most_general_shape = []
    for i in range(len(shape1)):
        if shape1[i] == -1 or issubclass(type(shape1[i]), sm.Basic):
            most_general_shape.append(shape1[i])
        elif shape2[i] == -1 or issubclass(type(shape2[i]), sm.Basic):
            most_general_shape.append(shape2[i])
        elif shape1[i] == shape2[i]:
            most_general_shape.append(shape1[i])
        elif shape1[i] != shape2[i]:
            return False, None

    return True, tensor(tensor_type1.get_primitive(), most_general_shape)


def tensor_has_complete_shape(tensor_type):
    if not is_tensor(tensor_type):
        return True
    s = tensor_type.get_shape()
    if -1 in s:
        return False
    elif len(s) == 0:
        return False
    else:
        return True

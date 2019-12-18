# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .annotate import annotate
from .type_void import void
from .type_int import *
from .type_spec import *
from .get_type_info import get_type_info
import sympy as sm


def memoize(f):
    memo = {}

    def helper(x, y):
        y = tuple(y)
        if (
                x,
                y,
        ) not in memo:
            memo[(
                x,
                y,
            )] = f(
                x,
                y,
            )
        return memo[(
            x,
            y,
        )]

    return helper


@memoize
def tensor(primitive, shape):
    shape = tuple(shape)

    class tensor:
        T = [primitive, shape]

        def __init__(self):
            self.val = []

        @classmethod
        def __type_info__(cls):
            return Type("tensor", [get_type_info(primitive)] + list(shape), python_class=cls)

        @classmethod
        def get_primitive(cls):
            return primitive

        @classmethod
        def get_shape(cls):
            return shape

    tensor.__template_name__ = "tensor[" + primitive.__name__ + "," + ",".join(
        str(s) for s in shape) + "]"
    return tensor


def is_tensor_and_is_compatible(tensor_type1, tensor_type2):
    # returns a pair of (bool, type)
    # If Both are tensors, and have compatible shape, the first return is true
    # The return will be the most specific version of the tensor type.
    # Note that this may not be either tensor types. i.e.
    #
    # is_tensor_and_is_compatible(tensor[fp32,[10,-1]] ,tensor[fp32,[-1,20]])
    # will return True, tensor[fp32, [10,20]]

    if tensor_type1 is None or tensor_type2 is None:
        return False, None
    if get_type_info(tensor_type1).name != 'tensor' or get_type_info(tensor_type2).name != 'tensor':
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

    return True, tensor(tensor_type1.get_primitive(), most_specific_shape)


def is_tensor_and_is_compatible_general_shape(tensor_type1, tensor_type2):
    # returns a pair of (bool, type)
    # If Both are tensors, and have compatible shape, the first return is true
    # The return will be the most general version of the tensor type.
    # Note that this may not be either tensor types. i.e.
    #
    # is_tensor_and_is_compatible(tensor[fp32,[10,-1]] ,tensor[fp32,[-1,20]])
    # will return True, tensor[fp32, [-1,-1]]

    if tensor_type1 is None or tensor_type2 is None:
        return False, None
    if get_type_info(tensor_type1).name != 'tensor' or get_type_info(tensor_type2).name != 'tensor':
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


def is_tensor(tensor_type):
    if tensor_type is None:
        return False
    return get_type_info(tensor_type).name == 'tensor'


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

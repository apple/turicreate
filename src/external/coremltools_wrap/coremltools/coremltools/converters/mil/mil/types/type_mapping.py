# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from .type_bool import bool as types_bool
from .type_double import (
    is_float,
    fp16 as types_fp16,
    fp32 as types_fp32,
    fp64 as types_fp64,
)
from .type_list import is_list
from .type_int import (
    is_int,
    int8 as types_int8,
    int16 as types_int16,
    int32 as types_int32,
    int64 as types_int64,
    uint8 as types_uint8,
    uint16 as types_uint16,
    uint32 as types_uint32,
    uint64 as types_uint64,
)
from .type_str import str as types_str
from .type_unknown import unknown
import numpy as np
import sympy as sm
import six
from .get_type_info import get_type_info

_types_TO_NPTYPES = {
    types_bool: np.bool_,
    types_int8: np.int8,
    types_int16: np.int16,
    types_int32: np.int32,
    types_int64: np.int64,
    types_uint8: np.uint8,
    types_uint16: np.uint16,
    types_uint32: np.uint32,
    types_uint64: np.uint64,
    types_fp16: np.float16,
    types_fp32: np.float32,
    types_fp64: np.float64,
    types_str: np.str_,
}

_types_TO_STRINGS = {
    types_bool: "bool",
    types_int8: "i8",
    types_int16: "i16",
    types_int32: "i32",
    types_int64: "i64",
    types_uint8: "u8",
    types_uint16: "u16",
    types_uint32: "u32",
    types_uint64: "u64",
    types_fp16: "fp16",
    types_fp32: "fp32",
    types_fp64: "fp64",
    types_str: "str",
}

def np_dtype_to_py_type(np_dtype):
    # Can't use dict, as hash(np.int32) != hash(val.dtype)
    if np_dtype in [np.int32, np.int64]:
        return int
    if np_dtype == np.bool:
        return bool
    if np_dtype in [np.float32, np.float64]:
        return float
    raise NotImplementedError('{} is not supported'.format(np_dtype))

_STRINGS_TO_types = {v: k for k, v in _types_TO_STRINGS.items()}


def string_to_builtin(s):
    """
    Given a str, return its corresponding builtin type.
    """
    return _STRINGS_TO_types.get(s, None)


def builtin_to_string(builtin_type):
    """
    Given a builtin type, return its corresponding string representation.
    """
    return _types_TO_STRINGS.get(builtin_type, None)


def nptype_from_builtin(btype):
    """
    Given a builtin type, return its corresponding Numpy dtype.
    """
    return _types_TO_NPTYPES.get(btype, None)


def promote_types(dtype1, dtype2):
    """
    Get the smallest type to which the given scalar types can be cast.

    Args:
        dtype1 (builtin):
        dtype2 (builtin):

    Returns:
        A builtin datatype or None.
    """
    nptype1 = nptype_from_builtin(dtype1)
    nptype2 = nptype_from_builtin(dtype2)
    # Circumvent the undesirable np type promotion:
    # >> np.promote_types(np.float32, np.int)
    # dtype('float64')
    if np.issubdtype(nptype1, np.floating) and np.issubdtype(nptype2, np.signedinteger):
        nppromoted = nptype1
    elif np.issubdtype(nptype2, np.floating) and np.issubdtype(
        nptype1, np.signedinteger
    ):
        nppromoted = nptype2
    else:
        nppromoted = np.promote_types(nptype1, nptype2)
    return numpy_type_to_builtin_type(nppromoted)


def is_primitive(btype):
    """
    Is the indicated builtin type a primitive?
    """
    return btype is types_bool or btype is types_str or is_float(btype) or is_int(btype)


def is_scalar(btype):
    """
    Is the given builtin type a scalar integer, float, or boolean?
    """
    return btype is types_bool or is_int(btype) or is_float(btype)


def is_tensor(tensor_type):
    if tensor_type is None:
        return False
    try:
        type_info = get_type_info(tensor_type).name
    except TypeError:
        return False
    return type_info == "tensor"


def is_str(t):
    if t is None:
        return False
    try:
        type_info = get_type_info(t).name
    except TypeError:
        return False
    return type_info == "str"


def is_tuple(t):
    if t is None:
        return False
    try:
        type_info = get_type_info(t).name
    except TypeError:
        return False
    return type_info == "tuple"


def is_builtin(t):
    return is_scalar(t) or is_tensor(t) or is_str(t) or is_tuple(t)


# Converts a numpy type to its types equivalent.
# Supports both dtypes and numpy primitive types.
def numpy_type_to_builtin_type(nptype):
    if type(nptype) == np.dtype:
        nptype = nptype.type

    if np.issubclass_(nptype, np.bool) or np.issubclass_(nptype, np.bool_):
        # numpy as 2 bool types it looks like. what is the difference?
        return types_bool
    elif np.issubclass_(nptype, np.int8):
        return types_int8
    elif np.issubclass_(nptype, np.int16):
        return types_int16
    elif np.issubclass_(nptype, np.int32):
        return types_int32
    elif np.issubclass_(nptype, np.int64):
        return types_int64
    elif np.issubclass_(nptype, np.uint8):
        return types_int8
    elif np.issubclass_(nptype, np.uint16):
        return types_int16
    elif np.issubclass_(nptype, np.uint32):
        return types_int32
    elif np.issubclass_(nptype, np.uint64):
        return types_int64
    elif np.issubclass_(nptype, np.int):
        # Catch all int
        return types_int32
    elif np.issubclass_(nptype, np.object_):
        # symbolic shape is considered int32
        return types_int32
    elif np.issubclass_(nptype, np.float16):
        return types_fp16
    elif np.issubclass_(nptype, np.float32) or np.issubclass_(nptype, np.single):
        return types_fp32
    elif np.issubclass_(nptype, np.float64) or np.issubclass_(nptype, np.double):
        return types_fp64
    elif (
        np.issubclass_(nptype, six.string_types)
        or np.issubclass_(nptype, np.string_)
        or np.issubclass_(nptype, np.str_)
    ):
        return types_str
    else:
        raise TypeError("Unsupported numpy type: %s" % (nptype))


# Tries to get the equivalent builtin type of a
# numpy or python type.
def type_to_builtin_type(type):
    # Infer from numpy type if it is one
    if type.__module__ == np.__name__:
        return numpy_type_to_builtin_type(type)

    # Otherwise, try to infer from a few generic python types
    if np.issubclass_(type, bool):
        return types_bool
    elif np.issubclass_(type, six.integer_types):
        return types_int32
    elif np.issubclass_(type, six.string_types):
        return types_str
    elif np.issubclass_(type, float):
        return types_fp32
    else:
        raise TypeError("Could not determine builtin type for " + str(type))


def numpy_val_to_builtin_val(npval):
    if np.isscalar(npval):
        ret_type = type_to_builtin_type(type(npval))
        ret = ret_type()
        ret.val = npval
        return ret, ret_type
    else:
        builtintype = numpy_type_to_builtin_type(npval.dtype)
        from . import tensor as types_tensor

        ret_type = types_tensor(builtintype, npval.shape)
        ret = ret_type()
        ret.val = npval
        return ret, ret_type


def is_subtype_tensor(type1, type2):
    # requires primitive types match
    if type1.get_primitive() != type2.get_primitive():
        return False

    shape1 = type1.get_shape()
    shape2 = type2.get_shape()
    # Same rank
    if len(shape1) != len(shape2):
        return False

    for d1, d2 in zip(shape1, shape2):
        if d1 == d2:
            continue

        # tensor with shape (3, s0) is not a subtype of tensor with shape (3,
        # 1), but is a subtype of tensor with shape (3, s1)
        d1_is_symbolic = issubclass(type(d1), sm.Basic)
        d2_is_symbolic = issubclass(type(d2), sm.Basic)
        if d1_is_symbolic and d2_is_symbolic:
            continue
        if d1_is_symbolic and not d2_is_symbolic:
            return False
        if not d1_is_symbolic and not d2_is_symbolic and d1 != d2:
            return False
    return True


def is_subtype(type1, type2):
    """
    Return True if type1 is a subtype of type2. False otherwise.
    """
    if type2 == unknown:
        return True  # any class is a subclass of unknown (None) type.
    if is_list(type2):
        return is_list(type1) and is_subtype(type1.T[0], type2.T[0])
    if is_tensor(type1) and is_tensor(type2):
        return is_subtype_tensor(type1, type2)
    return type1 == type2

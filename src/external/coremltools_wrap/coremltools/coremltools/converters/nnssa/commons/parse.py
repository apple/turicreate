# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as np

from . import builtins
from .builtins import get_type_info


def numpy_primitive_type_to_builtin_type(nptype):
    if np.issubclass_(nptype, np.bool) or np.issubclass_(nptype, np.bool_):
        # numpy as 2 bool types it looks like. what is the difference?
        return builtins.bool
    elif np.issubclass_(nptype, np.int8):
        return builtins.int8
    elif np.issubclass_(nptype, np.int16):
        return builtins.int16
    elif np.issubclass_(nptype, np.int32):
        return builtins.int32
    elif np.issubclass_(nptype, np.int64):
        return builtins.int64
    elif np.issubclass_(nptype, np.uint8):
        return builtins.int8
    elif np.issubclass_(nptype, np.uint16):
        return builtins.int16
    elif np.issubclass_(nptype, np.uint32):
        return builtins.int32
    elif np.issubclass_(nptype, np.uint64):
        return builtins.int64
    elif np.issubclass_(nptype, np.float16):
        return builtins.fp16
    elif np.issubclass_(nptype, np.float32):
        return builtins.fp32
    elif np.issubclass_(nptype, np.float64):
        return builtins.fp64
    else:
        raise TypeError("Not supported numpy type: %s" % (nptype))


def numpy_val_to_builtin_val(npval):
    if np.isscalar(npval):
        ret_type = numpy_primitive_type_to_builtin_type(type(npval))
        ret = ret_type()
        ret.val = npval
        return ret, ret_type
    else:
        builtintype = numpy_primitive_type_to_builtin_type(npval.dtype.type)
        ret_type = builtins.tensor(builtintype, npval.shape)
        ret = ret_type()
        ret.val = npval
        return ret, ret_type


def parse_reverse_shape(t):
    mapping = {
        1: builtins.float,
        2: builtins.double,
        3: builtins.int32,
        4: builtins.uint8,
        5: builtins.int16,
        6: builtins.int8,
        7: builtins.str,
        9: builtins.int64,
        10: builtins.bool,
        17: builtins.uint16,
        22: builtins.uint32,
        23: builtins.uint64
    }

    for v in mapping.values():
        if t == v:
            return []
    if builtins.is_tensor(t):
        return t.get_shape()
    if builtins.is_tuple(t) or builtins.is_list(t):
        if len(t.T) > 1:
            print(t.T)
            raise ValueError("parse_reverse_shape doesn't support nested non-simple tuple/list")
        return [-1] + list(parse_reverse_shape(t.T[0]))
    raise ValueError("Unsupported type (%s)" % (builtins.get_type_info(t)))


def parse_reverse_type(t):
    mapping = {
        1: builtins.float,
        2: builtins.double,
        3: builtins.int32,
        4: builtins.uint8,
        5: builtins.int16,
        6: builtins.int8,
        7: builtins.str,
        9: builtins.int64,
        10: builtins.bool,
        17: builtins.uint16,
        22: builtins.uint32,
        23: builtins.uint64
    }

    for k, v in mapping.items():
        if t == v:
            return k

    assert False, "%s cannot be parsed to builtin type" % (t)

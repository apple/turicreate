# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import copy

from ...commons import builtins
from ...commons.builtins import get_type_info
from .parsed_tf_node import ParsedTFNode

# https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/framework/types.proto
# DT_INT32 = 3;
# DT_UINT8 = 4;
# DT_INT16 = 5;
# DT_INT8 = 6;
# DT_STRING = 7;
# DT_COMPLEX64 = 8;  // Single-precision complex
# DT_INT64 = 9;
# DT_BOOL = 10;
# DT_QINT8 = 11;     // Quantized int8
# DT_QUINT8 = 12;    // Quantized uint8
# DT_QINT32 = 13;    // Quantized int32
# DT_BFLOAT16 = 14;  // Float32 truncated to 16 bits.  Only for cast ops.
# DT_QINT16 = 15;    // Quantized int16
# DT_QUINT16 = 16;   // Quantized uint16
# DT_UINT16 = 17;
# DT_COMPLEX128 = 18;  // Double-precision complex
# DT_HALF = 19;
# DT_RESOURCE = 20;
# DT_VARIANT = 21;  // Arbitrary C++ data types
# DT_UINT32 = 22;
# DT_UINT64 = 23;
#


def parse_type(t):
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
    t = int(t)
    if t in mapping:
        return mapping[t]
    else:
        print("Type %d cannot be mapped" % t)
        return None


def parse_shape(t):
    if t.unknown_rank:
        return []
    ret = [d.size for d in t.dim]
    return ret


def parse_tensor(t):
    typ = parse_type(t.dtype)
    shape = parse_shape(t.tensor_shape)
    if not t.tensor_shape.unknown_rank and len(shape) == 0:
        retobj = typ()
    else:
        rettype = builtins.tensor(typ, tuple(shape))
        retobj = rettype()
        retobj.shape = shape

    if len(t.half_val) > 0:
        retobj.val = t.half_val
    elif len(t.float_val) > 0:
        retobj.val = t.float_val
    elif len(t.double_val) > 0:
        retobj.val = t.double_val
    elif len(t.int_val) > 0:
        retobj.val = t.int_val
    elif len(t.int64_val) > 0:
        retobj.val = t.int64_val
    elif len(t.bool_val) > 0:
        retobj.val = t.bool_val
    elif hasattr(t, 'uint32_val') and len(t.uint32_val) > 0:
        retobj.val = t.uint32_val
    elif hasattr(t, 'uint64_val') and len(t.uint64_val) > 0:
        retobj.val = t.uint64_val
    return retobj


def parse_list(t):
    if len(t.s) > 0:
        return list(t.s)
    elif len(t.i) > 0:
        return list(t.i)
    elif len(t.f) > 0:
        return list(t.f)
    elif len(t.b) > 0:
        return list(t.b)
    elif len(t.type) > 0:
        return list(parse_type(z) for z in t.type)
    elif len(t.shape) > 0:
        return list(parse_shape(z) for z in t.shape)
    elif len(t.tensor) > 0:
        return list(parse_tensor(z) for z in t.tensor)
    else:
        return []


def parse_attr(attr):
    if attr.HasField('s'):
        return attr.s if isinstance(attr.s, str) else attr.s.decode()
    elif attr.HasField('i'):
        return attr.i
    elif attr.HasField('f'):
        return attr.f
    elif attr.HasField('b'):
        return attr.b
    elif attr.HasField('type'):
        return parse_type(attr.type)
    elif attr.HasField('shape'):
        return parse_shape(attr.shape)
    elif attr.HasField('tensor'):
        return parse_tensor(attr.tensor)
    elif attr.HasField('list'):
        return parse_list(attr.list)
    elif attr.HasField('func'):
        raise NotImplementedError("func not yet implemented")
    elif attr.HasField('placeholder'):
        raise NotImplementedError("placeholder not yet implemented")


def graphdef_to_dict(gd):
    ret = {}
    for node in gd.node:
        ret[node.name] = ParsedTFNode(node)
    return ret

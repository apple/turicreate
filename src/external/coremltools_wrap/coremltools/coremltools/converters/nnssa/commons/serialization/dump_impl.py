# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .file_writer import file_writer
import io
import sys
import numpy as np
from .. import builtins
from .types import *


def _dump_impl(obj, writer, expected_type=None):
    if isinstance(obj, bool) or isinstance(
            obj, int) or ((sys.version_info < (3, 0)) and isinstance(obj, long)) or issubclass(
                type(obj), int) or isinstance(obj, np.integer):
        assert (expected_type is None or builtins.get_type_info(expected_type).name == 'int')
        writer.write_byte(py_types.int.value)
        writer.write_int(obj)
    elif isinstance(obj, bytes):
        # str == bytes in python 2
        assert (expected_type is None or builtins.get_type_info(expected_type).name == 'str')
        writer.write_byte(py_types.str.value)
        writer.write_int(len(obj))
        writer.write_str(obj)
    elif isinstance(obj, str) or ((sys.version_info < (3, 0)) and isinstance(obj, unicode)):
        assert (expected_type is None or builtins.get_type_info(expected_type).name == 'str')
        writer.write_byte(py_types.str.value)
        obj = obj.encode('latin-1')
        writer.write_int(len(obj))
        writer.write_str(obj)
    elif isinstance(obj, float) or isinstance(obj, np.float32) or isinstance(obj, np.double):
        assert (expected_type is None or builtins.get_type_info(expected_type).name == 'double')
        writer.write_byte(py_types.double.value)
        writer.write_double(obj)
    elif isinstance(obj, list):
        assert (expected_type is None or builtins.get_type_info(expected_type).name == 'list')
        writer.write_byte(py_types.list.value)
        writer.write_int(len(obj))
        for i in obj:
            _dump_impl(i, writer)
    elif isinstance(obj, dict):
        assert (expected_type is None or builtins.get_type_info(expected_type).name == 'dict')
        writer.write_byte(py_types.dict.value)
        writer.write_int(len(obj))
        for i in obj:
            _dump_impl(i, writer)
            _dump_impl(obj[i], writer)
    elif isinstance(obj, np.ndarray):
        # dump the shape. Then the data.

        # TODO: Support more types.
        if obj.dtype == np.float64 or obj.dtype == np.bool:
            obj = obj.astype(np.float32)
        writer.write_byte(py_types.ndarray.value)
        writer.write_byte(dump_np_types(obj.dtype))
        writer.write_int(len(obj.shape))
        for i in obj.shape:
            writer.write_int(i)
        writer.write_str(np.ravel(obj, order='C').tobytes())

    elif hasattr(obj, '__slots__'):
        import pdb
        pdb.set_trace()
        assert (hasattr(obj, '__slot_types__'))
        assert (len(obj.__slots__) == len(obj.__slot_types__))
        _dump_impl(obj.__version__(), writer)
        slot_and_types = sorted(zip(obj.__slots__, obj.__slot_types__), key=lambda x: x[0])

        for s in sorted(slot_and_types):
            try:
                _dump_impl(getattr(obj, s[0]), writer, s[1])
            except AssertionError:
                received_type = type(getattr(obj, s[0]))
                raise TypeError(
                    "%s member is of the wrong type. Expected %s, got %s" %
                    (s[0], str(s[1]), str(received_type)))

    else:
        raise TypeError('Cannot serialize object of type %s' % str(type(obj)))


def dump_obj(obj, writer):
    _dump_impl(obj, writer)


def dump(obj, filename):
    handle = open(filename, 'wb')
    writer = file_writer(handle)
    _dump_impl(obj, writer)
    handle.close()

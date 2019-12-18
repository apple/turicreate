# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import struct
import numpy as np
from .types import *


class file_reader:
    def __init__(self, handle):
        self.handle = handle

    def _read_byte(self):
        s = struct.calcsize('b')
        i = self.handle.read(s)
        return struct.unpack('b', i)[0]

    def _read_int(self):
        s = struct.calcsize('q')
        i = self.handle.read(s)
        return int(struct.unpack('q', i)[0])

    def _read_double(self):
        s = struct.calcsize('d')
        i = self.handle.read(s)
        return float(struct.unpack('d', i)[0])

    def _read_str(self):
        s = self._read_int()
        i = self.handle.read(s).decode('latin-1')
        return i

    def _read_list(self):
        s = self._read_int()
        i = []
        for _ in range(s):
            i.append(self.read_value())
        return i

    def _read_dict(self):
        s = self._read_int()
        i = {}
        for _ in range(s):
            k = self.read_value()
            v = self.read_value()
            i[k] = v
        return i

    def _read_ndarray(self):
        get_np_type = np_types(self._read_byte()).name
        np_type = getattr(np, get_np_type)
        rank = self._read_int()
        shape = []
        for i in range(rank):
            shape.append(self._read_int())
        elements = 1
        for i in shape:
            elements *= i
        reader = self.handle.read(elements * np.dtype(np_type).itemsize)
        array_str = np.fromstring(reader, dtype=np_type, count=elements)
        return np.reshape(array_str, shape)

    def read_value(self):
        get_k_type = self._read_byte()
        k_type = py_types(get_k_type).name
        return getattr(self, '_read_' + k_type)()

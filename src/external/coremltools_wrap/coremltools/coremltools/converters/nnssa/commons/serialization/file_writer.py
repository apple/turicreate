# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import struct


class file_writer:
    def __init__(self, handle):
        self.handle = handle

    def write_bool(self, i):
        i = bool(i)
        self.handle.write(struct.pack('?', i))

    def write_byte(self, i):
        i = int(i)
        self.handle.write(struct.pack('b', i))

    def write_int(self, i):
        i = int(i)
        self.handle.write(struct.pack('q', i))

    def write_double(self, i):
        i = float(i)
        self.handle.write(struct.pack('d', i))

    def write_str(self, i):
        if isinstance(i, bytes) or isinstance(i, str):
            self.handle.write(i)
        else:
            i = str(i)
            self.handle.write(i)

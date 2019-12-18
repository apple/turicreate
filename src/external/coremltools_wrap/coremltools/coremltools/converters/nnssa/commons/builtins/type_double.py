# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .annotate import class_annotate, annotate, delay_type
from .type_bool import bool
from .get_type_info import get_type_info
from .type_spec import *
import math


def make_float(width):
    delay_type_float = getattr(delay_type, "fp" + str(width))

    @class_annotate()
    class double:
        _width = width

        def __init__(self, v=0.0):
            self.val = v

        @classmethod
        def __type_info__(cls):
            return Type("fp" + str(cls._width), python_class=cls)

        @classmethod
        def get_bitwidth(cls):
            return cls._width

        @annotate(delay_type_float, other=delay_type_float)
        def __add__(self, other):
            assert (isinstance(other, double))
            return double(self.val + other.val)

        @annotate(delay_type_float, other=delay_type_float)
        def __sub__(self, other):
            assert (isinstance(other, double))
            return double(self.val - other.val)

        @annotate(delay_type_float, other=delay_type_float)
        def __mul__(self, other):
            assert (isinstance(other, double))
            return double(self.val * other.val)

        @annotate(delay_type_float, other=delay_type_float)
        def __div__(self, other):
            assert (isinstance(other, double))
            return double(self.val / other.val)

        @annotate(delay_type_float, other=delay_type_float)
        def __mod__(self, other):
            assert (isinstance(other, double))
            return double(self.val % other.val)

        @annotate(delay_type.bool, other=delay_type_float)
        def __lt__(self, other):
            return bool(self.val < other.val)

        @annotate(delay_type.bool, other=delay_type_float)
        def __gt__(self, other):
            return bool(self.val > other.val)

        @annotate(delay_type.bool, other=delay_type_float)
        def __le__(self, other):
            return bool(self.val <= other.val)

        @annotate(delay_type.bool, other=delay_type_float)
        def __ge__(self, other):
            return bool(self.val >= other.val)

        @annotate(delay_type.bool, other=delay_type_float)
        def __eq__(self, other):
            return bool(self.val == other.val)

        @annotate(delay_type.bool, other=delay_type_float)
        def __ne__(self, other):
            return bool(self.val != other.val)

        @annotate(delay_type.bool)
        def __bool__(self):
            return self.val

        @annotate(delay_type.int)
        def __int__(self):
            return int(self)

        @annotate(delay_type_float)
        def __double__(self):
            return float(self.val)

        @annotate(delay_type.str)
        def __str__(self):
            return str(self.val)

        @annotate(delay_type_float)
        def __log__(self):
            return math.log(self.val)

        @annotate(delay_type_float)
        def __exp__(self):
            return math.exp(self.val)

        @annotate(delay_type_float)
        def __neg__(self):
            return double(-self.val)

    return double


fp16 = make_float(16)
fp32 = make_float(32)
fp64 = make_float(64)
float = fp32
double = fp64


def is_float(t):
    return any(t is i or isinstance(t,i) for i in [fp16, fp32, fp64])

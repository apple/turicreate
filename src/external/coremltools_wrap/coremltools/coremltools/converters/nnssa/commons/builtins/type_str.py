# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .annotate import class_annotate, annotate, delay_type
from .type_spec import *


@class_annotate()
class str:
    def __init__(self, v=""):
        self.val = v

    @classmethod
    def __type_info__(cls):
        return Type("str", python_class=cls)

    @annotate(delay_type.str, other=delay_type.str)
    def __add__(self, other):
        assert (isinstance(other, str))
        return str(self.val + other.val)

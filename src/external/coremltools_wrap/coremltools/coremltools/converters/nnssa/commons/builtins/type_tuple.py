# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .annotate import annotate
from .type_void import void
from .type_int import *
from .type_unknown import *
from .type_spec import *
from .get_type_info import get_type_info

_global_tuple = tuple


def memoize(f):
    memo = {}

    def helper(x):
        x = _global_tuple(x)
        if x not in memo:
            memo[x] = f(x)
        return memo[x]

    return helper


class empty_list:
    @classmethod
    def __type_info__(cls):
        return Type("empty_list", python_class=cls)


@memoize
def tuple(args):
    args = _global_tuple(i if i is not None else type_unknown.unknown for i in args)

    class tuple:
        T = args

        def __init__(self):
            self.val = [arg() for arg in args]

        @classmethod
        def __type_info__(cls):
            return Type("tuple", [get_type_info(arg) for arg in args], python_class=cls)

        @annotate(int)
        def __len__(self):
            return len(args)

    tuple.__template_name__ = "tuple[" + ",".join([get_type_info(arg).name for arg in args]) + "]"
    return tuple


def is_tuple(t):
    if t is None:
        return False
    return get_type_info(t).name == 'tuple'

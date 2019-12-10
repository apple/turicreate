# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .annotate import annotate
from .type_void import void
from .type_int import *
from .type_spec import *
from .get_type_info import get_type_info


def memoize(f):
    memo = {}

    def helper(x):
        if x not in memo:
            memo[x] = f(x)
        return memo[x]

    return helper


class empty_list:
    @classmethod
    def __type_info__(cls):
        return Type("empty_list", python_class=cls)


@memoize
def list(arg):
    class list:
        T = [arg]

        def __init__(self):
            self.val = []

        @classmethod
        def __type_info__(cls):
            return Type("list", [get_type_info(arg)], python_class=cls)

        @annotate(void, other=T[0])
        def append(self, other):
            assert (isinstance(other, self.T[0]))
            self.val.append(other)

        @annotate(T[0], index=int)
        def __getitem__(self, index):
            assert (isinstance(index, int))
            return self.val[index.val]

        @annotate(void, index=int, newval=T[0])
        def __setitem__(self, index, newval):
            assert (isinstance(index, int))
            assert (isinstance(newval, self.T[0]))
            self.val[index.val] = newval

        @annotate(int)
        def __len__(self):
            return int(len(self.val))

    list.__template_name__ = "list[" + arg.__name__ + "]"
    return list


def is_list(t):
    if t is None:
        return False
    return get_type_info(t).name == 'list'

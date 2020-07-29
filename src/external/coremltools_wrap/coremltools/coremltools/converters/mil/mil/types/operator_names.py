# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

__bin_operator_to_python_name = {}
__bin_operator_to_python_name["+"] = "__add__"
__bin_operator_to_python_name["-"] = "__sub__"
__bin_operator_to_python_name["*"] = "__mul__"
__bin_operator_to_python_name["/"] = "__div__"
__bin_operator_to_python_name["%"] = "__mod__"
__bin_operator_to_python_name["<"] = "__lt__"
__bin_operator_to_python_name["<="] = "__le__"
__bin_operator_to_python_name[">"] = "__gt__"
__bin_operator_to_python_name[">="] = "__ge__"
__bin_operator_to_python_name["=="] = "__eq__"
__bin_operator_to_python_name["!="] = "__ne__"
__bin_operator_to_python_name["in"] = "__contains__"
__bin_operator_to_python_name["getitem"] = "__getitem__"
__bin_operator_to_python_name["setitem"] = "__setitem__"

__unary_operator_to_python_name = {}
__unary_operator_to_python_name["-"] = "__neg__"
__unary_operator_to_python_name["!"] = "__not__"


def bin_operator_to_python_name(op):
    return __bin_operator_to_python_name.get(op, None)


def unary_operator_to_python_name(op):
    return __unary_operator_to_python_name.get(op, None)

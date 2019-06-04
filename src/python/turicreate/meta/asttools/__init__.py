# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Module to augment and analyze python ast nodes.

This module uses the python `ast` module exclusively not the deprecated `compiler.ast`.
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import _ast
import ast

from ..asttools.visitors import dont_visit, visit_children, Visitor


class Undedined: pass

def cmp_ast(node1, node2):
    '''
    Compare if two nodes are equal.
    '''

    if type(node1) != type(node2):
        return False

    if isinstance(node1, (list, tuple)):
        if len(node1) != len(node2):
            return False

        for left, right in zip(node1, node2):
            if not cmp_ast(left, right):
                return False

    elif isinstance(node1, ast.AST):
        for field in node1._fields:
            left = getattr(node1, field, Undedined)
            right = getattr(node2, field, Undedined)

            if not cmp_ast(left, right):
                return False
    else:
        return node1 == node2

    return True




#===============================================================================
# 
#===============================================================================

from ..asttools.visitors.print_visitor import print_ast, dump_ast as str_ast
from ..asttools.visitors.pysourcegen import python_source, dump_python_source
from ..asttools.visitors.cond_symbol_visitor import lhs, rhs
from ..asttools.visitors.cond_symbol_visitor import conditional_lhs, conditional_symbols
from ..asttools.visitors.symbol_visitor import get_symbols
from ..asttools.visitors.graph_visitor import make_graph

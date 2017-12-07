# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Aug 3, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import _ast
import ast

from ...asttools.visitors.graph_visitor import GraphGen
from ...asttools import Visitor, dont_visit, visit_children
from ...asttools.mutators.replace_mutator import replace_nodes
from ...asttools.visitors.symbol_visitor import get_symbols
from ...asttools.visitors.cond_symbol_visitor import conditional_lhs

class Assignment(object):

    def __init__(self, root, assignments):
        self.root = root
        self.assignments = assignments

def visit_conditional(self, node):

    conditional, stable = conditional_lhs(node)

    if not stable:
        return

    bgather = GatherAssignments()
    for stmnt in node.body: bgather.visit(stmnt)

    egather = GatherAssignments()
    for stmnt in node.orelse: egather.visit(stmnt)

    for symbol in stable:
        node_list = self.assign_id_map.setdefault(symbol, [])
        assignments = []

        for asgn_list in bgather.assign_id_map[symbol]:
            assignments.extend(asgn_list.assignments)
        for asgn_list in egather.assign_id_map[symbol]:
            assignments.extend(asgn_list.assignments)

        node_list.append(Assignment(root=node, assignments=assignments))

class GatherAssignments(Visitor):
    '''
    Collect ast nodes that assign to the same variable.
    '''

    def __init__(self):
        self.assign_id_map = {}

    visitTryExcept = dont_visit

    visitDefault = visit_children

    visitIf = visit_conditional
    visitFor = visit_conditional
    visitWhile = visit_conditional

    def visitAssign(self, node):

        target_ids = [get_symbols(target, ast.Store) for target in node.targets]
        target_ids = set.union(*target_ids)

        for id in target_ids:
            node_list = self.assign_id_map.setdefault(id, [])
            node_list.append(Assignment(root=node, assignments=(node,)))

def remove_trivial(root):
    '''
    Remove redundant statements.
    
    The statement `a = 1` will be removed::
        
        a = 1
        a = 2

    The statement `a = 1` will not be removed because `b` depends on it::
        
        a = 1
        b = a + 2
        a = 2
        
    :param root: ast node
    '''

    gen = GatherAssignments()
    gen.visit(root)

    to_remove = []

    for symbol, assignments in gen.assign_id_map.items():
        if len(assignments) < 2:
            continue

        for j in range(len(assignments) - 1):
            i1 = root.body.index(assignments[j].root)
            i2 = root.body.index(assignments[j + 1].root)

            body = root.body[i1 + 1:i2]
            grapher = GraphGen()
            for stmnt in body:
                grapher.visit(stmnt)

            if symbol not in grapher.used:
                to_remove.extend(assignments[j].assignments)

    Pass = lambda node: _ast.Pass(lineno=node.lineno, col_offset=node.col_offset)

    for old in to_remove:
        replace_nodes(root, old, Pass(old))

def remove_unused_assign(root, symbol):
    '''
    Remove redundant statements.
    
    The statement `a = 1` will be removed::
        
        a = 1
        a = 2

    The statement `a = 1` will not be removed because `b` depends on it::
        
        a = 1
        b = a + 2
        a = 2
        
    :param root: ast node
    '''

    gen = GatherAssignments()
    gen.visit(root)

    to_remove = []

    if symbol not in gen.assign_id_map:
        return


    assignments = gen.assign_id_map[symbol]

    if len(assignments) < 2:
        return 

    for j in range(len(assignments) - 1):
        i1 = root.body.index(assignments[j].root)
        i2 = root.body.index(assignments[j + 1].root)

        body = root.body[i1 + 1:i2]
        grapher = GraphGen()
        for stmnt in body:
            grapher.visit(stmnt)

        if symbol not in grapher.used:
            to_remove.extend(assignments[j].assignments)

    Pass = lambda node: _ast.Pass(lineno=node.lineno, col_offset=node.col_offset)

    for old in to_remove:
        replace_nodes(root, old, Pass(old))


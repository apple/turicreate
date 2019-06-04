# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Jul 18, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import _ast
from ...asttools import Visitor, visit_children

def removable(self, node):
    '''
    node is removable only if all of its children are as well.
    '''
    throw_away = []
    for child in self.children(node):
        throw_away.append(self.visit(child))

    if self.mode == 'exclusive':
        return all(throw_away)
    elif self.mode == 'inclusive':
        return any(throw_away)
    else:
        raise TypeError("mode must be one of 'exclusive' or 'inclusive'")

# Helper function to create a pass node with a line number and col_offset
Pass = lambda node: _ast.Pass(lineno=node.lineno, col_offset=node.col_offset)

class PruneVisitor(Visitor):
    '''
    Visitor to remove ast nodes
    
    :param symbols: set of symbol that are removable.
    
    '''
    def __init__(self, symbols, mode='exclusive'):
        self.remove_symbols = symbols
        
        if mode not in ['exclusive', 'inclusive']:
            raise TypeError("mode must be one of 'exclusive' or 'inclusive'")
        
        self.mode = mode

    visitDefault = removable
    
    def reduce(self, body):
        '''
        remove nodes from a list
        '''
        i = 0
        while i < len(body):
            stmnt = body[i]
            if self.visit(stmnt):
                body.pop(i)
            else:
                i += 1

    def visitModule(self, node):
        self.reduce(node.body)

    def visitFunctionDef(self, node):
        return node.name in self.remove_symbols

    def visitLambda(self, node):
        return False

    def visitName(self, node):
        return node.id in self.remove_symbols

    def visitReturn(self, node):
        return False

    def visitFor(self, node):
        target = self.visit(node.target)
        for_iter = self.visit(node.iter)
        self.reduce(node.body)

        len_body = len(node.body)
        if len_body == 0:
            node.body.append(_ast.Pass(lineno=node.lineno, col_offset=node.col_offset))


        self.reduce(node.orelse)

        len_orelse = len(node.orelse)

        return (len_body == 0) and (len_orelse == 0) and target and for_iter

    def visitIf(self, node):

        can_remove_test = self.visit(node.test)

        self.reduce(node.body)

        len_body = len(node.body)

        if len_body == 0:
            node.body.append(Pass(node))

        self.reduce(node.orelse)
        len_else = len(node.orelse)

        return len_body == 0 and len_else == 0

    def visitWith(self, node):
        self.reduce(node.body)

        if len(node.body) == 0:
            node.body.append(Pass(node))
            if node.optional_vars is None or self.visit(node.optional_vars):
                return True
            else:
                return False

    def visitWhile(self, node):

        discard_test = self.visit(node.test)

        self.reduce(node.body)

        len_body = len(node.body)
        if len_body == 0:
            node.body.append(Pass(node))

        self.reduce(node.orelse)

        return len_body == 0 and len(node.orelse) == 0

    def visitAttribute(self, node):
        return self.visit(node.value)

    visitGlobal = visit_children

    def visitExpr(self, node):
        return self.visit(node.value)

    def visitalias(self, node):
        if node.asname:
            return node.asname in self.remove_symbols
        else:
            return node.name in self.remove_symbols

    def visitTryFinally(self, node):

        assert len(node.body)
        remove_body = self.visit(node.body[0])

        self.reduce(node.finalbody)

        len_final = len(node.finalbody)
        if len_final == 0:
            node.finalbody.append(Pass(node))

        return remove_body and len_final == 0

    def visitTryExcept(self, node):

        self.reduce(node.body)
        self.reduce(node.orelse)

        len_body = len(node.body)

        if len_body == 0:
            node.body.append(Pass(node))

        for hndlr in node.handlers:
            self.reduce(hndlr.body)
            if len(hndlr.body) == 0:
                hndlr.body.append(Pass(hndlr))

        if len_body == 0:
            node.handlers = [_ast.ExceptHandler(type=None, name=None, body=[Pass(node)], lineno=node.lineno, col_offset=node.col_offset)]

        return len_body == 0 and len(node.orelse) == 0

    def visitExec(self, node):
        return False

    def visitRaise(self, node):
        return False


# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Aug 4, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ..visitors import Visitor, visit_children
from ..visitors.symbol_visitor import get_symbols
import ast
from ...utils import py2op

class ConditionalSymbolVisitor(Visitor):

    def __init__(self):
        self._cond_lhs = set()
        self._stable_lhs = set()

        self._cond_rhs = set()
        self._stable_rhs = set()

        self.undefined = set()

        self.seen_break = False

    visitModule = visit_children
    visitPass = visit_children


    def update_stable_rhs(self, symbols):
        new_symbols = symbols - self._stable_rhs
        self._update_undefined(new_symbols)

        if self.seen_break:
            self._cond_rhs.update(new_symbols)
        else:
            self._cond_rhs -= new_symbols
            self._stable_rhs.update(new_symbols)

    def update_stable_lhs(self, symbols):
        new_symbols = symbols - self._stable_lhs

        if self.seen_break:
            self._cond_lhs.update(new_symbols)
        else:
            self._cond_lhs -= new_symbols
            self._stable_lhs.update(new_symbols)

    def update_cond_rhs(self, symbols):

        new_symbols = symbols - self._stable_rhs
        self._update_undefined(new_symbols)
        self._cond_rhs.update(new_symbols)

    def update_cond_lhs(self, symbols):
        self._cond_lhs.update(symbols - self._stable_lhs)

    def _update_undefined(self, symbols):
        self.undefined.update(symbols - self._stable_lhs)
    update_undefined = _update_undefined
    @property
    def stable_lhs(self):
        assert not (self._stable_lhs & self._cond_lhs)
        return self._stable_lhs

    @property
    def stable_rhs(self):
        assert not (self._stable_rhs & self._cond_rhs)
        return self._stable_rhs

    @property
    def cond_rhs(self):
        assert not (self._stable_rhs & self._cond_rhs)
        return self._cond_rhs

    @property
    def cond_lhs(self):
        assert not (self._stable_lhs & self._cond_lhs)
        return self._cond_lhs

    @property
    def lhs(self):
        assert not (self._stable_lhs & self._cond_lhs)
        return self._cond_lhs | self._stable_lhs

    @property
    def rhs(self):
        assert not (self._stable_rhs & self._cond_rhs)
        return self._cond_rhs | self._stable_rhs

    def visitAugAssign(self, node):
        values = get_symbols(node.value)

        self.update_stable_rhs(values)

        targets = get_symbols(node.target)

        self.update_stable_rhs(targets)

        self.update_stable_lhs(targets)

    def visitAssign(self, node):
        ids = set()
        for target in node.targets:
            ids.update(get_symbols(target, ast.Store))

        rhs_ids = get_symbols(node.value, ast.Load)

        for target in node.targets:
            rhs_ids.update(get_symbols(target, ast.Load))

        self.update_stable_rhs(rhs_ids)
        self.update_stable_lhs(ids)

    def visitBreak(self, node):
        self.seen_break = True

    def visitContinue(self, node):
        self.seen_break = True


    def visit_loop(self, node):

        gen = ConditionalSymbolVisitor()
        for stmnt in node.body:
            gen.visit(stmnt)

        self.update_cond_lhs(gen.cond_lhs)
        self.update_cond_rhs(gen.cond_rhs)

        outputs = gen.stable_lhs
        inputs = gen.stable_rhs

        gen = ConditionalSymbolVisitor()
        for stmnt in node.orelse:
            gen.visit(stmnt)

        self.update_cond_rhs(gen.cond_rhs)
        self.update_cond_lhs(gen.cond_lhs)

        orelse_outputs = gen.stable_lhs
        orelse_inputs = gen.stable_rhs

        self.update_stable_lhs(outputs.intersection(orelse_outputs))
        self.update_stable_rhs(inputs.intersection(orelse_inputs))

        self.update_cond_lhs(outputs.symmetric_difference(orelse_outputs))
        self.update_cond_rhs(inputs.symmetric_difference(orelse_inputs))

    def visitFor(self, node):

        lhs_symbols = get_symbols(node.target, ast.Store)
        self.update_cond_lhs(lhs_symbols)

        rhs_symbols = get_symbols(node.iter, ast.Load)

        self.update_stable_rhs(rhs_symbols)

        remove_from_undef = lhs_symbols - self.undefined
        self.visit_loop(node)
        self.undefined -= remove_from_undef

    def visitExpr(self, node):

        rhs_ids = get_symbols(node, ast.Load)
        self.update_stable_rhs(rhs_ids)

    def visitPrint(self, node):

        rhs_ids = get_symbols(node, ast.Load)
        self.update_stable_rhs(rhs_ids)

    def visitWhile(self, node):

        rhs_symbols = get_symbols(node.test, ast.Load)

        self.update_stable_rhs(rhs_symbols)

        self.visit_loop(node)

    def visitIf(self, node):

        rhs_symbols = get_symbols(node.test, ast.Load)
        self.update_stable_rhs(rhs_symbols)

        gen = ConditionalSymbolVisitor()
        for stmnt in node.body:
            gen.visit(stmnt)

        if gen.seen_break:
            self.seen_break = True

        self.update_cond_lhs(gen._cond_lhs)
        self.update_cond_rhs(gen._cond_rhs)

        outputs = gen.stable_lhs
        inputs = gen.stable_rhs

        gen = ConditionalSymbolVisitor()
        for stmnt in node.orelse:
            gen.visit(stmnt)

        self.update_cond_lhs(gen._cond_lhs)
        self.update_cond_rhs(gen._cond_rhs)

        orelse_outputs = gen.stable_lhs
        orelse_inputs = gen.stable_rhs

        self.update_stable_lhs(outputs.intersection(orelse_outputs))
        self.update_stable_rhs(inputs.intersection(orelse_inputs))

        self.update_cond_lhs(outputs.symmetric_difference(orelse_outputs))
        self.update_cond_rhs(inputs.symmetric_difference(orelse_inputs))
    
    @py2op
    def visitExec(self, node):

        self.update_stable_rhs(get_symbols(node.body, ast.Load))

        if node.globals:
            self.update_stable_rhs(get_symbols(node.globals, ast.Load))

        if node.locals:
            self.update_stable_rhs(get_symbols(node.locals, ast.Load))

    def visitAssert(self, node):

        self.update_stable_rhs(get_symbols(node.test, ast.Load))

        if node.msg:
            self.update_stable_rhs(get_symbols(node.msg, ast.Load))
            
    @py2op
    def visitRaise(self, node):

        if node.type:
            self.update_stable_rhs(get_symbols(node.type, ast.Load))
        if node.inst:
            self.update_stable_rhs(get_symbols(node.inst, ast.Load))
        if node.tback:
            self.update_stable_rhs(get_symbols(node.tback, ast.Load))

    @visitRaise.py3op
    def visitRaise(self, node):

        if node.exc:
            self.update_stable_rhs(get_symbols(node.exc, ast.Load))
        if node.cause:
            self.update_stable_rhs(get_symbols(node.cause, ast.Load))

    def visitTryExcept(self, node):

        gen = ConditionalSymbolVisitor()
        gen.visit_list(node.body)

        self.update_undefined(gen.undefined)

        handlers = [csv(hndlr) for hndlr in node.handlers]

        for g in handlers:
            self.update_undefined(g.undefined)

        stable_rhs = gen.stable_rhs.intersection(*[g.stable_rhs for g in handlers])
        self.update_stable_rhs(stable_rhs)

        all_rhs = gen.rhs.union(*[g.rhs for g in handlers])

        self.update_cond_rhs(all_rhs - stable_rhs)

        stable_lhs = gen.stable_lhs.intersection(*[g.stable_lhs for g in handlers])
        self.update_stable_lhs(stable_lhs)

        all_lhs = gen.lhs.union(*[g.lhs for g in handlers])
        self.update_cond_lhs(all_lhs - stable_lhs)

        gen = ConditionalSymbolVisitor()
        gen.visit_list(node.orelse)

        self.update_undefined(gen.undefined)
        self.update_cond_lhs(gen.lhs)
        self.update_cond_rhs(gen.rhs)

    @py2op
    def visitExceptHandler(self, node):
        if node.type:
            self.update_stable_rhs(get_symbols(node.type, ast.Load))

        if node.name:
            self.update_stable_lhs(get_symbols(node.name, ast.Store))

        self.visit_list(node.body)

    @visitExceptHandler.py3op
    def visitExceptHandler(self, node):
        if node.type:
            self.update_stable_rhs(get_symbols(node.type, ast.Load))

        if node.name:
            self.update_stable_lhs({node.name})

        self.visit_list(node.body)

    def visitTryFinally(self, node):
        self.visit_list(node.body)
        self.visit_list(node.finalbody)

    def visitImportFrom(self, node):
        symbols = get_symbols(node)
        self.update_stable_lhs(symbols)

    def visitImport(self, node):
        symbols = get_symbols(node)
        self.update_stable_lhs(symbols)

    def visitLambda(self, node):

        gen = ConditionalSymbolVisitor()
        gen.update_stable_lhs(symbols={arg for arg in node.args.args})
        gen.visit_list(node.body)

        self.update_stable_rhs(gen.undefined)

    def visitFunctionDef(self, node):

        for decorator in node.decorator_list:
            self.update_stable_rhs(get_symbols(decorator, ast.Load))

        self.update_stable_lhs({node.name})

        gen = ConditionalSymbolVisitor()
        gen.update_stable_lhs(symbols={arg for arg in node.args.args})
        gen.visit_list(node.body)

        self.update_stable_rhs(gen.undefined)

    def visitGlobal(self, node):
        pass

    def visitWith(self, node):

        self.update_stable_rhs(get_symbols(node.context_expr, ast.Load))

        if node.optional_vars:
            self.update_stable_lhs(get_symbols(node.optional_vars, ast.Load))

        self.visit_list(node.body)

    def visitReturn(self, node):
        self.update_stable_rhs(get_symbols(node.value, ast.Load))
        
def csv(node):
    gen = ConditionalSymbolVisitor()
    gen.visit(node)
    return gen

def lhs(node):
    '''
    Return a set of symbols in `node` that are assigned.
    
    :param node: ast node 
    
    :returns: set of strings.
    '''

    gen = ConditionalSymbolVisitor()
    if isinstance(node, (list, tuple)):
        gen.visit_list(node)
    else:
        gen.visit(node)
    return gen.lhs

def rhs(node):
    '''
    Return a set of symbols in `node` that are used.
    
    :param node: ast node 
    
    :returns: set of strings.
    '''

    gen = ConditionalSymbolVisitor()
    if isinstance(node, (list, tuple)):
        gen.visit_list(node)
    else:
        gen.visit(node)
    return gen.rhs

def conditional_lhs(node):
    '''
    Group outputs into conditional and stable
    :param node: ast node 
    
    :returns: tuple of (conditional, stable)
    
    '''

    gen = ConditionalSymbolVisitor()
    gen.visit(node)
    return gen.cond_lhs, gen.stable_lhs


def conditional_symbols(node):
    '''
    Group lhs and rhs into conditional, stable and undefined
    :param node: ast node 
    
    :returns: tuple of (conditional_lhs, stable_lhs),(conditional_rhs, stable_rhs), undefined
    
    '''

    gen = ConditionalSymbolVisitor()
    gen.visit(node)
    lhs = gen.cond_lhs, gen.stable_lhs
    rhs = gen.cond_rhs, gen.stable_rhs
    undefined = gen.undefined
    return lhs, rhs, undefined

if __name__ == '__main__':

    source = '''
while k:
    a = 1
    b = 1
    break
    d = 1
else:
    a =2
    c= 3
    d = 1
    '''

    print(conditional_lhs(ast.parse(source)))



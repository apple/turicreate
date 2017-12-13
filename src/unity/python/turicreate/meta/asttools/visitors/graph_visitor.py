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
from ...asttools import Visitor, visit_children

import _ast
from ...asttools.visitors.symbol_visitor import get_symbols
try:
    from networkx import DiGraph
except ImportError:
    DiGraph = None

def collect_(self, node):
    names = set()
    for child in self.children(node):
        names.update(self.visit(child))

    if hasattr(node, 'ctx'):
        if isinstance(node.ctx, _ast.Store):
            self.modified.update(names)
        elif isinstance(node.ctx, _ast.Load):
            self.used.update(names)
    return names


class CollectNodes(Visitor):

    def __init__(self, call_deps=False):
        self.graph = DiGraph()
        self.modified = set()
        self.used = set()
        self.undefined = set()
        self.sources = set()
        self.targets = set()

        self.context_names = set()

        self.call_deps = call_deps

    visitDefault = collect_

    def visitName(self, node):
        if isinstance(node.ctx, _ast.Store):
            self.modified.add(node.id)

        elif isinstance(node.ctx, _ast.Load):
            self.used.update(node.id)

        if not self.graph.has_node(node.id):
            self.graph.add_node(node.id)
            if isinstance(node.ctx, _ast.Load):
                self.undefined.add(node.id)

        for ctx_var in self.context_names:
            if not self.graph.has_edge(node.id, ctx_var):
                self.graph.add_edge(node.id, ctx_var)

        return {node.id}

    def visitalias(self, node):
        name = node.asname if node.asname else node.name

        if '.' in name:
            name = name.split('.', 1)[0]

        if not self.graph.has_node(name):
            self.graph.add_node(name)

        return {name}

    def visitCall(self, node):
        left = self.visit(node.func)

        right = set()
        for attr in ('args', 'keywords'):
            for child in getattr(node, attr):
                if child:
                    right.update(self.visit(child))

        for attr in ('starargs', 'kwargs'):
            child = getattr(node, attr)
            if child:
                right.update(self.visit(child))

        for src in left | right:
            if not self.graph.has_node(src):
                self.undefined.add(src)

        if self.call_deps:
            add_edges(self.graph, left, right)
            add_edges(self.graph, right, left)

        right.update(left)
        return right

    def visitSubscript(self, node):
        if isinstance(node.ctx, _ast.Load):
            return collect_(self, node)
        else:
            sources = self.visit(node.slice)
            targets = self.visit(node.value)
            self.modified.update(targets)
            add_edges(self.graph, targets, sources)
            return targets
        
    def handle_generators(self, generators):
        defined = set()
        required = set()
        for generator in generators:
            get_symbols(generator, _ast.Load)
            required.update(get_symbols(generator, _ast.Load) - defined)
            defined.update(get_symbols(generator, _ast.Store))
            
        return defined, required
    
    def visitListComp(self, node):

        defined, required = self.handle_generators(node.generators)
        required.update(get_symbols(node.elt, _ast.Load) - defined)

        for symbol in required:
            if not self.graph.has_node(symbol):
                self.graph.add_node(symbol)
                self.undefined.add(symbol)

        return required

    def visitSetComp(self, node):

        defined, required = self.handle_generators(node.generators)
        required.update(get_symbols(node.elt, _ast.Load) - defined)

        for symbol in required:
            if not self.graph.has_node(symbol):
                self.graph.add_node(symbol)
                self.undefined.add(symbol)

        return required

    def visitDictComp(self, node):

        defined, required = self.handle_generators(node.generators)
        required.update(get_symbols(node.key, _ast.Load) - defined)
        required.update(get_symbols(node.value, _ast.Load) - defined)

        for symbol in required:
            if not self.graph.has_node(symbol):
                self.graph.add_node(symbol)
                self.undefined.add(symbol)

        return required


def add_edges(graph, targets, sources):
        for target in targets:
            for src in sources:
                edge = target, src
                if not graph.has_edge(*edge):
                    graph.add_edge(*edge)

class GlobalDeps(object):
    def __init__(self, gen, nodes):
        self.nodes = nodes
        self.gen = gen

    def __enter__(self):
        self._old_context_names = set(self.gen.context_names)
        self.gen.context_names.update(self.nodes)

    def __exit__(self, *args):
        self.gen.context_names = self._old_context_names

class GraphGen(CollectNodes):
    '''
    Create a graph from the execution flow of the ast
    '''

    visitModule = visit_children

    def depends_on(self, nodes):
        return GlobalDeps(self, set(nodes))

    def visit_lambda(self, node):
        sources = self.visit(node.args)
        self.sources.update(sources)

        self.visit(node.body)

    def visitLambda(self, node):
        gen = GraphGen()
        gen.visit_lambda(node)

        for undef in gen.undefined:
            if not self.graph.has_node(undef):
                self.graph.add_node(undef)

        return gen.undefined

    def visit_function_def(self, node):
        sources = self.visit(node.args)
        self.sources.update(sources)

        for stmnt in node.body:
            self.visit(stmnt)


    def visitFunctionDef(self, node):

        gen = GraphGen()
        gen.visit_function_def(node)

        if not self.graph.has_node(node.name):
            self.graph.add_node(node.name)

        for undef in gen.undefined:
            if not self.graph.has_node(undef):
                self.graph.add_node(undef)

        add_edges(self.graph, [node.name], gen.undefined)

        return gen.undefined

    def visitAssign(self, node):
        nodes = self.visit(node.value)

        tsymbols = get_symbols(node, _ast.Store)
        re_defined = tsymbols.intersection(set(self.graph.nodes()))
        if re_defined:
            add_edges(self.graph, re_defined, re_defined)

        targets = set()
        for target in node.targets:
            targets.update(self.visit(target))

        add_edges(self.graph, targets, nodes)

        return targets | nodes

    def visitAugAssign(self, node):

        targets = self.visit(node.target)
        values = self.visit(node.value)

        self.modified.update(targets)

        for target in targets:
            for value in values:
                edge = target, value
                if not self.graph.has_edge(*edge):
                    self.graph.add_edge(*edge)

            for tgt2 in targets:
                edge = target, tgt2
                if not self.graph.has_edge(*edge):
                    self.graph.add_edge(*edge)

        return targets | values

    def visitFor(self, node):

        nodes = set()
        targets = self.visit(node.target)
        for_iter = self.visit(node.iter)

        nodes.update(targets)
        nodes.update(for_iter)

        add_edges(self.graph, targets, for_iter)

        with self.depends_on(for_iter):

            for stmnt in node.body:
                nodes.update(self.visit(stmnt))

        return nodes

    def visitIf(self, node):

        nodes = set()
        names = self.visit(node.test)

        nodes.update(names)
        with self.depends_on(names):

            for stmnt in node.body:
                nodes.update(self.visit(stmnt))

            for stmnt in node.orelse:
                nodes.update(self.visit(stmnt))

        return nodes

    def visitReturn(self, node):

        targets = self.visit(node.value)

        self.targets.update(targets)

        return targets

    def visitWith(self, node):

        nodes = set()
        targets = self.visit(node.context_expr)

        nodes.update(targets)

        if node.optional_vars is None:
            vars = ()
        else:
            vars = self.visit(node.optional_vars)

        nodes.update(vars)
        add_edges(self.graph, vars, targets)

        with self.depends_on(targets):
            for stmnt in node.body:
                nodes.update(self.visit(stmnt))

        return nodes


    def visitWhile(self, node):

        nodes = set()
        targets = self.visit(node.test)

        nodes.update(targets)

        with self.depends_on(targets):
            for stmnt in node.body:
                nodes.update(self.visit(stmnt))

            for stmnt in node.orelse:
                nodes.update(self.visit(stmnt))

        return nodes

    def visitTryFinally(self, node):

        assert len(node.body) == 1

        nodes = self.visit(node.body[0])

        with self.depends_on(nodes):
            for stmnt in node.finalbody:
                nodes.update(self.visit(stmnt))

    def visitTryExcept(self, node):

        body_nodes = set()
        for stmnt in node.body:
            body_nodes.update(self.visit(stmnt))

        all_nodes = set(body_nodes)

        for hndlr in node.handlers:
            nodes = set(body_nodes)

            if hndlr.name:
                nodes.update(self.visit(hndlr.name))
            if hndlr.type:
                nodes.update(self.visit(hndlr.type))

            with self.depends_on(nodes):
                for stmnt in hndlr.body:
                    nodes.update(self.visit(stmnt))

            all_nodes.update(nodes)

        nodes = set(body_nodes)
        with self.depends_on(nodes):
            for stmnt in node.orelse:
                nodes.update(self.visit(stmnt))

        all_nodes.update(nodes)


        return all_nodes



def make_graph(node, call_deps=False):
    '''
    Create a dependency graph from an ast node.
    
    :param node: ast node.
    :param call_deps: if true, then the graph will create a cyclic dependence for all
                      function calls. (i.e for `a.b(c)` a depends on b and b depends on a)
                      
    :returns: a tuple of (graph, undefined)
    '''

    gen = GraphGen(call_deps=call_deps)
    gen.visit(node)

    return gen.graph, gen.undefined




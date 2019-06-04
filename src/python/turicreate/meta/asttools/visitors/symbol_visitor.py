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
from ...asttools.visitors import Visitor
import ast

class SymbolVisitor(Visitor):
    def __init__(self, ctx_types=(ast.Load, ast.Store)):

        if not isinstance(ctx_types, (list, tuple)):
            ctx_types = (ctx_types,)

        self.ctx_types = tuple(ctx_types)

    def visitDefault(self, node):
        ids = set()
        for child in self.children(node):

            if isinstance(child, (tuple, list)):
                for item in child:
                    ids.update(self.visit(item))

            elif isinstance(child, ast.AST):
                ids.update(self.visit(child))

        return ids

    def visitName(self, node):
        if isinstance(node.ctx, self.ctx_types):
            return {node.id}
        else:
            return set()

    def visitalias(self, node):

        name = node.asname if node.asname else node.name

        if '.' in name:
            name = name.split('.', 1)[0]

        if ast.Store in self.ctx_types:
            return {name}
        else:
            return set()



def get_symbols(node, ctx_types=(ast.Load, ast.Store)):
    '''
    Returns all symbols defined in an ast node. 
    
    if ctx_types is given, then restrict the symbols to ones with that context.
    
    :param node: ast node
    :param ctx_types: type or tuple of types that may be found assigned to the `ctx` attribute of 
                      an ast Name node.
        
    '''
    gen = SymbolVisitor(ctx_types)
    return gen.visit(node)

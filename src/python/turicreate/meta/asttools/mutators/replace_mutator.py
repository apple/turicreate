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
from ...asttools.visitors import Visitor

class Replacer(Visitor):
    '''
    Visitor to replace nodes. 
    '''

    def __init__(self, old, new):
        self.old = old
        self.new = new

    def visitDefault(self, node):
        for field in node._fields:
            value = getattr(node, field)

            if value == self.old:
                setattr(node, field, self.new)

            if isinstance(value, (list, tuple)):
                for i, item in enumerate(value):
                    if item == self.old:
                        value[i] = self.new
                    elif isinstance(item, _ast.AST):
                        self.visit(item)
                    else:
                        pass
            elif isinstance(value, _ast.AST):
                self.visit(value)
            else:
                pass

        return

def replace_nodes(root, old, new):

    '''
    Replace the old node with the new one. 
    Old must be an indirect child of root
     
    :param root: ast node that contains an indirect reference to old
    :param old: node to replace
    :param new: node to replace `old` with 
    '''

    rep = Replacer(old, new)
    rep.visit(root)
    return

class NodeRemover(Visitor):
    '''
    Remove a node.
    '''
    def __init__(self, to_remove):
        self.to_remove

    def visitDefault(self, node):
        for field in node._fields:
            value = getattr(node, field)

            if value in self.to_remove:
                setattr(node, field, self.new)

            if isinstance(value, (list, tuple)):
                for i, item in enumerate(value):
                    if item == self.old:
                        value[i] = self.new
                    elif isinstance(item, _ast.AST):
                        self.visit(item)
                    else:
                        pass
            elif isinstance(value, _ast.AST):
                self.visit(value)
            else:
                pass

        return


# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Dec 12, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..asttools import Visitor
import ast

#FIXME: add tests
class CopyVisitor(Visitor):
    '''
    Copy only ast nodes and lists
    '''
    def visitDefault(self, node):
        Node = type(node)
        new_node = Node()
        
        for _field in Node._fields:
            if hasattr(node, _field):
                field = getattr(node, _field)
                if isinstance(field, (list, tuple)):
                    new_list = []
                    for item in field:
                        if isinstance(item, ast.AST):
                            new_item = self.visit(item)
                        else:
                            new_item = item
                        new_list.append(new_item)
                        
                    setattr(new_node, _field, new_list)
                elif isinstance(field, ast.AST):
                    setattr(new_node, _field, self.visit(field))
                else:
                    setattr(new_node, _field, field)
        
        for _attr in node._attributes:
            if hasattr(node, _attr):
                setattr(new_node, _attr, getattr(node, _attr))

        return new_node


def copy_node(node):
    return CopyVisitor().visit(node)

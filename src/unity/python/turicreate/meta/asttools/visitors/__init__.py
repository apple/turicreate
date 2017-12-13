# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import _ast

def dont_visit(self, node):
    pass

def visit_children(self, node):
    for child in self.children(node):
        self.visit(child)


class Visitor(object):

    def children(self, node):
        for field in node._fields:
            value = getattr(node, field)
            if isinstance(value, (list, tuple)):
                for item in value:
                    if isinstance(item, _ast.AST):
                        yield item
                    else:
                        pass
            elif  isinstance(value, _ast.AST):
                yield value

        return



    def visit_list(self, nodes, *args, **kwargs):
        
        result = []
        for node in nodes:
            result.append(self.visit(node, *args, **kwargs))
        return result

    def visit(self, node, *args, **kwargs):
        node_name = type(node).__name__

        attr = 'visit' + node_name

        if hasattr(self, attr):
            method = getattr(self, 'visit' + node_name)
            return method(node, *args, **kwargs)
        elif hasattr(self, 'visitDefault'):
            method = getattr(self, 'visitDefault')
            return method(node, *args, **kwargs)
        else:
            method = getattr(self, 'visit' + node_name)
            return method(node, *args, **kwargs)



class Mutator(Visitor):
    
    def mutateDefault(self, node):
        for field in node._fields:
            value = getattr(node, field)
            if isinstance(value, (list, tuple)):
                for i, item in enumerate(value):
                    if isinstance(item, _ast.AST):
                        new_item = self.mutate(item)
                        if new_item is not None:
                            value[i] = new_item 
                    else:
                        pass
                    
            elif  isinstance(value, _ast.AST):
                new_value = self.mutate(value)
                if new_value is not None:
                    setattr(node, field, new_value)

        return None
    
    def mutate(self, node, *args, **kwargs):
        
        node_name = type(node).__name__

        attr = 'mutate' + node_name

        if hasattr(self, attr):
            mehtod = getattr(self, 'mutate' + node_name)
            return mehtod(node, *args, **kwargs)
        elif hasattr(self, 'mutateDefault'):
            mehtod = getattr(self, 'mutateDefault')
            return mehtod(node, *args, **kwargs)
        else:
            mehtod = getattr(self, 'mutate' + node_name)
            return mehtod(node, *args, **kwargs)


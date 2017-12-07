# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Jul 15, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import _ast
import sys
py3 = sys.version_info.major >= 3

def ast_keys(node):
    return node._fields

def ast_values(node):
    return [getattr(node, field, None) for field in node._fields]

def ast_items(node):
    return [(field, getattr(node, field, None)) for field in node._fields]


def depth(node):
    return len(flatten(node))

def flatten(node):

    result = []
    if isinstance(node, _ast.AST):
        for value in ast_values(node):
            result.extend(flatten(value))
    elif isinstance(node, (list, tuple)):
        for child in node:
            result.extend(flatten(child))
    else:
        result.append(node)

    return result

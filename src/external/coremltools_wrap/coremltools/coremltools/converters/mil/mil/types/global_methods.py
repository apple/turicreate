# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

"""
This defines a list of all the "global methods" like len. Or type cast
operators like int, list, double, etc.

The difficulty with some of these methods is that they don't have fixed types.
For instance len(x) allows x to be list or a dictionary.

However we don't support function overloading based on types, and we don't
intend to. (It is complicated, requires the parser to be far more intelligent
and do good type inference; will either require genre to support overloading
or do name mangling.

The final quirk is that we probably should not call these functions "len"
or "int" because that will conflict with the existing python methods.

So what we will simply do is to rewrite them to things like __len__, __str__
and __int__ and __double__
"""

global_remap = {
    "len": "__len__",
    "str": "__str__",
    "int": "__int__",
    "double": "__double__",
    "float": "__double__",
    "bool": "__bool__",
    "log": "__log__",
    "exp": "__exp__",
    "max": "__max__",
    "min": "__min__",
}

global_invremap = {
    "__len__": "len",
    "__str__": "str",
    "__int__": "int",
    "__double__": "float",
    "__bool__": "bool",
    "__log__": "math.log",
    "__exp__": "math.exp",
    "__max__": "max",
    "__min__": "min",
}

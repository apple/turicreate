# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import six

_varname_charset = set(
    [chr(i) for i in range(ord("A"), ord("Z") + 1)]
    + [chr(i) for i in range(ord("a"), ord("z") + 1)]
    + [chr(i) for i in range(ord("0"), ord("9") + 1)]
    + ["_"]
)


def escape_name(name):
    ret = "".join([i if i in _varname_charset else "_" for i in name])
    if ret.endswith("_"):
        return ret
    else:
        return ret + "_"


def escape_fn_name(name):
    ret = "".join([i if i in _varname_charset else "_" for i in name])
    ret = escape_name(name)
    if ret.startswith("f_"):
        return ret
    else:
        return "f_" + ret


def normalize_names(names):
    if isinstance(names, six.string_types):
        return names.replace(":", "__").replace("/", "__")
    return [i.replace(":", "__").replace("/", "__") for i in names]

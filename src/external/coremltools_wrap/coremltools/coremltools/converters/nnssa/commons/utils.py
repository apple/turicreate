# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import sys

_varname_charset = set([chr(i) for i in range(ord('A'),
                                              ord('Z') + 1)] +
                       [chr(i) for i in range(ord('a'),
                                              ord('z') + 1)] +
                       [chr(i) for i in range(ord('0'),
                                              ord('9') + 1)] + ['_'])

if sys.version_info >= (3, 0):
    str_types = (str)
else:
    str_types = (str, unicode)


def escape_name(name):
    ret = ''.join([i if i in _varname_charset else '_' for i in name])
    if ret.endswith('_'):
        return ret
    else:
        return ret + '_'


def escape_fn_name(name):
    ret = ''.join([i if i in _varname_charset else '_' for i in name])
    ret = escape_name(name)
    if ret.startswith('f_'):
        return ret
    else:
        return 'f_' + ret


def normalize_names(names):
    if isinstance(names, str_types):
        return names.replace(':', '__').replace('/', '__')
    return [i.replace(':', '__').replace('/', '__') for i in names]

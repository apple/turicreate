# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_flexible_type cimport common_typed_flex_list_from_iterable
from .cy_flexible_type cimport pylist_from_flex_list
from .cy_flexible_type cimport pytype_from_flex_type_enum

HAS_PANDAS = False

try:
    import pandas as pd
    HAS_PANDAS = True
except:
    HAS_PANDAS = False


cdef bint is_pandas_dataframe(object v):
    if HAS_PANDAS:
        return isinstance(v, pd.core.frame.DataFrame)
    else:
        return False
    
cdef gl_dataframe gl_dataframe_from_dict_of_arrays(dict df) except *:
    cdef gl_dataframe ret
    cdef flex_type_enum ftype
    cdef flex_list fl

    for key, value in sorted(df.iteritems()):
        ret.names.push_back(key.encode('utf-8'))
        ret.values[key.encode('utf-8')] = common_typed_flex_list_from_iterable(value, &ftype)
        ret.types[key.encode('utf-8')] = ftype

    return ret

cdef pd_from_gl_dataframe(gl_dataframe& df):
    assert HAS_PANDAS, 'Cannot find pandas library'
    ret = pd.DataFrame()
    for _name in df.names:
        _type = pytype_from_flex_type_enum(df.types[_name])
        ret[_name] = pylist_from_flex_list(df.values[_name])
        if len(ret[_name]) == 0:
            """ special handling of empty list, we need to force the type information """
            ret[_name] = ret[_name].astype(_type)
    return ret 


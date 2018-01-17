# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_variant cimport to_value 
from .cy_variant cimport from_dict as variant_map_from_dict
from .cy_cpp_utils cimport from_vector_of_strings, str_to_cpp

cdef create_model_from_proxy(const model_base_ptr& proxy):
    if proxy.get() == NULL:
        return None
    ret = UnityModel()
    ret._base_ptr = proxy
    ret.thisptr = <model_base*>(ret._base_ptr.get())
    return ret


cdef class UnityModel: 
    cpdef list_keys(self):
        return from_vector_of_strings(self.thisptr.list_keys())

    cpdef get(self, key, opts=None):
        cdef string cpp_key = str_to_cpp(key)
        cdef variant_map_type optional_args = variant_map_from_dict(opts)
        cdef variant_type value
        with nogil:
          value = self.thisptr.get_value(cpp_key, optional_args)
        return to_value(value)

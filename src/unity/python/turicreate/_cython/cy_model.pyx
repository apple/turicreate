# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_variant cimport to_value 
from .cy_variant cimport from_dict as variant_map_from_dict
from .cy_cpp_utils cimport str_to_cpp,\
                           cpp_to_str,\
                           from_vector_of_strings,\
                           from_map_of_strings_and_vectors_of_strings

cdef create_model_from_proxy(const model_base_ptr& proxy):
    if proxy.get() == NULL:
        return None
    ret = UnityModel()
    ret._base_ptr = proxy
    ret.thisptr = <model_base*>(ret._base_ptr.get())
    return ret


cdef class UnityModel: 
    cpdef get_uid(self):
        return cpp_to_str(self.thisptr.uid())

    cpdef list_functions(self):
        return from_map_of_strings_and_vectors_of_strings(self.thisptr.list_functions())

    cpdef list_get_properties(self):
        return from_vector_of_strings(self.thisptr.list_get_properties())

    cpdef list_set_properties(self):
        return from_vector_of_strings(self.thisptr.list_set_properties())

    cpdef call_function(self, fn_name, args=None):
        cdef string cpp_fn_name = str_to_cpp(fn_name)
        cdef variant_map_type cpp_args = variant_map_from_dict(args)
        cdef variant_type value
        with nogil:
            value = self.thisptr.call_function(cpp_fn_name, cpp_args)
        return to_value(value)

    cpdef get_property(self, prop_name):
        return to_value(self.thisptr.get_property(str_to_cpp(prop_name)))

    cpdef set_property(self, prop_name, args=None):
        cdef string cpp_prop_name = str_to_cpp(prop_name)
        cdef variant_map_type cpp_args = variant_map_from_dict(args)
        cdef variant_type value
        with nogil:
            value = self.thisptr.set_property(cpp_prop_name, cpp_args)
        return to_value(value)

    cpdef get_docstring(self, symbol):
        return cpp_to_str(self.thisptr.get_docstring(str_to_cpp(symbol)))

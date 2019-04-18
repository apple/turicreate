# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from libcpp.map cimport map
from libcpp.vector cimport vector
from libcpp.string cimport string 
from libcpp.pair cimport pair
from .cy_variant cimport variant_type, variant_map_type
from .cy_unity_base_types cimport model_base_ptr

cdef create_model_from_proxy(const model_base_ptr& proxy)

cdef extern from "<unity/lib/extensions/model_base.hpp>" namespace "turi":
    cdef cppclass model_base nogil:
        vector[string] list_keys() except +
        variant_type get_value(string, variant_map_type&) except +
        map[string, vector[string]] list_functions() except +
        vector[string] list_get_properties() except +
        vector[string] list_set_properties() except +
        variant_type call_function(string, variant_map_type&) except +
        variant_type get_property(string) except +
        variant_type set_property(string, variant_map_type&) except +
        string get_docstring(string) except +
        string uid() except +

cdef class UnityModel:
    cdef model_base* thisptr
    cdef model_base_ptr _base_ptr

    cpdef get_uid(self)
    cpdef list_functions(self)
    cpdef list_get_properties(self)
    cpdef list_set_properties(self)
    cpdef call_function(self, fn_name, args=*)
    cpdef get_property(self, prop_name)
    cpdef set_property(self, prop_name, args=*)
    cpdef get_docstring(self, symbol)

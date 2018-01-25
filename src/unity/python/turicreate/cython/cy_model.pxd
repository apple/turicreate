# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from libcpp.vector cimport vector
from libcpp.string cimport string 
from libcpp.pair cimport pair
from .cy_variant cimport variant_type, variant_map_type
from .cy_unity_base_types cimport model_base_ptr

cdef create_model_from_proxy(const model_base_ptr& proxy)

cdef extern from "<unity/lib/api/model_interface.hpp>" namespace "turi":
    cdef cppclass model_base nogil:
        vector[string] list_keys() except +
        variant_type get_value(string, variant_map_type&) except +

cdef class UnityModel:
    cdef model_base* thisptr
    cdef model_base_ptr _base_ptr

    cpdef list_keys(self)

    cpdef get(self, key, opts=*)


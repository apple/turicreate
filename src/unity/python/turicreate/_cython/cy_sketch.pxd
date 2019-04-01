# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_flexible_type cimport flexible_type
from .cy_sarray cimport UnitySArrayProxy
from libcpp.vector cimport vector
from libcpp.map cimport map
from libcpp.string cimport string
from libcpp.pair cimport pair
from .cy_unity_base_types cimport *

cdef extern from "<unity/lib/unity_sketch.hpp>" namespace "turi":
    cdef cppclass unity_sketch nogil:
        unity_sketch() except +
        void construct_from_sarray(unity_sarray_base_ptr, bint, vector[flexible_type]) except +
        double get_quantile(double) except +
        double frequency_count(flexible_type) except +
        vector[pair[flexible_type, size_t]] frequent_items() except +
        double num_unique() except +
        double mean() except +
        double var() except +
        double max() except +
        double min() except +
        double size() except +
        double sum() except +
        double num_undefined() except +
        bint sketch_ready() except +
        size_t num_elements_processed() except +
        unity_sketch_base_ptr element_length_summary() except +
        unity_sketch_base_ptr element_summary() except +
        unity_sketch_base_ptr dict_key_summary() except +
        unity_sketch_base_ptr dict_value_summary() except +
        map[flexible_type, unity_sketch_base_ptr] element_sub_sketch(vector[flexible_type] keys) except +
        void cancel() except +

cdef create_proxy_wrapper_from_existing_proxy(const unity_sketch_base_ptr& proxy)

cdef class UnitySketchProxy:
    cdef unity_sketch_base_ptr _base_ptr
    cdef unity_sketch* thisptr

    cpdef construct_from_sarray(self, UnitySArrayProxy sarray, bint background, object elements)

    cpdef get_quantile(self, double quantile)

    cpdef frequency_count(self, object element)

    cpdef frequent_items(self)

    cpdef num_unique(self)

    cpdef mean(self)

    cpdef var(self)

    cpdef max(self)

    cpdef min(self)

    cpdef size(self)

    cpdef sum(self)

    cpdef num_undefined(self)

    cpdef sketch_ready(self)

    cpdef num_elements_processed(self)

    cpdef element_length_summary(self)

    cpdef element_summary(self)

    cpdef dict_key_summary(self)

    cpdef dict_value_summary(self)

    cpdef element_sub_sketch(self, object key)

    cpdef cancel(self)

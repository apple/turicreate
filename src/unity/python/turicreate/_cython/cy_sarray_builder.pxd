# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_flexible_type cimport flex_type_enum
from .cy_flexible_type cimport flexible_type
from libcpp.vector cimport vector
from .cy_unity_base_types cimport *

cdef extern from "<unity/lib/unity_sarray_builder.hpp>" namespace "turi":
    cdef cppclass unity_sarray_builder nogil:
        unity_sarray_builder() except +
        void init(size_t, size_t, flex_type_enum) except +
        void append(flexible_type, size_t) except +
        void append_multiple(vector[flexible_type], size_t) except +
        flex_type_enum get_type() except +
        vector[flexible_type] read_history(size_t, size_t) except +
        unity_sarray_base_ptr close() except +

cdef create_proxy_wrapper_from_existing_proxy(const unity_sarray_builder_base_ptr& proxy)

cdef class UnitySArrayBuilderProxy:
    cdef unity_sarray_builder_base_ptr _base_ptr
    cdef unity_sarray_builder* thisptr

    cpdef init(self, size_t num_segments, size_t history_size, type dtype) 

    cpdef append(self, val, size_t segment)

    cpdef append_multiple(self, vals, size_t segment)

    cpdef read_history(self, size_t num_elems, size_t segment)

    cpdef get_type(self)

    cpdef close(self)

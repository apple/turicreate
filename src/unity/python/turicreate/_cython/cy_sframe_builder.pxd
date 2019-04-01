# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_flexible_type cimport flex_type_enum
from .cy_flexible_type cimport flexible_type
from libcpp.vector cimport vector
from libcpp.string cimport string
from .cy_unity_base_types cimport *

cdef extern from "<unity/lib/unity_sframe_builder.hpp>" namespace "turi":
    cdef cppclass unity_sframe_builder nogil:
        unity_sframe_builder() except +
        void init(size_t, size_t, vector[string], vector[flex_type_enum], string) except +
        void append(const vector[flexible_type]&, size_t) except +
        void append_multiple(const vector[vector[flexible_type]]&, size_t) except +
        vector[string] column_names() except +
        vector[flex_type_enum] column_types() except +
        vector[vector[flexible_type]] read_history(size_t, size_t) except +
        unity_sframe_base_ptr close() except +

cdef create_proxy_wrapper_from_existing_proxy(const unity_sframe_builder_base_ptr& proxy)

cdef class UnitySFrameBuilderProxy:
    cdef unity_sframe_builder_base_ptr _base_ptr
    cdef unity_sframe_builder* thisptr

    cpdef init(self, list column_types, _column_names, size_t num_segments, size_t history_size, _save_location)

    cpdef append(self, row, size_t segment)

    cpdef append_multiple(self, object rows, size_t segment)

    cpdef read_history(self, size_t num_elems, size_t segment)

    cpdef column_names(self)

    cpdef column_types(self)

    cpdef close(self)

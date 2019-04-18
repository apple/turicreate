# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

### cython utils ###
from cython.operator cimport dereference as deref

from .cy_unity_base_types cimport *
from .cy_flexible_type cimport flex_list

### flexible_type utils ###
from .cy_flexible_type cimport flex_type_enum_from_pytype
from .cy_flexible_type cimport pytype_from_flex_type_enum
from .cy_flexible_type cimport flex_list_from_iterable
from .cy_flexible_type cimport pylist_from_flex_list

from .cy_cpp_utils cimport str_to_cpp, to_vector_of_strings, from_vector_of_strings

### sframe ###
from .cy_sframe cimport create_proxy_wrapper_from_existing_proxy as sframe_proxy

cdef create_proxy_wrapper_from_existing_proxy(const unity_sframe_builder_base_ptr& proxy):
    if proxy.get() == NULL:
        return None
    ret = UnitySFrameBuilderProxy(True)
    ret._base_ptr = proxy
    ret.thisptr = <unity_sframe_builder*>(ret._base_ptr.get())
    return ret

cdef class UnitySFrameBuilderProxy:

    def __cinit__(self, do_not_allocate=None):
        if do_not_allocate:
            self._base_ptr.reset()
            self.thisptr = NULL
        else:
            self.thisptr = new unity_sframe_builder()
            self._base_ptr.reset(<unity_sframe_builder_base*>(self.thisptr))

    cpdef init(self, list column_types, _column_names, size_t num_segments,
               size_t history_size, _save_location):

        cdef vector[string] column_names = to_vector_of_strings(_column_names)
        cdef string save_location = str_to_cpp(_save_location)
    
        cdef vector[flex_type_enum] tmp_column_types
        tmp_column_types.reserve(len(column_types))
        for i in column_types:
            tmp_column_types.push_back(flex_type_enum_from_pytype(i))
        with nogil:
            self.thisptr.init(num_segments, history_size, column_names, tmp_column_types, save_location)

    cpdef append(self, row, size_t segment):
        cdef flex_list c_row = flex_list_from_iterable(row)
        with nogil:
            self.thisptr.append(c_row, segment)

    cpdef append_multiple(self, object rows, size_t segment):
        cdef vector[vector[flexible_type]] c_vals 
        for i in rows:
            c_vals.push_back(flex_list_from_iterable(i))
        self.thisptr.append_multiple(c_vals, segment)

    cpdef read_history(self, size_t num_elems, size_t segment):
        tmp_history = self.thisptr.read_history(num_elems, segment)
        return [pylist_from_flex_list(i) for i in tmp_history]

    cpdef column_names(self):
        return from_vector_of_strings(self.thisptr.column_names())

    cpdef column_types(self):
        return [pytype_from_flex_type_enum(t) for t in self.thisptr.column_types()]

    cpdef close(self):
        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = self.thisptr.close()
        return sframe_proxy(proxy)

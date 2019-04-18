# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from .cy_unity_base_types cimport *
from .cy_flexible_type cimport flex_list

### flexible_type utils ###
from .cy_flexible_type cimport flex_type_enum_from_pytype
from .cy_flexible_type cimport pytype_from_flex_type_enum
from .cy_flexible_type cimport flexible_type_from_pyobject
from .cy_flexible_type cimport flex_list_from_iterable
from .cy_flexible_type cimport pylist_from_flex_list

### sarray ###
from .cy_sarray cimport create_proxy_wrapper_from_existing_proxy as sarray_proxy

cdef create_proxy_wrapper_from_existing_proxy(const unity_sarray_builder_base_ptr& proxy):
    if proxy.get() == NULL:
        return None
    ret = UnitySArrayBuilderProxy(True)
    ret._base_ptr = proxy
    ret.thisptr = <unity_sarray_builder*>(ret._base_ptr.get())
    return ret

cdef class UnitySArrayBuilderProxy:

    def __cinit__(self, do_not_allocate=None):
        if do_not_allocate:
            self._base_ptr.reset()
            self.thisptr = NULL
        else:
            self.thisptr = new unity_sarray_builder()
            self._base_ptr.reset(<unity_sarray_builder_base*>(self.thisptr))

    cpdef init(self, size_t num_segments, size_t history_size, type dtype):
        cdef flex_type_enum tmp_type
        tmp_type = flex_type_enum_from_pytype(dtype)
        with nogil:
            self.thisptr.init(num_segments, history_size, tmp_type)

    cpdef append(self, val, size_t segment):
        cdef flexible_type c_val = flexible_type_from_pyobject(val)
        with nogil:
            self.thisptr.append(c_val, segment)

    cpdef append_multiple(self, object vals, size_t segment):
        cdef flex_list c_vals = flex_list_from_iterable(vals)
        with nogil:
            self.thisptr.append_multiple(c_vals, segment)

    cpdef read_history(self, size_t num_elems, size_t segment):
        cdef flex_list tmp_history
        with nogil:
            tmp_history = self.thisptr.read_history(num_elems, segment)
        return pylist_from_flex_list(tmp_history)

    cpdef get_type(self):
        cdef flex_type_enum ret_type
        with nogil:
            ret_type = self.thisptr.get_type()
        return pytype_from_flex_type_enum(ret_type)

    cpdef close(self):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = self.thisptr.close()
        return sarray_proxy(proxy)

# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from cython.operator cimport dereference as deref
from cython.operator cimport preincrement as inc
from .cy_flexible_type cimport flex_list
from .cy_flexible_type cimport flexible_type 
from .cy_flexible_type cimport flexible_type_from_pyobject
from .cy_flexible_type cimport pyobject_from_flexible_type 
from .cy_flexible_type cimport flex_list_from_iterable


cdef create_proxy_wrapper_from_existing_proxy(const unity_sketch_base_ptr& proxy):
    if proxy.get() == NULL:
        return None
    ret = UnitySketchProxy(True)
    ret._base_ptr = proxy
    ret.thisptr = <unity_sketch*>ret._base_ptr.get()
    return ret

cdef class UnitySketchProxy:

    def __cinit__(self, do_not_allocate=None):
        if do_not_allocate:
            self._base_ptr.reset()
            self.thisptr = NULL
            pass
        else:
            self.thisptr = new unity_sketch()
            self._base_ptr.reset(<unity_sketch_base*>(self.thisptr))

    cpdef construct_from_sarray(self, UnitySArrayProxy sarray, bint background, object elements):
        cdef flex_list keys = flex_list_from_iterable(elements)
        self.thisptr.construct_from_sarray(sarray._base_ptr, background, keys)

    cpdef get_quantile(self, double quantile):
      return self.thisptr.get_quantile(quantile)

    cpdef frequency_count(self, object element):
      return self.thisptr.frequency_count(flexible_type_from_pyobject(element))

    cpdef frequent_items(self):
      m = self.thisptr.frequent_items()
      ret = {}
      cdef vector[pair[flexible_type,size_t]].iterator it = m.begin()
      cdef pair[flexible_type, size_t] entry
      while (it != m.end()):
        entry = deref(it)
        ret[pyobject_from_flexible_type(entry.first)] = entry.second
        inc(it)
      return ret

    cpdef num_unique(self):
        return self.thisptr.num_unique()

    cpdef mean(self):
        return self.thisptr.mean()

    cpdef var(self):
        return self.thisptr.var()

    cpdef max(self):
        return self.thisptr.max()

    cpdef min(self):
        return self.thisptr.min()

    cpdef size(self):
        return self.thisptr.size()

    cpdef sum(self):
        return self.thisptr.sum()

    cpdef num_undefined(self):
        return self.thisptr.num_undefined()

    cpdef sketch_ready(self):
        return self.thisptr.sketch_ready()

    cpdef num_elements_processed(self):
        return self.thisptr.num_elements_processed()

    cpdef element_length_summary(self):
        cdef unity_sketch_base_ptr proxy = self.thisptr.element_length_summary()
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef element_summary(self):
        cdef unity_sketch_base_ptr proxy = self.thisptr.element_summary()
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef dict_key_summary(self):
        cdef unity_sketch_base_ptr proxy = self.thisptr.dict_key_summary()
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef dict_value_summary(self):
        cdef unity_sketch_base_ptr proxy = self.thisptr.dict_value_summary()
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef element_sub_sketch(self, object keys):
        sketches = self.thisptr.element_sub_sketch(flex_list_from_iterable(keys))
        ret = {}
        #cdef map[flexible_type,unity_sketch_proxy*].iterator it = sketches.begin()
        it = sketches.begin()
        cdef pair[flexible_type, unity_sketch_base_ptr] entry
        while (it != sketches.end()):
            entry = deref(it)
            ret[pyobject_from_flexible_type(entry.first)] = create_proxy_wrapper_from_existing_proxy(entry.second)
            inc(it)
        return ret

    cpdef cancel(self):
        self.thisptr.cancel()


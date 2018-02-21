### cython utils ###
from cython.operator cimport dereference as deref
### flexible_type utils ###
from .cy_flexible_type cimport flex_type_enum_from_pytype
from .cy_flexible_type cimport pytype_from_flex_type_enum
from .cy_flexible_type cimport flexible_type_from_pyobject
from .cy_flexible_type cimport flexible_type_from_pyobject_hint
from .cy_flexible_type cimport flex_list_from_iterable
from .cy_flexible_type cimport flex_list_from_typed_iterable
from .cy_flexible_type cimport common_typed_flex_list_from_iterable
from .cy_flexible_type cimport gl_options_map_from_pydict
from .cy_flexible_type cimport pyobject_from_flexible_type
from .cy_flexible_type cimport pylist_from_flex_list
from .cy_flexible_type cimport pydict_from_gl_options_map

from .cy_cpp_utils cimport str_to_cpp, cpp_to_str

import inspect
import array

cdef create_proxy_wrapper_from_existing_proxy_plot(const plot_base_ptr& proxy):
    if proxy.get() == NULL:
        return None
    ret = PlotProxy(True)
    ret._base_ptr = proxy
    ret.thisptr = <Plot*>(ret._base_ptr.get())
    return ret

cdef class PlotProxy:
    def __cinit__(self, do_not_allocate=None):
        if do_not_allocate:
            self._base_ptr.reset()
            self.thisptr = NULL
        else:
            self.thisptr = new Plot()
            self._base_ptr.reset(<plot_base*>(self.thisptr))

    cpdef show(self):
        with nogil:
            self.thisptr.show()

    cpdef materialize(self):
        with nogil:
            self.thisptr.materialize()

    cpdef get_spec(self):
        cdef string spec;
        with nogil:
            spec = self.thisptr.get_spec()
        return spec

    cpdef get_data(self):
        cdef string spec;
        with nogil:
            spec = self.thisptr.get_data()
        return spec

from .cy_flexible_type cimport flex_type_enum
from .cy_flexible_type cimport flexible_type
from .cy_flexible_type cimport flex_list
from .cy_flexible_type cimport gl_options_map
from libcpp.vector cimport vector
from libcpp.string cimport string
from .cy_unity_base_types cimport *
from .cy_unity cimport function_closure_info
from .cy_unity cimport make_function_closure_info

cdef extern from "<unity/lib/visualization/plot.hpp>" namespace "turi::visualization":
    cdef cppclass Plot nogil:
        Plot() except +
        void show() except +
        void materialize() except +
        string get_spec() except +
        string get_data() except +

cdef create_proxy_wrapper_from_existing_proxy_plot(const plot_base_ptr& proxy)

cdef class PlotProxy:
    cdef plot_base_ptr _base_ptr
    cdef Plot* thisptr
    cdef _cli

    cpdef show(self)
    cpdef materialize(self)
    cpdef get_spec(self)
    cpdef get_data(self)

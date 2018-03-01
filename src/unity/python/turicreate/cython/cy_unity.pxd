# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_flexible_type cimport flexible_type
from .cy_flexible_type cimport flex_list
from .cy_flexible_type cimport gl_options_map
from .cy_flexible_type cimport flex_image

from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp.map cimport map
from libcpp.pair cimport pair

from .cy_variant cimport variant_type
from .cy_variant cimport variant_map_type
from .cy_unity_base_types cimport *

cdef extern from "<unity/lib/unity_global.hpp>" namespace "turi":
    cdef struct toolkit_function_response_type:
        bint success
        string message
        variant_map_type params

    cdef cppclass unity_global nogil:

        string get_version() except +

        string get_graph_dag() except +

        unity_sgraph_base_ptr load_graph(string) except +

        vector[string] list_toolkit_functions() except +

        vector[string] list_toolkit_classes() except +

        gl_options_map describe_toolkit_function(string) except +

        gl_options_map describe_toolkit_class(string) except +

        model_base_ptr create_toolkit_class(string) except +

        variant_map_type load_model(string) except +

        void save_model(model_base_ptr, variant_map_type, string) except +

        void save_model2(string, variant_map_type, string) except +

        toolkit_function_response_type run_toolkit(string toolkit_name, variant_map_type arguments) except +

        # Internal testing APIs
        flexible_type eval_lambda(string, flexible_type) except +

        flexible_type eval_dict_lambda(string, vector[string], vector[flexible_type]) except +

        vector[flexible_type] parallel_eval_lambda(string, vector[flexible_type]) except +

        string __read__(const string&) except +

        void __write__(const string&, const string&) except +

        bint __mkdir__(const string&) except +

        bint __chmod__(const string&, short) except +

        size_t __get_heap_size__() except +

        size_t __get_allocated_size__() except +

        void set_log_level(size_t) except +

        gl_options_map list_globals(bint) except +

        string set_global(string, flexible_type) except +

        unity_sarray_base_ptr create_sequential_sarray(ssize_t, ssize_t, bint) except +

        string load_toolkit(string soname, string module_subpath) except +

        vector[string] list_toolkit_functions_in_dynamic_module(string soname) except +

        vector[string] list_toolkit_classes_in_dynamic_module(string soname) except +

        string get_current_cache_file_location() except +

        string get_turicreate_object_type(const string& url) except +

cdef extern from "<unity/lib/unity_global_singleton.hpp>" namespace "turi":
        shared_ptr[unity_global] get_unity_global_singleton()

ctypedef shared_ptr[unity_global] unity_global_ptr

cdef class UnityGlobalProxy:
    cdef unity_global* thisptr
    cdef unity_global_ptr _base_ptr

    cpdef get_version(self)

    cpdef get_graph_dag(self)

    cpdef load_graph(self, fname)

    cpdef list_toolkit_functions(self)

    cpdef list_toolkit_classes(self)

    cpdef describe_toolkit_function(self, fname)

    cpdef describe_toolkit_class(self, model)

    cpdef create_toolkit_class(self, model)

    cpdef run_toolkit(self, toolkit_name, object arguments)

    cpdef save_model(self, model, url, object sidedata=*)

    cpdef save_model2(self, modelname, url, object sidedata=*)

    cpdef load_model(self, url)

    cpdef eval_lambda(self, object fn, object argument)

    cpdef eval_dict_lambda(self, object fn, object argument)

    cpdef parallel_eval_lambda(self, object fn, object argument_list)

    cpdef __read__(self, object url)

    cpdef __write__(self, object url, object content)

    cpdef __mkdir__(self, object url)

    cpdef __chmod__(self, object url, short mode)

    cpdef __get_heap_size__(self)

    cpdef __get_allocated_size__(self)

    cpdef set_log_level(self, size_t level)

    cpdef list_globals(self, bint runtime_modifiable)

    cpdef set_global(self, key, object value)

    cpdef create_sequential_sarray(self, ssize_t size, ssize_t start, bint reverse)

    cpdef load_toolkit(self, soname, module_subpath)

    cpdef list_toolkit_functions_in_dynamic_module(self, soname)

    cpdef list_toolkit_classes_in_dynamic_module(self, soname)

    cpdef get_current_cache_file_location(self)

    cpdef get_turicreate_object_type(self, url)

cdef extern from "<unity/lib/api/function_closure_info.hpp>" namespace "turi":
    cdef struct function_closure_info:
        string native_fn_name
        vector[pair[ssize_t, shared_ptr[variant_type]]]  arguments

cdef bint is_function_closure_info(object) except *

cdef function_closure_info make_function_closure_info(object) except *

cdef extern from "<unity/lib/api/client_base_types.hpp>" namespace "turi":
    void variant_set_closure "turi::variant_set_value<turi::function_closure_info>" (variant_type& v, const function_closure_info& f)

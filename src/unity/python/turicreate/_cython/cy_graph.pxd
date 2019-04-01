# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_flexible_type cimport flex_list
from .cy_flexible_type cimport gl_options_map
from .cy_flexible_type cimport flex_type_enum 

from .cy_unity_base_types cimport *
from .cy_sarray cimport  UnitySArrayProxy
from .cy_sframe cimport  UnitySFrameProxy

from libcpp.vector cimport vector
from libcpp.string cimport string
from .cy_unity cimport function_closure_info
from .cy_unity cimport make_function_closure_info

cdef extern from "<unity/lib/unity_sgraph.hpp>" namespace "turi":
    cdef cppclass unity_sgraph nogil:
        unity_sgraph() except +
        vector[string] get_vertex_fields(size_t) except +
        vector[string] get_edge_fields(size_t, size_t) except +
        vector[flex_type_enum] get_vertex_field_types(size_t) except +
        vector[flex_type_enum] get_edge_field_types(size_t, size_t) except +

        gl_options_map summary() except +

        unity_sframe_base_ptr get_vertices(flex_list, gl_options_map, size_t) except +
        unity_sframe_base_ptr get_edges(flex_list, flex_list, gl_options_map, size_t, size_t) except +

        unity_sgraph_base_ptr add_vertices(unity_sframe_base_ptr, string, size_t) except +
        unity_sgraph_base_ptr add_edges(unity_sframe_base_ptr, string, string, size_t, size_t) except +

        unity_sgraph_base_ptr copy_vertex_field (string, string, size_t) except +
        unity_sgraph_base_ptr copy_edge_field (string, string, size_t, size_t) except +

        unity_sgraph_base_ptr add_vertex_field(unity_sarray_base_ptr, string) except +
        unity_sgraph_base_ptr add_edge_field(unity_sarray_base_ptr, string) except +

        unity_sgraph_base_ptr delete_vertex_field (string, size_t) except +
        unity_sgraph_base_ptr delete_edge_field (string, size_t, size_t) except +

        unity_sgraph_base_ptr select_vertex_fields(vector[string], size_t) except +
        unity_sgraph_base_ptr select_edge_fields(vector[string], size_t, size_t) except +

        unity_sgraph_base_ptr swap_edge_fields(string, string) except +
        unity_sgraph_base_ptr swap_vertex_fields(string, string) except +

        unity_sgraph_base_ptr rename_edge_fields(vector[string], vector[string]) except +
        unity_sgraph_base_ptr rename_vertex_fields(vector[string], vector[string]) except +

        unity_sgraph_base_ptr lambda_triple_apply(string, vector[string]) except +
        unity_sgraph_base_ptr lambda_triple_apply_native(const function_closure_info&, vector[string]) except +

        bint save_graph (string, string) except +
        bint load_graph (string) except +


cdef create_proxy_wrapper_from_existing_proxy(const unity_sgraph_base_ptr& proxy)

cdef class UnityGraphProxy:
    cdef unity_sgraph_base_ptr _base_ptr
    cdef unity_sgraph* thisptr

    cpdef get_vertices(self, object ids, object field_constraints, size_t group=*)

    cpdef get_edges(self, object src_ids, object dst_ids, object field_constraints, size_t groupa=*, size_t groupb=*) 

    cpdef add_vertices(self, UnitySFrameProxy sframe, id_field, size_t group=*)

    cpdef add_edges(self, UnitySFrameProxy sframe, src_id_field, dst_id_field, size_t groupa=*, size_t groupb=*)

    cpdef summary(self)


    cpdef get_vertex_fields(self, size_t group=*)

    cpdef get_vertex_field_types(self, size_t group=*)

    cpdef select_vertex_fields(self, field, size_t group=*)
    
    cpdef copy_vertex_field(self, src_field, dst_field, size_t group=*)

    cpdef delete_vertex_field(self, field, size_t group=*)

    cpdef add_vertex_field(self, UnitySArrayProxy data, name)

    cpdef swap_vertex_fields(self, field1, field2)

    cpdef rename_vertex_fields(self, oldnames, newnames)


    cpdef get_edge_fields(self, size_t groupa=*, size_t groupb=*)

    cpdef get_edge_field_types(self, size_t groupa=*, size_t groupb=*)

    cpdef select_edge_fields(self, fields, size_t groupa=*, size_t groupb=*)

    cpdef copy_edge_field(self, src_field, dst_field, size_t groupa=*, size_t groupb=*)

    cpdef delete_edge_field(self, field, size_t groupa=*, size_t groupb=*)

    cpdef add_edge_field(self, UnitySArrayProxy data, name)

    cpdef swap_edge_fields(self, field1, field2)

    cpdef rename_edge_fields(self,  oldnames,  newnames)


    cpdef lambda_triple_apply(self, object, object)

    cpdef lambda_triple_apply_native(self, object, object)

    cpdef save_graph(self, filename, format)

    cpdef load_graph(self, filename)

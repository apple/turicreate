# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_flexible_type cimport flexible_type
from .cy_flexible_type cimport flex_type_enum
from .cy_flexible_type cimport flex_list
from .cy_flexible_type cimport gl_options_map
from .cy_dataframe cimport gl_dataframe

from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp.map cimport map
from libcpp.list cimport list as cpplist

from .cy_unity_base_types cimport *
from .cy_sarray cimport UnitySArrayProxy
from .cy_unity cimport function_closure_info
from .cy_unity cimport make_function_closure_info

ctypedef map[string, unity_sarray_base_ptr] gl_error_map

cdef extern from "<unity/lib/unity_sframe.hpp>" namespace "turi":
   cdef cppclass unity_sframe nogil:
        unity_sframe() except +
        void construct_from_dataframe(const gl_dataframe&) except +
        void construct_from_sframe_index(string) except +
        gl_error_map construct_from_csvs(string, gl_options_map, map[string, flex_type_enum]) except +
        void save_frame(string) except +
        void save_frame_reference(string) except +
        void clear() except +
        size_t size() except +
        int num_columns() except +
        vector[flex_type_enum] dtype() except +
        vector[string] column_names() except +
        unity_sframe_base_ptr head(size_t) except +
        unity_sframe_base_ptr tail(size_t) except +
        unity_sarray_base_ptr transform(const string&, flex_type_enum, bint, int) except +
        unity_sarray_base_ptr transform_native(const function_closure_info&, flex_type_enum, bint, int) except +
        unity_sframe_base_ptr flat_map(const string&, vector[string], vector[flex_type_enum], bint, int) except +
        unity_sframe_base_ptr logical_filter(unity_sarray_base_ptr) except +
        unity_sframe_base_ptr select_columns(const vector[string]&) except +
        unity_sarray_base_ptr select_column(const string&) except +
        void add_column(unity_sarray_base_ptr, const string&) except +
        void add_columns(cpplist[unity_sarray_base_ptr], vector[string]) except +
        void set_column_name(size_t, string) except +
        void remove_column(size_t) except +
        void swap_columns(size_t, size_t) except +
        void begin_iterator() except +
        vector[vector[flexible_type]] iterator_get_next(size_t) except +
        void save_as_csv(const string&, gl_options_map) except +
        unity_sframe_base_ptr sample(float, int, bint) except +
        cpplist[unity_sframe_base_ptr] random_split(float, int, bint) except +
        unity_sframe_base_ptr groupby_aggregate(const vector[string]&, const vector[vector[string]]&, const vector[string]&, const vector[string]&) except +
        unity_sframe_base_ptr append(unity_sframe_base_ptr) except +
        void materialize() except +
        bint is_materialized() except +
        bint has_size() except +
        string query_plan_string() except +
        unity_sframe_base_ptr join(unity_sframe_base_ptr, const string, map[string, string]) except +
        unity_sarray_base_ptr pack_columns(const vector[string]&, const vector[string]&, flex_type_enum , const flexible_type&) except +
        unity_sframe_base_ptr stack (const string& , const vector[string]& , const vector[flex_type_enum]&, bint) except +
        unity_sframe_base_ptr sort(const vector[string]&, const vector[int]&) except +
        unity_sframe_base_ptr copy_range(size_t, size_t, size_t) except +
        cpplist[unity_sframe_base_ptr] drop_missing_values(const vector[string]&, bint, bint) except +
        void delete_on_close() except +
        void explore(const string&, const string&) except +
        void show(const string&) except +
        model_base_ptr plot() except +

cdef create_proxy_wrapper_from_existing_proxy(const unity_sframe_base_ptr& proxy)

cdef class UnitySFrameProxy:
    cdef unity_sframe* thisptr
    cdef unity_sframe_base_ptr _base_ptr

    cpdef load_from_dataframe(self, dataframe)

    cpdef load_from_sframe_index(self, index_file)

    cpdef load_from_csvs(self, url, object csv_config, dict column_type_hints)

    cpdef save(self, index_file)

    cpdef save_reference(self, index_file)

    cpdef num_rows(self)

    cpdef num_columns(self)

    cpdef dtype(self)

    cpdef column_names(self)

    cpdef head(self, size_t n)

    cpdef tail(self, size_t n)

    cpdef transform(self, fn, t, int seed)

    cpdef transform_native(self, fn, t, int seed)

    cpdef flat_map(self, object fn, column_names, object column_types, int seed)

    cpdef logical_filter(self, UnitySArrayProxy other)

    cpdef select_columns(self, keylist)

    cpdef select_column(self, key)

    cpdef add_column(self, UnitySArrayProxy data, name)

    cpdef add_columns(self, datalist, namelist)

    cpdef set_column_name(self, size_t i, name)

    cpdef remove_column(self, size_t i)

    cpdef swap_columns(self, size_t i, size_t j)

    cpdef begin_iterator(self)

    cpdef iterator_get_next(self, size_t length)

    cpdef save_as_csv(self, url, object csv_config)

    cpdef sample(self, float percent, int random_seed, bint exact=*)

    cpdef random_split(self, float percent, int random_seed, bint exact=*)

    cpdef groupby_aggregate(self, key_columns, group_columns, group_output_columns, column_ops)

    cpdef append(self, UnitySFrameProxy other)

    cpdef materialize(self)

    cpdef is_materialized(self)

    cpdef has_size(self)

    cpdef query_plan_string(self)

    cpdef join(self, UnitySFrameProxy right, how, dict on)

    cpdef pack_columns(self, columns, keys, dtype, fill_na)

    cpdef stack(self, column_name, new_column_names, new_column_types, drop_na)

    cpdef sort(self, column_names, vector[int] sort_orders)

    cpdef copy_range(self, size_t start, size_t step, size_t end)

    cpdef drop_missing_values(self, columns, bint is_all, bint split)

    cpdef __get_object_id(self)

    cpdef delete_on_close(self)

    cpdef explore(self, path_to_client, title)

    cpdef show(self, path_to_client)

    cpdef plot(self)

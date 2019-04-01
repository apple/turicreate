# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from .cy_flexible_type cimport flex_type_enum
from .cy_flexible_type cimport flexible_type
from .cy_flexible_type cimport flex_list
from .cy_flexible_type cimport gl_options_map
from libcpp.vector cimport vector
from libcpp.string cimport string
from .cy_unity_base_types cimport *
from .cy_unity cimport function_closure_info
from .cy_unity cimport make_function_closure_info

cdef extern from "<unity/lib/unity_sarray.hpp>" namespace "turi":
    cdef cppclass unity_sarray nogil:
        unity_sarray() except +
        void construct_from_vector(const flex_list&, flex_type_enum) except +
        void construct_from_const(const flexible_type&, size_t, flex_type_enum) except +
        void construct_from_files(string, flex_type_enum) except +
        void construct_from_json_record_files(string) except +
        void construct_from_sarray_index(string) except +
        void construct_from_autodetect(string, flex_type_enum) except +
        void clear() except +
        void save_array(string) except +
        size_t size() except +
        bint has_size() except +
        unity_sarray_base_ptr head(size_t) except +
        flex_type_enum dtype() except +
        unity_sarray_base_ptr vector_slice(size_t, size_t) except +
        unity_sarray_base_ptr transform(const string&, flex_type_enum, bint, int) except +
        unity_sarray_base_ptr transform_native(const function_closure_info&, flex_type_enum, bint, int) except +
        unity_sarray_base_ptr filter(const string&, bint, int) except +
        unity_sarray_base_ptr logical_filter(unity_sarray_base_ptr) except +
        unity_sarray_base_ptr topk_index(size_t, bint) except +
        bint all() except +
        bint any() except +
        flexible_type max() except +
        flexible_type min() except +
        flexible_type sum() except +
        flexible_type mean() except +
        flexible_type std(size_t) except +
        flexible_type var(size_t) except +
        size_t nnz() except +
        size_t num_missing() except +
        unity_sarray_base_ptr unary_negative() except +
        unity_sarray_base_ptr _pow(double v) except +
        unity_sarray_base_ptr astype(flex_type_enum, bint) except +
        unity_sarray_base_ptr str_to_datetime(string) except +
        unity_sarray_base_ptr datetime_to_str(string) except +
        unity_sarray_base_ptr clip(flexible_type, flexible_type) except +
        unity_sarray_base_ptr nonzero() except +
        unity_sarray_base_ptr tail(size_t) except +
        void begin_iterator() except +
        flex_list iterator_get_next(size_t) except +
        unity_sarray_base_ptr left_scalar_operator(flexible_type, string) except +
        unity_sarray_base_ptr right_scalar_operator(flexible_type, string) except +
        unity_sarray_base_ptr vector_operator(unity_sarray_base_ptr, string) except +
        unity_sarray_base_ptr drop_missing_values() except +
        unity_sarray_base_ptr fill_missing_values(flexible_type) except +
        unity_sarray_base_ptr sample(float, int, bint) except +
        unity_sarray_base_ptr hash(int) except +
        void materialize() except +
        bint is_materialized() except +
        unity_sarray_base_ptr append(unity_sarray_base_ptr) except +
        unity_sarray_base_ptr count_bag_of_words(gl_options_map) except +
        unity_sarray_base_ptr count_character_ngrams(size_t, gl_options_map) except +
        unity_sarray_base_ptr count_ngrams(size_t, gl_options_map) except +
        unity_sarray_base_ptr dict_trim_by_keys(vector[flexible_type], bint) except +
        unity_sarray_base_ptr dict_trim_by_values(flexible_type, flexible_type) except +
        unity_sarray_base_ptr dict_keys() except +
        unity_sarray_base_ptr dict_values() except +
        unity_sarray_base_ptr dict_has_any_keys(vector[flexible_type]) except +
        unity_sarray_base_ptr dict_has_all_keys(vector[flexible_type]) except +
        unity_sarray_base_ptr item_length() except +
        unity_sframe_base_ptr expand(string column_name_prefix, vector[flexible_type] limit, vector[flex_type_enum] value_types) except +
        unity_sframe_base_ptr unpack_dict(string column_name_prefix, vector[flexible_type] limit, flexible_type) except +
        unity_sframe_base_ptr unpack(string column_name_prefix, vector[flexible_type] limit, vector[flex_type_enum] value_types, flexible_type) except +
        size_t get_content_identifier() except +
        unity_sarray_base_ptr copy_range(size_t, size_t, size_t) except +
        unity_sarray_base_ptr builtin_rolling_apply(string, ssize_t, ssize_t, size_t) except +
        unity_sarray_base_ptr builtin_cumulative_aggregate(string) except +
        unity_sarray_base_ptr subslice(flexible_type, flexible_type, flexible_type) except +
        unity_sarray_base_ptr ternary_operator(unity_sarray_base_ptr, unity_sarray_base_ptr) except +
        unity_sarray_base_ptr to_const(const flexible_type&, flex_type_enum) except +
        void show(const string&, const string&, const string&, const string&) except +
        model_base_ptr plot(const string&, const string&, const string&) except +

cdef create_proxy_wrapper_from_existing_proxy(const unity_sarray_base_ptr& proxy)

cdef class UnitySArrayProxy:
    cdef unity_sarray_base_ptr _base_ptr
    cdef unity_sarray* thisptr
    cdef _cli

    cpdef load_from_iterable(self, object d, type t, bint ignore_cast_failure=*)

    cpdef load_from_url(self, url, type t)

    cpdef load_from_json_record_files(self, url)

    cpdef load_from_sarray_index(self, index_file)

    cpdef load_from_const(self, object value, size_t size, type t)

    cpdef load_autodetect(self, url, type t)

    cpdef save(self, index_file)

    cpdef size(self)

    cpdef has_size(self)

    cpdef head(self, size_t length)

    cpdef dtype(self)

    cpdef vector_slice(self, size_t start, size_t end)

    cpdef transform(self, fn, t, bint skip_undefined, int seed)

    cpdef transform_native(self, fn, t, bint skip_undefined, int seed)

    cpdef filter(self, fn, bint skip_undefined, int seed)

    cpdef logical_filter(self, UnitySArrayProxy other)

    cpdef topk_index(self, size_t k, bint reverse)

    cpdef num_missing(self)

    cpdef all(self)

    cpdef any(self)

    cpdef max(self)

    cpdef min(self)

    cpdef sum(self)

    cpdef mean(self)

    cpdef std(self, size_t ddof)

    cpdef var(self, size_t ddof)

    cpdef nnz(self)

    cpdef str_to_datetime(self, str_format)

    cpdef datetime_to_str(self, str_format)

    cpdef astype(self, type dtype, bint undefined_on_failure)

    cpdef clip(self, lower, upper)

    cpdef tail(self, size_t length)

    cpdef begin_iterator(self)

    cpdef iterator_get_next(self, size_t length)

    cpdef left_scalar_operator(self, object other, op)

    cpdef right_scalar_operator(self, object other, op)

    cpdef vector_operator(self, UnitySArrayProxy other, op)

    cpdef drop_missing_values(self)

    cpdef fill_missing_values(self, default_value)

    cpdef sample(self, float percent, int seed, bint exact=*)

    cpdef hash(self, int seed)

    cpdef materialize(self)

    cpdef is_materialized(self)

    cpdef append(self, UnitySArrayProxy other)

    cpdef count_bag_of_words(self, object op)

    cpdef count_character_ngrams(self, size_t n, object op)

    cpdef count_ngrams(self, size_t n, object op)

    cpdef dict_trim_by_keys(self, object keys, bint exclude)

    cpdef dict_trim_by_values(self, lower, upper)

    cpdef dict_keys(self)

    cpdef dict_values(self)

    cpdef dict_has_any_keys(self, object)

    cpdef dict_has_all_keys(self, object)

    cpdef item_length(self)

    cpdef unpack_dict(self, column_name_prefix, object, na_value)

    cpdef expand(self, column_name_prefix, object, value_types)

    cpdef unpack(self, column_name_prefix, object, value_types, na_value)

    cpdef __get_object_id(self)

    cpdef get_content_identifier(self)

    cpdef copy_range(self, size_t start, size_t step, size_t end)

    cpdef builtin_rolling_apply(self, fn_name, ssize_t before, ssize_t after, size_t min_observations)

    cpdef builtin_cumulative_aggregate(self, fn_name)

    cpdef subslice(self, start, step, stop)

    cpdef ternary_operator(self, UnitySArrayProxy istrue, UnitySArrayProxy isfalse)

    cpdef to_const(self, object value, type t)

    cpdef show(self, path_to_client, title, xlabel, ylabel)

    cpdef plot(self, title, xlabel, ylabel)

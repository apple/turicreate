# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from libcpp.pair cimport pair

from cython.operator cimport dereference as deref
from cython.operator cimport preincrement as inc

#### flexible_type utils ####
from .cy_flexible_type cimport flex_type_enum_from_pytype
from .cy_flexible_type cimport pytype_from_flex_type_enum
from .cy_flexible_type cimport flexible_type_from_pyobject
from .cy_flexible_type cimport flex_list_from_iterable
from .cy_flexible_type cimport gl_options_map_from_pydict
from .cy_flexible_type cimport pyobject_from_flexible_type
from .cy_flexible_type cimport pylist_from_flex_list
from .cy_flexible_type cimport pydict_from_gl_options_map

#### dataframe utils ####
from .cy_dataframe cimport gl_dataframe_from_dict_of_arrays
from .cy_dataframe cimport pd_from_gl_dataframe
from .cy_dataframe cimport is_pandas_dataframe

#### sarray ####
from .cy_sarray cimport create_proxy_wrapper_from_existing_proxy as sarray_proxy
from .cy_sarray cimport UnitySArrayProxy

from .cy_model cimport create_model_from_proxy

from .cy_cpp_utils cimport str_to_cpp, cpp_to_str
from .cy_cpp_utils cimport to_vector_of_strings, from_vector_of_strings
from .cy_cpp_utils cimport to_nested_vectors_of_strings, dict_to_string_string_map

cdef create_proxy_wrapper_from_existing_proxy(const unity_sframe_base_ptr& proxy):
    if proxy.get() == NULL:
        return None
    ret = UnitySFrameProxy(True)
    ret._base_ptr = proxy
    ret.thisptr = <unity_sframe*>(ret._base_ptr.get())
    return ret

cdef pydict_from_gl_error_map(gl_error_map& d):
    """
    Converting map[string, sarray] to dict
    """
    cdef unity_sarray_base_ptr proxy
    cdef dict ret = {}
    cdef map[string, unity_sarray_base_ptr].iterator it = d.begin()
    while (it != d.end()):
        ret[cpp_to_str(deref(it).first)] = sarray_proxy(deref(it).second)
        inc(it)
    return ret

cdef class UnitySFrameProxy:

    def __cinit__(self, do_not_allocate=None):
        if do_not_allocate:
            self._base_ptr.reset()
            self.thisptr = NULL
        else:
            self.thisptr = new unity_sframe()
            self._base_ptr.reset(<unity_sframe_base*>(self.thisptr))

    cpdef load_from_dataframe(self, dataframe):
        cdef gl_dataframe gldf = gl_dataframe_from_dict_of_arrays(dataframe)
        with nogil:
            self.thisptr.construct_from_dataframe(gldf)

    cpdef load_from_sframe_index(self, index_file):
        cdef string str_index_file = str_to_cpp(index_file)
        with nogil:
            self.thisptr.construct_from_sframe_index(str_index_file)

    cpdef load_from_csvs(self, _url, object csv_config, dict column_type_hints):
        cdef map[string, flex_type_enum] c_column_type_hints
        for key, value in column_type_hints.items():
            c_column_type_hints[str_to_cpp(key)] = flex_type_enum_from_pytype(value)
        cdef gl_options_map csv_options = gl_options_map_from_pydict(csv_config)
        cdef gl_error_map errors
        cdef string url = str_to_cpp(_url)
        with nogil:
            errors = self.thisptr.construct_from_csvs(url, csv_options, c_column_type_hints)
        return pydict_from_gl_error_map(errors)

    cpdef save(self, _index_file):
        cdef string index_file = str_to_cpp(_index_file)
        with nogil:
            self.thisptr.save_frame(index_file)

    cpdef save_reference(self, _index_file):
        cdef string index_file = str_to_cpp(_index_file)
        with nogil:
            self.thisptr.save_frame_reference(index_file)

    cpdef num_rows(self):
        return self.thisptr.size()

    cpdef num_columns(self):
        return self.thisptr.num_columns()

    cpdef dtype(self):
        return [pytype_from_flex_type_enum(t) for t in self.thisptr.dtype()]

    cpdef column_names(self):
        return from_vector_of_strings(self.thisptr.column_names())

    cpdef head(self, size_t n):
        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.head(n))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef tail(self, size_t n):
        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.tail(n))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef transform(self, fn, t, int seed):
        cdef flex_type_enum flex_type_en = flex_type_enum_from_pytype(t)
        cdef string lambda_str
        if type(fn) == str:
            lambda_str = fn
        else:
            from .. import util
            lambda_str = str_to_cpp(util._pickle_to_temp_location_or_memory(fn))
        # skip_undefined options is not used for now.
        skip_undefined = 0
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.transform(lambda_str, flex_type_en, skip_undefined, seed))
        return sarray_proxy(proxy)

    cpdef show(self, _path_to_client):
        cdef string path_to_client = str_to_cpp(_path_to_client)
        with nogil:
            self.thisptr.show(path_to_client)

    cpdef plot(self):
        cdef model_base_ptr proxy
        with nogil:
            proxy = self.thisptr.plot()
        return create_model_from_proxy(proxy)

    cpdef explore(self, _path_to_client, _title):
        cdef string path_to_client = str_to_cpp(_path_to_client)
        cdef string title = str_to_cpp(_title)
        with nogil:
            self.thisptr.explore(path_to_client, title)

    cpdef transform_native(self, closure, t, int seed):
        cdef flex_type_enum flex_type_en = flex_type_enum_from_pytype(t)
        cl = make_function_closure_info(closure)
        # skip_undefined options is not used for now.
        skip_undefined = 0
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.transform_native(cl, flex_type_en, skip_undefined, seed))
        return sarray_proxy(proxy)

    cpdef flat_map(self, object fn, _column_names, object py_column_types, int seed):
        cdef vector[string] column_names = to_vector_of_strings(_column_names)
        cdef vector[flex_type_enum] column_types
        cdef string lambda_str
        for t in py_column_types:
            column_types.push_back(flex_type_enum_from_pytype(t))
        if type(fn) == str:
            lambda_str = fn
        else:
            from .. import util
            lambda_str = util._pickle_to_temp_location_or_memory(fn)
        cdef unity_sframe_base_ptr proxy
        skip_undefined = 0
        with nogil:
            proxy = (self.thisptr.flat_map(lambda_str,
              column_names, column_types, skip_undefined, seed))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef logical_filter(self, UnitySArrayProxy other):
        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.logical_filter(other._base_ptr))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef select_columns(self, _keylist):
        cdef vector[string] keylist = to_vector_of_strings(_keylist)
        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.select_columns(keylist))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef select_column(self, _key):
        cdef string key = str_to_cpp(_key)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.select_column(key))
        return sarray_proxy(proxy)

    cpdef add_column(self, UnitySArrayProxy data, _name):
        cdef string name = str_to_cpp(_name)
        with nogil:
            self.thisptr.add_column(data._base_ptr, name)

    cpdef add_columns(self, object datalist, _namelist):
        cdef vector[string] namelist = to_vector_of_strings(_namelist)
        cdef cpplist[unity_sarray_base_ptr] proxies
        cdef UnitySArrayProxy proxy
        for i in datalist:
          proxy = i
          proxies.push_back(proxy._base_ptr)

        with nogil:
            self.thisptr.add_columns(proxies, namelist)

    cpdef remove_column(self, size_t i):
        with nogil:
            self.thisptr.remove_column(i)

    cpdef swap_columns(self, size_t i, size_t j):
        with nogil:
            self.thisptr.swap_columns(i, j)

    cpdef set_column_name(self, size_t i, _name):
        cdef string name = str_to_cpp(_name)
        with nogil:
            self.thisptr.set_column_name(i, name)

    cpdef begin_iterator(self):
        self.thisptr.begin_iterator()

    cpdef iterator_get_next(self, size_t length):
        tmp = self.thisptr.iterator_get_next(length)
        return [pylist_from_flex_list(x) for x in tmp]

    cpdef save_as_csv(self, _url, object csv_config):
        cdef string url = str_to_cpp(_url)
        cdef gl_options_map csv_options = gl_options_map_from_pydict(csv_config)
        with nogil:
            self.thisptr.save_as_csv(url, csv_options)

    cpdef sample(self, float percent, int random_seed, bint exact=False):
        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = self.thisptr.sample(percent, random_seed, exact)
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef random_split(self, float percent, int random_seed, bint exact=False):
        cdef cpplist[unity_sframe_base_ptr] sf_array
        with nogil:
            sf_array = self.thisptr.random_split(percent, random_seed, exact)
        assert sf_array.size() == 2
        cdef unity_sframe_base_ptr proxy_first = (sf_array.front())
        cdef unity_sframe_base_ptr proxy_second = (sf_array.back())
        first = create_proxy_wrapper_from_existing_proxy(proxy_first)
        second = create_proxy_wrapper_from_existing_proxy(proxy_second)
        return (first, second)

    cpdef groupby_aggregate(self, _key_columns, _group_column, _group_output_columns, _column_ops):
        cdef vector[string] key_columns          = to_vector_of_strings(_key_columns)
        cdef vector[vector[string]] group_column = to_nested_vectors_of_strings(_group_column)
        cdef vector[string] group_output_columns = to_vector_of_strings(_group_output_columns)
        cdef vector[string] column_ops           = to_vector_of_strings(_column_ops)

        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = self.thisptr.groupby_aggregate(key_columns, group_column,
                                                   group_output_columns, column_ops)
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef append(self, UnitySFrameProxy other):
        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.append(other._base_ptr))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef materialize(self):
        self.thisptr.materialize()

    cpdef is_materialized(self):
        return self.thisptr.is_materialized()

    cpdef has_size(self):
        return self.thisptr.has_size()

    cpdef query_plan_string(self):
        return cpp_to_str(self.thisptr.query_plan_string())

    cpdef join(self, UnitySFrameProxy right, _how, dict _on):
        cdef unity_sframe_base_ptr proxy
        cdef map[string,string] on = dict_to_string_string_map(_on)
        cdef string how = str_to_cpp(_how)
        with nogil:
            proxy = (self.thisptr.join(right._base_ptr, how, on))

        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef pack_columns(self, _column_names, _key_names, dtype, fill_na):
        cdef vector[string] column_names = to_vector_of_strings(_column_names)
        cdef vector[string] key_names = to_vector_of_strings(_key_names)
        cdef unity_sarray_base_ptr proxy
        cdef flex_type_enum fl_type = flex_type_enum_from_pytype(dtype)
        cdef flexible_type na_val = flexible_type_from_pyobject(fill_na)
        with nogil:
            proxy = self.thisptr.pack_columns(column_names, key_names, fl_type, na_val)
        return sarray_proxy(proxy)

    cpdef stack(self, _column_name, _new_column_names, new_column_types, drop_na):
        cdef string column_name = str_to_cpp(_column_name)
        cdef vector[string] new_column_names = to_vector_of_strings(_new_column_names)
        cdef vector[flex_type_enum] column_types
        cdef bint b_drop_na = drop_na
        for t in new_column_types:
            column_types.push_back(flex_type_enum_from_pytype(t))

        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = self.thisptr.stack(column_name, new_column_names, column_types, b_drop_na)
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef sort(self, _sort_columns, vector[int] sort_orders):
        cdef vector[string] sort_columns = to_vector_of_strings(_sort_columns)
        cdef unity_sframe_base_ptr proxy
        # some how c++ side doesn't support vector[bool], using vector[int] here
        cdef vector[int] orders = [int(i) for i in sort_orders]
        with nogil:
            proxy = (self.thisptr.sort(sort_columns, orders))

        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef drop_missing_values(self, _columns, bint is_all, bint split):
        cdef vector[string] columns = to_vector_of_strings(_columns)
        cdef cpplist[unity_sframe_base_ptr] sf_array
        with nogil:
            sf_array = self.thisptr.drop_missing_values(columns, is_all, split)
        assert sf_array.size() == 2
        cdef unity_sframe_base_ptr proxy_first = (sf_array.front())
        cdef unity_sframe_base_ptr proxy_second
        first = create_proxy_wrapper_from_existing_proxy(proxy_first)
        if split:
            proxy_second = (sf_array.back())
            second = create_proxy_wrapper_from_existing_proxy(proxy_second)
            return (first, second)
        else:
            return first

    cpdef __get_object_id(self):
        return <size_t>(self.thisptr)


    cpdef copy_range(self, size_t start, size_t step, size_t end):
        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.copy_range(start, step, end))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef delete_on_close(self):
        with nogil:
          self.thisptr.delete_on_close()

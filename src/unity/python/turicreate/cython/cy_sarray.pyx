# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

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

from .cy_model cimport create_model_from_proxy

from .cy_cpp_utils cimport str_to_cpp

from .cy_unity cimport make_function_closure_info

### sframe ###
from .cy_sframe cimport create_proxy_wrapper_from_existing_proxy as sframe_proxy

cdef create_proxy_wrapper_from_existing_proxy(const unity_sarray_base_ptr& proxy):
    if proxy.get() == NULL:
        return None
    ret = UnitySArrayProxy(True)
    ret._base_ptr = proxy
    ret.thisptr = <unity_sarray*>(ret._base_ptr.get())
    return ret

cdef class UnitySArrayProxy:

    def __cinit__(self, do_not_allocate=None):
        if do_not_allocate:
            self._base_ptr.reset()
            self.thisptr = NULL
        else:
            self.thisptr = new unity_sarray()
            self._base_ptr.reset(<unity_sarray_base*>(self.thisptr))

    cpdef load_from_iterable(self, object d, type t, bint ignore_cast_failure=True):
        assert hasattr(d, '__iter__'), "object %s is not iterable" % str(d)

        cdef flex_list data
        cdef flex_type_enum datatype

        # If the type of t is None, then do it a bit differently.
        if t is None:
            data = common_typed_flex_list_from_iterable(d, &datatype)
        else:
            datatype = flex_type_enum_from_pytype(t)
            data = flex_list_from_typed_iterable(d, datatype, ignore_cast_failure)

        with nogil:
            self.thisptr.construct_from_vector(data, datatype)

    cpdef load_from_url(self, _url, type t):
        cdef string url = str_to_cpp(_url)
        cdef flex_type_enum datatype = flex_type_enum_from_pytype(t)
        with nogil:
            self.thisptr.construct_from_files(url, datatype)

    cpdef load_from_json_record_files(self, _url):
        cdef string url = str_to_cpp(_url)
        with nogil:
            self.thisptr.construct_from_json_record_files(url)

    cpdef load_from_sarray_index(self, _index_file):
        cdef string index_file = str_to_cpp(_index_file)
        with nogil:
            self.thisptr.construct_from_sarray_index(index_file)

    cpdef load_autodetect(self, _url, type t):
        cdef string url = str_to_cpp(_url)
        cdef flex_type_enum datatype = flex_type_enum_from_pytype(t)
        with nogil:
            self.thisptr.construct_from_autodetect(url, datatype)

    cpdef load_from_const(self, object value, size_t size, type t):
        cdef flexible_type val
        cdef flex_type_enum datatype = flex_type_enum_from_pytype(t)
        # if value is None, the hinted type coercion is bad.
        # We do want to keep val as None
        # If t is None or NoneType, we assume there is no hint
        if value is None or t is None or t is type(None):
            val = flexible_type_from_pyobject(value)
        else:
            val = flexible_type_from_pyobject_hint(value, t)
        with nogil:
            self.thisptr.construct_from_const(val, size, datatype)

    cpdef save(self, _index_file):
        cdef string index_file = str_to_cpp(_index_file)
        with nogil:
            self.thisptr.save_array(index_file)

    cpdef size(self):
        return self.thisptr.size()

    cpdef has_size(self):
        return self.thisptr.has_size()

    cpdef head(self, size_t length):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = self.thisptr.head(length)
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef dtype(self):
        cdef flex_type_enum flex_type_en = self.thisptr.dtype()
        return pytype_from_flex_type_enum(flex_type_en)

    cpdef vector_slice(self, size_t start, size_t end):
        if end <= start:
            raise RuntimeError("End of slice must be after start of slice")
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = self.thisptr.vector_slice(start, end)
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef transform(self, fn, t, bint skip_undefined, int seed):
        cdef flex_type_enum datatype = flex_type_enum_from_pytype(t)
        cdef string lambda_str
        if type(fn) == str or type(fn) == bytes:
            lambda_str = bytes(fn)
        else:
            from .. import util
            lambda_str = str_to_cpp(util._pickle_to_temp_location_or_memory(fn))

        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.transform(lambda_str, datatype, skip_undefined, seed))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef transform_native(self, closure, t, bint skip_undefined, int seed):
        cdef flex_type_enum datatype = flex_type_enum_from_pytype(t)
        cdef unity_sarray_base_ptr proxy

        cl = make_function_closure_info(closure)
        with nogil:
            proxy = (self.thisptr.transform_native(cl, datatype, skip_undefined, seed))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef filter(self, fn, bint skip_undefined, int seed):
        cdef string lambda_str
        if type(fn) == str or type(fn) == bytes:
            lambda_str = bytes(fn)  # We don't want to decode or encode here
        else:
            from .. import util
            lambda_str = util._pickle_to_temp_location_or_memory(fn)

        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.filter(lambda_str, skip_undefined, seed))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef logical_filter(self, UnitySArrayProxy other):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.logical_filter(other._base_ptr))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef topk_index(self, size_t topk, bint reverse):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.topk_index(topk, reverse))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef num_missing(self):
        cdef size_t ret
        with nogil:
            ret = self.thisptr.num_missing()
        return ret

    cpdef all(self):
        cdef bint ret
        with nogil:
            ret = self.thisptr.all()
        return ret

    cpdef any(self):
        cdef bint ret
        with nogil:
            ret = self.thisptr.any()
        return ret

    cpdef max(self):
        cdef flexible_type tmp
        with nogil:
            tmp = self.thisptr.max()
        return pyobject_from_flexible_type(tmp)

    cpdef min(self):
        cdef flexible_type tmp
        with nogil:
            tmp = self.thisptr.min()
        return pyobject_from_flexible_type(tmp)

    cpdef sum(self):
        cdef flexible_type tmp
        with nogil:
            tmp = self.thisptr.sum()
        return pyobject_from_flexible_type(tmp)

    cpdef mean(self):
        cdef flexible_type tmp
        with nogil:
            tmp = self.thisptr.mean()
        return pyobject_from_flexible_type(tmp)

    cpdef std(self, size_t ddof):
        cdef flexible_type tmp
        with nogil:
            tmp = self.thisptr.std(ddof)
        return pyobject_from_flexible_type(tmp)

    cpdef var(self, size_t ddof):
        cdef flexible_type tmp
        with nogil:
            tmp = self.thisptr.var(ddof)
        return pyobject_from_flexible_type(tmp)

    cpdef nnz(self):
        return self.thisptr.nnz()

    cpdef astype(self, type dtype, bint undefined_on_failure):
        cdef flex_type_enum datatype = flex_type_enum_from_pytype(dtype)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.astype(datatype, undefined_on_failure))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef str_to_datetime(self, _str_format):
        cdef unity_sarray_base_ptr proxy
        cdef string str_format = str_to_cpp(_str_format)
        with nogil:
            proxy = (self.thisptr.str_to_datetime(str_format))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef datetime_to_str(self, _str_format):
        cdef unity_sarray_base_ptr proxy
        cdef string str_format = str_to_cpp(_str_format)
        with nogil:
            proxy = (self.thisptr.datetime_to_str(str_format))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef clip(self, lower, upper):
        cdef flexible_type l = flexible_type_from_pyobject(lower)
        cdef flexible_type u = flexible_type_from_pyobject(upper)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.clip(l, u))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef tail(self, size_t length):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.tail(length))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef begin_iterator(self):
        self.thisptr.begin_iterator()

    cpdef iterator_get_next(self, size_t length):
        cdef flex_list tmp
        tmp = self.thisptr.iterator_get_next(length)
        return pylist_from_flex_list(tmp)

    cpdef left_scalar_operator(self, object other, op):
        cdef unity_sarray_base_ptr proxy
        cdef flexible_type val = flexible_type_from_pyobject(other)
        cdef string cpp_op = str_to_cpp(op)
        with nogil:
            proxy = (self.thisptr.left_scalar_operator(val, cpp_op))
        return create_proxy_wrapper_from_existing_proxy(proxy)


    cpdef right_scalar_operator(self, object other, op):
        cdef unity_sarray_base_ptr proxy
        cdef flexible_type val = flexible_type_from_pyobject(other)
        cdef string cpp_op = str_to_cpp(op)
        with nogil:
            proxy = (self.thisptr.right_scalar_operator(val, cpp_op))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef vector_operator(self, UnitySArrayProxy other, op):
        cdef unity_sarray_base_ptr proxy
        cdef string cpp_op = str_to_cpp(op)
        with nogil:
            proxy = (self.thisptr.vector_operator(other._base_ptr, cpp_op))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef drop_missing_values(self):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.drop_missing_values())
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef fill_missing_values(self, default_value):
        cdef flexible_type val = flexible_type_from_pyobject(default_value)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.fill_missing_values(val))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef sample(self, float percent, int seed, bint exact=False):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.sample(percent, seed, exact))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef hash(self, int seed):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.hash(seed))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef materialize(self):
        with nogil:
            self.thisptr.materialize()

    cpdef is_materialized(self):
        return self.thisptr.is_materialized()

    cpdef append(self, UnitySArrayProxy other):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.append(other._base_ptr))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef count_bag_of_words(self, object op):
        cdef gl_options_map option_values = gl_options_map_from_pydict(op)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.count_bag_of_words(option_values))
        return create_proxy_wrapper_from_existing_proxy(proxy)


    cpdef count_character_ngrams(self, size_t n, object op):
        cdef gl_options_map option_values = gl_options_map_from_pydict(op)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.count_character_ngrams(n, option_values))
        return create_proxy_wrapper_from_existing_proxy(proxy)


    cpdef count_ngrams(self, size_t n, object op):
        cdef gl_options_map option_values = gl_options_map_from_pydict(op)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.count_ngrams(n, option_values))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef dict_trim_by_keys(self, object keys, bint exclude):
        cdef unity_sarray_base_ptr proxy = (self.thisptr.dict_trim_by_keys(
            flex_list_from_iterable(keys), exclude))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef dict_trim_by_values(self, lower, upper):
        cdef flexible_type l = flexible_type_from_pyobject(lower)
        cdef flexible_type u = flexible_type_from_pyobject(upper)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.dict_trim_by_values(l, u))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef dict_keys(self):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.dict_keys())
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef dict_values(self):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.dict_values())
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef dict_has_any_keys(self, object keys):
        cdef flex_list vec = flex_list_from_iterable(keys)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.dict_has_any_keys(vec))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef dict_has_all_keys(self, object keys):
        cdef flex_list vec = flex_list_from_iterable(keys)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.dict_has_all_keys(vec))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef item_length(self):
        cdef unity_sarray_base_ptr proxy = (self.thisptr.item_length())
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef unpack_dict(self, _column_name_prefix, object limit, na_value):
        cdef string column_name_prefix = str_to_cpp(_column_name_prefix)
        cdef unity_sframe_base_ptr proxy
        cdef flexible_type gl_na_value = flexible_type_from_pyobject(na_value)
        cdef vector[flexible_type] sub_keys
        for key in limit:
            sub_keys.push_back(flexible_type_from_pyobject(key))

        with nogil:
            proxy = self.thisptr.unpack_dict(column_name_prefix, sub_keys, gl_na_value)
        return sframe_proxy(proxy)

    cpdef unpack(self, _column_name_prefix, object limit, value_types, na_value):
        cdef string column_name_prefix = str_to_cpp(_column_name_prefix)
        cdef vector[flex_type_enum] column_types
        for t in value_types:
            column_types.push_back(flex_type_enum_from_pytype(t))

        cdef vector[flexible_type] sub_keys
        for key in limit:
            sub_keys.push_back(flexible_type_from_pyobject(key))

        cdef unity_sframe_base_ptr proxy
        cdef flexible_type gl_na_value = flexible_type_from_pyobject(na_value)
        with nogil:
            proxy = self.thisptr.unpack(column_name_prefix, sub_keys, column_types, gl_na_value)
        return sframe_proxy(proxy)

    cpdef expand(self, _column_name_prefix, object limit, value_types):
        cdef string column_name_prefix = str_to_cpp(_column_name_prefix)
        cdef vector[flex_type_enum] column_types
        for t in value_types:
            column_types.push_back(flex_type_enum_from_pytype(t))

        cdef vector[flexible_type] sub_keys
        for key in limit:
            sub_keys.push_back(flexible_type_from_pyobject(key))

        cdef unity_sframe_base_ptr proxy
        with nogil:
            proxy = self.thisptr.expand(column_name_prefix, sub_keys, column_types)
        return sframe_proxy(proxy)

    cpdef __get_object_id(self):
        return <size_t>(self.thisptr)

    cpdef get_content_identifier(self):
        return self.thisptr.get_content_identifier()

    cpdef copy_range(self, size_t start, size_t step, size_t end):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.copy_range(start, step, end))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef builtin_rolling_apply(self, _fn_name, ssize_t before, ssize_t after, size_t min_observations):
        cdef string fn_name = str_to_cpp(_fn_name)
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.builtin_rolling_apply(fn_name, before, after, min_observations))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef show(self, _path_to_client, _title, _xlabel, _ylabel):
        cdef string path_to_client = str_to_cpp(_path_to_client)
        cdef string title = str_to_cpp(_title)
        cdef string xlabel = str_to_cpp(_xlabel)
        cdef string ylabel = str_to_cpp(_ylabel)
        with nogil:
            self.thisptr.show(path_to_client, title, xlabel, ylabel)

    cpdef plot(self, _path_to_client, _title, _xlabel, _ylabel):
        cdef string path_to_client = str_to_cpp(_path_to_client)
        cdef string title = str_to_cpp(_title)
        cdef string xlabel = str_to_cpp(_xlabel)
        cdef string ylabel = str_to_cpp(_ylabel)
        cdef model_base_ptr proxy
        with nogil:
            proxy = self.thisptr.plot(path_to_client, title, xlabel, ylabel)
        return create_model_from_proxy(proxy)

    cpdef builtin_cumulative_aggregate(self, _fn_name):
        cdef unity_sarray_base_ptr proxy
        cdef string fn_name = str_to_cpp(_fn_name)
        with nogil:
            proxy = (self.thisptr.builtin_cumulative_aggregate(fn_name))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef subslice(self, start, step, stop):
        cdef unity_sarray_base_ptr proxy
        cdef flexible_type fstart = flexible_type_from_pyobject(start)
        cdef flexible_type fstep = flexible_type_from_pyobject(step)
        cdef flexible_type fstop = flexible_type_from_pyobject(stop)
        with nogil:
            proxy = (self.thisptr.subslice(fstart, fstep, fstop))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef ternary_operator(self, UnitySArrayProxy istrue, UnitySArrayProxy isfalse):
        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = (self.thisptr.ternary_operator(istrue._base_ptr, isfalse._base_ptr))
        return create_proxy_wrapper_from_existing_proxy(proxy)

    cpdef to_const(self, object value, type t):
        cdef flexible_type val
        cdef flex_type_enum datatype = flex_type_enum_from_pytype(t)
        # if value is None, the hinted type coercion is bad.
        # We do want to keep val as None
        # If t is None or NoneType, we assume there is no hint
        if value is None or t is None or t is type(None):
            val = flexible_type_from_pyobject(value)
        else:
            val = flexible_type_from_pyobject_hint(value, t)

        cdef unity_sarray_base_ptr proxy
        with nogil:
            proxy = self.thisptr.to_const(val, datatype)
        return create_proxy_wrapper_from_existing_proxy(proxy)

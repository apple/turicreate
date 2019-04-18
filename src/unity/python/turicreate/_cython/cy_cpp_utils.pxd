# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.map cimport map
from cpython.version cimport PY_MAJOR_VERSION

cdef string _attempt_cast_str_to_cpp(object ) except *

cdef inline string str_to_cpp(py_s) except *:
    """
    Use this function to convert any string-like object to the c++
    string class.
    """
    cdef type t = type(py_s)
    
    if PY_MAJOR_VERSION >= 3:
        if t is str:
            return (<str>py_s).encode()
        elif t is bytes:
            return (<bytes>py_s)
        else:
            return _attempt_cast_str_to_cpp(py_s)
    else:
        if t is str:
            return (<str>py_s)
        elif t is unicode:
            return (<unicode>py_s).encode('UTF-8')
        else:
            return _attempt_cast_str_to_cpp(py_s)            

cdef inline string unsafe_str_to_cpp(py_s) except *:
    """
    Use this version if you know for sure that type(py_s) is str.
    """
    if PY_MAJOR_VERSION >= 3:
        return (<str>py_s).encode()
    else:
        return (<str>py_s)

cdef inline string unsafe_unicode_to_cpp(py_s) except *:
    """
    Use this version if you know for sure that type(py_s) is unicode
    (same as str in python 3).
    """
    if PY_MAJOR_VERSION >= 3:
        return (<str>py_s).encode()
    else:
        return (<unicode>py_s).encode('UTF-8')

# With cpp_to_str, a c++ string is decoded into bytes if
# disable_cpp_str_decode() has been called, or str otherwise.  If
# decoding is done into bytes, as is needed by some contexts, then
# enable_cpp_str_decode() should be called immediately afterwards --
# i.e. use a try...finally block!

cpdef disable_cpp_str_decode()
cpdef enable_cpp_str_decode()
cdef _cpp_to_str_py3_decode(const string& cpp_s)

cdef inline cpp_to_str(const string& cpp_s):
    cdef const char* c_s = cpp_s.data()
    
    if PY_MAJOR_VERSION >= 3:
        return _cpp_to_str_py3_decode(cpp_s)
    else:
        return str(c_s[:cpp_s.size()])


cdef inline vector[string] to_vector_of_strings(object v) except *:
    """
    Translate a vector of strings with proper encoding.. 
    """
    cdef list fl
    cdef tuple ft
    cdef long i
    cdef vector[string] ret

    if type(v) is list:
        fl = <list>v
        ret.resize(len(fl))
        for i in range(len(fl)):
            ret[i] = str_to_cpp(fl[i])
    elif type(v) is tuple:
        ft = <tuple>v
        ret.resize(len(ft))
        for i in range(len(ft)):
            ret[i] = str_to_cpp(ft[i])
    else:
        raise TypeError("Cannot interpret type '%s' as list of strings." % str(type(v)))

    return ret
    
cdef inline list from_vector_of_strings(const vector[string]& sv):
    """
    Translate a vector of strings with proper decoding. 
    """
    cdef list ret = [None]*sv.size()
    cdef long i

    for i in range(<long>(sv.size())):
        ret[i] = cpp_to_str(sv[i])

    return ret

# This function accepts its argument by value because cython does not yet know
# how to obtain a const_iterator from a const container.
cdef inline dict from_map_of_strings_and_vectors_of_strings(map[string, vector[string]] m):
    """
    Translate a map whose keys are strings and whose values are vectors of
    strings
    """
    ret = {}
    for value in m:
        ret[cpp_to_str(value.first)] = from_vector_of_strings(value.second)
    return ret
    
cdef inline vector[vector[string]] to_nested_vectors_of_strings(object v) except *:
    """
    Translate a nested vector of strings with proper encoding. 
    """
    cdef list fl
    cdef tuple ft
    cdef long i
    cdef vector[vector[string]] ret

    if type(v) is list:
        fl = <list>v
        ret.resize(len(fl))
        for i in range(len(fl)):
            ret[i] = to_vector_of_strings(fl[i])
    elif type(v) is tuple:
        ft = <tuple>v
        ret.resize(len(ft))
        for i in range(len(ft)):
            ret[i] = to_vector_of_strings(ft[i])
    else:
        vs = str(v)
        if len(vs) > 50:
            vs = vs[:50] + "..."
        raise TypeError("Cannot interpret '%s' as nested lists of strings." % vs)

    return ret    
        
cdef inline map[string, string] dict_to_string_string_map(dict d) except *:
    """
    Translate a map of strings to strings with proper encoding. 
    """
    cdef map[string,string] ret

    for k, v in d.iteritems():
        ret[str_to_cpp(k)] = str_to_cpp(v)

    return ret


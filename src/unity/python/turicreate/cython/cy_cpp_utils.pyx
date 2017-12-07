# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from libcpp.string cimport string
from cpython.version cimport PY_MAJOR_VERSION

cdef string _attempt_cast_str_to_cpp(py_s) except *:
    """
    The last resort conversion routine for strings cast 
    """

    cdef bint success

    # Try this version first, as casting something to bytes is much
    # restrictive and tends to succeed only if it's a legit cast.
    # However, the cast to str can succeed even if it's actually a
    # bytes class -- e.g. np.string_.
    if PY_MAJOR_VERSION >= 3:
    
        try:
            py_s = bytes(py_s)
            success = True
        except:
            success = False

        if success:
            return (<bytes>py_s)

    # Some classes (e.g. np.string_) will succeed with a string
    # representation of the class, e.g. np.string_('a') == b'a', but
    # str(np.string_('a')) == "b'a'".  Thus we need to ensure it's
    # actually a legit representation as well.
    try:
        new_py_s = str(py_s)
        success = (new_py_s == py_s)
        py_s = new_py_s
    except:
        success = False

    if success:
        return unsafe_str_to_cpp(py_s)

    # Now,see about the unicode route. 
    if PY_MAJOR_VERSION == 2:
        try:
            new_py_s = unicode(py_s)
            success = (new_py_s == py_s)
            py_s = new_py_s
        except:
            success = False

        if success:
            return unsafe_unicode_to_cpp(py_s)

    # Okay, none of these worked, so error out.
    raise TypeError("Type '%s' cannot be interpreted as str." % str(type(py_s)))


cdef bint _cpp_to_str_py3_decode_enabled = True

cpdef disable_cpp_str_decode():
    global _cpp_to_str_py3_decode_enabled
    assert _cpp_to_str_py3_decode_enabled == True
    _cpp_to_str_py3_decode_enabled = False

cpdef enable_cpp_str_decode():
    global _cpp_to_str_py3_decode_enabled
    assert _cpp_to_str_py3_decode_enabled == False
    _cpp_to_str_py3_decode_enabled = True

cdef _cpp_to_str_py3_decode(const string& cpp_s):
    """
    Decodes a c++ string into bytes if disable_cpp_str_decode() has
    been called, or str otherwise.
    """
    cdef const char* c_s = cpp_s.data()

    if _cpp_to_str_py3_decode_enabled:
        return c_s[:cpp_s.size()].decode()
    else:
        return (<bytes>c_s[:cpp_s.size()])

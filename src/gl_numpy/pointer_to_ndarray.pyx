# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
_numpy_imported = False
try:
    import numpy 
    _numpy_imported = True
except:
    pass

# code adapted from 
# http://stackoverflow.com/questions/23872946/force-numpy-ndarray-to-take-ownership-of-its-memory-in-cython
# cdef data_to_int_numpy_array(void* ptr, np.npy_intp N):
#     if _numpy_imported == False:
#         raise RuntimeError("Numpy not available")
#     cdef np.npy_intp shape[1]
#     shape[0] = <np.npy_intp> N
#     cdef np.ndarray arr = np.PyArray_SimpleNewFromData(1, shape, np.NPY_INT64, <void*>ptr)
#     PyArray_ENABLEFLAGS(arr, np.NPY_OWNDATA)
#     return arr
#
# cdef data_to_double_numpy_array(void* ptr, np.npy_intp N):
#     if _numpy_imported == False:
#         raise RuntimeError("Numpy not available")
#     cdef np.npy_intp shape[1]
#     shape[0] = <np.npy_intp> N
#     cdef np.ndarray arr = np.PyArray_SimpleNewFromData(1, shape, np.NPY_DOUBLE, <void*>ptr)
#     PyArray_ENABLEFLAGS(arr, np.NPY_OWNDATA)
#     return arr
#
cpdef numpy_own_array(arr):
    PyArray_ENABLEFLAGS(arr, np.NPY_OWNDATA)

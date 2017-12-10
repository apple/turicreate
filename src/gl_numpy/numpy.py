# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Scalable numpy integration.
"""
from __future__ import absolute_import
from .numpy_loader import *
from .numpy_loader import _get_unity_dll

def _sframe_to_nparray(sf):
    """
    Converts a numeric SFrame to a numpy Array.
    Every column in the SFrame must be of numeric (integer, float) or
    array.array type. The resultant Numpy array the same number of rows as the
    input SFrame, and with each row merged into a single array.

    Missing values in integer columns are converted to 0, and missing values
    in float columns are converted to NaN.

    Example:

    >>> sf = tc.SFrame({'a':[1,1,None],
    >>>                 'b':[2.0,2.0,None],
    >>>                 'c':[[3.0,3.0],[3.0,3.0],[3.0,3.0]]})
    >>> sf
    Columns:
            a	int
            b	float
            c	array
    Rows: 3
    Data:
    +------+------+------------+
    |  a   |  b   |     c      |
    +------+------+------------+
    |  1   | 2.0  | [3.0, 3.0] |
    |  1   | 2.0  | [3.0, 3.0] |
    | None | None | [3.0, 3.0] |
    +------+------+------------+
    [3 rows x 3 columns]
    >>> n = tc.numpy.sframe_to_nparray(sf)
    >>> n
    array([[ 1.,  2.,  3.,  3.],
           [ 1.,  2.,  3.,  3.],
           [ 0., nan,  3.,  3.]])
    """
    if not numpy_activation_successful():
        raise RuntimeError("This function cannot be used if Scalable Numpy activation failed")
    import turicreate
    import turicreate.util
    import ctypes
    import numpy as np
    import turicreate.cython.pointer_to_ndarray
    import array
    if not all(d in [int, float, array.array] for d in sf.dtype()):
        raise TypeError("Only integer, float or array column types are supported")
    temp_dir = turicreate.util._make_temp_directory("numpy_convert_")
    # only sframes have save_reference
    sf._save_reference(temp_dir)
    unity_dll = _get_unity_dll()
    if unity_dll is None:
        raise RuntimeError("fast Converter not loaded")
    if all(d in [int] for d in sf.dtype()):
        ptr = unity_dll.pointer_from_sframe(temp_dir.encode(), True)
        if not ptr:
            raise RuntimeError("Unable to convert to numpy array")
        arrlen = unity_dll.pointer_length(ptr) // 8
        ArrayType = ctypes.c_int64 * arrlen
        addr = ctypes.addressof(ptr.contents)
        array = np.frombuffer(ArrayType.from_address(addr), np.int64)
        turicreate.cython.pointer_to_ndarray.numpy_own_array(array)
        width = arrlen / len(sf)
        if width > 1:
            return array.reshape(len(sf), width)
        else:
            return array
    else:
        unity_dll.pointer_from_sframe.restype = ctypes.POINTER(ctypes.c_double)
        ptr = unity_dll.pointer_from_sframe(temp_dir.encode(), True)
        if not ptr:
            raise RuntimeError("Unable to convert to numpy array")
        arrlen = unity_dll.pointer_length(ptr) // 8
        ArrayType = ctypes.c_double * arrlen
        addr = ctypes.addressof(ptr.contents)
        array = np.frombuffer(ArrayType.from_address(addr), np.double)
        turicreate.cython.pointer_to_ndarray.numpy_own_array(array)
        width = arrlen / len(sf)
        if width > 1:
            return array.reshape(len(sf), arrlen / len(sf))
        else:
            return array

def _sarray_to_nparray(arr):
    """
    Converts a numeric SArray to a numpy Array.
    The array must be of numeric (integer, float) or
    array.array type. The resultant Numpy array the same number of rows as the
    input SArray.

    Missing values in integer columns are converted to 0, and missing values
    in float columns are converted to NaN.

    Example:

    >>> sa = tc.SArray([1,1,None])
    >>> tc.numpy.sarray_to_nparray(sa)
    array([1, 1, 0])
    >>> sa = tc.SArray([1.0,1.0,None])
    array([  1.,   1.,  nan])
    >>> tc.numpy.sarray_to_nparray(sa)
    >>> sa = tc.SArray([[1.0,1.0],[2.0,2.0],[3.0,3.0]])
    >>> n
    array([[ 1.,  1.],
           [ 2.,  2.],
           [ 3.,  3.]])
    """
    import turicreate as tc
    return _sframe_to_nparray(tc.SFrame({'X1':arr}))

def array(data):
    """
    Converts numeric SFrames or SArrays to numpy arrays.

    Every column in the SFrame/SArray  must be of numeric (integer, float) or
    array.array type. The resultant Numpy array the same number of rows as the
    input, and with each row merged into a single array.

    Missing values in integer columns are converted to 0, and missing values
    in float columns are converted to NaN.

    **SFrame Conversion Example**

    >>> sf = tc.SFrame({'a':[1,1,None],
    >>>                 'b':[2.0,2.0,None],
    >>>                 'c':[[3.0,3.0],[3.0,3.0],[3.0,3.0]]})
    >>> sf
    Columns:
            a	int
            b	float
            c	array
    Rows: 3
    Data:
    +------+------+------------+
    |  a   |  b   |     c      |
    +------+------+------------+
    |  1   | 2.0  | [3.0, 3.0] |
    |  1   | 2.0  | [3.0, 3.0] |
    | None | None | [3.0, 3.0] |
    +------+------+------------+
    [3 rows x 3 columns]
    >>> n = tc.numpy.array(sf)
    >>> n
    array([[ 1.,  2.,  3.,  3.],
           [ 1.,  2.,  3.,  3.],
           [ 0., nan,  3.,  3.]])

    **SArray Conversion Example**

    >>> sa = tc.SArray([1,1,None])
    >>> tc.numpy.array(sa)
    array([1, 1, 0])
    >>> sa = tc.SArray([1.0,1.0,None])
    array([  1.,   1.,  nan])
    >>> tc.numpy.sarray(sa)
    >>> sa = tc.SArray([[1.0,1.0],[2.0,2.0],[3.0,3.0]])
    >>> n
    array([[ 1.,  1.],
           [ 2.,  2.],
           [ 3.,  3.]])

    Parameters
    ----------
    data : turicreate.SFrame | turicreate.SArray | Any iterable type
        When data is an SFrame or an SArray, a fast scalable conversion process
        is used. Otherwise for all other types, a simple conversion
        (np.array(data)) is returned.

    """
    import turicreate as tc
    if issubclass(type(data), tc.SFrame):
        return _sframe_to_nparray(data)
    elif issubclass(type(data), tc.SArray):
        return _sarray_to_nparray(data)
    else:
        import numpy as np
        return np.array(data)

def get_memory_limit():
    """
    Gets the maximum amount of resident memory used for numpy arrays.
    """
    unity_dll = _get_unity_dll()
    return unity_dll.get_memory_limit()

def set_memory_limit(limit):
    """
    Sets the maximum amount of resident memory used for numpy arrays (in bytes)
    The value only impacts performance: the larger the value, the faster
    the numpy arrays are.
    """
    unity_dll = _get_unity_dll()
    unity_dll.set_memory_limit(limit)


def _pagefile_total_allocated_bytes():
    """
    Sets the maximum amount of resident memory used for numpy arrays (in bytes)
    The value only impacts performance: the larger the value, the faster
    the numpy arrays are.
    """
    unity_dll = _get_unity_dll()
    return unity_dll.pagefile_total_allocated_bytes()

def _pagefile_total_stored_bytes():
    """
    Sets the maximum amount of resident memory used for numpy arrays (in bytes)
    The value only impacts performance: the larger the value, the faster
    the numpy arrays are.
    """
    unity_dll = _get_unity_dll()
    return unity_dll.pagefile_total_stored_bytes()



def _pagefile_compression_ratio():
    """
    Returns the effective compression ratio on disk
    """
    unity_dll = _get_unity_dll()
    return unity_dll.pagefile_compression_ratio()

def _fast_numpy_to_sarray(arr):
    """
    Converts a restricted class of numpy arrays to an SArray.
    This conversion is efficient and does not involve communication to
    the GLC C++ backend.

    numpy array must be of numeric type, must have no more than 2 dimensions.
    """

    unity_dll = _get_unity_dll()
    if unity_dll is None:
        raise RuntimeError("Fast Conversion cannot be performed")

    import numpy as np
    import ctypes

    if len(arr.shape) > 2:
        raise TypeError("Numpy Array must have at most 2 dimensions")

    if not arr.dtype in [np.int8, np.int16, np.int32, np.int64,
            np.uint8, np.uint16, np.uint32, np.uint64,
            np.float32, np.float64]:
        raise TypeError("Numpy Array of unknown type")

    if not arr.flags['C_CONTIGUOUS']:
        arr = np.ascontiguousarray(arr)

    # get the properties I need to make the call
    # array must be in C layout
    if np.isfortran(arr):
        arr = np.asarray(arr, order='C')

    # pointer
    ptr = arr.ctypes.data_as(ctypes.c_void_p)
    # data length

    ptr_length = arr.size
    # row length
    if len(arr.shape) == 1:
        row_length = 1
    else:
        row_length = arr.shape[1]

    # is integral
    is_integer = arr.dtype in [np.int8, np.int16, np.int32, np.int64,
                               np.uint8, np.uint16, np.uint32, np.uint64]
    # is signed
    # this only matters if is_integer is true
    signed_type = arr.dtype in [np.int8, np.int16, np.int32, np.int64]

    # width
    if arr.dtype in [np.int8, np.uint8]:
        element_width = 1
    elif arr.dtype in [np.int16, np.uint16]:
        element_width = 2
    elif arr.dtype in [np.int32, np.uint32, np.float32]:
        element_width = 4
    elif arr.dtype in [np.int64, np.uint64, np.float64]:
        element_width = 8

    # output location
    import turicreate
    import turicreate.util
    output_location = turicreate.util._make_temp_filename("numpy_to_sa") + ".sidx"
    coutput_location = ctypes.c_char_p(output_location)
    ret = unity_dll.numpy_to_sarray(ptr, ptr_length, row_length,
            is_integer, signed_type, element_width, coutput_location)

    if ret:
        ret = turicreate.SArray(output_location)
        # little hack to make this automatically delete on close
        # mark this sarray to delete on close
        turicreate.SFrame({'X1':ret}).__proxy__.delete_on_close()
        return ret
    else:
        raise RuntimeError("Unable to convert. Unknown error")


if not numpy_activation_successful():
    activate_scalable_numpy(True)
    if numpy_activation_successful():
        print("Scalable Numpy Activation Successful")
    else:
        print("Scalable Numpy Activation Failed")

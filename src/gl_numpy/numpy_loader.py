# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import absolute_import
################### Numpy Integration ########################
_unity_dll = None
_load_attempted = False
def _get_unity_dll():
    global _unity_dll
    global _load_attempted
    import sys
    if sys.platform == 'win32':
        _load_attempted = True
        return None
    if _unity_dll is None and not _load_attempted:
        import ctypes
        import os
        import sys
        _load_attempted = True
        curpath = os.path.dirname(sys.modules[__name__].__file__)
        # currently, only linux and mac
        candidates = ['libunity_shared.so',
                      'libunity_shared.dylib']

        good_file = None
        for candidate in candidates:
            sopath = os.path.join(curpath, candidate)
            if os.path.isfile(sopath):
                good_file = sopath
                break

        if good_file is None:
            return

        _unity_dll=ctypes.CDLL(good_file, mode=ctypes.RTLD_GLOBAL)
        # pointer_from_sframe
        _unity_dll.pointer_from_sframe.argtypes = [ctypes.c_void_p, ctypes.c_bool]
        _unity_dll.pointer_from_sframe.restype = ctypes.POINTER(ctypes.c_int64)

        # set_memory_limit
        _unity_dll.set_memory_limit.argtypes = [ctypes.c_int64]

        # get_memory_limit
        _unity_dll.get_memory_limit.restype = ctypes.c_int64

        # pagefile_allocated_total_bytes
        _unity_dll.pagefile_total_allocated_bytes.restype = ctypes.c_int64

        # pagefile_total_stored_types
        _unity_dll.pagefile_total_stored_bytes.restype = ctypes.c_int64

        # pagefile_compression_ratio
        _unity_dll.pagefile_compression_ratio.restype = ctypes.c_double

        # pointer_length
        _unity_dll.pointer_length.restype = ctypes.c_int64
        _unity_dll.pointer_length.argtypes = [ctypes.c_void_p]

        # numpy to sarray
        _unity_dll.numpy_to_sarray.restype = ctypes.c_bool
        _unity_dll.numpy_to_sarray.argtypes = [ctypes.c_void_p,
                                               ctypes.c_uint64,
                                               ctypes.c_uint64,
                                               ctypes.c_bool,
                                               ctypes.c_bool,
                                               ctypes.c_uint64,
                                               ctypes.c_char_p]

        # malloc_injection_successful
        _unity_dll.malloc_injection_successful.restype = ctypes.c_bool
    return _unity_dll

# attempt 2 loading strategies
def _load_numpy_alt_malloc_via_environment_variable():
    import sys
    if sys.platform == 'win32':
        return
    import os
    path = os.path.dirname(os.path.realpath(__file__))
    candidates = [os.path.join(path, "libunity_shared.so"),
            os.path.join(path, "libunity_shared.dylib")]
    for i in candidates:
        if os.path.exists(i):
            os.environ["NUMPY_ALTERNATE_MALLOC"] = i
            os.environ["NUMPY_ALTERNATE_MALLOC_PREFIX"] = "my_"
            break


def _load_numpy_alt_malloc_via_binary_injection():
    import sys
    if sys.platform == 'win32':
        return
    import ctypes
    import subprocess
    import numpy as np
    addrsearch = subprocess.check_output(["nm", np.core.multiarray.__file__])
    addrsearch = addrsearch.decode()
    symbols = [x.split(' ') for x in addrsearch.splitlines()]
    # nm returns stuff in the following format
    #
    # [address] [type]  [symbol]
    #
    # address is a hex string, type is like "t" or "T" or "W", etc.
    # symbol is the symbol name.
    #
    # Somme lines have less stuff because some lines have no address.
    # This selects all lines with length 3 (has address, type and symbol)
    # and turns it into a dict of symbol:address, parsing the address as base 16
    symboldict = {x[2]:int(x[0], 16) for x in symbols if len(x) == 3}

    # order is MALLOC, CALLOC, FREE, REALLOC, Npy_alloc_cache, npy_alloc_cache_zero, npy_free_cache
    addresses = [-1, -1, -1, -1, -1, -1, -1]
    candidate_names = [['PyDataMem_NEW', '_PyDataMem_NEW'],
                       ['PyDataMem_NEW_ZEROED', '_PyDataMem_NEW_ZEROED'],
                       ['PyDataMem_RENEW', '_PyDataMem_RENEW'],
                       ['PyDataMem_FREE', '_PyDataMem_FREE'],
                       ['npy_alloc_cache', '_npy_alloc_cache'],
                       ['npy_alloc_cache_zero', '_npy_alloc_cache_zero'],
                       ['npy_free_cache', '_npy_free_cache']]

    for i in range(len(addresses)):
        for c in candidate_names[i]:
            if c in symboldict:
                addresses[i] = symboldict[c]
                break
        if addresses[i] == -1:
            raise RuntimeError("Cannot find symbol to override")


    udll = _get_unity_dll()
    udll.perform_numpy_malloc_override.argtypes = [ctypes.c_char_p,
            ctypes.c_int64, ctypes.c_int64, ctypes.c_int64, ctypes.c_int64,
            ctypes.c_int64, ctypes.c_int64, ctypes.c_int64]
    udll.perform_numpy_malloc_override.rettype = ctypes.c_bool
    udll.perform_numpy_malloc_override(np.core.multiarray.__file__.encode(),
            addresses[0], addresses[1], addresses[2], addresses[3],
            addresses[4], addresses[5], addresses[6])

def numpy_activation_successful():
    import sys
    if sys.platform == 'win32':
        return False
    try:
        import numpy as np
    except:
        return False

    np.arange(100)
    unity_dll = _get_unity_dll()
    if unity_dll is None:
        return False
    try:
        return unity_dll.malloc_injection_successful()
    except:
        return False

# We try to activate numpy twice
# Once via environment variable. This is the preferred way.
# However, this does not work if numpy as loaded first, or if we have an
# unmodified numpy.
#
# The 2nd way is much more intrusive and attempts to do runtime binary
# modification on the numpy symbols.
# It basically:
#  - uses "nm" to try to locate the functions we need to override.
#  - And then modifies the instructions at the functions.
def activate_scalable_numpy(intrusive_method=False):
    try:
        _load_numpy_alt_malloc_via_environment_variable()
    except:
        pass

    if intrusive_method:
        try:
            if not numpy_activation_successful():
                _load_numpy_alt_malloc_via_binary_injection()
        except:
            pass

    return numpy_activation_successful()

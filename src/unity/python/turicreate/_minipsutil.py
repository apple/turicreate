# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import ctypes as _ctypes
import glob as _glob
import sys as _sys
from os.path import dirname as _dirname, join as _join

_thismodule = _sys.modules[__name__]
_current_path = _dirname(_thismodule.__file__)
library_locations = _glob.glob(_join(_current_path, 'libminipsutil.so'))
library_locations += _glob.glob(_join(_current_path, 'libminipsutil.dylib'))
library_locations += _glob.glob(_join(_current_path, 'libminipsutil.dll'))

_totalmem = 0
_ncpus = 0
_lib = None
try:
    _lib = _ctypes.cdll.LoadLibrary(library_locations[0])
    _lib.num_cpus.restype = _ctypes.c_int32
    _lib.total_mem.restype = _ctypes.c_uint64
    _lib.pid_is_running.restype = _ctypes.c_int32
    _lib.kill_process.restype = _ctypes.c_int32
    _ncpus = _lib.num_cpus()
    _totalmem = _lib.total_mem()
except:
    pass

def total_memory():
    global _totalmem
    return _totalmem

def cpu_count():
    global _ncpus
    return _ncpus

def pid_is_running(i):
    global _lib
    if _lib is None:
        return True
    else:
        return _lib.pid_is_running(_ctypes.c_int32(i)) == 1

def kill_process(i):
    global _lib
    if _lib is None:
        return True
    else:
        return _lib.kill_process(_ctypes.c_int32(i)) == 1

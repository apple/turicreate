# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import sys
import time 

cdef int access_null_ptr():
  cdef int* p = NULL
  p[0] = 10
  return p[0]

def bad_memory_access_fun():
  access_null_ptr()

def force_exit_fun():
  abort()

# Need to put something here to avoid an annoying compile error with
# some cython versions
# cy_test_utils.cxx:691:12: error: unused variable '__pyx_clineno' [-Werror,-Wunused-variable]
cpdef unused():
    abort()


# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp.map cimport map
from libcpp.list cimport list as cpplist

from .cy_flexible_type cimport flex_type_enum
from .cy_flexible_type cimport flexible_type 
from .cy_flexible_type cimport flex_list

cdef extern from "<sframe/dataframe.hpp>" namespace "turi":
    cdef cppclass dataframe_t:
        vector[string] names
        map[string, flex_type_enum] types
        map[string, vector[flexible_type]] values
        int nrows()
        int ncols()
        void clear()

ctypedef dataframe_t gl_dataframe

cdef bint is_pandas_dataframe(object v)

cdef gl_dataframe gl_dataframe_from_dict_of_arrays(dict) except *

#/**************************************************************************/
#/*                                                                        */
#/*                Convert gl_dataframe to pandas dataframe                */
#/*                                                                        */
#/**************************************************************************/
cdef pd_from_gl_dataframe(gl_dataframe&)

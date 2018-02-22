# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

#cython libcpp types
from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp.map cimport map
from libcpp.set cimport set as stdset
from libcpp.pair cimport pair

#### Define flexible type base types ####
# From "<flexible_type/flexible_type_base_types.hpp>" namespace "turi":
ctypedef long long flex_int
ctypedef double flex_float
ctypedef pair[long long,int] pflex_date_time
ctypedef vector[double] flex_vec
ctypedef string flex_string
ctypedef vector[flexible_type] flex_list
ctypedef vector[pair[flexible_type, flexible_type]] flex_dict


####Flex_image externs
cdef extern from "<image/image_type.hpp>" namespace "turi":
   cdef cppclass flex_image:
        flex_image()
        flex_image(char* image_data, size_t height, size_t width, size_t channels, \
                   size_t image_data_size, int version, int format)
        const char* get_image_data()
        size_t m_height
        size_t m_width
        size_t m_channels
        size_t m_image_data_size
        char m_format
        char m_version

#### Externs from flexible_type.hpp
cdef extern from "<flexible_type/flexible_type.hpp>" namespace "turi":

    cdef enum flex_type_enum:
        INTEGER         "turi::flex_type_enum::INTEGER"  = 0
        FLOAT           "turi::flex_type_enum::FLOAT"    = 1
        STRING          "turi::flex_type_enum::STRING"   = 2
        VECTOR          "turi::flex_type_enum::VECTOR"   = 3
        LIST            "turi::flex_type_enum::LIST"     = 4
        DICT            "turi::flex_type_enum::DICT"     = 5
        DATETIME        "turi::flex_type_enum::DATETIME" = 6
        UNDEFINED       "turi::flex_type_enum::UNDEFINED"= 7
        IMAGE           "turi::flex_type_enum::IMAGE"    = 8
        ND_VECTOR       "turi::flex_type_enum::ND_VECTOR"   = 9

    cdef cppclass flexible_type:
        flexible_type()
        flexible_type(flex_type_enum)
        flexible_type& soft_assign(const flexible_type& other) except +

        flex_type_enum get_type()
        # Cython bug:
        # we should put "except + TypeError" after the delcaration,
        # but it does not compile.
        const flex_int& get_int "get<turi::flex_int>"()
        pflex_date_time get_date_time "get_date_time_as_timestamp_and_offset"()
        int get_microsecond "get_date_time_microsecond"()
        const flex_float& get_double "get<turi::flex_float>"()
        const flex_string& get_string "get<turi::flex_string>"()
        const flex_vec& get_vec "get<turi::flex_vec>"()
        flex_vec& get_vec_m "mutable_get<turi::flex_vec>"()
        const flex_nd_vec& get_nd_vec "get<turi::flex_nd_vec>"()
        flex_nd_vec& get_nd_vec_m "mutable_get<turi::flex_nd_vec>"()
        const flex_list& get_list "get<turi::flex_list>"()
        flex_list& get_list_m "mutable_get<turi::flex_list>"()
        const flex_dict& get_dict "get<turi::flex_dict>"()
        flex_dict& get_dict_m "mutable_get<turi::flex_dict>"()
        const flex_image& get_img "get<turi::flex_image>"()

        flex_float as_double "to<turi::flex_float>"()
        flex_list as_list "to<turi::flex_list>"()
        flex_string as_string "to<turi::flex_string>"()

        flexible_type& set_int "operator=<int64_t>"(const flex_int& other)
        flexible_type& set_date_time "set_date_time_from_timestamp_and_offset"(const pflex_date_time& other, int)
        flexible_type& set_double "operator=<double>"(const flex_float& other)
        flexible_type& set_string "operator=<std::string>"(const flex_string& other)
        flexible_type& set_vec "operator=<std::vector<double> >"(const flex_vec& other)
        flexible_type& set_nd_vec "operator=<turi::flex_nd_vec>"(const flex_nd_vec& other)
        flexible_type& set_list "operator=<turi::flex_list>"(const flex_list& other)
        flexible_type& set_dict "operator=<turi::flex_dict>"(const flex_dict& other)
        flexible_type& set_img "operator=<turi::flex_image>"(const flex_image& other)
        void reset()

    cdef flexible_type FLEX_UNDEFINED

    cdef void swap "std::swap"(flexible_type&, flexible_type&)

    cdef int TIMEZONE_RESOLUTION_IN_SECONDS "turi::flex_date_time::TIMEZONE_RESOLUTION_IN_SECONDS"
    cdef float TIMEZONE_RESOLUTION_IN_HOURS "turi::flex_date_time::TIMEZONE_RESOLUTION_IN_HOURS"
    cdef int EMPTY_TIMEZONE "turi::flex_date_time::EMPTY_TIMEZONE"
    cdef string flex_type_enum_to_name(flex_type_enum)
    cdef bint flex_type_is_convertible(flex_type_enum, flex_type_enum)

    cdef cppclass flex_nd_vec:
        flex_nd_vec()
        flex_nd_vec(vector[double])
        flex_nd_vec(vector[double], vector[size_t])
        flex_nd_vec(vector[double], vector[size_t], vector[size_t])
        const vector[double]& elements()
        const vector[size_t]& shape()
        const vector[size_t]& stride()
        const size_t start()
        const size_t num_elem()
        bint is_full()
        bint is_valid()



### Other unity types ###
ctypedef map[string, flexible_type] gl_options_map

# If we just want to work with the enum types, use these.
cdef flex_type_enum flex_type_enum_from_pytype(type t) except *
cdef type pytype_from_flex_type_enum(flex_type_enum e)
cpdef type pytype_from_type_name(str)
cpdef type pytype_from_dtype(object dt)
cdef flex_type_enum flex_type_from_dtype(object dt)
cpdef type pytype_from_array_typecode(str a)

cpdef type infer_type_of_list(list l)
cpdef type infer_type_of_sequence(object t)

#/**************************************************************************/
#/*                                                                        */
#/*            Converting from python objects to flexible_type             */
#/*                                                                        */
#/**************************************************************************/
cdef flexible_type flexible_type_from_pyobject(object) except *
cdef flexible_type flexible_type_from_pyobject_hint(object, type) except *
cdef gl_options_map gl_options_map_from_pydict(dict) except *
cdef flex_list common_typed_flex_list_from_iterable(object, flex_type_enum* common_type) except *
cdef flex_list flex_list_from_iterable(object) except *
cdef flex_list flex_list_from_typed_iterable(object, flex_type_enum t, bint ignore_cast_failure) except *
cdef process_common_typed_list(flexible_type* out_ptr, list v, flex_type_enum common_type)

#/**************************************************************************/
#/*                                                                        */
#/*            Converting from flexible_type to python objects             */
#/*                                                                        */
#/**************************************************************************/
cdef object pyobject_from_flexible_type(const flexible_type&)
cdef list   pylist_from_flex_list(const flex_list&)
cdef dict   pydict_from_gl_options_map(const gl_options_map&)

# In some cases, a numeric list can be converted to a vector.  This
# function checks for that case, and converts the underlying type as
# needed.
cdef check_list_to_vector_translation(flexible_type& v)

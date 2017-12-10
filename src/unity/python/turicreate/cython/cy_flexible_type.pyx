# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
There are essentially three tasks that need to be done in flexible_type translation;
each of these are implemented in this file.

1. Type inference.  This simply determines the type.

2. Type inference while translating.  Potential casts / re-encodings are
done as needed to ensure a consistent type at the end.

3. Translation, possibly with a predefined type.

################################################################################

Consistent rules for type inference when mixed types or numeric lists are also
implemented.  There is generally a one-to-one correspondence between some python
types and each flexible type, with the exception of a numeric list which is
treated as a vector.  When types are mixed, then the following 5 rules govern
the resolution:

Rule 1: Integer can be upgraded to a float.
Rule 2: A numeric list and a vector together make a vector
Rule 3: None does not affect the type.  I.e. [t1,t2,None, t3] has the same
        infered type as [t1, t2, t3]
Rule 4: If a list or vector is present, an empty list does not
        change the infered type.
Rule 5: A vector can be upgraded to a list.

(Note -- these rules were not implemented prior to this pull request.)

################################################################################

Type casting -- now, if a type is specified (e.g. as the constructor to an SArray),
the same inference rules apply as the astype() method.

################################################################################

Examples of type inference when a common type is expected.

The defintions used are:

IntegerValue: int, long, bool, any numpy integer/boolean type.
FloatValue: float, any numpy floating point type.
StringValue: str, unicode, any numpy string/unicode type.
DictValue: dict.
DatetimeValue: date or datetime.
AnyValue: Any of the above values.
FloatSequence: List/tuple/iterable of floats, non-integer array, any numpy float array.
FloatSequenceWithNAN: Same as above, but contains a NAN.
FloatSequenceWithNone: Same as above, but contains a None.
IntegerSequence: List/tuple/iterable of floats, integer array, any numpy integer array.
IntegerSequenceWithNAN: Same as above, but contains a NAN.
IntegerSequenceWithNone: Same as above, but contains a None.
EmptyFloatArray: Empty python / numpy array with float type.
EmptyIntegerArray: Empty python / numpy array with integer  type.
EmptyArray: Either of the above.
EmptySequence: Empty list or tuple.
BooleanSequence: List/tuple of bools, boolean typed numpy array.
StringSequence: List/tuple of strings or unicode; numpy array of str/unicode.
AnySequence: Any of the above.

[IntegerValue]                        --> int
[IntegerValue, IntegerValue]          --> int
[IntegerValue, FloatValue]            --> float
[IntegerValue, nan]                   --> float
[]                                    --> int
[None]                                --> int
[IntegerValue, nan]                   --> float
[IntegerValue, None, nan]             --> float
[IntegerValue, None, FloatValue]      --> float
[IntegerValue, None, FloatValue, nan] --> float
[StringValue]                         --> str
[StringValue, StringValue]            --> str
[StringValue, IntegerValue]           --> NoneType
[StringValue, FloatValue]             --> NoneType
[DictValue]                           --> dict
[DictValue, DictValue]                --> dict
[AnySequence, AnyValue]               --> NoneType
[AnySequence, AnyValue, AnySequence]  --> NoneType
[AnySequence, AnyValue, AnyValue]     --> NoneType
[DatetimeValue, StringValue]          --> NoneType
[DatetimeValue, IntegerValue]         --> NoneType
[DatetimeValue, FloatValue]           --> NoneType
[EmptySequence]                       --> list
[IntegerSequence]                     --> array
[IntegerSequenceWithNone]             --> list
[IntegerSequenceWithNAN]              --> array
[FloatSequence]                       --> array
[FloatSequenceWithNAN]                --> array
[FloatSequenceWithNone]               --> list
[EmptyIntegerArray]                   --> array
[EmptyFloatArray]                     --> array
[BooleanSequence]                     --> array
[StringSequence]                      --> list
[IntegerSequence, FloatSequence]      --> array
[IntegerSequence, FloatSequence]      --> array
[EmptySequence, EmptyFloatArray]      --> array
[EmptySequence, EmptyIntegerArray]    --> array
[EmptySequence, IntegerSequence]      --> array
[EmptySequence, FloatSequence]        --> array
[EmptySequence, EmptyFloatArray]      --> array
[EmptySequence, EmptyIntegerArray]    --> array
[EmptySequence, IntegerSequence]      --> array
[EmptySequence, FloatSequence]        --> array
[StringSequence, EmptyFloatArray]     --> list
[StringSequence, EmptyIntegerArray]   --> list
[StringSequence, IntegerSequence]     --> list
[StringSequence, FloatSequence]       --> list

(Note, Adding a None in any of these does not change it.)
'''

DEF DEBUG_MODE = True

# Turn off a couple things in the code that we don't need here, for
# performance reasons.

#!python
#cython: boundscheck=False
#cython: always_allow_keywords=False
#cython: c_string_encoding='ascii'
#cython: wraparound=False

cimport cython
from cython.operator cimport dereference as deref
from cython.operator cimport preincrement as inc
from libcpp cimport bool as cbool
from cpython cimport array

from cy_cpp_utils cimport str_to_cpp, cpp_to_str, unsafe_str_to_cpp, unsafe_unicode_to_cpp

from cpython.ref cimport PyObject, PyTypeObject
import itertools
import datetime
import calendar
import collections
import types
import decimal

from libc.stdint cimport int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t

ctypedef fused _numeric:
    int8_t
    uint8_t
    int16_t
    uint16_t
    int32_t
    uint32_t
    int64_t
    uint64_t
    double
    float

ctypedef unsigned char uchar
ctypedef unsigned int uint
ctypedef unsigned long ulong
ctypedef long long longlong
ctypedef unsigned long long ulonglong
ctypedef PyObject* object_ptr

# For fast checking of, e.g. numpy buffers or the like
cdef extern from "Python.h":
    int PyObject_Hash(PyObject*)
    longlong PyObject_CheckBuffer(object)

cdef extern from "math.h":
    double NAN

###### Date time stuff
DEF _NUM_FLEX_TYPES = 9

from datetime import tzinfo
from datetime import timedelta

class GMT(tzinfo):
    __slots__ = ['offset']

    def __init__(self,ofs=None):
        if(ofs is None):
          self.offset = 0;
        else:
          self.offset = ofs
    def utcoffset(self, dt):
        return timedelta(minutes=self.offset * 60)
    def dst(self, dt):
        return timedelta(seconds=0)
    def tzname(self,dt):
        if(self.offset >= 0):
            return "GMT +"+str(self.offset)
        elif(self.offset < 0):
            return "GMT "+str(self.offset)
    def __str__(self):
        return self.tzname(self.offset)
    def  __repr__(self):
        return self.tzname(self.offset)


################################################################################
# Some specific types require specific handling between python 2 and
# python 3, namely string and unicode types.

from cpython.version cimport PY_MAJOR_VERSION

cdef bint is_python_3 = (PY_MAJOR_VERSION >= 3)

cdef type xrange_type, array_type, datetime_type, none_type
cdef type decimal_type, timedelta_type

if is_python_3:
    xrange_type             = range
else:
    xrange_type             = types.XRangeType

array_type     = array.array
datetime_type  = datetime.datetime
none_type      = type(None)
decimal_type   = decimal.Decimal
timedelta_type = datetime.timedelta



################################################################################
# In some contexts, we need to be able to use the

cdef object _image_type
cdef bint have_imagetype

class __bad_image(object):
    def __init__(*args, **kwargs):
        raise TypeError("Image type not supported outside of full sframe/turicreate package.")

try:
    from ..data_structures import image
    _image_type = image.Image
    have_imagetype = True
except Exception:  # relative import can raise ValueError or
                   #ImportError, so catch anything
    have_imagetype = False
    _image_type = __bad_image


# Get the appropriate type mappings of things
DEF FT_INT_TYPE       = 0
DEF FT_FLOAT_TYPE     = 1
DEF FT_STR_TYPE       = 2
DEF FT_UNICODE_TYPE   = 3
DEF FT_LIST_TYPE      = 4
DEF FT_TUPLE_TYPE     = 5
DEF FT_DICT_TYPE      = 6
DEF FT_BUFFER_TYPE    = 7
DEF FT_ARRAY_TYPE     = 8
DEF FT_NONE_TYPE      = 9
DEF FT_DATETIME_TYPE  = 10
DEF FT_IMAGE_TYPE     = 11

# The robust versions of the previous ones, which perform extra checks
# and possible casting.  The robust versions of the above are FT_SAFE
# plus the previous value.
DEF FT_SAFE = 12
DEF FT_LARGEST = 2*FT_SAFE
DEF FT_FAILURE = 2*FT_LARGEST + 1

cdef map[object_ptr, int] _code_by_type_lookup = map[object_ptr, int]()

# Ids in this case are known to be unique
_code_by_type_lookup[<object_ptr>(dict)]                = FT_DICT_TYPE
_code_by_type_lookup[<object_ptr>(float)]               = FT_FLOAT_TYPE
_code_by_type_lookup[<object_ptr>(types.GeneratorType)] = FT_LIST_TYPE + FT_SAFE
_code_by_type_lookup[<object_ptr>(int)]                 = FT_INT_TYPE
_code_by_type_lookup[<object_ptr>(bool)]                = FT_INT_TYPE  + FT_SAFE
_code_by_type_lookup[<object_ptr>(list)]                = FT_LIST_TYPE
_code_by_type_lookup[<object_ptr>(long)]                = FT_INT_TYPE  + FT_SAFE
_code_by_type_lookup[<object_ptr>(none_type)]           = FT_NONE_TYPE
_code_by_type_lookup[<object_ptr>(str)]                 = FT_STR_TYPE
_code_by_type_lookup[<object_ptr>(tuple)]               = FT_TUPLE_TYPE
_code_by_type_lookup[<object_ptr>(unicode)]             = FT_UNICODE_TYPE
_code_by_type_lookup[<object_ptr>(array_type)]          = FT_ARRAY_TYPE
_code_by_type_lookup[<object_ptr>(xrange_type)]         = FT_LIST_TYPE + FT_SAFE
_code_by_type_lookup[<object_ptr>(datetime_type)]       = FT_DATETIME_TYPE
_code_by_type_lookup[<object_ptr>(_image_type)]         = FT_IMAGE_TYPE

cdef map[object_ptr, int] _code_by_map_force = map[object_ptr, int]()

_code_by_map_force[<object_ptr>(int)]           = FT_INT_TYPE       + FT_SAFE
_code_by_map_force[<object_ptr>(long)]          = FT_INT_TYPE       + FT_SAFE
_code_by_map_force[<object_ptr>(float)]         = FT_FLOAT_TYPE     + FT_SAFE
_code_by_map_force[<object_ptr>(str)]           = FT_STR_TYPE       + FT_SAFE
_code_by_map_force[<object_ptr>(array_type)]    = FT_ARRAY_TYPE     + FT_SAFE
_code_by_map_force[<object_ptr>(list)]          = FT_LIST_TYPE      + FT_SAFE
_code_by_map_force[<object_ptr>(dict)]          = FT_DICT_TYPE      + FT_SAFE
_code_by_map_force[<object_ptr>(datetime_type)] = FT_DATETIME_TYPE  + FT_SAFE
_code_by_map_force[<object_ptr>(none_type)]     = FT_NONE_TYPE
_code_by_map_force[<object_ptr>(_image_type)]   = FT_IMAGE_TYPE     + FT_SAFE

cdef dict _code_by_name_lookup = {
    'str'      : FT_STR_TYPE     + FT_SAFE,
    'str_'     : FT_STR_TYPE     + FT_SAFE,
    'string'   : FT_STR_TYPE     + FT_SAFE,
    'string_'  : FT_STR_TYPE     + FT_SAFE,
    'bytes'    : FT_STR_TYPE     + FT_SAFE,
    'bytes_'   : FT_STR_TYPE     + FT_SAFE,
    'unicode'  : FT_UNICODE_TYPE,
    'unicode_' : FT_UNICODE_TYPE + FT_SAFE,
    'int'      : FT_INT_TYPE     + FT_SAFE,
    'int_'     : FT_INT_TYPE     + FT_SAFE,
    'long'     : FT_INT_TYPE     + FT_SAFE,
    'long_'    : FT_INT_TYPE     + FT_SAFE,
    'bool'     : FT_INT_TYPE     + FT_SAFE,
    'bool_'    : FT_INT_TYPE     + FT_SAFE,
    'int8'     : FT_INT_TYPE     + FT_SAFE,
    'int16'    : FT_INT_TYPE     + FT_SAFE,
    'int32'    : FT_INT_TYPE     + FT_SAFE,
    'int64'    : FT_INT_TYPE     + FT_SAFE,
    'int128'   : FT_INT_TYPE     + FT_SAFE,
    'uint'     : FT_INT_TYPE     + FT_SAFE,
    'uint8'    : FT_INT_TYPE     + FT_SAFE,
    'uint16'   : FT_INT_TYPE     + FT_SAFE,
    'uint32'   : FT_INT_TYPE     + FT_SAFE,
    'uint64'   : FT_INT_TYPE     + FT_SAFE,
    'uint128'  : FT_INT_TYPE     + FT_SAFE,
    'short'    : FT_INT_TYPE     + FT_SAFE,
    'float'    : FT_FLOAT_TYPE   + FT_SAFE,
    'double'   : FT_FLOAT_TYPE   + FT_SAFE,
    'float16'  : FT_FLOAT_TYPE   + FT_SAFE,
    'float32'  : FT_FLOAT_TYPE   + FT_SAFE,
    'float64'  : FT_FLOAT_TYPE   + FT_SAFE,
    'float128' : FT_FLOAT_TYPE   + FT_SAFE,
    'Decimal'  : FT_FLOAT_TYPE   + FT_SAFE,
    'timedelta': FT_FLOAT_TYPE   + FT_SAFE,
    'datetime' : FT_DATETIME_TYPE + FT_SAFE,
    'date'     : FT_DATETIME_TYPE + FT_SAFE,
    'time'     : FT_DATETIME_TYPE + FT_SAFE,
    'datetime64': FT_DATETIME_TYPE + FT_SAFE,
    'Timestamp': FT_DATETIME_TYPE + FT_SAFE,
    'set'      : FT_LIST_TYPE + FT_SAFE,
    'frozenset': FT_LIST_TYPE + FT_SAFE,
    'NaTType'  : FT_NONE_TYPE + FT_SAFE,
    'ndarray'  : FT_BUFFER_TYPE  # Just go by name on this one since it's not always imported
}

################################################################################
# Looking up the translation code

# Secondary one that isn't inlined; adds new types to the map as needed
cdef int _secondary_get_translation_code(type t, object v = None):
    cdef int tr_code
    try:
        tr_code = _code_by_name_lookup[t.__name__]
        _code_by_type_lookup[<object_ptr>t] = tr_code
        return tr_code
    except KeyError:
        pass

    if v is None:
        return FT_FAILURE

    if PyObject_CheckBuffer(v):
        _code_by_type_lookup[<object_ptr>t] = FT_BUFFER_TYPE
        return FT_BUFFER_TYPE

    # If it's iterable, then it can be cast to a list.
    if isinstance(v, collections.Iterable):
        _code_by_type_lookup[<object_ptr>t] = FT_LIST_TYPE + FT_SAFE
        return FT_LIST_TYPE + FT_SAFE

    return FT_FAILURE

# Fast lookups for the last
cdef object_ptr __last_lookup = NULL
cdef int __last_code = FT_FAILURE

cdef inline int get_translation_code(type t, object v = None):
    cdef int code = -1
    cdef object_ptr optr = <object_ptr>t

    global __last_lookup
    global __last_code

    if __last_lookup == optr:
        return __last_code

    cdef map[object_ptr,int].iterator it = _code_by_type_lookup.find(optr)

    if it !=  _code_by_type_lookup.end():
        __last_lookup = optr
        __last_code = deref(it).second
        return __last_code

    return _secondary_get_translation_code(t, v)



################################################################################

cdef map[object_ptr, int] _code_by_forced_type = map[object_ptr, int]()

# These ids are unique
_code_by_forced_type[<object_ptr>(dict)]           = FT_DICT_TYPE     + FT_SAFE
_code_by_forced_type[<object_ptr>(list)]           = FT_LIST_TYPE     + FT_SAFE
_code_by_forced_type[<object_ptr>(none_type)]      = FT_NONE_TYPE     + FT_SAFE
_code_by_forced_type[<object_ptr>(datetime_type)]  = FT_DATETIME_TYPE + FT_SAFE
_code_by_forced_type[<object_ptr>(array_type)]     = FT_BUFFER_TYPE
_code_by_forced_type[<object_ptr>(str)]            = FT_STR_TYPE      + FT_SAFE

################################################################################
# Enum type only.

cdef list _type_lookup_by_type_enum = [None] * (_NUM_FLEX_TYPES)
_type_lookup_by_type_enum[<int>INTEGER]   = int
_type_lookup_by_type_enum[<int>FLOAT]     = float
_type_lookup_by_type_enum[<int>STRING]    = str
_type_lookup_by_type_enum[<int>VECTOR]    = array_type
_type_lookup_by_type_enum[<int>LIST]      = list
_type_lookup_by_type_enum[<int>DICT]      = dict
_type_lookup_by_type_enum[<int>DATETIME]  = datetime_type
_type_lookup_by_type_enum[<int>UNDEFINED] = none_type
_type_lookup_by_type_enum[<int>IMAGE]     = _image_type

cdef type pytype_from_flex_type_enum(flex_type_enum e):
    return _type_lookup_by_type_enum[<int> e]

################################################################################
# Name of enum type only

cdef dict _type_lookup_by_type_enum_name = {
    "integer"    : int,
    "datetime"   : datetime_type,
    "dictionary" : dict,
    "float"      : float,
    "string"     : str,
    "array"      : array_type,
    "list"       : list,
    "image"      : _image_type,
    "undefined"  : none_type}
    
# Also add in the names of each of the types in order to recognize
# them as well.
for _t in list(_type_lookup_by_type_enum_name.values()):
    _type_lookup_by_type_enum_name[_t.__name__.lower()] = _t
    
cpdef type pytype_from_type_name(str s):
    global _type_lookup_by_type_enum_name
    try:
        return _type_lookup_by_type_enum_name[s.lower()]
    except KeyError:
        raise ValueError("'%s' not a recogizable type name; valid names are %s."
                         % (s, ','.join(sorted(set(_type_lookup_by_type_enum_name.keys())))))


################################################################################
# Looking up the translation code to enum type

cdef vector[flex_type_enum] _enum_tr_codes = vector[flex_type_enum](FT_FAILURE + 1)

_enum_tr_codes[FT_INT_TYPE]                = INTEGER
_enum_tr_codes[FT_FLOAT_TYPE]              = FLOAT
_enum_tr_codes[FT_STR_TYPE]                = STRING
_enum_tr_codes[FT_UNICODE_TYPE]            = STRING
_enum_tr_codes[FT_LIST_TYPE]               = LIST
_enum_tr_codes[FT_TUPLE_TYPE]              = LIST
_enum_tr_codes[FT_DICT_TYPE]               = DICT
_enum_tr_codes[FT_BUFFER_TYPE]             = VECTOR
_enum_tr_codes[FT_ARRAY_TYPE]              = VECTOR
_enum_tr_codes[FT_NONE_TYPE]               = UNDEFINED
_enum_tr_codes[FT_DATETIME_TYPE]           = DATETIME
_enum_tr_codes[FT_IMAGE_TYPE]              = IMAGE
_enum_tr_codes[FT_SAFE + FT_INT_TYPE]      = INTEGER
_enum_tr_codes[FT_SAFE + FT_FLOAT_TYPE]    = FLOAT
_enum_tr_codes[FT_SAFE + FT_STR_TYPE]      = STRING
_enum_tr_codes[FT_SAFE + FT_UNICODE_TYPE]  = STRING
_enum_tr_codes[FT_SAFE + FT_LIST_TYPE]     = LIST
_enum_tr_codes[FT_SAFE + FT_TUPLE_TYPE]    = LIST
_enum_tr_codes[FT_SAFE + FT_DICT_TYPE]     = DICT
_enum_tr_codes[FT_SAFE + FT_BUFFER_TYPE]   = VECTOR
_enum_tr_codes[FT_SAFE + FT_ARRAY_TYPE]    = VECTOR
_enum_tr_codes[FT_SAFE + FT_NONE_TYPE]     = UNDEFINED
_enum_tr_codes[FT_SAFE + FT_DATETIME_TYPE] = DATETIME
_enum_tr_codes[FT_SAFE + FT_IMAGE_TYPE]    = IMAGE
_enum_tr_codes[FT_FAILURE]                 = UNDEFINED

cdef inline flex_type_enum flex_type_from_tr_code(int tr_code):
    return _enum_tr_codes[tr_code]

cdef inline flex_type_enum flex_type_enum_from_pytype(type t) except *:
    """
    Given a type, returns the flex_type_enum associated with that type.
    """
    return flex_type_from_tr_code(get_translation_code(t))

################################################################################
# Controlling whether a list element can be part of an implicit
# translation of the list to a vector

cdef vector[bint] __ft_is_vector_implicit_castable = vector[bint](_NUM_FLEX_TYPES, False)

# A list will be interpreted as a flex_vec only if all the elements are floats or ints.
__ft_is_vector_implicit_castable[<int>FLOAT]     = True
__ft_is_vector_implicit_castable[<int>INTEGER]   = True
__ft_is_vector_implicit_castable[<int>UNDEFINED] = False

cdef inline bint flex_type_is_vector_implicit_castable(flex_type_enum ft_type):
    return __ft_is_vector_implicit_castable[<int>ft_type]

################################################################################

cdef bint HAS_NUMPY = False
cdef bint HAS_PANDAS = False

try:
    import numpy as np
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False

try:
    import pandas as pd
    HAS_PANDAS = True
except ImportError:
    HAS_PANDAS = False

cdef vector[flex_type_enum] _dtype_to_flex_enum_lookup = vector[flex_type_enum](128, UNDEFINED)

cdef __fill_numpy_info():
    cdef dict __typecodes = np.typecodes

    for c in <str>(__typecodes.get("AllFloat", "")):
        _dtype_to_flex_enum_lookup[<int>(ord(c))] = FLOAT
    for c in <str>(__typecodes.get("AllInteger", "")):
        _dtype_to_flex_enum_lookup[<int>(ord(c))] = INTEGER
    for c in <str>(__typecodes.get("Character", "") + "SU"):
        _dtype_to_flex_enum_lookup[<int>(ord(c))] = STRING

cdef type np_ndarray
if HAS_NUMPY:
    np_ndarray = np.ndarray
    __fill_numpy_info()

cdef flex_type_enum flex_type_from_dtype(object dt):
    cdef flex_type_enum ft_type
    cdef int dt_code = ord(dt.char)

    if dt_code >= _dtype_to_flex_enum_lookup.size():
        ft_type = UNDEFINED
    else:
        ft_type = _dtype_to_flex_enum_lookup[dt_code]

    # An annoying special case.
    if ft_type == UNDEFINED and dt == bool:
        ft_type = INTEGER

    return ft_type

cpdef type pytype_from_dtype(object dt):
    if not HAS_NUMPY:
        return object

    assert isinstance(dt, np.dtype)

    cdef flex_type_enum ft_type = flex_type_from_dtype(dt)

    if ft_type == UNDEFINED:
        return object
    else:
        return pytype_from_flex_type_enum(ft_type)

################################################################################
# Translate from array types

cdef vector[flex_type_enum] _typecode_to_flex_enum_lookup = vector[flex_type_enum](128, UNDEFINED)

# capture the integers -- these are the defined type characters for integers in array.array.
for c in 'cbBhHiIlL':
    _typecode_to_flex_enum_lookup[<int>(ord(c))] = INTEGER

# capture the floats
for c in 'fd':
    _typecode_to_flex_enum_lookup[<int>(ord(c))] = FLOAT

# Choose the flexible type based on the array typecode from array.array
cdef flex_type_enum flex_type_from_array_typecode(str type_code) except *:
    if len(type_code) != 1:
        raise ValueError("Type '%s' does not appear to be a valid array type code." % type_code)

    cdef int t_code = ord(type_code)

    if t_code >= _typecode_to_flex_enum_lookup.size():
        ft_type = UNDEFINED
    else:
        ft_type = _typecode_to_flex_enum_lookup[t_code]

    if ft_type == UNDEFINED:
        raise ValueError("Type '%s' does not appear to be a valid array type code." % type_code)

    return ft_type

cpdef type pytype_from_array_typecode(str type_code):
    return pytype_from_flex_type_enum(flex_type_from_array_typecode(type_code))

################################################################################
#
#  Type inference.  Basically, these are all methods to consistently
#  enforce that numeric lists are to be treated like arrays.
#
################################################################################

cdef inline bint _type_is_vector_

ctypedef fused _listlike:
    list
    tuple
    object[:]

cdef int _listlike_can_be_vector(_listlike v, vector[int]* tr_code_buffer = NULL):
    cdef int tr_code
    cdef size_t i
    cdef size_t n = len(v)

    if tr_code_buffer != NULL:
        tr_code_buffer[0].assign(n, -1)

    if n == 0:
        return False

    cdef type first_type = type(v[0])
    tr_code = get_translation_code(first_type, v[0])

    if tr_code_buffer != NULL:
        tr_code_buffer[0][0] = tr_code

    if not flex_type_is_vector_implicit_castable(flex_type_from_tr_code(tr_code)):
        return False

    cdef type t

    for i in range(1, n):
        t = type(v[i])

        if t is first_type:
            if tr_code_buffer != NULL:
                tr_code_buffer[0][i] = tr_code_buffer[0][0]
            continue

        tr_code = get_translation_code(type(v[i]), v[i])

        if tr_code_buffer != NULL:
            tr_code_buffer[0][i] = tr_code

        if not flex_type_is_vector_implicit_castable(flex_type_from_tr_code(tr_code)):
            return False

    return True

cdef inline bint _flex_list_can_be_vector(const flex_list& v):
    cdef size_t i
    cdef size_t n = v.size()

    for i in range(n):
        if not flex_type_is_vector_implicit_castable(v[i].get_type()):
            return False

    return True


cdef check_list_to_vector_translation(flexible_type& v):
    """
    If a numerical list, translates it to a vector.
    """

    cdef flexible_type alt_v

    if (v.get_type() == LIST
        and v.get_list().size() != 0
        and _flex_list_can_be_vector(v.get_list())):

        alt_v = flexible_type(VECTOR)
        try:
            alt_v.soft_assign(v)
        except:
            assert False, "Cannot convert flexible_type to vector"

        swap(alt_v, v)


################################################################################
# Common type resolution utilities

# Codes for inference in finding the common type of a list
DEF FTI_INTEGER        = 1
DEF FTI_FLOAT          = 2
DEF FTI_STRING         = 4
DEF FTI_VECTOR         = 8
DEF FTI_LIST           = 16
DEF FTI_DICT           = 32
DEF FTI_DATETIME       = 64
DEF FTI_NONE           = 128
DEF FTI_IMAGE          = 256

# additional things that are handled specially
DEF FTI_NUMERIC_LIST   = 512
DEF FTI_EMPTY_LIST     = 1024

cdef map[size_t, flex_type_enum] _common_type_inference_rules = map[size_t, flex_type_enum]()

# The empty case
_common_type_inference_rules[0]                = FLOAT

# Standalone types
_common_type_inference_rules[FTI_INTEGER]      = INTEGER
_common_type_inference_rules[FTI_FLOAT]        = FLOAT
_common_type_inference_rules[FTI_STRING]       = STRING
_common_type_inference_rules[FTI_VECTOR]       = VECTOR
_common_type_inference_rules[FTI_LIST]         = LIST
_common_type_inference_rules[FTI_DICT]         = DICT
_common_type_inference_rules[FTI_DATETIME]     = DATETIME
_common_type_inference_rules[FTI_NONE]         = INTEGER
_common_type_inference_rules[FTI_IMAGE]        = IMAGE
_common_type_inference_rules[FTI_NUMERIC_LIST] = VECTOR
_common_type_inference_rules[FTI_EMPTY_LIST]   = LIST

# Upgrade rules:

# Rule 1: Integer can be upgraded to a float.
_common_type_inference_rules[FTI_INTEGER | FTI_FLOAT]  = FLOAT

# Rule 2: A numeric list and a vector together make a vector
_common_type_inference_rules[FTI_NUMERIC_LIST | FTI_VECTOR] = VECTOR

# Rule 3: None does not affect things.
cdef map[size_t, flex_type_enum].iterator _it
_it = _common_type_inference_rules.begin()
while _it != _common_type_inference_rules.end():
    _common_type_inference_rules[deref(_it).first | FTI_NONE] = deref(_it).second
    inc(_it)

# Rule 4: If a list or vector is present, an empty list does not change things.
cdef size_t _k
_it = _common_type_inference_rules.begin()
while _it != _common_type_inference_rules.end():
    _k = deref(_it).first
    if (_k & (FTI_VECTOR | FTI_NUMERIC_LIST | FTI_LIST)) != 0:
        _common_type_inference_rules[_k | FTI_EMPTY_LIST] = deref(_it).second
    inc(_it)

# Rule 5: A list with any of a vector, numeric list, empty list, etc. makes a list.
_it = _common_type_inference_rules.begin()
while _it != _common_type_inference_rules.end():
    _k = deref(_it).first
    if (_k & (FTI_VECTOR | FTI_NUMERIC_LIST | FTI_EMPTY_LIST)) != 0:
        _common_type_inference_rules[_k | FTI_LIST] = LIST
    inc(_it)


################################################################################
#
#  Choosing the inference code
#
################################################################################

# Choosing it from the translation code
cdef vector[size_t] _inference_code_from_tr_code = vector[size_t](FT_FAILURE + 1)

_inference_code_from_tr_code[FT_INT_TYPE]                = FTI_INTEGER
_inference_code_from_tr_code[FT_FLOAT_TYPE]              = FTI_FLOAT
_inference_code_from_tr_code[FT_STR_TYPE]                = FTI_STRING
_inference_code_from_tr_code[FT_UNICODE_TYPE]            = FTI_STRING
_inference_code_from_tr_code[FT_LIST_TYPE]               = 0 # More needed
_inference_code_from_tr_code[FT_TUPLE_TYPE]              = 0
_inference_code_from_tr_code[FT_DICT_TYPE]               = FTI_DICT
_inference_code_from_tr_code[FT_BUFFER_TYPE]             = 0
_inference_code_from_tr_code[FT_ARRAY_TYPE]              = FTI_VECTOR
_inference_code_from_tr_code[FT_NONE_TYPE]               = FTI_NONE
_inference_code_from_tr_code[FT_DATETIME_TYPE]           = FTI_DATETIME
_inference_code_from_tr_code[FT_IMAGE_TYPE]              = FTI_IMAGE
_inference_code_from_tr_code[FT_SAFE + FT_INT_TYPE]      = 0
_inference_code_from_tr_code[FT_SAFE + FT_FLOAT_TYPE]    = FTI_FLOAT
_inference_code_from_tr_code[FT_SAFE + FT_STR_TYPE]      = FTI_STRING
_inference_code_from_tr_code[FT_SAFE + FT_UNICODE_TYPE]  = FTI_STRING
_inference_code_from_tr_code[FT_SAFE + FT_LIST_TYPE]     = 0
_inference_code_from_tr_code[FT_SAFE + FT_TUPLE_TYPE]    = 0
_inference_code_from_tr_code[FT_SAFE + FT_DICT_TYPE]     = FTI_DICT
_inference_code_from_tr_code[FT_SAFE + FT_BUFFER_TYPE]   = 0
_inference_code_from_tr_code[FT_SAFE + FT_ARRAY_TYPE]    = FTI_VECTOR
_inference_code_from_tr_code[FT_SAFE + FT_NONE_TYPE]     = FTI_NONE
_inference_code_from_tr_code[FT_SAFE + FT_DATETIME_TYPE] = FTI_DATETIME
_inference_code_from_tr_code[FT_SAFE + FT_IMAGE_TYPE]    = FTI_IMAGE
_inference_code_from_tr_code[FT_FAILURE]                 = <size_t>(-1)

# Choosing it from the flexible type code
cdef vector[size_t] _inference_code_from_flex_type_enum = vector[size_t](_NUM_FLEX_TYPES)

_inference_code_from_flex_type_enum[<int>INTEGER]   = FTI_INTEGER
_inference_code_from_flex_type_enum[<int>FLOAT]     = FTI_FLOAT
_inference_code_from_flex_type_enum[<int>STRING]    = FTI_STRING
_inference_code_from_flex_type_enum[<int>LIST]      = 0 # More work needed
_inference_code_from_flex_type_enum[<int>VECTOR]    = FTI_VECTOR
_inference_code_from_flex_type_enum[<int>DICT]      = FTI_DICT
_inference_code_from_flex_type_enum[<int>IMAGE]     = FTI_IMAGE
_inference_code_from_flex_type_enum[<int>DATETIME]  = FTI_DATETIME
_inference_code_from_flex_type_enum[<int>UNDEFINED] = FTI_NONE

cdef size_t _choose_inference_code(int tr_code, object v) except -2:

    cdef size_t _infer_code = _inference_code_from_tr_code[tr_code]
    cdef flex_type_enum ft_type
    cdef bint is_object_buffer = False

    if _infer_code == (<size_t>(-1)):
        raise TypeError("Cannot convert type '" + type(v).__name__ + "' into flexible type.")

    if _infer_code != 0:
        return _infer_code

    # Handle doubles and floats
    if tr_code == (FT_INT_TYPE + FT_SAFE):
        if v < -9223372036854775808 or v > 9223372036854775807:
            return FTI_FLOAT
        else:
            return FTI_INTEGER

    # Handle safe casts of tuples and lists.
    elif tr_code == (FT_LIST_TYPE + FT_SAFE) and type(v) is not list:
        v = list(v)
        tr_code = FT_LIST_TYPE
    elif tr_code == (FT_TUPLE_TYPE + FT_SAFE) and type(v) is not tuple:
        v = tuple(v)
        tr_code = FT_TUPLE_TYPE

    # resolve the remaining issues
    if tr_code == FT_LIST_TYPE:
        if len(<list>v) == 0:
            return FTI_EMPTY_LIST
        elif _listlike_can_be_vector(<list>v):
            return FTI_NUMERIC_LIST
        else:
            return FTI_LIST
    elif tr_code == FT_TUPLE_TYPE:
        if len(<tuple>v) == 0:
            return FTI_EMPTY_LIST
        elif _listlike_can_be_vector(<tuple>v):
            return FTI_NUMERIC_LIST
        else:
            return FTI_LIST
    elif tr_code == FT_BUFFER_TYPE or (tr_code == FT_BUFFER_TYPE + FT_SAFE):
        ft_type = _infer_buffer_element_type(v, False, False, &is_object_buffer)
        if is_object_buffer:
            if len(v) == 0:
                return FTI_EMPTY_LIST
            elif flex_type_is_vector_implicit_castable(ft_type):
                return FTI_NUMERIC_LIST
            else:
                return FTI_LIST
        else:
            if flex_type_is_vector_implicit_castable(ft_type):
                return FTI_VECTOR
            else:
                return FTI_LIST

cdef size_t _choose_inference_code_from_flexible_type(flexible_type ft) except -2:

    cdef size_t _infer_code = _inference_code_from_flex_type_enum[<int>ft.get_type()]
    cdef flex_type_enum ft_type
    cdef size_t i

    if _infer_code != 0:
        return _infer_code

    if ft.get_type() == LIST:
        if ft.get_list().size() == 0:
            return FTI_EMPTY_LIST
        elif _flex_list_can_be_vector(ft.get_list()):
            return FTI_NUMERIC_LIST
        else:
            return FTI_LIST
    else:
        assert False

# The logic to determine the available types
cdef inline flex_type_enum infer_common_type(size_t present_types, bint undefined_on_error = False) except *:

    cdef map[size_t,flex_type_enum].iterator it = _common_type_inference_rules.find(present_types)

    if it == _common_type_inference_rules.end():
        if undefined_on_error:
            return UNDEFINED
        else:
            types = []
            if (present_types & FTI_INTEGER) != 0:
                types.append(flex_type_enum_to_name(INTEGER))
                present_types -= FTI_INTEGER
            if (present_types & FTI_FLOAT) != 0:
                types.append(flex_type_enum_to_name(FLOAT))
                present_types -= FTI_FLOAT
            if (present_types & FTI_STRING) != 0:
                types.append(flex_type_enum_to_name(STRING))
                present_types -= FTI_STRING
            if (present_types & FTI_LIST) != 0:
                types.append(flex_type_enum_to_name(LIST))
                present_types -= FTI_LIST
            if (present_types & FTI_VECTOR) != 0:
                types.append(flex_type_enum_to_name(VECTOR))
                present_types -= FTI_VECTOR
            if (present_types & FTI_DICT) != 0:
                types.append(flex_type_enum_to_name(DICT))
                present_types -= FTI_DICT
            if (present_types & FTI_DATETIME) != 0:
                types.append(flex_type_enum_to_name(DATETIME))
                present_types -= FTI_DATETIME
            if (present_types & FTI_NONE) != 0:
                types.append(flex_type_enum_to_name(UNDEFINED))
                present_types -= FTI_NONE
            if (present_types & FTI_IMAGE) != 0:
                types.append(flex_type_enum_to_name(IMAGE))
                present_types -= FTI_IMAGE
            if (present_types & FTI_EMPTY_LIST) != 0:
                types.append("empty list")
                present_types -= FTI_EMPTY_LIST
            if (present_types & FTI_NUMERIC_LIST) != 0:
                types.append("numeric list")
                present_types -= FTI_NUMERIC_LIST

            assert present_types == 0

            raise TypeError("A common type cannot be infered from types %s."
                            % (", ".join(types)))
    else:
        return deref(it).second


cdef flex_type_enum _infer_common_type_of_listlike(_listlike vl, bint undefined_on_error = False,
                                                   vector[int]* tr_code_buffer = NULL) except *:
    """
    Chooses a common type for a list / tuple / object buffer
    """

    cdef size_t seen_types = 0, tc
    cdef size_t i
    cdef int tr_code

    if tr_code_buffer != NULL:
        tr_code_buffer[0].assign(len(vl), -1)

    for i in range(len(vl)):
        v = vl[i]
        tr_code = get_translation_code(type(v), v)
        tc = _choose_inference_code(tr_code, v)

        seen_types |= tc

        if tr_code_buffer != NULL:
            tr_code_buffer[0][i] = tr_code

    return infer_common_type(seen_types, undefined_on_error)

cdef flex_type_enum infer_common_type_of_flex_list(const flex_list& fl, bint undefined_on_error = False):
    """
    Chooses a common type for a flex_list.
    """

    cdef size_t seen_types = 0

    for i in range(fl.size()):
        seen_types |= _choose_inference_code_from_flexible_type(fl[i])

    return infer_common_type(seen_types, undefined_on_error)

########################################
# Type of buffers

@cython.boundscheck(False)
cdef inline bint __try_buffer_numeric_check(object v, _numeric t):

    cdef _numeric[:] buf

    try:
        buf = v
        return True
    except:
        return False

@cython.boundscheck(False)
cdef inline flex_type_enum _infer_buffer_element_type(
    object v,
    bint expect_common_type = False,
    bint undefined_on_error = False,
    bint *is_object_buffer = NULL) except *:

    if is_object_buffer != NULL:
        is_object_buffer[0] = False

    # Short cut for numpy arrays
    cdef flex_type_enum ft_type
    if HAS_NUMPY and type(v) is np_ndarray:
        ft_type = flex_type_from_dtype(v.dtype)
        if ft_type != UNDEFINED:
            return ft_type

    # Most common cases
    if __try_buffer_numeric_check(v, <double>(0)):   return FLOAT
    if __try_buffer_numeric_check(v, <int64_t>(0)):  return INTEGER

    # object
    cdef object[:] object_buffer
    cdef bint _is_object_buffer = False

    try:
        object_buffer = v
        _is_object_buffer = True
    except:
        _is_object_buffer = False

    if _is_object_buffer:
        if is_object_buffer != NULL:
            is_object_buffer[0] = True

        if expect_common_type:
            return _infer_common_type_of_listlike(object_buffer, undefined_on_error)
        else:
            # See if it has a common type; if so, use that; otherwise return undefined.
            return _infer_common_type_of_listlike(object_buffer, True)

    # Less common cases
    if __try_buffer_numeric_check(v, <float>(0)):    return FLOAT
    if __try_buffer_numeric_check(v, <int8_t>(0)):   return INTEGER
    if __try_buffer_numeric_check(v, <uint8_t>(0)):  return INTEGER
    if __try_buffer_numeric_check(v, <int16_t>(0)):  return INTEGER
    if __try_buffer_numeric_check(v, <uint16_t>(0)): return INTEGER
    if __try_buffer_numeric_check(v, <int32_t>(0)):  return INTEGER
    if __try_buffer_numeric_check(v, <uint32_t>(0)): return INTEGER
    if __try_buffer_numeric_check(v, <int64_t>(0)):  return INTEGER
    if __try_buffer_numeric_check(v, <uint64_t>(0)): return INTEGER

    # If it's a numpy buffer, then we can cast it to a list
    if HAS_NUMPY and isinstance(v, np_ndarray):
        if expect_common_type:
            return _infer_common_type_of_listlike(list(v), undefined_on_error)
        else:
            # See if it has a common type; if so, use that; otherwise return undefined.
            return _infer_common_type_of_listlike(list(v), True)
    else:
        raise TypeError("Buffer type of type '%s' not understood." % (type(v)))

########################################
# General

cdef flex_type_enum infer_flex_type_of_sequence(object l, bint undefined_on_error = False) except *:
    """
    Infer a common type for the elements of a sequence.
    """

    cdef int tr_code = get_translation_code(type(l), l)

    if tr_code == FT_LIST_TYPE:
        return _infer_common_type_of_listlike(<list>l, undefined_on_error)
    elif tr_code == FT_TUPLE_TYPE:
        return _infer_common_type_of_listlike(<tuple>l, undefined_on_error)
    elif tr_code == FT_BUFFER_TYPE:
        return _infer_buffer_element_type(l, True, undefined_on_error)
    elif tr_code == FT_ARRAY_TYPE:
        return flex_type_from_array_typecode( (<array.array>l).typecode)
    elif tr_code == FT_LIST_TYPE + FT_SAFE:
        if type(l) is list:
            return _infer_common_type_of_listlike(<list>l, undefined_on_error)
        else:
            # Could be optimized
            return _infer_common_type_of_listlike(list(l), undefined_on_error)
    elif tr_code == FT_TUPLE_TYPE:
        if type(l) is tuple:
            return _infer_common_type_of_listlike(<tuple>l, undefined_on_error)
        else:
            # Could be optimized
            return _infer_common_type_of_listlike(tuple(l), undefined_on_error)
    elif tr_code == FT_BUFFER_TYPE + FT_SAFE:
        return _infer_buffer_element_type(l, True, undefined_on_error)
    elif tr_code == FT_ARRAY_TYPE + FT_SAFE:
        return flex_type_from_array_typecode( (<array.array>l).typecode)
    else:
        if undefined_on_error:
            return UNDEFINED
        else:
            raise TypeError("Cannot interpret type '" + type(l).__name__ + "' as sequence.")

##################################################
# Interface functions to the world.

cpdef type infer_type_of_list(list l):
    return pytype_from_flex_type_enum(_infer_common_type_of_listlike(l))

cpdef type infer_type_of_sequence(object l):
    return pytype_from_flex_type_enum(infer_flex_type_of_sequence(l))

################################################################################
#
#   Translation functions list or tuple to flex_list / flex_vec
#
################################################################################

@cython.boundscheck(False)
cdef inline fill_list(flex_list& retl, _listlike v,
                      flex_type_enum* common_type = NULL,
                      vector[int]* tr_code_buffer = NULL):
    """
    Fills a list.  If common_type is not NULL, then a common type
    for the list is expected, and the result is stored in common_type[0].
    If tr_code_buffer is not null, then the translation codes are taken from that.
    """

    cdef size_t i
    cdef int tr_code = -1
    cdef size_t seen_types = 0
    cdef flexible_type alt_ft

    if len(v) == 0:
        retl.clear()
        if common_type != NULL:
            common_type[0] = infer_common_type(seen_types)
        return

    retl.resize(len(v))
    for i in range(len(v)):

        if tr_code_buffer != NULL and tr_code_buffer[0][i] != -1: # Avoid unneeded lookups
            tr_code = tr_code_buffer[0][i]
        else:
            tr_code = get_translation_code(type(v[i]), v[i])

        retl[i] = _ft_translate(v[i], tr_code)

        if common_type != NULL:
            seen_types |= _choose_inference_code(tr_code, v[i])
        else:
            # With no common type expected, translate lists to vectors.
            check_list_to_vector_translation(retl[i])

    if(common_type != NULL):
        common_type[0] = infer_common_type(seen_types)

        for i in range(retl.size()):
            if retl[i].get_type() != common_type[0]:

                # None/UNDEFINED gets a free pass.
                if retl[i].get_type() == UNDEFINED:
                    continue

                alt_ft = flexible_type(common_type[0])
                swap(alt_ft, retl[i])
                try:
                    retl[i].soft_assign(alt_ft)
                except:
                    raise TypeError("Error converting type %s to %s."
                                    % (flex_type_enum_to_name(alt_ft.get_type()),
                                       flex_type_enum_to_name(retl[i].get_type())))


@cython.boundscheck(False)
cdef inline long _fill_typed_sequence(flexible_type* retl, _listlike v,
                                 flex_type_enum common_type,
                                 bint ignore_translation_errors) except -1:


    cdef int tr_code = -1
    cdef flexible_type ft

    if len(v) == 0:
        return 0

    cdef size_t i
    cdef size_t write_pos = 0
    cdef bint success
    cdef bint error_occured

    for i in range(len(v)):
        retl[write_pos] = _ft_translate(v[i], get_translation_code(type(v[i]), v[i]))

        if common_type != UNDEFINED and retl[write_pos].get_type() != common_type:

            # UNDEFINED gets a free pass.
            if retl[write_pos].get_type() == UNDEFINED:
                success = True
            else:
                ft = retl[write_pos]
                retl[write_pos] = flexible_type(common_type)

                try:
                    retl[write_pos].soft_assign(ft)
                    success = True
                except:
                    success = False

            if not success:
                if ignore_translation_errors:
                    continue
                else:
                    raise TypeError(
                        "Type " + flex_type_enum_to_name(ft.get_type())
                         + " cannot be cast to type " + flex_type_enum_to_name(common_type))

        write_pos += 1

    return write_pos


@cython.boundscheck(False)
cdef inline fill_typed_list(flex_list& retl, _listlike v,
                            flex_type_enum common_type,
                            bint ignore_translation_errors):
    """
    Fills a list, casting all elements to the common type.  Anything that
    cannot be losslessly translated is ignored.
    """

    cdef size_t out_len = len(v)
    retl.resize(out_len)

    cdef size_t new_size = _fill_typed_sequence(&(retl[0]), v, common_type, ignore_translation_errors)

    if new_size != out_len:
        retl.resize(new_size)



cdef process_common_typed_list(flexible_type* out_ptr, list v, flex_type_enum common_type):
    """
    External wrapper to the list filling function.

    If common_type is UNDEFINED, then no processing is done.
    """
    _fill_typed_sequence(out_ptr, v, common_type, False);


@cython.boundscheck(False)
cdef inline tr_listlike_to_ft(flexible_type& ret, _listlike v, flex_type_enum* common_type = NULL):
    """
    Translates list-like objects into a flex_list.  This may be upgraded
    later on to a vector type if appropriate.
    """

    ret.set_list(flex_list(len(v)))
    fill_list(ret.get_list_m(), v, common_type)


################################################################################
# Translate DateTime type

cdef inline tr_datetime_to_ft(flexible_type& ret, v):
    # Restriction from Boost.Date_Time because calculations are inaccurate
    # before the introduction of the Gregorian calendar
    if(v.year < 1400 or v.year > 10000):
        raise TypeError('Year is out of valid range: 1400..10000')
    if(v.tzinfo is not None):
        offset = int(v.tzinfo.utcoffset(v).total_seconds() / TIMEZONE_RESOLUTION_IN_SECONDS) #store timezone offset at the granularity of half an hour.
        ret.set_date_time((<long long>(calendar.timegm(v.utctimetuple())),offset), v.microsecond)
    else:
        ret.set_date_time((<long long>(calendar.timegm(v.utctimetuple())),EMPTY_TIMEZONE), v.microsecond)


cdef inline tr_datetime64_to_ft(flexible_type& ret, v):
    # Since flexible type datetime only goes down to microseconds, convert to
    # this. If higher resolution, this will truncate values
    cdef object as_py_datetime = v.astype('M8[us]').astype('O')
    if as_py_datetime is not None:
        as_py_datetime = as_py_datetime.replace(tzinfo=GMT(0))
        tr_datetime_to_ft(ret, as_py_datetime)
    # else, ret stays as FLEX_UNDEFINED


################################################################################
# Translate Dictionary type

cdef tr_dict_to_ft(flexible_type& ret, dict d):
    cdef flex_dict _ft_dict
    _ft_dict.resize(len(d))
    cdef size_t i = 0

    for k, v in d.iteritems():
        _ft_dict[i].first = flexible_type_from_pyobject(k)
        _ft_dict[i].second = flexible_type_from_pyobject(v)
        i += 1

    ret.set_dict(_ft_dict)

################################################################################
# Translate image type

cdef inline translate_image(flexible_type& ret, object v):
    """ Convert a python value v to flex image """
    cdef flex_image ret_i

    ret_i = flex_image(v._image_data, v._height, v._width, v._channels,
                       v._image_data_size, <char> v._version, v._format_enum)

    ret.set_img(ret_i)


################################################################################
# Buffer translation

@cython.boundscheck(False)
cdef inline bint __try_buffer_type_vec(flex_vec& retv, object v, _numeric t):

    cdef _numeric[:] buf

    try:
        buf = v
    except:
        return False

    cdef size_t i
    retv.resize(len(buf))
    for i in range(len(buf)):
        retv[i] = <flex_float>(buf[i])
    return True

@cython.boundscheck(False)
cdef inline bint _tr_buffer_to_flex_vec(flex_vec& retv, object v):
    cdef flex_type_enum ft_type

    if HAS_NUMPY and type(v) is np_ndarray:
        dt = v.dtype
        if dt == np.bool:
            v = np.asarray(v, dtype = np.uint8)
            if not __try_buffer_type_vec(retv, v, <uint8_t>(0)):
                assert False

        ft_type = flex_type_from_dtype(dt)

        if ft_type != INTEGER and ft_type != FLOAT:
            return False

    if __try_buffer_type_vec(retv, v, <double>(0)):   return True
    if __try_buffer_type_vec(retv, v, <int64_t>(0)):  return True
    if __try_buffer_type_vec(retv, v, <float>(0)):    return True
    if __try_buffer_type_vec(retv, v, <int8_t>(0)):   return True
    if __try_buffer_type_vec(retv, v, <uint8_t>(0)):  return True
    if __try_buffer_type_vec(retv, v, <int16_t>(0)):  return True
    if __try_buffer_type_vec(retv, v, <uint16_t>(0)): return True
    if __try_buffer_type_vec(retv, v, <int32_t>(0)):  return True
    if __try_buffer_type_vec(retv, v, <uint32_t>(0)): return True
    if __try_buffer_type_vec(retv, v, <uint64_t>(0)): return True

    return False

################################################################################

cdef inline tr_buffer_to_ft(flexible_type& ret, object v, flex_type_enum* common_type = NULL):

    cdef flex_vec _ft_vec

    # this handles the common cases.
    if _tr_buffer_to_flex_vec(_ft_vec, v):
        ret.set_vec(_ft_vec)
        if common_type != NULL:
            common_type[0] = FLOAT
        return

    # object
    cdef object[:] object_buffer
    cdef bint is_object_buffer = False

    try:
        object_buffer = v
        is_object_buffer = True
    except:
        pass

    if is_object_buffer:
        tr_listlike_to_ft(ret, object_buffer, common_type)
        return

    # if it's a numpy array, then do something special
    if HAS_NUMPY and isinstance(v, np_ndarray):
        tr_listlike_to_ft(ret, list(v), common_type)
        return

    # Error if there are no more options.
    raise TypeError("Could not convert python object with type " + str(type(v)) + " to flexible_type.")


################################################################################

cdef flexible_type _ft_translate(object v, int tr_code) except *:

    cdef flexible_type ret

    # These are optimized by the cython compiler into a big switch statement.
    if tr_code == FT_INT_TYPE:
        ret.set_int(<flex_int>v)
        return ret
    elif tr_code == FT_FLOAT_TYPE:
        ret.set_double(<double>v)
        return ret
    elif tr_code == FT_STR_TYPE:
        ret.set_string(unsafe_str_to_cpp(v))
        return ret
    elif tr_code == FT_UNICODE_TYPE:
        ret.set_string(unsafe_unicode_to_cpp(v))
        return ret
    elif tr_code == FT_LIST_TYPE:
        tr_listlike_to_ft(ret, <list>v)
        return ret
    elif tr_code == FT_TUPLE_TYPE:
        tr_listlike_to_ft(ret, <tuple>v)
        return ret
    elif tr_code == FT_DICT_TYPE:
        tr_dict_to_ft(ret, <dict>v)
        return ret
    elif tr_code == FT_NONE_TYPE:
        ret = FLEX_UNDEFINED
        return ret
    elif tr_code == FT_DATETIME_TYPE:
        tr_datetime_to_ft(ret, v)
        return ret
    elif tr_code == FT_IMAGE_TYPE:
        translate_image(ret, v)
        return ret
    elif tr_code == FT_BUFFER_TYPE or tr_code == FT_ARRAY_TYPE:
        tr_buffer_to_ft(ret, v)
        return ret
    
    # Now, versions of the above with a cast and/or check.
    elif tr_code == (FT_INT_TYPE + FT_SAFE):
        try:
            ret.set_int(v)
        except OverflowError:

            # Explicitly handle the maximum value that can fit in an
            # int64_t, aka std::numeric_limits<int64_t>::max().
            # Python puts this value into a long type, which causes
            # the cast to C-long to fail even though it can still be
            # represented in C.
            if v == 9223372036854775808:
                ret.set_int(9223372036854775808)
            else:
                ret.set_double(float(v))
        return ret
    elif tr_code == (FT_FLOAT_TYPE + FT_SAFE):
        if type(v) is timedelta_type:
            v = v.total_seconds()
        ret.set_double(v)
        return ret
    elif tr_code == (FT_STR_TYPE + FT_SAFE):
        ret.set_string(str_to_cpp(v))
        return ret
    elif tr_code == (FT_UNICODE_TYPE + FT_SAFE):
        ret.set_string(str_to_cpp(v))
        return ret
    elif tr_code == (FT_LIST_TYPE + FT_SAFE):
        if type(v) is list:
            tr_listlike_to_ft(ret, <list>(v))
        else:
            tr_listlike_to_ft(ret, list(v))
        return ret
    elif tr_code == (FT_TUPLE_TYPE + FT_SAFE):
        if type(v) is tuple:
            tr_listlike_to_ft(ret, <tuple>(v))
        else:
            tr_listlike_to_ft(ret, tuple(v))
        return ret
    elif tr_code == (FT_DICT_TYPE + FT_SAFE):
        if type(v) is dict:
            tr_dict_to_ft(ret, <dict>(v))
        else:
            tr_dict_to_ft(ret, dict(v))
        return ret
    elif tr_code == (FT_BUFFER_TYPE + FT_SAFE) or tr_code == (FT_ARRAY_TYPE + FT_SAFE):
        tr_buffer_to_ft(ret, v)
        return ret
    elif tr_code == (FT_NONE_TYPE + FT_SAFE):
        # Here for forced type conversion semantics
        ret = FLEX_UNDEFINED
        return ret
    elif tr_code == (FT_DATETIME_TYPE + FT_SAFE):
        ret = FLEX_UNDEFINED
        if HAS_NUMPY and isinstance(v, np.datetime64):
          tr_datetime64_to_ft(ret, v)
        elif HAS_PANDAS and isinstance(v, pd.Timestamp):
          tr_datetime_to_ft(ret, v.to_datetime())
        elif isinstance(v, datetime.datetime):
          tr_datetime_to_ft(ret, v)
        else:
          # This should catch only datetime.date, since the check for
          # datetime.datetime is before it
          tr_datetime_to_ft(ret, datetime.datetime(v.year, v.month, v.day))
        return ret
    elif tr_code == (FT_IMAGE_TYPE + FT_SAFE):
        if type(v) != _image_type:
            raise TypeError("Cannot interpret type '" + str(type(v)) + "' as turicreate.Image type.")
        translate_image(ret, v)
        return ret
    elif tr_code == FT_FAILURE:
        raise TypeError("Cannot convert type '" + type(v).__name__ + "' into flexible type.")
    else:
        assert False, ("Error: Code " + str(tr_code) + " not accounted for.")

cdef flexible_type flexible_type_from_pyobject(object v) except *:
    """
    Converting python object into flexible_type.  This is the function
    to use when the translation context is translating a single value.

    Manually perform a list upgrade if needed.  The internal
    utilities will only alter these types in casting to a common
    type.  This function, however, is meant to be used when
    translating values with no context.
    """

    cdef type t = type(v)
    cdef flexible_type ret

    cdef int tr_code = get_translation_code(t, v)
    #print( "type of %s = %s, tr_code = %d." % (repr(v), str(t), tr_code))
    
    ret = _ft_translate(v, tr_code)
    check_list_to_vector_translation(ret)
    return ret

cdef flexible_type flexible_type_from_pyobject_hint(object v, type t) except *:
    """
    Converting python object into flexible_type, with type hint.
    Possible types are int, float, dict, list, str, array.array,
    datetime.datetime, type(None), and image.Image.
    """

    cdef object_ptr optr = <object_ptr>t
    cdef map[object_ptr,int].iterator it = _code_by_map_force.find(optr)
    cdef int tr_code

    if it !=  _code_by_map_force.end():
        tr_code = deref(it).second
    else:
        raise TypeError("Type '%s' not valid type hint." % str(t))

    cdef flexible_type ret

    ret = _ft_translate(v, tr_code)
    return ret

def _check_ft_pyobject_hint_path(object v, type t):
    cdef flexible_type ft = flexible_type_from_pyobject_hint(v, t)
    assert ft.get_type() == flex_type_enum_from_pytype(t)



################################################################################
# Translation from various flexible types to the corresponding types.

@cython.boundscheck(False)
cdef inline array.array pyvec_from_flex_vec(const flex_vec& fv):
    cdef size_t n = fv.size()
    cdef array.array ret = array.array('d')
    array.extend_buffer(ret, <char*>fv.data(), n)
    return ret

@cython.boundscheck(False)
cdef list pylist_from_flex_list(const flex_list& vec):
    """
    Converting vector[flexible_type] to list
    """
    cdef list ret = [None]*vec.size()
    cdef size_t i

    for i in range(vec.size()):
        ret[i] = pyobject_from_flexible_type(vec[i])

    return ret


cdef inline dict pydict_from_flex_dict(const flex_dict& fd):
    cdef size_t n = fd.size()
    cdef dict ret = {}
    cdef size_t i

    cdef object first, second

    for i in range(n):
        first = pyobject_from_flexible_type(fd[i].first)
        second = pyobject_from_flexible_type(fd[i].second)
        ret[first] = second

    return ret

cdef inline pyimage_from_image(const flex_image& c_image):

    cdef const char* c_image_data = <const char*>c_image.get_image_data()

    from ..data_structures import image

    if c_image.m_image_data_size == 0:
        image_data =  <bytearray> ()
    else:
        assert c_image_data != NULL, "image_data is Null"
        image_data =  <bytearray> c_image_data[:c_image.m_image_data_size]

    ret = _image_type(_image_data = image_data, _height = c_image.m_height,
                      _width = c_image.m_width, _channels = c_image.m_channels,
                      _image_data_size = c_image.m_image_data_size,
                      _version = <int>c_image.m_version, _format_enum = <int>c_image.m_format)

    return ret

cdef inline pydatetime_from_flex_datetime(const pflex_date_time& dt, int us):
    utc = datetime.datetime(1970,1,1) + datetime.timedelta(seconds=dt.first, microseconds=us)
    if dt.second != EMPTY_TIMEZONE:
        to_zone = GMT(dt.second * TIMEZONE_RESOLUTION_IN_HOURS)
        utc = utc.replace(tzinfo=GMT(0))
        return utc.astimezone(to_zone)
    else:
        return utc


########################################


cdef pyobject_from_flexible_type(const flexible_type& v):
    """
    Convert a flexible_type to the python object.
    """

    cdef flex_type_enum f_type = v.get_type()
    
    if f_type == INTEGER:
        return v.get_int()
    elif f_type == FLOAT:
        return v.get_double()
    elif f_type == STRING:
        return cpp_to_str(v.get_string())
    elif f_type == LIST:
        return pylist_from_flex_list(v.get_list())
    elif f_type == VECTOR:
        return pyvec_from_flex_vec(v.get_vec())
    elif f_type == DICT:
        return pydict_from_flex_dict(v.get_dict())
    elif f_type == IMAGE:
        return pyimage_from_image(v.get_img())
    elif f_type == DATETIME:
        return pydatetime_from_flex_datetime(v.get_date_time(), v.get_microsecond())
    elif f_type == UNDEFINED:
        return None
    else:
        assert False


ctypedef map[string, flexible_type].const_iterator options_map_iter

################################################################################
# Options map translation

@cython.boundscheck(False)
cdef dict pydict_from_gl_options_map(const gl_options_map& m):
    """
    Converting  map[string, flexible_type] into python dict
    """
    cdef dict ret = {}
    cdef options_map_iter it = <options_map_iter>m.begin()

    while it != <options_map_iter>m.end():
        ret[cpp_to_str(deref(it).first)] = pyobject_from_flexible_type(deref(it).second)
        inc(it)

    return ret

@cython.boundscheck(False)
cdef gl_options_map gl_options_map_from_pydict(dict d) except *:
    """
    Converting python dict into map[string, flexible_type]
    """
    cdef gl_options_map ret

    for k,v in d.iteritems():
        ret[str_to_cpp(k)] = flexible_type_from_pyobject(v)

    return ret

################################################################################

@cython.boundscheck(False)
cdef inline bint __try_buffer_typed_list(flex_list& retl, object v, _numeric n,
                                         flex_type_enum common_type,
                                         bint numeric_is_integer,
                                         bint ignore_translation_errors = False) except *:
    """
    if common type is UNDEFINED, then it's not typed.  Otherwise, it must be castable to this.
    """

    cdef _numeric[:] buf

    try:
        buf = v
    except:
        return False

    cdef bint translation_okay = True

    cdef flexible_type src, dest

    # See if we can indeed cast things.
    if common_type != UNDEFINED:

        src = flexible_type(INTEGER if numeric_is_integer else FLOAT)
        dest = flexible_type(common_type)

        try:
            dest.soft_assign(src)
        except:
            translation_okay = False

        if not translation_okay:
            if ignore_translation_errors:
                retl.assign(len(buf), flexible_type(UNDEFINED))
                return True
            else:
                if numeric_is_integer:
                    raise TypeError("Integer type cannot be cast to type "
                                    + flex_type_enum_to_name(common_type))
                else:
                    raise TypeError("Float type cannot be cast to type "
                                    + flex_type_enum_to_name(common_type))

    cdef size_t i

    retl.resize(len(buf))
    for i in range(len(buf)):
        if numeric_is_integer:
            retl[i].set_int(<flex_int>buf[i])
        else:
            retl[i].set_double(<flex_float>buf[i])

    return True

@cython.boundscheck(False)
cdef inline flex_type_enum __tr_numeric_buffer_to_flex_list(
    flex_list& retl, object v, flex_type_enum ct, bint ite):

    # Common cases
    if __try_buffer_typed_list(retl, v, <double>(0), ct, False, ite):  return FLOAT
    if __try_buffer_typed_list(retl, v, <int64_t>(0), ct, True, ite):  return INTEGER
    if __try_buffer_typed_list(retl, v, <float>(0),ct, False, ite):    return FLOAT
    if __try_buffer_typed_list(retl, v, <int8_t>(0), ct, True, ite):   return INTEGER
    if __try_buffer_typed_list(retl, v, <uint8_t>(0), ct, True, ite):  return INTEGER
    if __try_buffer_typed_list(retl, v, <int16_t>(0), ct, True, ite):  return INTEGER
    if __try_buffer_typed_list(retl, v, <uint16_t>(0), ct, True, ite): return INTEGER
    if __try_buffer_typed_list(retl, v, <int32_t>(0), ct, True, ite):  return INTEGER
    if __try_buffer_typed_list(retl, v, <uint32_t>(0), ct, True, ite): return INTEGER
    if __try_buffer_typed_list(retl, v, <int64_t>(0), ct, True, ite):  return INTEGER
    if __try_buffer_typed_list(retl, v, <uint64_t>(0), ct, True, ite): return INTEGER

    return UNDEFINED

@cython.boundscheck(False)
cdef inline flex_type_enum tr_listlike_to_flex_list(
    flex_list& retl, _listlike v,
    flex_type_enum common_type = UNDEFINED, bint ignore_translation_errors = False) except *:

    if common_type == UNDEFINED:
        fill_list(retl, v, &common_type)
        return common_type
    else:
        fill_typed_list(retl, v, common_type, ignore_translation_errors)
        return common_type


@cython.boundscheck(False)
cdef inline flex_type_enum tr_buffer_to_flex_list(
    flex_list& retl, object v,
    flex_type_enum common_type = UNDEFINED,
    bint ignore_translation_errors = False) except *:

    """
    Translates a buffer object to a common type.  If common_type ==
    UNDEFINED, then it is not expected to be a common type.

    If a common type is detected, then it is returned.

    """

    # for prettification
    cdef flex_type_enum ft_type, ft_rec_type
    cdef bint ite = ignore_translation_errors
    cdef bint tried_numeric_cases_already

    # If we can accomodate NUMPY type inference, let's do it.
    if HAS_NUMPY and type(v) is np_ndarray:
        dt = v.dtype
        # annoyingly, have to special case this one
        if dt == bool:
            v = np.asarray(v, dtype = np.uint8)
            if not __try_buffer_typed_list(retl, v, <uint8_t>(0), common_type, True, ite):
                assert False
            return INTEGER

        ft_type = flex_type_from_dtype(dt)

        if ft_type == INTEGER or ft_type == FLOAT:
            ft_rec_type = __tr_numeric_buffer_to_flex_list(retl, v, common_type, True)

            if ft_rec_type == UNDEFINED:
                return tr_listlike_to_flex_list(retl, list(v), common_type, ite)
            else:
                return ft_rec_type
        else:
            common_type = ft_type # Cast to a common type to correctly handle strings
    else:
        # Try the numeric ones
        ft_rec_type = __tr_numeric_buffer_to_flex_list(retl, v, common_type, True)

        if ft_rec_type != UNDEFINED:
            return ft_rec_type
        else:
            pass # Go on the the object stuff

    # object
    cdef object[:] object_buffer
    cdef bint is_object_buffer = False

    try:
        object_buffer = v
        is_object_buffer = True
    except:
        pass

    if is_object_buffer:
        return tr_listlike_to_flex_list(retl, object_buffer, common_type, ignore_translation_errors)

    # Numpy array that is not an object array?
    if isinstance(v, collections.Iterable) or (HAS_NUMPY and type(v) is np_ndarray):
        return tr_listlike_to_flex_list(retl, list(v), common_type, ignore_translation_errors)

    # Error if there are no more options.
    raise TypeError("Could not convert python object with type " + type(v).__name__ + " to flex list.")

@cython.boundscheck(False)
cdef flex_list common_typed_flex_list_from_iterable(object v, flex_type_enum* common_type) except *:
    """
    Converting any iterable into a common typed flex_list.
    """

    cdef int tr_code = get_translation_code(type(v), v)
    cdef flex_list ret
    cdef flex_type_enum ct

    # These are optimized by the cython compiler into a big switch statement.
    if tr_code == FT_LIST_TYPE:
        fill_list(ret, <list>v, common_type)
        return ret
    elif tr_code == FT_TUPLE_TYPE:
        fill_list(ret, <tuple>v, common_type)
        return ret
    elif tr_code == FT_BUFFER_TYPE or FT_ARRAY_TYPE:
        ct = tr_buffer_to_flex_list(ret, v, UNDEFINED)
        if common_type != NULL:
            common_type[0] = ct
        return ret
    elif tr_code == (FT_LIST_TYPE + FT_SAFE):
        if type(v) is list:
            fill_list(ret, <list>v, common_type)
        else:
            fill_list(ret, list(v), common_type)
        return ret
    elif tr_code == (FT_TUPLE_TYPE + FT_SAFE):
        if type(v) is tuple:
            fill_list(ret, <tuple>(v), common_type)
        else:
            fill_list(ret, tuple(v), common_type)
        return ret
    elif tr_code == (FT_BUFFER_TYPE + FT_SAFE) or tr_code == (FT_ARRAY_TYPE + FT_SAFE):
        ct = tr_buffer_to_flex_list(ret, v, UNDEFINED)
        if common_type != NULL:
            common_type[0] = ct
        return ret
    else:
        raise TypeError("Cannot convert type '" + type(v).__name__ + "' into flexible list.")


cdef flex_list flex_list_from_iterable(object v) except *:
    return common_typed_flex_list_from_iterable(v, NULL)

@cython.boundscheck(False)
cdef flex_list flex_list_from_typed_iterable(object v, flex_type_enum common_type,
                                             bint ignore_cast_failure) except *:
    """
    Converting any iterable into a list of a certain type.
    """

    cdef int tr_code = get_translation_code(type(v), v)
    cdef flex_list ret

    # These are optimized by the cython compiler into a big switch statement.
    if tr_code == FT_LIST_TYPE:
        fill_typed_list(ret, <list>v, common_type, ignore_cast_failure)
        return ret
    elif tr_code == FT_TUPLE_TYPE:
        fill_typed_list(ret, <tuple>v, common_type, ignore_cast_failure)
        return ret
    elif tr_code == FT_BUFFER_TYPE or tr_code == FT_ARRAY_TYPE:
        tr_buffer_to_flex_list(ret, v, common_type, ignore_cast_failure)
        return ret
    elif tr_code == (FT_LIST_TYPE + FT_SAFE):
        if type(v) is list:
            fill_typed_list(ret, <list>v, common_type, ignore_cast_failure)
        else:
            fill_typed_list(ret, list(v), common_type, ignore_cast_failure)
        return ret
    elif tr_code == (FT_TUPLE_TYPE + FT_SAFE):
        if type(v) is tuple:
            fill_typed_list(ret, <tuple>v, common_type, ignore_cast_failure)
        else:
            fill_typed_list(ret, tuple(v), common_type, ignore_cast_failure)
        return ret
    elif tr_code == (FT_BUFFER_TYPE + FT_SAFE) or tr_code == (FT_ARRAY_TYPE + FT_SAFE):
        tr_buffer_to_flex_list(ret, v, common_type, ignore_cast_failure)
        return ret
    else:
        raise TypeError("Cannot convert type '" + type(v).__name__ + "' into flexible list.")

################################################################################
# Testing utilities

def _translate_through_flexible_type(object p):
    cdef flexible_type ft = flexible_type_from_pyobject(p)
    #print("Translated %s into %s" % (repr(p), cpp_to_str(ft.as_string())))
    cdef object pt = pyobject_from_flexible_type(ft)
    return pt


def _translate_through_flex_list(object v, type t=None, bint ignore_cast_failure=False):
    """ convert to vector[flexible_type] and back """

    if t is None:
        return pylist_from_flex_list(flex_list_from_iterable(v))
    else:
        return pylist_from_flex_list(
            flex_list_from_typed_iterable(
                v, flex_type_enum_from_pytype(t), ignore_cast_failure))


def _get_inferred_column_type(list v):
    """
    Returns the inferred type of a column, or "undefined" if no common type exists.
    """
    cdef flex_type_enum ft = _infer_common_type_of_listlike(v, True)
    cdef flex_type_enum alt_ft = UNDEFINED
    cdef flex_list vl, alt_vl

    if ft != UNDEFINED:
        # make sure that the three different methods are consistent
        vl = flex_list_from_typed_iterable(v, ft, False)

        # make sure that doing it through the common
        alt_vl = common_typed_flex_list_from_iterable(v, &alt_ft)

        assert alt_ft == ft, (("Infered types differ between methods: "
                               "infered = %s, constructed with common = %s")
                              % (flex_type_enum_to_name(ft), flex_type_enum_to_name(alt_ft)))

        return pytype_from_flex_type_enum(ft), pylist_from_flex_list(vl)
    else:
        return none_type, None


def _all_convertable(type t, list v):
    """
    Returns the inferred type of a column, or "undefined" if no common type exists.
    """
    try:
        flex_list_from_typed_iterable(v, flex_type_enum_from_pytype(t), False)
        return True
    except TypeError:
        return False

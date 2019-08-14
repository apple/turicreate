# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

# cython: c_string_type=bytes, c_string_encoding=UTF-8
# cython: boundscheck=False
# cython: wraparound=False
# distutils: language = c++
import cython
import types
from libcpp.string cimport string
from libcpp.pair cimport pair
from . cimport cy_graph
from . cimport cy_sframe
from . cimport cy_sarray
from .cy_graph cimport UnityGraphProxy
from .cy_model cimport create_model_from_proxy
from .cy_model cimport UnityModel
from .cy_sframe cimport UnitySFrameProxy
from .cy_sarray cimport UnitySArrayProxy

from .cy_flexible_type cimport flexible_type, flex_list, flex_dict, flex_int
from .cy_flexible_type cimport flexible_type_from_pyobject
from .cy_flexible_type cimport pyobject_from_flexible_type
from .cy_flexible_type cimport check_list_to_vector_translation
from .cy_flexible_type cimport flex_type_enum, UNDEFINED

from .cy_dataframe cimport gl_dataframe
from .cy_dataframe cimport pd_from_gl_dataframe

from .cy_cpp_utils cimport str_to_cpp, cpp_to_str, unsafe_str_to_cpp, unsafe_unicode_to_cpp

from cython.operator cimport dereference as deref
from cython.operator cimport preincrement as inc
from .cy_unity cimport is_function_closure_info
from .cy_unity cimport make_function_closure_info
from .cy_unity cimport variant_set_closure
from .cy_unity cimport function_closure_info
from cpython.ref cimport PyObject
from cpython.version cimport PY_MAJOR_VERSION

from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp.map cimport map

################################################################################
# Codes for the different translation paths for variant types, just
# like in cy_flexible_type.pyx.

# Codes for translating python types to variant types.

# Fast track simple flexible types
DEF VAR_TR_FT_INT                      = 0
DEF VAR_TR_FT_FLOAT                    = 1
DEF VAR_TR_FT_STR                      = 2
DEF VAR_TR_FT_UNICODE                  = 3
DEF VAR_TR_FT_NONE                     = 4

# Nested types
DEF VAR_TR_DICT                        = 5
DEF VAR_TR_TUPLE                       = 6
DEF VAR_TR_LIST                        = 7

# Other general types
DEF VAR_TR_SFRAME                      = 8
DEF VAR_TR_SARRAY                      = 9
DEF VAR_TR_GRAPH                       = 10
DEF VAR_TR_UNITY_MODEL                 = 11
DEF VAR_TR_UNITY_MODEL_TKCLASS         = 12
DEF VAR_TR_SFRAME_PROXY                = 13
DEF VAR_TR_SARRAY_PROXY                = 14
DEF VAR_TR_GRAPH_PROXY                 = 15
DEF VAR_TR_FUNCTION                    = 16
DEF VAR_TR_CLOSURE                     = 17

# The last resort -- attempt to convert it to a general flexible type
DEF VAR_TR_ATTEMPT_OTHER_FLEXIBLE_TYPE = 20  #

# Codes for translating variant type back to python types.
DEF VAR_TYPE_FLEXIBLE_TYPE       = 0
DEF VAR_TYPE_GRAPH               = 1
DEF VAR_TYPE_DATAFRAME           = 2
DEF VAR_TYPE_MODEL               = 3
DEF VAR_TYPE_SFRAME              = 4
DEF VAR_TYPE_SARRAY              = 5
DEF VAR_TYPE_VARIANT_MAP         = 6
DEF VAR_TYPE_VARIANT_VECTOR      = 7
DEF VAR_TYPE_VARIANT_CLOSURE     = 8

# Some hard coded class types.
cdef type instance_type = getattr(types, 'InstanceType', object)
cdef type function_type = types.FunctionType
cdef type lambda_type   = types.LambdaType

# Handle the sframe types.  We don't want to start the server before
# we need to, which means that can be imported on demand

cdef bint internal_classes_set = False
cdef object sframe_class = None
cdef object sarray_class = None
cdef object sgraph_class = None
cdef object gframe_class = None

cdef object build_native_function_call = None

# Load all the internal classes
cdef import_internal_classes():

    global sframe_class
    from ..data_structures.sframe import SFrame
    sframe_class = SFrame

    global sarray_class
    from ..data_structures.sarray import SArray
    sarray_class = SArray

    global sgraph_class
    from ..data_structures.sgraph import SGraph
    sgraph_class = SGraph

    global gframe_class
    from ..data_structures.gframe import GFrame
    gframe_class = GFrame

    global build_native_function_call
    from ..extensions import _build_native_function_call
    build_native_function_call = _build_native_function_call

    global internal_classes_set
    internal_classes_set = True

cdef int _get_tr_code_by_type_string(object v) except -1:
    """
    Attempt to choose a translation code based on the object
    properties.  If it's not a handful of special types, defer it to
    the flexible_type translators.
    """

    cdef int ret = -1

    if not internal_classes_set:
        import_internal_classes()

    if type(v) is int or type(v) is long:
        ret =  VAR_TR_FT_INT
    elif type(v) is float:
        ret =  VAR_TR_FT_FLOAT
    elif type(v) is str:
        ret =  VAR_TR_FT_STR
    elif type(v).__name__  == "unicode":
        ret =  VAR_TR_FT_UNICODE
    elif type(v) is bool:
        ret =  VAR_TR_FT_INT
    elif v is None:
        ret =  VAR_TR_FT_NONE
    elif type(v) is list:
        ret =  VAR_TR_LIST
    elif type(v) is tuple:
        ret =  VAR_TR_TUPLE
    elif type(v) is dict:
        ret =  VAR_TR_DICT
    elif type(v) is sframe_class or v.__class__ is sframe_class:
        ret =  VAR_TR_SFRAME
    elif type(v) is gframe_class or v.__class__ is gframe_class:
        ret =  VAR_TR_SFRAME
    elif v.__class__ is sgraph_class:
        ret =  VAR_TR_GRAPH
    elif type(v) is sarray_class or v.__class__ is sarray_class:
        ret =  VAR_TR_SARRAY
    elif type(v) is UnityModel:
        ret =  VAR_TR_UNITY_MODEL
    elif type(v) is function_type:
        ret =  VAR_TR_FUNCTION
    elif type(v) is lambda_type:
        ret =  VAR_TR_FUNCTION
    elif hasattr(v, '_tkclass') and type(v._tkclass) is UnityModel:
        ret =  VAR_TR_UNITY_MODEL_TKCLASS
    elif type(v) is UnitySFrameProxy:
        ret =  VAR_TR_SFRAME_PROXY
    elif type(v) is UnitySArrayProxy:
        ret =  VAR_TR_SARRAY_PROXY
    elif type(v) is UnityGraphProxy:
        ret =  VAR_TR_GRAPH_PROXY
    elif is_function_closure_info(v):
        ret =  VAR_TR_CLOSURE
    else:
        ret =  VAR_TR_ATTEMPT_OTHER_FLEXIBLE_TYPE

    return ret

################################################################################
# Translation functions.

# This table gets built up automatically.
ctypedef PyObject* object_ptr

cdef map[object_ptr, int] _code_by_id_lookup = map[object_ptr,int]()

cdef inline int get_var_tr_code(object v) except -1:
    """
    The main internal function to find the code used for translating a
    python object.
    """
    cdef type t = type(v)
    cdef object_ptr lookup_code = NULL
    cdef int tr_code = -1

    # Choose the lookup code based on whether it's a native
    # type/extension type, a function_type, or a class instance.
    if t is type or t is instance_type:
        try:
            lookup_code = <object_ptr>(v.__class__)
        except AttributeError:
            raise ValueError("Unable to encode object as variant type (old-style class / non-instance type).")

    elif t is function_type or t is lambda_type:
        lookup_code = <object_ptr>(v)
    else:
        lookup_code = <object_ptr>(t)

    cdef map[object_ptr,int].iterator it = _code_by_id_lookup.find(lookup_code)

    if it != _code_by_id_lookup.end():
        return deref(it).second
    else:
        # Put this in a separate function to make this one more inlinable.
        tr_code = _get_tr_code_by_type_string(v)
        _code_by_id_lookup[lookup_code] = tr_code
        return tr_code


################################################################################
# Specific translation functions

cdef raise_translation_error(object v):
    """
    Raises an informative translation error.
    """

    raise TypeError("Unable to encode object of type '%s' as variant type." % (str(type(v))))

cdef inline flexible_type _translate_to_flexible_type(object x) except *:
    """
    Wraps the flexible_type translator so that errors are raised for
    variant type conversions instead of for flexible_types, which
    would be misleading.
    """

    try:
        return flexible_type_from_pyobject(x)
    except TypeError:
        raise_translation_error(x)

############################################################
# Translation routines for lists and vectors and dicts

ctypedef fused _listlike:
    list
    tuple

cdef inline __move_flex_list_to_variant_vector(variant_vector_type& vv, flex_list& fl, long idx):
    vv.resize(fl.size())
    cdef long i
    for i in range(idx):
        variant_set_flexible_type(vv[i], fl[i])


cdef bint _var_set_listlike_internal(variant_vector_type& ret_as_vv,
                                     flex_list& ret_as_fl, _listlike v, bint require_var_vector) except *:
    """
    Performs a translation of a list into either a variant vector or a
    flex_list, depending on which is possible.

    returns True if the object was converted to a list of flexible
    types, in which case the data is in ret_as_fl. Returns False if
    the object was converted to a variant_vector_type, in which case
    it's in ret_as_vv.

    If require_var_vector is True, then the output is always stuck in
    ret_as_vv.

    This function is useful to properly handle the recursions possible
    with list and dict types.
    """

    # First, attempt to convert everything to flexible type.  If that
    # succeeds, then we simply return that.

    cdef long i
    cdef int tr_code = 0

    cdef int sub_code = 0
    cdef bint value_contained_in_next = False

    cdef bint sub_is_flex_type = False
    cdef variant_vector_type sub_vv = variant_vector_type()
    cdef variant_map_type sub_vm = variant_map_type()

    # We start off assuming everything is a flexible type.
    cdef bint writing_to_flexible_types = True
    cdef bint element_stored_in_flex_list = False

    ret_as_fl.resize(len(v))

    if require_var_vector:
        writing_to_flexible_types = False
        ret_as_vv.resize(len(v))

    for i in range(len(v)):
        x = v[i]
        tr_code = get_var_tr_code(x)
        element_stored_in_flex_list = False

        if tr_code == VAR_TR_FT_INT:
            ret_as_fl[i].set_int(<flex_int>x)
            element_stored_in_flex_list = True
        elif tr_code == VAR_TR_FT_FLOAT:
            ret_as_fl[i].set_double(<double>x)
            element_stored_in_flex_list = True
        elif tr_code == VAR_TR_FT_STR:
            ret_as_fl[i].set_string(unsafe_str_to_cpp(x))
            element_stored_in_flex_list = True
        elif tr_code == VAR_TR_FT_UNICODE:
            ret_as_fl[i].set_string(unsafe_unicode_to_cpp(x))
            element_stored_in_flex_list = True
        elif tr_code == VAR_TR_FT_NONE:
            ret_as_fl[i] = flexible_type(UNDEFINED)
            element_stored_in_flex_list = True
        elif tr_code == VAR_TR_FT_NONE:
            ret_as_fl[i] = flexible_type(UNDEFINED)
            element_stored_in_flex_list = True
        elif tr_code == VAR_TR_LIST or tr_code == VAR_TR_TUPLE:
            ret_as_fl[i].set_list(flex_list())

            if tr_code == VAR_TR_LIST:
                sub_is_flex_type = _var_set_listlike_internal(sub_vv, ret_as_fl[i].get_list_m(), <list>x, False)
            else:
                sub_is_flex_type = _var_set_listlike_internal(sub_vv, ret_as_fl[i].get_list_m(), <tuple>x, False)

            if sub_is_flex_type:
                check_list_to_vector_translation(ret_as_fl[i])
                element_stored_in_flex_list = True
            else:
                if writing_to_flexible_types:
                    __move_flex_list_to_variant_vector(ret_as_vv, ret_as_fl, i)
                    writing_to_flexible_types = False

                variant_set_variant_vector(ret_as_vv[i], sub_vv)

        elif tr_code == VAR_TR_DICT:
            ret_as_fl[i].set_dict(flex_dict())

            sub_is_flex_type = _var_set_dict_internal(sub_vm, ret_as_fl[i].get_dict_m(), <dict>x, False)

            if sub_is_flex_type:
                element_stored_in_flex_list = True
            else:
                if writing_to_flexible_types:
                    __move_flex_list_to_variant_vector(ret_as_vv, ret_as_fl, i)
                    writing_to_flexible_types = False

                variant_set_variant_map(ret_as_vv[i], sub_vm)

        elif tr_code == VAR_TR_ATTEMPT_OTHER_FLEXIBLE_TYPE:
            ret_as_fl[i] = _translate_to_flexible_type(x)
            element_stored_in_flex_list = True

        else:
            if writing_to_flexible_types:
                __move_flex_list_to_variant_vector(ret_as_vv, ret_as_fl, i)
                writing_to_flexible_types = False

            _convert_to_variant_type(ret_as_vv[i], x, tr_code)

        # See if we need to move that element over to the variant type list
        if element_stored_in_flex_list and not writing_to_flexible_types:
            variant_set_flexible_type(ret_as_vv[i], ret_as_fl[i])

    if writing_to_flexible_types:
        ret_as_vv.clear()
        return True
    else:
        ret_as_fl.clear()
        return False

############################################################
# Translation routines for a dict

cdef inline bint _var_set_dict_internal(variant_map_type& ret_as_vm, flex_dict& ret_as_fd,
                                        dict d, bint require_varmap) except *:
    """
    Attempts conversion of a dictionary to a flex_dict or
    variant_map_type.

    Returns True if the dict was converted to a flex_dict, in which
    case the data is stored in ret_as_fd.  Otherwise, the data is
    stored in ret_as_vm.

    This function is useful to properly handle the recursions possible
    with list and dict types.
    """

    # First, check if all the keys are string values.  If they are, we
    # are doing a variant_map_type.  Otherwise, encode it as a
    # flexible type.

    cdef bint output_can_be_varmap = True

    for k in d.iterkeys():
        if type(k) is not str:
            output_can_be_varmap = False
            break

    if require_varmap and not output_can_be_varmap:
        raise TypeError("Dictionary cannot be translated into a variant type map (keys not strings)")

    cdef long pos
    cdef int tr_code
    cdef bint writing_to_flex_dict, sub_is_flex_type
    cdef variant_vector_type sub_vv = variant_vector_type()
    cdef variant_map_type sub_vm = variant_map_type()

    if output_can_be_varmap:
        # First see if it can be translated as flexible type, which
        # would be great.

        if not require_varmap:
            ret_as_fd.resize(len(d))

            writing_to_flex_dict = True

            pos = 0
            for k, v in d.iteritems():

                if writing_to_flex_dict:
                    ret_as_fd[pos].first.set_string(unsafe_str_to_cpp(k))

                    tr_code = get_var_tr_code(v)

                    if tr_code == VAR_TR_FT_INT:
                        ret_as_fd[pos].second.set_int(<flex_int>v)
                    elif tr_code == VAR_TR_FT_FLOAT:
                        ret_as_fd[pos].second.set_double(<double>v)
                    elif tr_code == VAR_TR_FT_STR:
                        ret_as_fd[pos].second.set_string(unsafe_str_to_cpp(v))
                    elif tr_code == VAR_TR_FT_UNICODE:
                        ret_as_fd[pos].second.set_string(unsafe_unicode_to_cpp(v))
                    elif tr_code == VAR_TR_FT_NONE:
                        ret_as_fd[pos].second = flexible_type(UNDEFINED)
                    elif tr_code == VAR_TR_ATTEMPT_OTHER_FLEXIBLE_TYPE:
                        ret_as_fd[pos].second = _translate_to_flexible_type(v)
                    elif tr_code == VAR_TR_LIST or tr_code == VAR_TR_TUPLE:
                        ret_as_fd[pos].second.set_list(flex_list())

                        if tr_code == VAR_TR_LIST:
                            sub_is_flex_type = _var_set_listlike_internal(
                                sub_vv, ret_as_fd[pos].second.get_list_m(), <list>v, False)
                        else:
                            sub_is_flex_type = _var_set_listlike_internal(
                                sub_vv, ret_as_fd[pos].second.get_list_m(), <tuple>v, False)

                        if sub_is_flex_type:
                            check_list_to_vector_translation(ret_as_fd[pos].second)

                            if not writing_to_flex_dict:
                                variant_set_flexible_type(ret_as_vm[ret_as_fd[pos].first.get_string()],
                                                          ret_as_fd[pos].second)
                        else:
                            if writing_to_flex_dict:
                                ret_as_vm.clear()
                                for i in range(pos):
                                    variant_set_flexible_type(ret_as_vm[ret_as_fd[i].first.get_string()],
                                                              ret_as_fd[i].second)

                                writing_to_flex_dict = False


                            variant_set_variant_vector(ret_as_vm[ret_as_fd[pos].first.get_string()], sub_vv)

                    elif tr_code == VAR_TR_DICT:
                        ret_as_fd[pos].second.set_dict(flex_dict())

                        sub_is_flex_type = _var_set_dict_internal(sub_vm, ret_as_fd[pos].second.get_dict_m(), <dict>v, False)

                        if sub_is_flex_type:
                            if not writing_to_flex_dict:
                                variant_set_flexible_type(ret_as_vm[ret_as_fd[pos].first.get_string()],
                                                          ret_as_fd[pos].second)
                        else:
                            if writing_to_flex_dict:
                                ret_as_vm.clear()
                                for i in range(pos):
                                    variant_set_flexible_type(ret_as_vm[ret_as_fd[i].first.get_string()],
                                                              ret_as_fd[i].second)

                                writing_to_flex_dict = False

                            variant_set_variant_map(ret_as_vm[ret_as_fd[pos].first.get_string()], sub_vm)

                    else:

                        # Move things out of the flex_dict container
                        ret_as_vm.clear()
                        for i in range(pos):
                            variant_set_flexible_type(ret_as_vm[ret_as_fd[i].first.get_string()],
                                                      ret_as_fd[i].second)

                        # Convert the current one
                        _convert_to_variant_type(ret_as_vm[str_to_cpp(k)], v, tr_code)

                        # Make the rest of them write to the variant type map
                        writing_to_flex_dict = False

                    pos += 1
                else:
                    tr_code = get_var_tr_code(v)
                    _convert_to_variant_type(ret_as_vm[str_to_cpp(k)], v, tr_code)

            if writing_to_flex_dict:
                ret_as_vm.clear()
                return True
            else:
                ret_as_fd.clear()
                return False

        else: # require_varmap is true, so we're going to do that.
            ret_as_vm.clear()
            for k, v in d.iteritems():
                tr_code = get_var_tr_code(v)
                _convert_to_variant_type(ret_as_vm[str_to_cpp(k)], v, tr_code)
            ret_as_fd.clear()
            return False
    else:
        ret_as_fd.resize(len(d))

        pos = 0
        for k, v in d.iteritems():
            ret_as_fd[pos].first = _translate_to_flexible_type(k)
            ret_as_fd[pos].second = _translate_to_flexible_type(v)
            pos += 1

        ret_as_vm.clear()
        return True

cdef inline _var_set_dict(variant_type& v, dict d):
    """
    Internal funnction for setting a dict to a variant type.  Wraps
    the internal function above.
    """
    cdef variant_map_type vm
    cdef flexible_type ft
    cdef bint ret_is_fd

    ft.set_dict(flex_dict())

    ret_is_fd = _var_set_dict_internal(vm, ft.get_dict_m(), d, False)

    if ret_is_fd:
        variant_set_flexible_type(v, ft)
    else:
        variant_set_variant_map(v, vm)

cdef inline _var_set_listlike(variant_type& v, _listlike ll):
    cdef variant_vector_type vv
    cdef flexible_type ft
    cdef bint ret_is_ft

    ft.set_list(flex_list())

    ret_is_ft = _var_set_listlike_internal(vv, ft.get_list_m(), ll, False)

    if ret_is_ft:
        check_list_to_vector_translation(ft)
        variant_set_flexible_type(v, ft)
    else:
        variant_set_variant_vector(v, vv)


############################################################

cdef variant_map_type from_dict(dict d) except *:
    """
    Casts a python dict to a variant map.
    """
    cdef variant_map_type ret
    cdef flex_dict fd
    if d is None:
        return ret
    else:
        _var_set_dict_internal(ret, fd, d, True)
    return ret


cdef variant_vector_type from_list(list v) except *:
    """
    Casts a list to a variant vector type.
    """
    cdef variant_vector_type ret
    cdef flex_list fl

    if v is None:
        return ret
    else:
        _var_set_listlike_internal(ret, fl, v, True)
    return ret



cdef _convert_to_variant_type(variant_type& ret, object v, int tr_code):
    """
    Main function for doing the conversions.
    """
    cdef flexible_type ft

    # Fast-tracked flexible type versions
    if tr_code == VAR_TR_FT_INT:
        ft.set_int(<flex_int>v)
        variant_set_flexible_type(ret, ft)
    elif tr_code == VAR_TR_FT_FLOAT:
        ft.set_double(<double>v)
        variant_set_flexible_type(ret, ft)
    elif tr_code == VAR_TR_FT_STR:
        ft.set_string(unsafe_str_to_cpp(v))
        variant_set_flexible_type(ret, ft)
    elif tr_code == VAR_TR_FT_UNICODE:
        ft.set_string(unsafe_unicode_to_cpp(v))
        variant_set_flexible_type(ret, ft)
    elif tr_code == VAR_TR_FT_NONE:
        variant_set_flexible_type(ret, flexible_type(UNDEFINED))

    # Nested container types
    elif tr_code == VAR_TR_DICT:
        _var_set_dict(ret, <dict>v)
    elif tr_code == VAR_TR_LIST:
        _var_set_listlike(ret, <list>v)
    elif tr_code == VAR_TR_TUPLE:
        _var_set_listlike(ret, <tuple>v)

    # SFrame/SArray/SGraph objects
    elif tr_code == VAR_TR_SFRAME:
        variant_set_sframe(ret, (<UnitySFrameProxy?>(v.__proxy__))._base_ptr)
    elif tr_code == VAR_TR_SARRAY:
        variant_set_sarray(ret, (<UnitySArrayProxy?>(v.__proxy__))._base_ptr)
    elif tr_code == VAR_TR_GRAPH:
        variant_set_graph(ret, (<UnityGraphProxy?>(v.__proxy__))._base_ptr)

    # Unity models
    elif tr_code == VAR_TR_UNITY_MODEL:
        variant_set_model(ret, (<UnityModel?>v)._base_ptr)
    elif tr_code == VAR_TR_UNITY_MODEL_TKCLASS:
        variant_set_model(ret, (<UnityModel?>(v._tkclass))._base_ptr)

    # Proxy objects
    elif tr_code == VAR_TR_SFRAME_PROXY:
        variant_set_sframe(ret, (<UnitySFrameProxy>(v))._base_ptr)
    elif tr_code == VAR_TR_SARRAY_PROXY:
        variant_set_sarray(ret, (<UnitySArrayProxy>(v))._base_ptr)
    elif tr_code == VAR_TR_GRAPH_PROXY:
        variant_set_graph(ret, (<UnityGraphProxy>v)._base_ptr)

    # Functions and closures
    elif tr_code == VAR_TR_FUNCTION:
        variant_set_closure(ret, make_function_closure_info(build_native_function_call(v)))
    elif tr_code == VAR_TR_CLOSURE:
        variant_set_closure(ret, make_function_closure_info(v))

    # Flexible type -- this is actually the last resort since we
    # assume the above types are exact matches and do not include the
    # flexible type stuff, save for the fast paths for common types.
    elif tr_code == VAR_TR_ATTEMPT_OTHER_FLEXIBLE_TYPE:
        try:
            variant_set_flexible_type(ret, flexible_type_from_pyobject(v))
        except TypeError:
            raise_translation_error(v)

    else:
        assert False

################################################################################
# The main translation function

cdef variant_type from_value(object v) except *:
    """
    Converts a python type to a variant type.
    """
    cdef variant_type ret
    cdef int tr_code = get_var_tr_code(v)

    _convert_to_variant_type(ret, v, tr_code)

    return ret

################################################################################


cdef dict to_dict(variant_map_type& d):
    """
    Converts a variant map type to a python dictionary.
    """
    cdef dict ret = {}
    cdef variant_map_type_iterator it = d.begin()
    while (it != d.end()):
        ret[cpp_to_str(deref(it).first)] = to_value(deref(it).second)
        inc(it)
    return ret


cdef list to_vector(variant_vector_type& v):
    """
    Converts a variant vector type to a python list
    """
    cdef list ret = [None]*v.size()
    cdef variant_vector_type_iterator it = v.begin()

    cdef long pos = 0
    while (it != v.end()):
        ret[pos] = to_value(deref(it))
        pos += 1
        inc(it)
    return ret

cdef to_value(variant_type& v):
    """
    Converts a variant type into the proper python type.
    """

    cdef int var_type = v.which()

    if var_type == VAR_TYPE_FLEXIBLE_TYPE:
        return pyobject_from_flexible_type(variant_get_flexible_type(v))
    elif var_type == VAR_TYPE_GRAPH:
        return cy_graph.create_proxy_wrapper_from_existing_proxy(
            variant_get_graph(v))
    elif var_type == VAR_TYPE_DATAFRAME:
        return pd_from_gl_dataframe(variant_get_dataframe(v))
    elif var_type == VAR_TYPE_MODEL:
        return create_model_from_proxy(variant_get_model(v))
    elif var_type == VAR_TYPE_SFRAME:
        return cy_sframe.create_proxy_wrapper_from_existing_proxy(
            variant_get_sframe(v))
    elif var_type == VAR_TYPE_SARRAY:
        return cy_sarray.create_proxy_wrapper_from_existing_proxy(
            variant_get_sarray(v))
    elif var_type == VAR_TYPE_VARIANT_MAP:
        return to_dict(variant_get_variant_map(v))
    elif var_type == VAR_TYPE_VARIANT_VECTOR:
        return to_vector(variant_get_variant_vector(v))
    else:
        raise TypeError("Unsupported variant type.")

################################################################################
# Routines to assist with debugging.

def _debug_is_flexible_type_encoded(object obj):
    """
    Checks to make sure that if an object can be encoded as a flexible
    type, then it is.
    """
    cdef variant_type vt = from_value(obj)
    return (vt.which() == VAR_TYPE_FLEXIBLE_TYPE)

# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import types
from .type_spec import *
from .type_void import void


def get_python_method_type(py_function):
    # given a python class method, parse the annotations to figure out the type
    function_inputs = []
    function_output = get_type_info(void)
    annotations = {}
    if hasattr(py_function, 'type_annotations'):
        annotations = {k: get_type_info(v) for k, v in py_function.type_annotations.items()}
    if hasattr(py_function, 'return_type'):
        function_output = get_type_info(py_function.return_type)
    try:
        if hasattr(py_function, '__func__'):
            argcount = py_function.__func__.__code__.co_argcount
            argnames = py_function.__func__.__code__.co_varnames[:argcount]
        else:
            argcount = py_function.__code__.co_argcount
            argnames = py_function.__code__.co_varnames[:argcount]
    except:
        raise TypeError(
            "Unable to derive type information from method %s. "
            "You might have a misspecified type. Ex: use compyler.int and not int" % py_function)

    for arg in argnames:
        if arg in annotations:
            function_inputs.append(annotations[arg])
        elif arg != 'self':
            raise TypeError(
                "Function " + str(py_function) + " insufficient annotations. " + arg +
                " needs a type")
    typeinfo = FunctionType(function_inputs, function_output, py_function)
    return typeinfo


def get_type_info(t):
    if hasattr(t, '__type_info__'):
        ret = t.__type_info__()
        assert (ret.python_class is not None)
        return ret
    elif isinstance(t, type):
        return Type(t.__name__, python_class=t)
    elif hasattr(t, '__call__'):
        return get_python_method_type(t)
    else:
        raise TypeError("Unsupported type %s" % t)


def get_python_class_methods(cls):
    ret = {}
    for key, value in cls.__dict__.items():
        if hasattr(value, '__call__'):
            ret[key] = value
    return ret


def get_python_class_slots(class_type):
    if hasattr(class_type, '__slots__'):
        if len(class_type.__slots__) != len(class_type.__slot_types__):
            raise RuntimeError(
                "__slots__ and __slot_types__ length mismatch in class %s" % (str(class_type)))
        return class_type.__slots__
    else:
        return []


def get_python_class_slot_types(class_type):
    if hasattr(class_type, '__slots__'):
        if len(class_type.__slots__) != len(class_type.__slot_types__):
            raise RuntimeError("__slots__ and __slot_types__ length mismatch")
        return [get_type_info(x) for x in class_type.__slot_types__]
    else:
        return []

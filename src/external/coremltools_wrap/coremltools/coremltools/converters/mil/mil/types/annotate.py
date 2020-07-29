# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from six import string_types as _string_types


class delay_type_cls:
    def __getattr__(self, t):
        return t


# this delay type thingee is useful for class annotations.
# for instance: the following code is invalid because when the annotate
# function is invoked, the "double" class does not yet exist
#
# class double:
#    @annotate(double, other=double)
#    def __add__(self, other):
#
# So it is necessary to add one level of laziness and delay the type
#
# class double:
#    @annotate(delay_type.double, other=delay_type.double)
#    def __add__(self, other):
#
# This basically replaces the annotation with the string "double" which we will
# then replace with the actual type later
#
delay_type = delay_type_cls()

annotated_function_list = []
annotated_class_list = {}


class _invalid_placeholder_type:
    pass


def annotate(return_type=_invalid_placeholder_type, **kwargs):
    """
    A decorator that informs the compyler about the return type of a function
    and a collection of hint for other variable names. These can include
     - captured variables
     - function arguments
     - other variables within the function

    Ex:

        @annotate(compyler.double, a=compyler.double, b=compyler.double)
        def add(a, b):

    In certain cases when the class members are annotated this does not work.
    For instance this fails because the annotate decorator is called before
    the class double is fully defined.

        class double:
            @annotate(double, other=double)
            def __add__(self, other):

     So it is necessary to add one level of laziness and delay the type

        @class_annotate()
        class double:
            @annotate(delay_type.double, other=delay_type.double)
            def __add__(self, other):

    After which apply_delayed_types() must be called to fill in the delayed
    type.
    """
    global annotated_function_list

    def decorator(func):
        global annotated_function_list
        func.type_annotations = kwargs
        if return_type is not _invalid_placeholder_type:
            func.return_type = return_type
        annotated_function_list += [func]
        return func

    return decorator


def class_annotate():
    """
    Registers a class to be used by delay_type. See annotate()
    """
    global annotated_class_list

    def decorator(cls):
        global annotated_class_list
        annotated_class_list[cls.__name__] = cls
        return cls

    return decorator


def apply_delayed_types(
    type_map=annotated_class_list, fnlist=annotated_function_list
):  # pylint: disable=dangerous-default-value
    """
    Apply all delayed types. See annotate()
    """
    # pylint: disable=no-member
    # type name is a dict from str to type
    for func in fnlist:
        if (
            hasattr(func, "return_type")
            and isinstance(func.return_type, _string_types)
            and func.return_type in type_map
        ):
            func.return_type = type_map[func.return_type]
        if hasattr(func, "type_annotations"):
            for key in func.type_annotations:
                if func.type_annotations[key] in type_map:
                    func.type_annotations[key] = type_map[func.type_annotations[key]]

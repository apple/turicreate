# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import numpy as np
from numpy import nan
import array
import datetime as dt
from ..data_structures import image
from .. import SArray
import os
from .._cython.cy_flexible_type import (
    _translate_through_flexible_type as _flexible_type,
)
from .._cython.cy_flexible_type import _translate_through_flex_list as _tr_flex_list
from .._cython.cy_flexible_type import infer_type_of_list
from .._cython.cy_flexible_type import _get_inferred_column_type, _all_convertable
from .._cython.cy_flexible_type import _check_ft_pyobject_hint_path
from .._cython.cy_flexible_type import pytype_from_type_name
from .._cython.cy_flexible_type import GMT
import datetime
from itertools import product
from copy import copy

import sys

if sys.version_info.major > 2:
    long = int
    unicode = str

NoneType = type(None)

current_file_dir = os.path.dirname(os.path.realpath(__file__))


def from_lambda(v):
    from .._connect import main as glconnect

    u = glconnect.get_unity()
    return u.eval_lambda(lambda x: x, v)


special_types = set()

IntegerValue = [int(0), long(1)] + [
    _dt(0)
    for _dt in (np.sctypes["int"] + np.sctypes["uint"] + [np.bool, bool, np.bool_])
]
special_types.add(id(IntegerValue))

# 2**63 and -2**63-1 are not representable by a C int64_t, so it's
# treated as a float.
FloatValue = (
    [float(0)] + [_dt(0) for _dt in np.sctypes["float"]] + [2 ** 63, -(2 ** 63) - 1]
)
special_types.add(id(FloatValue))

StringValue = (
    [str("bork"), unicode("bork"), b"bork", b""]
    + [
        _dt("bork")
        for _dt in [np.unicode, np.unicode_, str, unicode, np.str, np.str_, np.string_]
    ]
    + [str(""), unicode("")]
    + [
        _dt("")
        for _dt in [np.unicode, np.unicode_, str, unicode, np.str, np.str_, np.string_]
    ]
)

special_types.add(id(StringValue))

DictValue = [{"a": 12}, dict()]
special_types.add(id(DictValue))

DatetimeValue = [
    datetime.date(2000, 6, 12),
    datetime.date(1100, 1, 1),
    datetime.datetime(2000, 6, 12),
]
special_types.add(id(DatetimeValue))

AnyValue = IntegerValue + FloatValue + StringValue + DatetimeValue + DictValue
special_types.add(id(AnyValue))

# All the different types of float sequences we support
FloatSequence = [
    [0.5, 1.5, 2.5],
    (0.5, 1.5, 2.5),
    {0.5, 1.5, 2.5},
    frozenset([0.5, 1.5, 2.5]),
] + [array.array(c, [0.5, 1.5, 2.5]) for c in "fd"]
special_types.add(id(FloatSequence))

# All the different types of float sequences we support
FloatSequenceWithNAN = [
    [0.5, 1.5, 2.5, nan],
    (0.5, 1.5, 2.5, nan),
    {0.5, 1.5, 2.5, nan},
    frozenset([0.5, 1.5, 2.5, nan]),
] + [array.array(c, [0.5, 1.5, 2.5, nan]) for c in "fd"]
special_types.add(id(FloatSequenceWithNAN))

# All the different types of float sequences we support
FloatSequenceWithNone = [[0.5, 1.5, 2.5, None], (0.5, 1.5, 2.5, None)]
special_types.add(id(FloatSequenceWithNone))

# All the different integer sequences we support
IntegerSequence = [
    [int(i) for i in range(3)],
    [long(i) for i in range(3)],
    tuple(range(3)),
    tuple(long(i) for i in range(3)),
    set(range(3)),
    frozenset(range(3)),
] + [array.array(c, range(3)) for c in "bBhHiIlL"]
special_types.add(id(IntegerSequence))

# All the different integer sequences we support, with a Nan
IntegerSequenceWithNAN = [
    [int(i) for i in range(3)] + [nan],
    [long(i) for i in range(3)] + [nan],
    tuple(range(3)) + (nan,),
    tuple(long(i) for i in range(3)) + (nan,),
    set([long(i) for i in range(3)] + [nan]),
    frozenset([long(i) for i in range(3)] + [nan]),
]
special_types.add(id(IntegerSequenceWithNAN))

# All the different types of string
IntegerSequenceWithNone = [
    [int(i) for i in range(3)] + [None],
    [long(i) for i in range(3)] + [None],
    tuple(range(3)) + (None,),
    tuple(long(i) for i in range(3)) + (None,),
    set([long(i) for i in range(3)] + [None]),
    frozenset([long(i) for i in range(3)] + [None]),
]
special_types.add(id(IntegerSequenceWithNone))

# Empty but typed float arrays
EmptyFloatArray = [array.array(c, []) for c in "fd"]
special_types.add(id(EmptyFloatArray))

# Empty but typed integer arrays
type_codes = "bBhHiIlL"
if sys.version_info.major == 2:
    type_codes += "c"
EmptyIntegerArray = [array.array(c, []) for c in type_codes]
special_types.add(id(EmptyIntegerArray))

# All empty arrays
EmptyArray = EmptyIntegerArray + EmptyFloatArray
special_types.add(id(EmptyArray))

EmptySequence = [[], tuple(), set()]
special_types.add(id(EmptySequence))

# Boolean Sequences
BooleanSequence = [
    list((i % 2 == 0) for i in range(3)),
    tuple((i % 2 == 0) for i in range(3)),
    set([True]),
    set([False]),
    set([True, False]),
]
special_types.add(id(BooleanSequence))

# String sequences
StringSequence = [
    list(str(i) for i in range(3)),
    tuple(str(i) for i in range(3)),
    set(str(i) for i in range(3)),
    frozenset(str(i) for i in range(3)),
]
special_types.add(id(StringSequence))

AnySequence = (
    EmptySequence
    + BooleanSequence
    + StringSequence
    + IntegerSequence
    + IntegerSequenceWithNone
    + IntegerSequenceWithNAN
    + FloatSequence
    + FloatSequenceWithNone
    + FloatSequenceWithNAN
    + EmptyArray
)
special_types.add(id(AnySequence))


def verify_inference(values, expected_type):

    # Go through and build a list of all the possible value enumerations that need to be tested.
    def build_lookups(values, L):
        for v in values:
            if id(v) in special_types:
                L.append(range(len(v)))
            elif type(v) is list:
                build_lookups(v, L)

    indices = []
    build_lookups(values, indices)

    def get_value(values, idx_set):
        ret = copy(values)
        for i, v in enumerate(values):
            if id(v) in special_types:
                ret[i] = v[idx_set[-1]]
                del idx_set[-1]
            elif type(v) is list:
                ret[i] = get_value(v, idx_set)
        return ret

    for idx_set in product(*reversed(indices)):
        _v_list = get_value(values, list(idx_set))

        for add_none in [True, False]:
            v_list = _v_list + [None] if add_none else _v_list

            inferred_type, result = _get_inferred_column_type(v_list)

            if inferred_type != expected_type:
                assert False, "Expected type %s, got type %s; input value = %s." % (
                    str(expected_type),
                    str(inferred_type),
                    str(v_list),
                )

            if inferred_type != NoneType:
                reconverted_result = _tr_flex_list(result, inferred_type)
                assert str(result) == str(reconverted_result), (
                    "Values in type translated inconsistently: "
                    "\nInput value  = %s"
                    "\nOutput value = %s"
                    "\nReconverted  = %s"
                ) % (str(v_list), str(result), reconverted_result)


class FlexibleTypeInference(unittest.TestCase):
    def test_int_float(self):
        verify_inference([IntegerValue], int)
        verify_inference([IntegerValue, IntegerValue], int)
        verify_inference([IntegerValue, FloatValue], float)
        verify_inference([IntegerValue, nan], float)
        verify_inference([], float)
        verify_inference([None], float)
        verify_inference([IntegerValue, nan], float)
        verify_inference([IntegerValue, None, nan], float)
        verify_inference([IntegerValue, None, FloatValue], float)
        verify_inference([IntegerValue, None, FloatValue, nan], float)

    def test_string(self):
        verify_inference([StringValue], str)
        verify_inference([StringValue, StringValue], str)
        verify_inference([StringValue, IntegerValue], NoneType)
        verify_inference([StringValue, FloatValue], NoneType)

    def test_dict(self):
        verify_inference([DictValue], dict)
        verify_inference([DictValue, DictValue], dict)

    def test_mixed_types(self):
        verify_inference([AnySequence, AnyValue], NoneType)
        verify_inference([AnySequence, AnyValue, AnySequence], NoneType)
        verify_inference([AnySequence, AnyValue, AnyValue], NoneType)
        verify_inference([DatetimeValue, StringValue], NoneType)
        verify_inference([DatetimeValue, IntegerValue], NoneType)
        verify_inference([DatetimeValue, FloatValue], NoneType)

    def test_array_list(self):

        tests = [
            # Individual types
            ([EmptySequence], list),
            ([IntegerSequence], array.array),
            ([IntegerSequenceWithNone], list),
            ([IntegerSequenceWithNAN], array.array),
            ([FloatSequence], array.array),
            ([FloatSequenceWithNAN], array.array),
            ([FloatSequenceWithNone], list),
            ([EmptyIntegerArray], array.array),
            ([EmptyFloatArray], array.array),
            ([BooleanSequence], array.array),
            ([StringSequence], list),
            # Multiple types
            ([IntegerSequence, FloatSequence], array.array),
            ([IntegerSequence, FloatSequence], array.array),
            # Multiple types
            ([EmptySequence, EmptyFloatArray], array.array),
            ([EmptySequence, EmptyIntegerArray], array.array),
            ([EmptySequence, IntegerSequence], array.array),
            ([EmptySequence, FloatSequence], array.array),
            # Multiple types
            ([EmptySequence, EmptyFloatArray], array.array),
            ([EmptySequence, EmptyIntegerArray], array.array),
            ([EmptySequence, IntegerSequence], array.array),
            ([EmptySequence, FloatSequence], array.array),
            # Arrays and lists
            ([StringSequence, EmptyFloatArray], list),
            ([StringSequence, EmptyIntegerArray], list),
            ([StringSequence, IntegerSequence], list),
            ([StringSequence, FloatSequence], list),
        ]

        # Add in additional rules for testing
        for tv, res in copy(tests):
            tests.append((tv + [EmptySequence], res))

        for tv, res in copy(tests):
            tests.append((tv + [[None]], list))

        for tv, res in copy(tests):
            tests.append((tv + [StringSequence], list))

        # Run the tests
        for tv, res in tests:
            verify_inference(tv, res)

    def test_nparray(self):
        NPSequence = (
            [np.array(range(3), "d"), None],
            [np.array(range(3), "i"), None],
            [np.array(range(3), "f"), None],
            [np.array(range(3), "d"), array.array("d", [1, 2, 3])],
            [np.array(range(3), "i"), array.array("d", [1, 2, 3])],
            [np.array(range(3), "f"), array.array("d", [1, 2, 3])],
            [np.array(range(3), "d"), array.array("d", [1, 2, 3]), None],
            [np.array(range(3), "i"), array.array("d", [1, 2, 3]), None],
            [np.array(range(3), "f"), array.array("d", [1, 2, 3]), None],
        )

        # Run the tests
        for seq in NPSequence:
            inferred_type, result = _get_inferred_column_type(seq)
            self.assertEqual(inferred_type, np.ndarray)
            reconverted_result = _tr_flex_list(result, inferred_type)


class FlexibleTypeTest(unittest.TestCase):

    # On lambda return, if the return value is a non-empty of list of
    # all numerical values, we try hard to use array.array
    def numeric_list_to_array(self, v):
        if (
            (type(v) is list)
            and (len(v) > 0)
            and all((type(x) is int) or (type(x) is float) for x in v)
        ):
            return array.array("d", v)
        elif type(v) is list:
            return [self.numeric_list_to_array(x) for x in v]
        else:
            return v

    def assert_equal_with_lambda_check(self, translated, correct):
        self.assertEqual(translated, correct)
        self.assertEqual(from_lambda(translated), self.numeric_list_to_array(correct))

    def test_none(self):
        self.assert_equal_with_lambda_check(_flexible_type(None), None)

    def test_date_time(self):
        d = datetime.datetime(2010, 10, 10, 10, 10, 10)
        self.assert_equal_with_lambda_check(_flexible_type(d), d)

    def test_int(self):
        self.assert_equal_with_lambda_check(_flexible_type(1), 1)
        self.assert_equal_with_lambda_check(_flexible_type(long(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(True), 1)
        self.assert_equal_with_lambda_check(_flexible_type(False), 0)
        # numpy types
        self.assert_equal_with_lambda_check(_flexible_type(np.int_(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.int64(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.int32(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.int16(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.uint64(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.uint32(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.uint16(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.bool(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.bool(0)), 0)
        self.assert_equal_with_lambda_check(_flexible_type(np.bool_(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.bool_(0)), 0)
        self.assert_equal_with_lambda_check(_flexible_type(np.bool8(1)), 1)
        self.assert_equal_with_lambda_check(_flexible_type(np.bool8(0)), 0)

    def test_float(self):
        self.assert_equal_with_lambda_check(_flexible_type(0.25), 0.25)
        # numpy types
        self.assert_equal_with_lambda_check(_flexible_type(np.float(0.25)), 0.25)
        self.assert_equal_with_lambda_check(_flexible_type(np.float_(0.25)), 0.25)
        self.assert_equal_with_lambda_check(_flexible_type(np.float16(0.25)), 0.25)
        self.assert_equal_with_lambda_check(_flexible_type(np.float32(0.25)), 0.25)
        self.assert_equal_with_lambda_check(_flexible_type(np.float64(0.25)), 0.25)

    def test_string(self):
        self.assert_equal_with_lambda_check(_flexible_type("a"), "a")
        if sys.version_info.major == 2:
            self.assert_equal_with_lambda_check(_flexible_type(unicode("a")), "a")
        # numpy types
        self.assert_equal_with_lambda_check(_flexible_type(np.string_("a")), "a")
        self.assert_equal_with_lambda_check(_flexible_type(np.unicode_("a")), "a")

    def test_array(self):
        # float array
        expected = array.array("d", [0.1, 0.2, 0.3])
        self.assert_equal_with_lambda_check(_flexible_type(expected), expected)

        # int array
        expected = array.array("d", [1, 2, 3])
        self.assert_equal_with_lambda_check(_flexible_type([1, 2, 3]), expected)
        self.assert_equal_with_lambda_check(_flexible_type([1.0, 2.0, 3.0]), expected)
        self.assert_equal_with_lambda_check(_flexible_type([1, 2, 3.0]), expected)

        # numpy ndarray
        expected = np.asarray([1, 2, 3])
        self.assertSequenceEqual(list(_flexible_type(expected)), list(expected))
        self.assertSequenceEqual(
            list(from_lambda(expected)), array.array("d", expected)
        )

        expected = np.asarray([0.1, 0.2, 0.3])
        self.assertSequenceEqual(list(_flexible_type(expected)), list(expected))
        self.assertSequenceEqual(
            list(from_lambda(expected)), array.array("d", expected)
        )

    def test_dict(self):
        d = dt.datetime(2010, 10, 10, 10, 10, 10)
        img = image.Image(current_file_dir + "/images/nested/sample_grey.jpg", "JPG")
        expected = {
            "int": 0,
            "float": 0.1,
            "str": "str",
            "list": ["a", "b", "c"],
            "array": array.array("d", [1, 2, 3]),
            "datetime": [d],
            "image": img,
            "none": None,
        }
        self.assert_equal_with_lambda_check(_flexible_type(expected), expected)
        self.assert_equal_with_lambda_check(_flexible_type({}), {})

        expected = [{"a": 1, "b": 20, "c": None}, {"b": 4, None: 5}, None, {"a": 0}]
        self.assert_equal_with_lambda_check(_flexible_type(expected), expected)

    def test_list(self):
        d = dt.datetime(2010, 10, 10, 10, 10, 10)
        img = image.Image(current_file_dir + "/images/nested/sample_grey.jpg", "JPG")
        expected = [
            None,
            img,
            1,
            0.1,
            "1",
            d,
            array.array("d", [1, 2, 3]),
            {"foo": array.array("d", [1, 2, 3])},
        ]

        self.assert_equal_with_lambda_check(_flexible_type(expected), expected)
        self.assert_equal_with_lambda_check(_flexible_type([]), [])
        self.assert_equal_with_lambda_check(_flexible_type([[], []]), [[], []])

    def test_image(self):
        img_gray_jpg = image.Image(
            current_file_dir + "/images/nested/sample_grey.jpg", "JPG"
        )
        img_gray_png = image.Image(
            current_file_dir + "/images/nested/sample_grey.png", "PNG"
        )
        img_gray_auto_jpg = image.Image(
            current_file_dir + "/images/nested/sample_grey.jpg"
        )
        img_gray_auto_png = image.Image(
            current_file_dir + "/images/nested/sample_grey.png"
        )
        img_color_jpg = image.Image(current_file_dir + "/images/sample.jpg", "JPG")
        img_color_png = image.Image(current_file_dir + "/images/sample.png", "PNG")
        img_color_auto_jpg = image.Image(current_file_dir + "/images/sample.jpg")
        img_color_auto_png = image.Image(current_file_dir + "/images/sample.png")

        self.assert_equal_with_lambda_check(_flexible_type(img_gray_jpg), img_gray_jpg)
        self.assert_equal_with_lambda_check(_flexible_type(img_gray_png), img_gray_png)
        self.assert_equal_with_lambda_check(
            _flexible_type(img_gray_auto_jpg), img_gray_auto_jpg
        )
        self.assert_equal_with_lambda_check(
            _flexible_type(img_gray_auto_png), img_gray_png
        )
        self.assert_equal_with_lambda_check(
            _flexible_type(img_color_jpg), img_color_jpg
        )
        self.assert_equal_with_lambda_check(
            _flexible_type(img_color_png), img_color_png
        )
        self.assert_equal_with_lambda_check(
            _flexible_type(img_color_auto_jpg), img_color_auto_jpg
        )
        self.assert_equal_with_lambda_check(
            _flexible_type(img_color_auto_png), img_color_auto_png
        )

    def test_tr_flex_list(self):
        expected = []
        self.assert_equal_with_lambda_check(_tr_flex_list(expected), expected)

        # test int list
        expected = [1, 2, 3, 4, 5, None]
        self.assert_equal_with_lambda_check(_tr_flex_list(expected), expected)
        self.assert_equal_with_lambda_check(_tr_flex_list(expected, int), expected)
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, int, ignore_cast_failure=True), expected
        )

        # test datetime list
        from_zone = GMT(0)
        to_zone = GMT(4.5)
        d1 = dt.datetime(2010, 10, 10, 10, 10, 10).replace(tzinfo=from_zone)
        d2 = d1.astimezone(to_zone)
        expected = [d1, d2, None]
        self.assert_equal_with_lambda_check(_tr_flex_list(expected), expected)
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, dt.datetime), expected
        )
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, dt.datetime, ignore_cast_failure=True), expected
        )
        # test image list
        img_gray_auto_png = image.Image(
            current_file_dir + "/images/nested/sample_grey.png"
        )
        img_color_jpg = image.Image(current_file_dir + "/images/sample.jpg", "JPG")
        expected = [img_gray_auto_png, img_color_jpg, None]
        self.assert_equal_with_lambda_check(_tr_flex_list(expected), expected)
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, image.Image), expected
        )
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, image.Image, ignore_cast_failure=True), expected
        )
        # test str list
        expected = ["a", "b", "c", None]
        self.assert_equal_with_lambda_check(_tr_flex_list(expected), expected)
        self.assert_equal_with_lambda_check(_tr_flex_list(expected, str), expected)

        # test array list
        expected = [array.array("d", range(5)), array.array("d", range(5)), None]
        self.assert_equal_with_lambda_check(_tr_flex_list(expected), expected)
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, array.array), expected
        )
        expected = [[float(i) for i in range(5)], range(5), None]
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected),
            [array.array("d", range(5)), array.array("d", range(5)), None],
        )

        # test int array
        expected = array.array("d", range(5))
        self.assert_equal_with_lambda_check(_tr_flex_list(expected), list(range(5)))

        expected = [1, 1.0, "1", [1.0, 1.0, 1.0], ["a", "b", "c"], {}, {"a": 1}, None]
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, int, ignore_cast_failure=True), [1, 1, None]
        )
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, float, ignore_cast_failure=True), [1.0, 1.0, None]
        )
        # Anything can be cast to a string
        # self.assert_equal_with_lambda_check(_tr_flex_list(expected, str, ignore_cast_failure=True), ['1', '1', None])
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, array.array, ignore_cast_failure=True),
            [array.array("d", [1.0, 1.0, 1.0]), None],
        )
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, list, ignore_cast_failure=True),
            [[1.0, 1.0, 1.0], ["a", "b", "c"], None],
        )
        self.assert_equal_with_lambda_check(
            _tr_flex_list(expected, dict, ignore_cast_failure=True),
            [{}, {"a": 1}, None],
        )

    def test_infer_list_type(self):
        self.assertEqual(
            infer_type_of_list(
                [
                    image.Image(current_file_dir + "/images/nested/sample_grey.png"),
                    image.Image(current_file_dir + "/images/sample.jpg", "JPG"),
                    image.Image(current_file_dir + "/images/sample.png"),
                ]
            ),
            image.Image,
        )
        self.assertEqual(
            infer_type_of_list(
                [
                    dt.datetime(2010, 10, 10, 10, 10, 10),
                    dt.datetime(2000, 5, 7, 10, 4, 10),
                    dt.datetime(1845, 5, 7, 4, 4, 10),
                ]
            ),
            dt.datetime,
        )
        self.assertEqual(infer_type_of_list([0, 1, 2]), int)
        self.assertEqual(infer_type_of_list([0, 1, 2.0]), float)
        self.assertEqual(infer_type_of_list(["foo", u"bar"]), str)
        self.assertEqual(
            infer_type_of_list(
                [array.array("d", [1, 2, 3]), array.array("d", [1, 2, 3])]
            ),
            array.array,
        )
        self.assertEqual(
            infer_type_of_list([[], [1.0, 2.0, 3.0], array.array("d", [1, 2, 3])]),
            array.array,
        )
        self.assertEqual(
            infer_type_of_list([[], [1, 2, 3], array.array("d", [1, 2, 3])]),
            array.array,
        )
        self.assertEqual(infer_type_of_list([{"a": 1}, {"b": 2}]), dict)

    def test_datetime_lambda(self):
        d = dt.datetime.now()
        sa = SArray([d])

        # Lambda returning self
        sa_self = sa.apply(lambda x: x)
        for i in range(len(sa_self)):
            self.assertEqual(sa[i], sa_self[i])

        # Lambda returning year
        sa_year = sa.apply(lambda x: x.year)
        for i in range(len(sa_year)):
            self.assertEqual(sa[i].year, sa_year[i])

        # Lambda returning second
        sa_sec = sa.apply(lambda x: x.second)
        for i in range(len(sa_sec)):
            self.assertEqual(sa[i].second, sa_sec[i])

    def test_flexible_type_hint(self):

        _check_ft_pyobject_hint_path(1, int)
        _check_ft_pyobject_hint_path(1, float)
        _check_ft_pyobject_hint_path(1.5, float)

        _check_ft_pyobject_hint_path([], list)
        _check_ft_pyobject_hint_path([1], list)
        _check_ft_pyobject_hint_path((1, 2), list)

        _check_ft_pyobject_hint_path({1: 1}, dict)
        _check_ft_pyobject_hint_path(array.array("i", [1, 2]), array.array)
        _check_ft_pyobject_hint_path(array.array("d", [1, 2]), array.array)

    def test_pytype_from_type_name(self):

        self.assertEqual(pytype_from_type_name("str"), str)
        self.assertEqual(pytype_from_type_name("string"), str)
        self.assertEqual(pytype_from_type_name("float"), float)
        self.assertEqual(pytype_from_type_name("datetime"), datetime.datetime)
        self.assertEqual(pytype_from_type_name("image"), image.Image)
        self.assertEqual(pytype_from_type_name("list"), list)
        self.assertEqual(pytype_from_type_name("undefined"), type(None))

        self.assertRaises(ValueError, lambda: pytype_from_type_name("happiness"))

    def test_type_conversions(self):
        # testing valid sarray of inf's (inf is a float)
        sa_all_inf = SArray(["inf", "Inf", "iNf", "inF", "INF"])
        sa_all_inf.astype(float)  # should not raise error so we good
        # testing invalid sarray of float words
        sa_float_words = SArray(["inf", "infiltrate", "nanana", "2.0version"])
        with self.assertRaises(RuntimeError):
            sa_float_words.astype(float)
        # testing invalid sarray of int words
        sa_int_words = SArray(["1world", "2dreams", "3000apples"])
        with self.assertRaises(RuntimeError):
            sa_int_words.astype(int)

    def test_hashable_dict_keys(self):
        # Make sure that the keys of a dictionary are actually expressable as keys.
        sa_dictionary = SArray([{(1, 2): 3}])
        out = list(sa_dictionary)

        self.assertEqual(out[0][(1, 2)], 3)

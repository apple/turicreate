# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
# This software may be modified and distributed under the terms
# of the BSD license. See the LICENSE file for details.
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..data_structures.sarray import SArray
from ..data_structures.sframe import SFrame
from ..data_structures.sarray import load_sarray
from ..toolkits.image_analysis.image_analysis import load_images
from .._cython.cy_flexible_type import GMT
from . import util

import pandas as pd
import numpy as np
import unittest
import random
import datetime as dt
import copy
import os
import math
import shutil
import array
import time
import warnings
import functools
import tempfile
import sys
import six
import json


class SArrayTest(unittest.TestCase):
    def setUp(self):
        self.int_data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        self.bool_data = [x % 2 == 0 for x in range(10)]
        self.datetime_data = [
            dt.datetime(2013, 5, 7, 10, 4, 10),
            dt.datetime(1902, 10, 21, 10, 34, 10).replace(tzinfo=GMT(0.0)),
            None,
        ]
        self.datetime_data2 = [
            dt.datetime(2013, 5, 7, 10, 4, 10, 109321),
            dt.datetime(1902, 10, 21, 10, 34, 10, 991111).replace(tzinfo=GMT(0.0)),
            None,
        ]
        self.float_data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        self.string_data = [
            "abc",
            "def",
            "hello",
            "world",
            "pika",
            "chu",
            "hello",
            "world",
        ]
        self.vec_data = [array.array("d", [i, i + 1]) for i in self.int_data]
        self.np_array_data = [np.array(x) for x in self.vec_data]
        self.empty_np_array_data = [np.array([])]
        self.np_matrix_data = [np.matrix(x) for x in self.vec_data]
        self.list_data = [[i, str(i), i * 1.0] for i in self.int_data]
        self.dict_data = [{str(i): i, i: float(i)} for i in self.int_data]
        # json dict only allows string keys
        self.dict_json_data = [{str(i): i} for i in self.int_data]
        self.url = "http://s3-us-west-2.amazonaws.com/testdatasets/a_to_z.txt.gz"

    def __test_equal(self, _sarray, _data, _type):
        self.assertEqual(_sarray.dtype, _type)
        self.assertEqual(len(_sarray), len(_data))
        sarray_contents = list(_sarray.head(len(_sarray)))
        if _type == np.ndarray:
            # Special case for np.ndarray elements, which assertSequenceEqual
            # does not handle.
            np.testing.assert_array_equal(sarray_contents, _data)
        else:
            # Use unittest methods when possible for better consistency.
            self.assertSequenceEqual(sarray_contents, _data)

    def __test_almost_equal(self, _sarray, _data, _type):
        self.assertEqual(_sarray.dtype, _type)
        self.assertEqual(len(_sarray), len(_data))
        l = list(_sarray)
        for i in range(len(l)):
            if type(l[i]) in (list, array.array):
                for j in range(len(l[i])):
                    self.assertAlmostEqual(l[i][j], _data[i][j])
            else:
                self.assertAlmostEqual(l[i], _data[i])

    def __test_creation_raw(self, data, dtype, expected):
        s = SArray(data, dtype)
        self.__test_equal(s, expected, dtype)

    def __test_creation_pd(self, data, dtype, expected):
        s = SArray(pd.Series(data), dtype)
        self.__test_equal(s, expected, dtype)

    def __test_creation(self, data, dtype, expected):
        """
        Create sarray from data with dtype, and test it equals to
        expected.
        """
        self.__test_creation_raw(data, dtype, expected)
        self.__test_creation_pd(data, dtype, expected)

    def __test_creation_type_inference_raw(self, data, expected_dtype, expected):
        s = SArray(data)
        self.__test_equal(s, expected, expected_dtype)

    def __test_creation_type_inference_pd(self, data, expected_dtype, expected):
        s = SArray(pd.Series(data))
        self.__test_equal(s, expected, expected_dtype)

    def __test_creation_type_inference(self, data, expected_dtype, expected):
        """
        Create sarray from data with dtype, and test it equals to
        expected.
        """
        self.__test_creation_type_inference_raw(data, expected_dtype, expected)
        self.__test_creation_type_inference_pd(data, expected_dtype, expected)

    def test_creation(self):
        self.__test_creation(self.int_data, int, self.int_data)
        self.__test_creation(self.int_data, float, [float(x) for x in self.int_data])
        self.__test_creation(self.int_data, str, [str(x) for x in self.int_data])

        self.__test_creation(self.float_data, float, self.float_data)
        self.assertRaises(TypeError, self.__test_creation, [self.float_data, int])

        self.__test_creation(self.string_data, str, self.string_data)
        self.assertRaises(TypeError, self.__test_creation, [self.string_data, int])
        self.assertRaises(TypeError, self.__test_creation, [self.string_data, float])

        expected_output = [chr(x) for x in range(ord("a"), ord("a") + 26)]
        self.__test_equal(SArray(self.url, str), expected_output, str)

        self.__test_creation(self.vec_data, array.array, self.vec_data)
        self.__test_creation(self.np_array_data, np.ndarray, self.np_array_data)
        self.__test_creation(
            self.empty_np_array_data, np.ndarray, self.empty_np_array_data
        )
        self.__test_creation(self.np_matrix_data, np.ndarray, self.np_matrix_data)
        self.__test_creation(self.list_data, list, self.list_data)

        self.__test_creation(self.dict_data, dict, self.dict_data)

        # test with map/filter type
        self.__test_creation_raw(
            map(lambda x: x + 10, self.int_data), int, [x + 10 for x in self.int_data]
        )
        self.__test_creation_raw(
            map(lambda x: x * 10, self.int_data),
            float,
            [float(x) * 10 for x in self.int_data],
        )
        self.__test_creation_raw(
            map(lambda x: x * 10, self.string_data),
            str,
            [x * 10 for x in self.string_data],
        )

        self.__test_creation_raw(
            filter(lambda x: x < 5, self.int_data),
            int,
            list(filter(lambda x: x < 5, self.int_data)),
        )
        self.__test_creation_raw(
            filter(lambda x: x > 5, self.float_data),
            float,
            list(filter(lambda x: x > 5, self.float_data)),
        )
        self.__test_creation_raw(
            filter(lambda x: len(x) > 3, self.string_data),
            str,
            list(filter(lambda x: len(x) > 3, self.string_data)),
        )

        self.__test_creation_pd(
            map(lambda x: x + 10, self.int_data), int, [x + 10 for x in self.int_data]
        )
        self.__test_creation_pd(
            map(lambda x: x * 10, self.int_data),
            float,
            [float(x) * 10 for x in self.int_data],
        )
        self.__test_creation_pd(
            map(lambda x: x * 10, self.string_data),
            str,
            [x * 10 for x in self.string_data],
        )

        # test with type inference
        self.__test_creation_type_inference(self.int_data, int, self.int_data)
        self.__test_creation_type_inference(self.float_data, float, self.float_data)
        self.__test_creation_type_inference(
            self.bool_data, int, [int(x) for x in self.bool_data]
        )
        self.__test_creation_type_inference(self.string_data, str, self.string_data)
        self.__test_creation_type_inference(self.vec_data, array.array, self.vec_data)
        self.__test_creation_type_inference(
            self.np_array_data, np.ndarray, self.np_array_data
        )
        self.__test_creation_type_inference(
            self.empty_np_array_data, np.ndarray, self.empty_np_array_data
        )
        self.__test_creation_type_inference(
            self.np_matrix_data, np.ndarray, self.np_matrix_data
        )
        self.__test_creation_type_inference(
            [np.bool_(True), np.bool_(False)], int, [1, 0]
        )
        self.__test_creation((1, 2, 3, 4), int, [1, 2, 3, 4])

        self.__test_creation_type_inference_raw(
            map(lambda x: x + 10, self.int_data), int, [x + 10 for x in self.int_data]
        )
        self.__test_creation_type_inference_raw(
            map(lambda x: x * 10, self.float_data),
            float,
            [x * 10 for x in self.float_data],
        )
        self.__test_creation_type_inference_raw(
            map(lambda x: x * 10, self.string_data),
            str,
            [x * 10 for x in self.string_data],
        )

        self.__test_creation_type_inference_pd(
            map(lambda x: x + 10, self.int_data), int, [x + 10 for x in self.int_data]
        )
        self.__test_creation_type_inference_pd(
            map(lambda x: x * 10, self.float_data),
            float,
            [float(x) * 10 for x in self.float_data],
        )
        self.__test_creation_type_inference_pd(
            map(lambda x: x * 10, self.string_data),
            str,
            [x * 10 for x in self.string_data],
        )

        self.__test_creation_type_inference_raw(
            filter(lambda x: x < 5, self.int_data),
            int,
            list(filter(lambda x: x < 5, self.int_data)),
        )
        self.__test_creation_type_inference_raw(
            filter(lambda x: x > 5, self.float_data),
            float,
            list(filter(lambda x: x > 5, self.float_data)),
        )
        self.__test_creation_type_inference_raw(
            filter(lambda x: len(x) > 3, self.string_data),
            str,
            list(filter(lambda x: len(x) > 3, self.string_data)),
        )

        # genertors
        def __generator_parrot(data):
            for ii in data:
                yield ii

        self.__test_creation_raw(__generator_parrot(self.int_data), int, self.int_data)
        self.__test_creation_raw(
            __generator_parrot(self.float_data), float, self.float_data
        )
        self.__test_creation_raw(
            __generator_parrot(self.string_data), str, self.string_data
        )

        self.__test_creation_pd(__generator_parrot(self.int_data), int, self.int_data)
        self.__test_creation_pd(
            __generator_parrot(self.float_data), float, self.float_data
        )
        self.__test_creation_pd(
            __generator_parrot(self.string_data), str, self.string_data
        )

        self.__test_creation_type_inference_raw(
            __generator_parrot(self.int_data), int, self.int_data
        )
        self.__test_creation_type_inference_raw(
            __generator_parrot(self.float_data), float, self.float_data
        )
        self.__test_creation_type_inference_raw(
            __generator_parrot(self.string_data), str, self.string_data
        )

        self.__test_creation_type_inference_pd(
            __generator_parrot(self.int_data), int, self.int_data
        )
        self.__test_creation_type_inference_pd(
            __generator_parrot(self.float_data), float, self.float_data
        )
        self.__test_creation_type_inference_pd(
            __generator_parrot(self.string_data), str, self.string_data
        )

        # Test numpy types, which are not compatible with the pd.Series path in
        # __test_creation and __test_creation_type_inference
        self.__test_equal(
            SArray(np.array(self.vec_data), array.array), self.vec_data, array.array
        )
        self.__test_equal(
            SArray(np.matrix(self.vec_data), array.array), self.vec_data, array.array
        )
        self.__test_equal(SArray(np.array(self.vec_data)), self.vec_data, array.array)
        self.__test_equal(SArray(np.matrix(self.vec_data)), self.vec_data, array.array)

        # Test python 3
        self.__test_equal(
            SArray(filter(lambda x: True, self.int_data)), self.int_data, int
        )
        self.__test_equal(SArray(map(lambda x: x, self.int_data)), self.int_data, int)

    def test_list_with_none_creation(self):
        tlist = [[2, 3, 4], [5, 6], [4, 5, 10, None]]
        g = SArray(tlist)
        self.assertEqual(len(g), len(tlist))
        for i in range(len(tlist)):
            self.assertEqual(g[i], tlist[i])

    def test_append_empty_sarray(self):
        existing_sa = SArray([1, 2, 3])
        new_sa = SArray()
        existing_sa = existing_sa.append(new_sa)
        self.assertTrue(int, existing_sa.dtype)

    def test_append_sarray_to_empty_sarray(self):
        existing_sa = SArray()
        new_sa = SArray([1, 2, 3])
        existing_sa = existing_sa.append(new_sa)
        self.assertTrue(int, existing_sa.dtype)

    def test_nan_to_none_type_conversion(self):
        sa = SArray([1.0, 2.0, float("nan"), 3.0])
        sa = sa.astype(int)
        self.assertTrue(None in sa)

    def test_list_with_array_creation(self):
        import array

        t = array.array("d", [1.1, 2, 3, 4, 5.5])
        g = SArray(t)
        self.assertEqual(len(g), len(t))
        self.assertEqual(g.dtype, float)
        glist = list(g)
        for i in range(len(glist)):
            self.assertAlmostEqual(glist[i], t[i])

        t = array.array("i", [1, 2, 3, 4, 5])
        g = SArray(t)
        self.assertEqual(len(g), len(t))
        self.assertEqual(g.dtype, int)
        glist = list(g)
        for i in range(len(glist)):
            self.assertEqual(glist[i], t[i])

    def test_in(self):
        sint = SArray(self.int_data, int)
        self.assertTrue(5 in sint)
        self.assertFalse(20 in sint)
        sstr = SArray(self.string_data, str)
        self.assertTrue("abc" in sstr)
        self.assertFalse("zzzzzz" in sstr)
        self.assertFalse("" in sstr)
        self.__test_equal(
            sstr.contains("ll"), ["ll" in i for i in self.string_data], int
        )
        self.__test_equal(sstr.contains("a"), ["a" in i for i in self.string_data], int)

        svec = SArray([[1.0, 2.0], [2.0, 3.0], [3.0, 4.0], [4.0, 5.0]], array.array)
        self.__test_equal(svec.contains(1.0), [1, 0, 0, 0], int)
        self.__test_equal(svec.contains(0.0), [0, 0, 0, 0], int)
        self.__test_equal(svec.contains(2), [1, 1, 0, 0], int)

        slist = SArray([[1, "22"], [2, "33"], [3, "44"], [4, None]], list)
        self.__test_equal(slist.contains(1.0), [1, 0, 0, 0], int)
        self.__test_equal(slist.contains(3), [0, 0, 1, 0], int)
        self.__test_equal(slist.contains("33"), [0, 1, 0, 0], int)
        self.__test_equal(slist.contains("3"), [0, 0, 0, 0], int)
        self.__test_equal(slist.contains(None), [0, 0, 0, 1], int)

        sdict = SArray([{1: "2"}, {2: "3"}, {3: "4"}, {"4": "5"}], dict)
        self.__test_equal(sdict.contains(1.0), [1, 0, 0, 0], int)
        self.__test_equal(sdict.contains(3), [0, 0, 1, 0], int)
        self.__test_equal(sdict.contains("4"), [0, 0, 0, 1], int)
        self.__test_equal(sdict.contains("3"), [0, 0, 0, 0], int)

        self.__test_equal(SArray(["ab", "bc", "cd"]).is_in("abc"), [1, 1, 0], int)
        self.__test_equal(SArray(["a", "b", "c"]).is_in(["a", "b"]), [1, 1, 0], int)
        self.__test_equal(
            SArray([1, 2, 3]).is_in(array.array("d", [1.0, 2.0])), [1, 1, 0], int
        )
        self.__test_equal(SArray([1, 2, None]).is_in([1, None]), [1, 0, 1], int)
        self.__test_equal(SArray([1, 2, None]).is_in([1]), [1, 0, 0], int)

    def test_save_load(self):

        # Check top level load function
        with util.TempDirectory() as f:
            sa = SArray(self.float_data)
            sa.save(f)
            sa2 = load_sarray(f)
            self.__test_equal(sa2, self.float_data, float)

        # Make sure these files don't exist before testing
        self._remove_sarray_files("intarr")
        self._remove_sarray_files("fltarr")
        self._remove_sarray_files("strarr")
        self._remove_sarray_files("vecarr")
        self._remove_sarray_files("listarr")
        self._remove_sarray_files("dictarr")

        sint = SArray(self.int_data, int)
        sflt = SArray([float(x) for x in self.int_data], float)
        sstr = SArray([str(x) for x in self.int_data], str)
        svec = SArray(self.vec_data, array.array)
        slist = SArray(self.list_data, list)
        sdict = SArray(self.dict_data, dict)

        sint.save("intarr.sidx")
        sflt.save("fltarr.sidx")
        sstr.save("strarr.sidx")
        svec.save("vecarr.sidx")
        slist.save("listarr.sidx")
        sdict.save("dictarr.sidx")

        sint2 = SArray("intarr.sidx")
        sflt2 = SArray("fltarr.sidx")
        sstr2 = SArray("strarr.sidx")
        svec2 = SArray("vecarr.sidx")
        slist2 = SArray("listarr.sidx")
        sdict2 = SArray("dictarr.sidx")
        self.assertRaises(IOError, lambda: SArray("__no_such_file__.sidx"))

        self.__test_equal(sint2, self.int_data, int)
        self.__test_equal(sflt2, [float(x) for x in self.int_data], float)
        self.__test_equal(sstr2, [str(x) for x in self.int_data], str)
        self.__test_equal(svec2, self.vec_data, array.array)
        self.__test_equal(slist2, self.list_data, list)
        self.__test_equal(sdict2, self.dict_data, dict)

        # cleanup
        del sint2
        del sflt2
        del sstr2
        del svec2
        del slist2
        del sdict2
        self._remove_sarray_files("intarr")
        self._remove_sarray_files("fltarr")
        self._remove_sarray_files("strarr")
        self._remove_sarray_files("vecarr")
        self._remove_sarray_files("listarr")
        self._remove_sarray_files("dictarr")

    def test_save_load_text(self):
        self._remove_single_file("txt_int_arr.txt")
        sint = SArray(self.int_data, int)
        sint.save("txt_int_arr.txt")
        self.assertTrue(os.path.exists("txt_int_arr.txt"))
        f = open("txt_int_arr.txt")
        lines = f.readlines()
        for i in range(len(sint)):
            self.assertEqual(int(lines[i]), sint[i])
        self._remove_single_file("txt_int_arr.txt")

        self._remove_single_file("txt_int_arr")
        sint.save("txt_int_arr", format="text")
        self.assertTrue(os.path.exists("txt_int_arr"))
        f = open("txt_int_arr")
        lines = f.readlines()
        for i in range(len(sint)):
            self.assertEqual(int(lines[i]), sint[i])
        self._remove_single_file("txt_int_arr")

    def test_read_json(self):
        # boolean type will be read in as int
        data_pairs = [
            ("int_data", int),
            ("bool_data", int),
            ("float_data", float),
            ("string_data", str),
            ("list_data", list),
            ("dict_json_data", dict),
        ]
        for attr, data_type in data_pairs:
            filename = attr + ".json"
            self._remove_single_file(filename)
            data = getattr(self, attr)

            with open(filename, "w") as f:
                json.dump(data, f)

            read_sarray = SArray.read_json(filename)
            self.__test_equal(read_sarray, data, data_type)
            self._remove_single_file(filename)

    def test_read_json_infer_type(self):
        data = [None, 1, 2, None, 3.0, 4, 5.0, 6, None]
        converted_data = [float(i) if i is not None else i for i in data]
        filename = "read_json_infer_type.json"
        self._remove_single_file(filename)

        with open(filename, "w") as f:
            json.dump(data, f)

        read_sarray = SArray.read_json(filename)
        self.__test_equal(read_sarray, converted_data, float)
        self._remove_single_file(filename)

    def _remove_single_file(self, filename):
        try:
            os.remove(filename)
        except:
            pass

    def _remove_sarray_files(self, prefix):
        filelist = [f for f in os.listdir(".") if f.startswith(prefix)]
        for f in filelist:
            shutil.rmtree(f)

    def test_transform(self):
        sa_char = SArray(self.url, str)
        sa_int = sa_char.apply(lambda char: ord(char), int)

        expected_output = [x for x in range(ord("a"), ord("a") + 26)]
        self.__test_equal(sa_int, expected_output, int)

        # Test randomness across segments, randomized sarray should have different elements.
        sa_random = SArray(range(0, 16), int).apply(
            lambda x: random.randint(0, 1000), int
        )
        vec = list(sa_random.head(len(sa_random)))
        self.assertFalse(all([x == vec[0] for x in vec]))

        # test transform with missing values
        sa = SArray([1, 2, 3, None, 4, 5])
        sa1 = sa.apply(lambda x: x + 1)
        self.__test_equal(sa1, [2, 3, 4, None, 5, 6], int)

    def test_transform_with_multiple_lambda(self):
        sa_char = SArray(self.url, str)
        sa_int = sa_char.apply(lambda char: ord(char), int)
        sa2_int = sa_int.apply(lambda val: val + 1, int)

        expected_output = [x for x in range(ord("a") + 1, ord("a") + 26 + 1)]
        self.__test_equal(sa2_int, expected_output, int)

    def test_transform_with_exception(self):
        sa_char = SArray(["a" for i in range(10000)], str)
        # # type mismatch exception
        self.assertRaises(
            TypeError, lambda: sa_char.apply(lambda char: char, int).head(1)
        )
        # # divide by 0 exception
        self.assertRaises(
            ZeroDivisionError, lambda: sa_char.apply(lambda char: ord(char) / 0, float)
        )

    def test_transform_with_type_inference(self):
        sa_char = SArray(self.url, str)
        sa_int = sa_char.apply(lambda char: ord(char))

        expected_output = [x for x in range(ord("a"), ord("a") + 26)]
        self.__test_equal(sa_int, expected_output, int)

        sa_bool = sa_char.apply(lambda char: ord(char) > ord("c"))
        expected_output = [int(x > ord("c")) for x in range(ord("a"), ord("a") + 26)]
        self.__test_equal(sa_bool, expected_output, int)

        # # divide by 0 exception
        self.assertRaises(
            ZeroDivisionError, lambda: sa_char.apply(lambda char: ord(char) / 0)
        )

        # Test randomness across segments, randomized sarray should have different elements.
        sa_random = SArray(range(0, 16), int).apply(lambda x: random.randint(0, 1000))
        vec = list(sa_random.head(len(sa_random)))
        self.assertFalse(all([x == vec[0] for x in vec]))

    def test_transform_on_lists(self):
        sa_int = SArray(self.int_data, int)
        sa_vec2 = sa_int.apply(lambda x: [x, x + 1, str(x)])
        expected = [[i, i + 1, str(i)] for i in self.int_data]
        self.__test_equal(sa_vec2, expected, list)
        sa_int_again = sa_vec2.apply(lambda x: int(x[0]))
        self.__test_equal(sa_int_again, self.int_data, int)

        # transform from vector to vector
        sa_vec = SArray(self.vec_data, array.array)
        sa_vec2 = sa_vec.apply(lambda x: x)
        self.__test_equal(sa_vec2, self.vec_data, array.array)

        # transform on list
        sa_list = SArray(self.list_data, list)
        sa_list2 = sa_list.apply(lambda x: x)
        self.__test_equal(sa_list2, self.list_data, list)

        # transform dict to list
        sa_dict = SArray(self.dict_data, dict)
        # Python 3 doesn't return keys in same order from identical dictionaries.
        sort_by_type = lambda x: str(type(x))
        sa_list = sa_dict.apply(lambda x: sorted(list(x), key=sort_by_type))
        self.__test_equal(
            sa_list, [sorted(list(x), key=sort_by_type) for x in self.dict_data], list
        )

    def test_transform_dict(self):
        # lambda accesses dict
        sa_dict = SArray([{"a": 1}, {1: 2}, {"c": "a"}, None], dict)
        sa_bool_r = sa_dict.apply(
            lambda x: "a" in x if x is not None else None, skip_na=False
        )
        expected_output = [1, 0, 0, None]
        self.__test_equal(sa_bool_r, expected_output, int)

        # lambda returns dict
        expected_output = [{"a": 1}, {1: 2}, None, {"c": "a"}]
        sa_dict = SArray(expected_output, dict)
        lambda_out = sa_dict.apply(lambda x: x)
        self.__test_equal(lambda_out, expected_output, dict)

    def test_filter_dict(self):
        expected_output = [{"a": 1}]
        sa_dict = SArray(expected_output, dict)
        ret = sa_dict.filter(lambda x: "a" in x)
        self.__test_equal(ret, expected_output, dict)

        # try second time to make sure the lambda system still works
        expected_output = [{1: 2}]
        sa_dict = SArray(expected_output, dict)
        lambda_out = sa_dict.filter(lambda x: 1 in x)
        self.__test_equal(lambda_out, expected_output, dict)

    def test_filter(self):
        # test empty
        s = SArray([], float)
        no_change = s.filter(lambda x: x == 0)
        self.assertEqual(len(no_change), 0)

        # test normal case
        s = SArray(self.int_data, int)
        middle_of_array = s.filter(lambda x: x > 3 and x < 8)
        self.assertEqual(list(middle_of_array.head(10)), [x for x in range(4, 8)])

        # test normal string case
        s = SArray(self.string_data, str)
        exp_val_list = [x for x in self.string_data if x != "world"]
        # Remove all words whose second letter is not in the first half of the alphabet
        second_letter = s.filter(
            lambda x: len(x) > 1 and (ord(x[1]) > ord("a")) and (ord(x[1]) < ord("n"))
        )
        self.assertEqual(list(second_letter.head(10)), exp_val_list)

        # test not-a-lambda
        def a_filter_func(x):
            return (x > 4.4) and (x < 6.8)

        s = SArray(self.int_data, float)
        another = s.filter(a_filter_func)
        self.assertEqual(list(another.head(10)), [5.0, 6.0])

        sa = SArray(self.float_data)

        # filter by self
        sa2 = sa[sa]
        self.assertEqual(list(sa.head(10)), list(sa2.head(10)))

        # filter by zeros
        sa_filter = SArray([0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
        sa2 = sa[sa_filter]
        self.assertEqual(len(sa2), 0)

        # filter by wrong size
        sa_filter = SArray([0, 2, 5])
        with self.assertRaises(IndexError):
            sa2 = sa[sa_filter]

    def test_any_all(self):
        s = SArray([0, 1, 2, 3, 4, 5, 6, 7, 8, 9], int)
        self.assertEqual(s.any(), True)
        self.assertEqual(s.all(), False)
        s = SArray([0, 0, 0, 0, 0], int)
        self.assertEqual(s.any(), False)
        self.assertEqual(s.all(), False)

        s = SArray(self.string_data, str)
        self.assertEqual(s.any(), True)
        self.assertEqual(s.all(), True)

        s = SArray(self.int_data, int)
        self.assertEqual(s.any(), True)
        self.assertEqual(s.all(), True)

        # test empty
        s = SArray([], int)
        self.assertEqual(s.any(), False)
        self.assertEqual(s.all(), True)

        s = SArray([[], []], array.array)
        self.assertEqual(s.any(), False)
        self.assertEqual(s.all(), False)

        s = SArray([[], [1.0]], array.array)
        self.assertEqual(s.any(), True)
        self.assertEqual(s.all(), False)

    def test_astype(self):
        # test empty
        s = SArray([], int)
        as_out = s.astype(float)
        self.assertEqual(as_out.dtype, float)

        # test float -> int
        s = SArray(list(map(lambda x: x + 0.2, self.float_data)), float)
        as_out = s.astype(int)
        self.assertEqual(list(as_out.head(10)), self.int_data)

        # test int->string
        s = SArray(self.int_data, int)
        as_out = s.astype(str)
        self.assertEqual(
            list(as_out.head(10)), list(map(lambda x: str(x), self.int_data))
        )

        i_out = as_out.astype(int)
        self.assertEqual(list(i_out.head(10)), list(s.head(10)))

        s = SArray(self.vec_data, array.array)

        with self.assertRaises(RuntimeError):
            s.astype(int)
        with self.assertRaises(RuntimeError):
            s.astype(float)

        s = SArray(["a", "1", "2", "3"])
        with self.assertRaises(RuntimeError):
            s.astype(int)

        self.assertEqual(list(s.astype(int, True).head(4)), [None, 1, 2, 3])

        s = SArray(["[1 2 3]", "[4;5]"])
        ret = list(s.astype(array.array).head(2))
        self.assertEqual(ret, [array.array("d", [1, 2, 3]), array.array("d", [4, 5])])

        s = SArray(['[1,"b",3]', "[4,5]"])
        ret = list(s.astype(list).head(2))
        self.assertEqual(ret, [[1, "b", 3], [4, 5]])

        s = SArray(['{"a":2,"b":3}', "{}"])
        ret = list(s.astype(dict).head(2))
        self.assertEqual(ret, [{"a": 2, "b": 3}, {}])

        s = SArray(["[1abc]"])
        ret = list(s.astype(list).head(1))
        self.assertEqual(ret, [["1abc"]])

        s = SArray(["{1xyz:1a,2b:2}"])
        ret = list(s.astype(dict).head(1))
        self.assertEqual(ret, [{"1xyz": "1a", "2b": 2}])

        # astype between list and array
        s = SArray([array.array("d", [1.0, 2.0]), array.array("d", [2.0, 3.0])])
        ret = list(s.astype(list))
        self.assertEqual(ret, [[1.0, 2.0], [2.0, 3.0]])
        ret = list(s.astype(list).astype(array.array))
        self.assertEqual(list(s), list(ret))
        with self.assertRaises(RuntimeError):
            ret = list(SArray([["a", 1.0], ["b", 2.0]]).astype(array.array))

        badcast = list(
            SArray([["a", 1.0], ["b", 2.0]]).astype(
                array.array, undefined_on_failure=True
            )
        )
        self.assertEqual(badcast, [None, None])

        with self.assertRaises(TypeError):
            s.astype(None)

    def test_clip(self):
        # invalid types
        s = SArray(self.string_data, str)
        with self.assertRaises(RuntimeError):
            s.clip(25, 26)
        with self.assertRaises(RuntimeError):
            s.clip_lower(25)
        with self.assertRaises(RuntimeError):
            s.clip_upper(26)

        # int w/ int, test lower and upper functions too
        # int w/float, no change
        s = SArray(self.int_data, int)
        clip_out = s.clip(3, 7).head(10)
        # test that our list isn't cast to float if nothing happened
        clip_out_nc = s.clip(0.2, 10.2).head(10)
        lclip_out = s.clip_lower(3).head(10)
        rclip_out = s.clip_upper(7).head(10)
        self.assertEqual(len(clip_out), len(self.int_data))
        self.assertEqual(len(lclip_out), len(self.int_data))
        self.assertEqual(len(rclip_out), len(self.int_data))
        for i in range(0, len(clip_out)):
            if i < 2:
                self.assertEqual(clip_out[i], 3)
                self.assertEqual(lclip_out[i], 3)
                self.assertEqual(rclip_out[i], self.int_data[i])
                self.assertEqual(clip_out_nc[i], self.int_data[i])
            elif i > 6:
                self.assertEqual(clip_out[i], 7)
                self.assertEqual(lclip_out[i], self.int_data[i])
                self.assertEqual(rclip_out[i], 7)
                self.assertEqual(clip_out_nc[i], self.int_data[i])
            else:
                self.assertEqual(clip_out[i], self.int_data[i])
                self.assertEqual(clip_out_nc[i], self.int_data[i])

        # int w/float, change
        # float w/int
        # float w/float
        clip_out = s.clip(2.8, 7.2).head(10)
        fs = SArray(self.float_data, float)
        ficlip_out = fs.clip(3, 7).head(10)
        ffclip_out = fs.clip(2.8, 7.2).head(10)
        for i in range(0, len(clip_out)):
            if i < 2:
                self.assertAlmostEqual(clip_out[i], 2.8)
                self.assertAlmostEqual(ffclip_out[i], 2.8)
                self.assertAlmostEqual(ficlip_out[i], 3.0)
            elif i > 6:
                self.assertAlmostEqual(clip_out[i], 7.2)
                self.assertAlmostEqual(ffclip_out[i], 7.2)
                self.assertAlmostEqual(ficlip_out[i], 7.0)
            else:
                self.assertAlmostEqual(clip_out[i], self.float_data[i])
                self.assertAlmostEqual(ffclip_out[i], self.float_data[i])
                self.assertAlmostEqual(ficlip_out[i], self.float_data[i])

        vs = SArray(self.vec_data, array.array)
        clipvs = vs.clip(3, 7).head(100)
        self.assertEqual(len(clipvs), len(self.vec_data))
        for i in range(0, len(clipvs)):
            a = clipvs[i]
            b = self.vec_data[i]
            self.assertEqual(len(a), len(b))
            for j in range(0, len(b)):
                if b[j] < 3:
                    b[j] = 3
                elif b[j] > 7:
                    b[j] = 7
            self.assertEqual(a, b)

    def test_missing(self):
        s = SArray(self.int_data, int)
        self.assertEqual(s.countna(), 0)
        s = SArray(self.int_data + [None], int)
        self.assertEqual(s.countna(), 1)

        s = SArray(self.float_data, float)
        self.assertEqual(s.countna(), 0)
        s = SArray(self.float_data + [None], float)
        self.assertEqual(s.countna(), 1)

        s = SArray(self.string_data, str)
        self.assertEqual(s.countna(), 0)
        s = SArray(self.string_data + [None], str)
        self.assertEqual(s.countna(), 1)

        s = SArray(self.vec_data, array.array)
        self.assertEqual(s.countna(), 0)
        s = SArray(self.vec_data + [None], array.array)
        self.assertEqual(s.countna(), 1)

    def test_nonzero(self):
        # test empty
        s = SArray([], int)
        nz_out = s.nnz()
        self.assertEqual(nz_out, 0)

        # test all nonzero
        s = SArray(self.float_data, float)
        nz_out = s.nnz()
        self.assertEqual(nz_out, len(self.float_data))

        # test all zero
        s = SArray([0 for x in range(0, 10)], int)
        nz_out = s.nnz()
        self.assertEqual(nz_out, 0)

        # test strings
        str_list = copy.deepcopy(self.string_data)
        str_list.append("")
        s = SArray(str_list, str)
        nz_out = s.nnz()
        self.assertEqual(nz_out, len(self.string_data))

    def test_std_var(self):
        # test empty
        s = SArray([], int)
        self.assertTrue(s.std() is None)
        self.assertTrue(s.var() is None)

        # increasing ints
        s = SArray(self.int_data, int)
        self.assertAlmostEqual(s.var(), 8.25)
        self.assertAlmostEqual(s.std(), 2.8722813)

        # increasing floats
        s = SArray(self.float_data, float)
        self.assertAlmostEqual(s.var(), 8.25)
        self.assertAlmostEqual(s.std(), 2.8722813)

        # vary ddof
        self.assertAlmostEqual(s.var(ddof=3), 11.7857143)
        self.assertAlmostEqual(s.var(ddof=6), 20.625)
        self.assertAlmostEqual(s.var(ddof=9), 82.5)
        self.assertAlmostEqual(s.std(ddof=3), 3.4330328)
        self.assertAlmostEqual(s.std(ddof=6), 4.5414755)
        self.assertAlmostEqual(s.std(ddof=9), 9.08295106)

        # bad ddof
        with self.assertRaises(RuntimeError):
            s.var(ddof=11)
        with self.assertRaises(RuntimeError):
            s.std(ddof=11)
        # bad type
        s = SArray(self.string_data, str)
        with self.assertRaises(RuntimeError):
            s.std()
        with self.assertRaises(RuntimeError):
            s.var()

        # overflow test
        huge_int = 9223372036854775807
        s = SArray([1, huge_int], int)
        self.assertAlmostEqual(s.var(), 21267647932558653957237540927630737409.0)
        self.assertAlmostEqual(s.std(), 4611686018427387900.0)

    def test_tail(self):
        # test empty
        s = SArray([], int)
        self.assertEqual(len(s.tail()), 0)

        # test standard tail
        s = SArray([x for x in range(0, 40)], int)
        self.assertEqual(list(s.tail()), [x for x in range(30, 40)])

        # smaller amount
        self.assertEqual(list(s.tail(3)), [x for x in range(37, 40)])

        # larger amount
        self.assertEqual(list(s.tail(40)), [x for x in range(0, 40)])

        # too large
        self.assertEqual(list(s.tail(81)), [x for x in range(0, 40)])

    def test_max_min_sum_mean(self):
        # negative and positive
        s = SArray([-2, -1, 0, 1, 2], int)
        self.assertEqual(s.max(), 2)
        self.assertEqual(s.min(), -2)
        self.assertEqual(s.sum(), 0)
        self.assertAlmostEqual(s.mean(), 0.0)

        # test valid and invalid types
        s = SArray(self.string_data, str)
        with self.assertRaises(RuntimeError):
            s.max()
        with self.assertRaises(RuntimeError):
            s.min()
        with self.assertRaises(RuntimeError):
            s.sum()
        with self.assertRaises(RuntimeError):
            s.mean()

        s = SArray(self.int_data, int)
        self.assertEqual(s.max(), 10)
        self.assertEqual(s.min(), 1)
        self.assertEqual(s.sum(), 55)
        self.assertAlmostEqual(s.mean(), 5.5)

        s = SArray(self.float_data, float)
        self.assertEqual(s.max(), 10.0)
        self.assertEqual(s.min(), 1.0)
        self.assertEqual(s.sum(), 55.0)
        self.assertAlmostEqual(s.mean(), 5.5)

        # test all negative
        s = SArray(list(map(lambda x: x * -1, self.int_data)), int)
        self.assertEqual(s.max(), -1)
        self.assertEqual(s.min(), -10)
        self.assertEqual(s.sum(), -55)
        self.assertAlmostEqual(s.mean(), -5.5)

        # test empty
        s = SArray([], float)
        self.assertTrue(s.max() is None)
        self.assertTrue(s.min() is None)
        self.assertTrue(s.mean() is None)

        # test sum
        t = SArray([], float).sum()
        self.assertTrue(type(t) == float)
        self.assertTrue(t == 0.0)
        t = SArray([], int).sum()
        self.assertTrue(type(t) == int or type(t) == long)
        self.assertTrue(t == 0)
        self.assertTrue(SArray([], array.array).sum() == array.array("d", []))

        # test big ints
        huge_int = 9223372036854775807
        s = SArray([1, huge_int], int)
        self.assertEqual(s.max(), huge_int)
        self.assertEqual(s.min(), 1)
        # yes, we overflow
        self.assertEqual(s.sum(), (huge_int + 1) * -1)
        # ...but not here
        self.assertAlmostEqual(s.mean(), 4611686018427387904.0)

        a = SArray([[1, 2], [1, 2], [1, 2]], array.array)
        self.assertEqual(a.sum(), array.array("d", [3, 6]))
        self.assertEqual(a.mean(), array.array("d", [1, 2]))
        with self.assertRaises(RuntimeError):
            a.max()
        with self.assertRaises(RuntimeError):
            a.min()

        a = SArray([[1, 2], [1, 2], [1, 2, 3]], array.array)
        with self.assertRaises(RuntimeError):
            a.sum()
        with self.assertRaises(RuntimeError):
            a.mean()

    def test_max_min_sum_mean_missing(self):
        # negative and positive
        s = SArray([-2, 0, None, None, None], int)
        self.assertEqual(s.max(), 0)
        self.assertEqual(s.min(), -2)
        self.assertEqual(s.sum(), -2)
        self.assertAlmostEqual(s.mean(), -1)

        s = SArray([None, None, None], int)
        self.assertEqual(s.max(), None)
        self.assertEqual(s.min(), None)
        self.assertEqual(s.sum(), 0)
        self.assertEqual(s.mean(), None)

    def test_python_special_functions(self):
        s = SArray([], int)
        self.assertEqual(len(s), 0)
        self.assertEqual(str(s), "[]")
        self.assertRaises(ValueError, lambda: bool(s))

        # increasing ints
        s = SArray(self.int_data, int)
        self.assertEqual(len(s), len(self.int_data))
        self.assertEqual(list(s), self.int_data)
        self.assertRaises(ValueError, lambda: bool(s))

        realsum = sum(self.int_data)
        sum1 = sum([x for x in s])
        sum2 = s.sum()
        sum3 = s.apply(lambda x: x, int).sum()

        self.assertEqual(sum1, realsum)
        self.assertEqual(sum2, realsum)
        self.assertEqual(sum3, realsum)

        # abs
        s = np.array(range(-10, 10))
        t = SArray(s, int)
        self.__test_equal(abs(t), list(abs(s)), int)
        t = SArray(s, float)
        self.__test_equal(abs(t), list(abs(s)), float)
        t = SArray([s], array.array)
        self.__test_equal(SArray(abs(t)[0]), list(abs(s)), float)

    def test_scalar_operators(self):
        s = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
        t = SArray(s, int)
        self.__test_equal(t + 1, list(s + 1), int)
        self.__test_equal(t - 1, list(s - 1), int)
        # we handle division differently. All divisions cast to float
        self.__test_equal(t / 2, list(s / 2.0), float)
        self.__test_equal(t * 2, list(s * 2), int)
        self.__test_equal(t ** 2, list(s ** 2), float)
        self.__test_almost_equal(t ** 0.5, list(s ** 0.5), float)
        self.__test_equal(((t ** 2) ** 0.5 + 1e-8).astype(int), list(s), int)
        self.__test_equal(t < 5, list(s < 5), int)
        self.__test_equal(t > 5, list(s > 5), int)
        self.__test_equal(t <= 5, list(s <= 5), int)
        self.__test_equal(t >= 5, list(s >= 5), int)
        self.__test_equal(t == 5, list(s == 5), int)
        self.__test_equal(t != 5, list(s != 5), int)
        self.__test_equal(t % 5, list(s % 5), int)
        self.__test_equal(t // 5, list(s // 5), int)
        self.__test_equal(t + 1, list(s + 1), int)
        self.__test_equal(+t, list(+s), int)
        self.__test_equal(-t, list(-s), int)
        self.__test_equal(1.5 - t, list(1.5 - s), float)
        self.__test_equal(2.0 / t, list(2.0 / s), float)
        self.__test_equal(2 / t, list(2.0 / s), float)
        self.__test_equal(2.5 * t, list(2.5 * s), float)
        self.__test_equal(2 ** t, list(2 ** s), float)

        s_neg = np.array([-1, -2, -3, 5, 6, 7, 8, 9, 10])
        t_neg = SArray(s_neg, int)
        self.__test_equal(t_neg // 5, list(s_neg // 5), int)
        self.__test_equal(t_neg % 5, list(s_neg % 5), int)

        s = ["a", "b", "c"]
        t = SArray(s, str)
        self.__test_equal(t + "x", [i + "x" for i in s], str)
        with self.assertRaises(RuntimeError):
            t - "x"
        with self.assertRaises(RuntimeError):
            t * "x"
        with self.assertRaises(RuntimeError):
            t / "x"

        s = SArray(self.vec_data, array.array)
        self.__test_equal(
            s + 1,
            [array.array("d", [float(j) + 1 for j in i]) for i in self.vec_data],
            array.array,
        )
        self.__test_equal(
            s - 1,
            [array.array("d", [float(j) - 1 for j in i]) for i in self.vec_data],
            array.array,
        )
        self.__test_equal(
            s * 2,
            [array.array("d", [float(j) * 2 for j in i]) for i in self.vec_data],
            array.array,
        )
        self.__test_equal(
            s / 2,
            [array.array("d", [float(j) / 2 for j in i]) for i in self.vec_data],
            array.array,
        )
        s = SArray([1, 2, 3, 4, None])
        self.__test_equal(s == None, [0, 0, 0, 0, 1], int)
        self.__test_equal(s != None, [1, 1, 1, 1, 0], int)

    def test_modulus_operator(self):
        l = [-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5]
        t = SArray(l, int)
        self.__test_equal(t % 2, [i % 2 for i in l], int)
        self.__test_equal(t % -2, [i % -2 for i in l], int)

    def test_vector_operators(self):
        s = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
        s2 = np.array([5, 4, 3, 2, 1, 10, 9, 8, 7, 6])
        t = SArray(s, int)
        t2 = SArray(s2, int)
        self.__test_equal(t + t2, list(s + s2), int)
        self.__test_equal(t - t2, list(s - s2), int)
        # we handle division differently. All divisions cast to float
        self.__test_equal(t / t2, list(s.astype(float) / s2), float)
        self.__test_equal(t * t2, list(s * s2), int)
        self.__test_equal(t ** t2, list(s ** s2), float)
        self.__test_almost_equal(t ** (1.0 / t2), list(s ** (1.0 / s2)), float)
        self.__test_equal(t > t2, list(s > s2), int)
        self.__test_equal(t <= t2, list(s <= s2), int)
        self.__test_equal(t >= t2, list(s >= s2), int)
        self.__test_equal(t == t2, list(s == s2), int)
        self.__test_equal(t != t2, list(s != s2), int)

        s = SArray(self.vec_data, array.array)
        self.__test_almost_equal(
            s + s,
            [array.array("d", [float(j) + float(j) for j in i]) for i in self.vec_data],
            array.array,
        )
        self.__test_almost_equal(
            s - s,
            [array.array("d", [float(j) - float(j) for j in i]) for i in self.vec_data],
            array.array,
        )
        self.__test_almost_equal(
            s * s,
            [array.array("d", [float(j) * float(j) for j in i]) for i in self.vec_data],
            array.array,
        )
        self.__test_almost_equal(
            s / s,
            [array.array("d", [float(j) / float(j) for j in i]) for i in self.vec_data],
            array.array,
        )
        self.__test_almost_equal(
            s ** s,
            [
                array.array("d", [float(j) ** float(j) for j in i])
                for i in self.vec_data
            ],
            array.array,
        )
        self.__test_almost_equal(
            s // s,
            [
                array.array("d", [float(j) // float(j) for j in i])
                for i in self.vec_data
            ],
            array.array,
        )
        t = SArray(self.float_data, float)

        self.__test_almost_equal(
            s + t,
            [
                array.array("d", [float(j) + i[1] for j in i[0]])
                for i in zip(self.vec_data, self.float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s - t,
            [
                array.array("d", [float(j) - i[1] for j in i[0]])
                for i in zip(self.vec_data, self.float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s * t,
            [
                array.array("d", [float(j) * i[1] for j in i[0]])
                for i in zip(self.vec_data, self.float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s / t,
            [
                array.array("d", [float(j) / i[1] for j in i[0]])
                for i in zip(self.vec_data, self.float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s ** t,
            [
                array.array("d", [float(j) ** i[1] for j in i[0]])
                for i in zip(self.vec_data, self.float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s // t,
            [
                array.array("d", [float(j) // i[1] for j in i[0]])
                for i in zip(self.vec_data, self.float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            +s,
            [array.array("d", [float(j) for j in i]) for i in self.vec_data],
            array.array,
        )
        self.__test_almost_equal(
            -s,
            [array.array("d", [-float(j) for j in i]) for i in self.vec_data],
            array.array,
        )

        neg_float_data = [-v for v in self.float_data]
        t = SArray(neg_float_data, float)
        self.__test_almost_equal(
            s + t,
            [
                array.array("d", [float(j) + i[1] for j in i[0]])
                for i in zip(self.vec_data, neg_float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s - t,
            [
                array.array("d", [float(j) - i[1] for j in i[0]])
                for i in zip(self.vec_data, neg_float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s * t,
            [
                array.array("d", [float(j) * i[1] for j in i[0]])
                for i in zip(self.vec_data, neg_float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s / t,
            [
                array.array("d", [float(j) / i[1] for j in i[0]])
                for i in zip(self.vec_data, neg_float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s ** t,
            [
                array.array("d", [float(j) ** i[1] for j in i[0]])
                for i in zip(self.vec_data, neg_float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            s // t,
            [
                array.array("d", [float(j) // i[1] for j in i[0]])
                for i in zip(self.vec_data, neg_float_data)
            ],
            array.array,
        )
        self.__test_almost_equal(
            t // s,
            [
                array.array("d", [i[1] // float(j) for j in i[0]])
                for i in zip(self.vec_data, neg_float_data)
            ],
            array.array,
        )

        s = SArray([1, 2, 3, 4, None])
        self.assertTrue((s == s).all())
        s = SArray([1, 2, 3, 4, None])
        self.assertFalse((s != s).any())

    def test_div_corner(self):
        def try_eq_sa_val(left_val, right_val):
            if type(left_val) is list:
                left_val = array.array("d", left_val)
            if type(right_val) is list:
                right_val = array.array("d", right_val)

            left_type = type(left_val)
            v1 = (SArray([left_val], left_type) // right_val)[0]

            if type(right_val) is array.array:
                if type(left_val) is array.array:
                    v2 = array.array(
                        "d", [lv // rv for lv, rv in zip(left_val, right_val)]
                    )
                else:
                    v2 = array.array("d", [left_val // rv for rv in right_val])
            else:
                if type(left_val) is array.array:
                    v2 = array.array("d", [lv // right_val for lv in left_val])
                else:
                    v2 = left_val // right_val

            if type(v1) in six.integer_types:
                self.assertTrue(type(v2) in six.integer_types)
            else:
                self.assertEqual(type(v1), type(v2))
            self.assertEqual(v1, v2)

        try_eq_sa_val(1, 2)
        try_eq_sa_val(1.0, 2)
        try_eq_sa_val(1, 2.0)
        try_eq_sa_val(1.0, 2.0)

        try_eq_sa_val(-1, 2)
        try_eq_sa_val(-1.0, 2)
        try_eq_sa_val(-1, 2.0)
        try_eq_sa_val(-1.0, 2.0)

        try_eq_sa_val([1, -1], 2)
        try_eq_sa_val([1, -1], 2.0)

        try_eq_sa_val(2, [3, -3])
        try_eq_sa_val(2.0, [3, -3])

    def test_floodiv_corner(self):
        def try_eq_sa_val(left_val, right_val):
            if type(left_val) is list:
                left_val = array.array("d", left_val)
            if type(right_val) is list:
                right_val = array.array("d", right_val)

            left_type = type(left_val)
            v1 = (SArray([left_val], left_type) // right_val)[0]

            if type(right_val) is array.array:
                if type(left_val) is array.array:
                    v2 = array.array(
                        "d", [lv // rv for lv, rv in zip(left_val, right_val)]
                    )
                else:
                    v2 = array.array("d", [left_val // rv for rv in right_val])
            else:
                if type(left_val) is array.array:
                    v2 = array.array("d", [lv // right_val for lv in left_val])
                else:
                    v2 = left_val // right_val

            if type(v1) in six.integer_types:
                self.assertTrue(type(v2) in six.integer_types)
            else:
                self.assertEqual(type(v1), type(v2))

            self.assertEqual(v1, v2)

        try_eq_sa_val(1, 2)
        try_eq_sa_val(1.0, 2)
        try_eq_sa_val(1, 2.0)
        try_eq_sa_val(1.0, 2.0)

        try_eq_sa_val(-1, 2)
        try_eq_sa_val(-1.0, 2)
        try_eq_sa_val(-1, 2.0)
        try_eq_sa_val(-1.0, 2.0)

        try_eq_sa_val([1, -1], 2)
        try_eq_sa_val([1, -1], 2.0)

        try_eq_sa_val(2, [3, -3])
        try_eq_sa_val(2.0, [3, -3])

        from math import isnan

        def try_eq_sa_correct(left_val, right_val, correct):
            if type(left_val) is list:
                left_val = array.array("d", left_val)
            if type(right_val) is list:
                right_val = array.array("d", right_val)

            left_type = type(left_val)
            v1 = (SArray([left_val], left_type) // right_val)[0]

            if type(correct) is not list:
                v1 = [v1]
                correct = [correct]

            for v, c in zip(v1, correct):
                if type(v) is float and isnan(v):
                    assert isnan(c)
                else:
                    self.assertEqual(type(v), type(c))
                    self.assertEqual(v, c)

        try_eq_sa_correct(1, 0, None)
        try_eq_sa_correct(0, 0, None)
        try_eq_sa_correct(-1, 0, None)

        try_eq_sa_correct(1.0, 0, float("inf"))
        try_eq_sa_correct(0.0, 0, float("nan"))
        try_eq_sa_correct(-1.0, 0, float("-inf"))

        try_eq_sa_correct([1.0, 0, -1], 0, [float("inf"), float("nan"), float("-inf")])
        try_eq_sa_correct(1, [1.0, 0], [1.0, float("inf")])
        try_eq_sa_correct(-1, [1.0, 0], [-1.0, float("-inf")])
        try_eq_sa_correct(0, [1.0, 0], [0.0, float("nan")])

    def test_logical_ops(self):
        s = np.array([0, 0, 0, 0, 1, 1, 1, 1])
        s2 = np.array([0, 1, 0, 1, 0, 1, 0, 1])
        t = SArray(s, int)
        t2 = SArray(s2, int)
        self.__test_equal(t & t2, list(((s & s2) > 0).astype(int)), int)
        self.__test_equal(t | t2, list(((s | s2) > 0).astype(int)), int)

    def test_logical_ops_missing_value_propagation(self):
        s = [0, 0, 0, None, None, None, 1, 1, 1]
        s2 = [0, None, 1, 0, None, 1, 0, None, 1]
        t = SArray(s, int)
        t2 = SArray(s2, int)

        and_result = [0, 0, 0, 0, None, None, 0, None, 1]
        or_result = [0, None, 1, None, None, 1, 1, 1, 1]
        self.__test_equal(t & t2, and_result, int)
        self.__test_equal(t | t2, or_result, int)

    def test_string_operators(self):
        s = ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j"]
        s2 = ["e", "d", "c", "b", "a", "j", "i", "h", "g", "f"]

        t = SArray(s, str)
        t2 = SArray(s2, str)
        self.__test_equal(t + t2, ["".join(x) for x in zip(s, s2)], str)
        self.__test_equal(t + "x", [x + "x" for x in s], str)
        self.__test_equal(t < t2, [x < y for (x, y) in zip(s, s2)], int)
        self.__test_equal(t > t2, [x > y for (x, y) in zip(s, s2)], int)
        self.__test_equal(t == t2, [x == y for (x, y) in zip(s, s2)], int)
        self.__test_equal(t != t2, [x != y for (x, y) in zip(s, s2)], int)
        self.__test_equal(t <= t2, [x <= y for (x, y) in zip(s, s2)], int)
        self.__test_equal(t >= t2, [x >= y for (x, y) in zip(s, s2)], int)

    def test_vector_operator_missing_propagation(self):
        t = SArray([1, 2, 3, 4, None, 6, 7, 8, 9, None], float)  # missing 4th and 9th
        t2 = SArray(
            [None, 4, 3, 2, np.nan, 10, 9, 8, 7, 6], float
        )  # missing 0th and 4th
        self.assertEqual(len((t + t2).dropna()), 7)
        self.assertEqual(len((t - t2).dropna()), 7)
        self.assertEqual(len((t * t2).dropna()), 7)

    def test_sarray_image_equality(self):
        current_file_dir = os.path.dirname(os.path.realpath(__file__))
        image_url_1 = current_file_dir + "/images/sample.png"
        image_url_2 = current_file_dir + "/images/sample.jpg"
        i = load_images(image_url_1)["image"]
        j = load_images(image_url_2)["image"]

        self.__test_equal(i == i, [x == y for (x, y) in zip(i, i)], int)
        self.__test_equal(j == j, [x == y for (x, y) in zip(j, j)], int)
        self.__test_equal(i == j, [x == y for (x, y) in zip(i, j)], int)

    def test_dropna(self):
        no_nas = ["strings", "yeah", "nan", "NaN", "NA", "None"]
        t = SArray(no_nas)
        self.assertEqual(len(t.dropna()), 6)
        self.assertEqual(list(t.dropna()), no_nas)
        t2 = SArray([None, np.nan])
        self.assertEqual(len(t2.dropna()), 0)
        self.assertEqual(list(SArray(self.int_data).dropna()), self.int_data)
        self.assertEqual(list(SArray(self.float_data).dropna()), self.float_data)

    def test_fillna(self):
        # fillna shouldn't fill anything
        no_nas = ["strings", "yeah", "nan", "NaN", "NA", "None"]
        t = SArray(no_nas)
        out = t.fillna("hello")
        self.assertEqual(list(out), no_nas)

        # Normal integer case (float auto casted to int)
        t = SArray([53, 23, None, np.nan, 5])
        self.assertEqual(list(t.fillna(-1.0)), [53, 23, -1, -1, 5])

        # dict type
        t = SArray(self.dict_data + [None])
        self.assertEqual(list(t.fillna({1: "1"})), self.dict_data + [{1: "1"}])

        # list type
        t = SArray(self.list_data + [None])
        self.assertEqual(list(t.fillna([0, 0, 0])), self.list_data + [[0, 0, 0]])

        # vec type
        t = SArray(self.vec_data + [None])
        self.assertEqual(
            list(t.fillna(array.array("f", [0.0, 0.0]))),
            self.vec_data + [array.array("f", [0.0, 0.0])],
        )

        # empty sarray
        t = SArray()
        self.assertEqual(len(t.fillna(0)), 0)

    def test_sample(self):
        sa = SArray(data=self.int_data)
        sa_sample = sa.sample(0.5, 9)
        sa_sample2 = sa.sample(0.5, 9)

        self.assertEqual(list(sa_sample.head()), list(sa_sample2.head()))

        for i in sa_sample:
            self.assertTrue(i in self.int_data)

        with self.assertRaises(ValueError):
            sa.sample(3)

        sa_sample = SArray().sample(0.5, 9)
        self.assertEqual(len(sa_sample), 0)
        self.assertEqual(len(SArray.from_sequence(100).sample(0.5, 1, exact=True)), 50)
        self.assertEqual(len(SArray.from_sequence(100).sample(0.5, 2, exact=True)), 50)

    def test_hash(self):
        a = SArray([0, 1, 0, 1, 0, 1, 0, 1], int)
        b = a.hash()
        zero_hash = b[0]
        one_hash = b[1]
        self.assertTrue((b[a] == one_hash).all())
        self.assertTrue((b[1 - a] == zero_hash).all())

        # I can hash other stuff too
        # does not throw
        a.astype(str).hash().materialize()

        a.apply(lambda x: [x], list).hash().materialize()

        # Nones hash too!
        a = SArray([None, None, None], int).hash()
        self.assertTrue(a[0] is not None)
        self.assertTrue((a == a[0]).all())

        # different seeds give different hash values
        self.assertTrue((a.hash(seed=0) != a.hash(seed=1)).all())

    def test_random_integers(self):
        a = SArray.random_integers(0)
        self.assertEqual(len(a), 0)
        a = SArray.random_integers(1000)
        self.assertEqual(len(a), 1000)

    def test_vector_slice(self):
        d = [[1], [1, 2], [1, 2, 3]]
        g = SArray(d, array.array)
        self.assertEqual(list(g.vector_slice(0).head()), [1, 1, 1])
        self.assertEqual(
            list(g.vector_slice(0, 2).head()),
            [None, array.array("d", [1, 2]), array.array("d", [1, 2])],
        )
        self.assertEqual(
            list(g.vector_slice(0, 3).head()), [None, None, array.array("d", [1, 2, 3])]
        )

        g = SArray(self.vec_data, array.array)
        self.__test_equal(g.vector_slice(0), self.float_data, float)
        self.__test_equal(g.vector_slice(0, 2), self.vec_data, array.array)

    def _my_element_slice(self, arr, start=None, stop=None, step=1):
        return arr.apply(lambda x: x[slice(start, stop, step)], arr.dtype)

    def _slice_equality_test(self, arr, start=None, stop=None, step=1):
        self.assertEqual(
            list(arr.element_slice(start, stop, step)),
            list(self._my_element_slice(arr, start, stop, step)),
        )

    def test_element_slice(self):
        # string slicing
        g = SArray(range(1, 1000, 10)).astype(str)
        self._slice_equality_test(g, 0, 2)
        self._slice_equality_test(g, 0, -1, 2)
        self._slice_equality_test(g, -1, -3)
        self._slice_equality_test(g, -1, -2, -1)
        self._slice_equality_test(g, None, None, -1)
        self._slice_equality_test(g, -100, -1)

        # list slicing
        g = SArray(range(1, 10)).apply(lambda x: list(range(x)), list)
        self._slice_equality_test(g, 0, 2)
        self._slice_equality_test(g, 0, -1, 2)
        self._slice_equality_test(g, -1, -3)
        self._slice_equality_test(g, -1, -2, -1)
        self._slice_equality_test(g, None, None, -1)
        self._slice_equality_test(g, -100, -1)

        # array slicing
        import array

        g = SArray(range(1, 10)).apply(lambda x: array.array("d", range(x)))
        self._slice_equality_test(g, 0, 2)
        self._slice_equality_test(g, 0, -1, 2)
        self._slice_equality_test(g, -1, -3)
        self._slice_equality_test(g, -1, -2, -1)
        self._slice_equality_test(g, None, None, -1)
        self._slice_equality_test(g, -100, -1)

        # this should fail
        with self.assertRaises(TypeError):
            g = SArray(range(1, 1000)).element_slice(1)

        with self.assertRaises(TypeError):
            g = SArray(range(1, 1000)).astype(float).element_slice(1)

    def test_lazy_eval(self):
        sa = SArray(range(-10, 10))
        sa = sa + 1
        sa1 = sa >= 0
        sa2 = sa <= 0
        sa3 = sa[sa1 & sa2]
        item_count = len(sa3)
        self.assertEqual(item_count, 1)

    def __test_append(self, data1, data2, dtype):
        sa1 = SArray(data1, dtype)
        sa2 = SArray(data2, dtype)
        sa3 = sa1.append(sa2)
        self.__test_equal(sa3, data1 + data2, dtype)

        sa3 = sa2.append(sa1)
        self.__test_equal(sa3, data2 + data1, dtype)

    def test_append(self):
        n = len(self.int_data)
        m = n // 2

        self.__test_append(self.int_data[0:m], self.int_data[m:n], int)
        self.__test_append(self.bool_data[0:m], self.bool_data[m:n], int)
        self.__test_append(self.string_data[0:m], self.string_data[m:n], str)
        self.__test_append(self.float_data[0:m], self.float_data[m:n], float)
        self.__test_append(self.vec_data[0:m], self.vec_data[m:n], array.array)
        self.__test_append(self.dict_data[0:m], self.dict_data[m:n], dict)

    def test_append_exception(self):
        val1 = [i for i in range(1, 1000)]
        val2 = [str(i) for i in range(-10, 1)]
        sa1 = SArray(val1, int)
        sa2 = SArray(val2, str)
        with self.assertRaises(RuntimeError):
            sa3 = sa1.append(sa2)

    def test_dict_keys(self):
        # self.dict_data =  [{str(i): i, i : float(i)} for i in self.int_data]
        sa = SArray(self.dict_data)
        sa_keys = sa.dict_keys()
        self.assertEqual(
            [set(i) for i in sa_keys], [{str(i), i} for i in self.int_data]
        )

        # na value
        d = [{"a": 1}, {None: 2}, {"b": None}, None]
        sa = SArray(d)
        sa_keys = sa.dict_keys()
        self.assertEqual(list(sa_keys), [["a"], [None], ["b"], None])

        # empty SArray
        sa = SArray()
        with self.assertRaises(RuntimeError):
            sa.dict_keys()

        # empty SArray with type
        sa = SArray([], dict)
        self.assertEqual(list(sa.dict_keys().head(10)), [], list)

    def test_dict_values(self):
        # self.dict_data =  [{str(i): i, i : float(i)} for i in self.int_data]
        sa = SArray(self.dict_data)
        sa_values = sa.dict_values()
        self.assertEqual(list(sa_values), [[i, float(i)] for i in self.int_data])

        # na value
        d = [{"a": 1}, {None: "str"}, {"b": None}, None]
        sa = SArray(d)
        sa_values = sa.dict_values()
        self.assertEqual(list(sa_values), [[1], ["str"], [None], None])

        # empty SArray
        sa = SArray()
        with self.assertRaises(RuntimeError):
            sa.dict_values()

        # empty SArray with type
        sa = SArray([], dict)
        self.assertEqual(list(sa.dict_values().head(10)), [], list)

    def test_dict_trim_by_keys(self):
        # self.dict_data =  [{str(i): i, i : float(i)} for i in self.int_data]
        d = [{"a": 1, "b": [1, 2]}, {None: "str"}, {"b": None, "c": 1}, None]
        sa = SArray(d)
        sa_values = sa.dict_trim_by_keys(["a", "b"])
        self.assertEqual(list(sa_values), [{}, {None: "str"}, {"c": 1}, None])

        # empty SArray
        sa = SArray()
        with self.assertRaises(RuntimeError):
            sa.dict_trim_by_keys([])

        sa = SArray([], dict)
        self.assertEqual(list(sa.dict_trim_by_keys([]).head(10)), [], list)

    def test_dict_trim_by_values(self):
        # self.dict_data =  [{str(i): i, i : float(i)} for i in self.int_data]
        d = [{"a": 1, "b": 20, "c": None}, {"b": 4, None: 5}, None]
        sa = SArray(d)
        sa_values = sa.dict_trim_by_values(5, 10)
        self.assertEqual(list(sa_values), [{"c": None}, {None: 5}, None])

        # no upper key
        sa_values = sa.dict_trim_by_values(2)
        self.assertEqual(
            list(sa_values), [{"b": 20, "c": None}, {"b": 4, None: 5}, None]
        )

        # no param
        sa_values = sa.dict_trim_by_values()
        self.assertEqual(
            list(sa_values), [{"a": 1, "b": 20, "c": None}, {"b": 4, None: 5}, None]
        )

        # no lower key
        sa_values = sa.dict_trim_by_values(upper=7)
        self.assertEqual(
            list(sa_values), [{"a": 1, "c": None}, {"b": 4, None: 5}, None]
        )

        # empty SArray
        sa = SArray()
        with self.assertRaises(RuntimeError):
            sa.dict_trim_by_values()

        sa = SArray([], dict)
        self.assertEqual(list(sa.dict_trim_by_values().head(10)), [], list)

    def test_dict_has_any_keys(self):
        d = [{"a": 1, "b": 20, "c": None}, {"b": 4, None: 5}, None, {"a": 0}]
        sa = SArray(d)
        sa_values = sa.dict_has_any_keys([])
        self.assertEqual(list(sa_values), [0, 0, None, 0])

        sa_values = sa.dict_has_any_keys(["a"])
        self.assertEqual(list(sa_values), [1, 0, None, 1])

        # one value is auto convert to list
        sa_values = sa.dict_has_any_keys("a")
        self.assertEqual(list(sa_values), [1, 0, None, 1])

        sa_values = sa.dict_has_any_keys(["a", "b"])
        self.assertEqual(list(sa_values), [1, 1, None, 1])

        with self.assertRaises(TypeError):
            sa.dict_has_any_keys()

        # empty SArray
        sa = SArray()
        with self.assertRaises(TypeError):
            sa.dict_has_any_keys()

        sa = SArray([], dict)
        self.assertEqual(list(sa.dict_has_any_keys([]).head(10)), [], list)

    def test_dict_has_all_keys(self):
        d = [{"a": 1, "b": 20, "c": None}, {"b": 4, None: 5}, None, {"a": 0}]
        sa = SArray(d)
        sa_values = sa.dict_has_all_keys([])
        self.assertEqual(list(sa_values), [1, 1, None, 1])

        sa_values = sa.dict_has_all_keys(["a"])
        self.assertEqual(list(sa_values), [1, 0, None, 1])

        # one value is auto convert to list
        sa_values = sa.dict_has_all_keys("a")
        self.assertEqual(list(sa_values), [1, 0, None, 1])

        sa_values = sa.dict_has_all_keys(["a", "b"])
        self.assertEqual(list(sa_values), [1, 0, None, 0])

        sa_values = sa.dict_has_all_keys([None, "b"])
        self.assertEqual(list(sa_values), [0, 1, None, 0])

        with self.assertRaises(TypeError):
            sa.dict_has_all_keys()

        # empty SArray
        sa = SArray()
        with self.assertRaises(TypeError):
            sa.dict_has_all_keys()

        sa = SArray([], dict)
        self.assertEqual(list(sa.dict_has_all_keys([]).head(10)), [], list)

    def test_save_load_cleanup_file(self):
        # similarly for SArray
        with util.TempDirectory() as f:
            sa = SArray(range(1, 1000000))
            sa.save(f)

            # 17 for each sarray, 1 object.bin, 1 ini
            file_count = len(os.listdir(f))
            self.assertTrue(file_count > 2)

            # sf1 now references the on disk file
            sa1 = SArray(f)

            # create another SFrame and save to the same location
            sa2 = SArray([str(i) for i in range(1, 100000)])
            sa2.save(f)

            file_count = len(os.listdir(f))
            self.assertTrue(file_count > 2)

            # now sf1 should still be accessible
            self.__test_equal(sa1, list(sa), int)

            # and sf2 is correct too
            sa3 = SArray(f)
            self.__test_equal(sa3, list(sa2), str)

            # when sf1 goes out of scope, the tmp files should be gone
            sa1 = 1
            time.sleep(1)  # give time for the files being deleted
            file_count = len(os.listdir(f))
            self.assertTrue(file_count > 2)

    # list_to_compare must have all unique values for this to work
    def __generic_unique_test(self, list_to_compare):
        test = SArray(list_to_compare + list_to_compare)
        self.assertEqual(sorted(list(test.unique())), sorted(list_to_compare))

    def test_unique(self):
        # Test empty SArray
        test = SArray([])
        self.assertEqual(list(test.unique()), [])

        # Test one value
        test = SArray([1])
        self.assertEqual(list(test.unique()), [1])

        # Test many of one value
        test = SArray([1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1])
        self.assertEqual(list(test.unique()), [1])

        # Test all unique values
        test = SArray(self.int_data)
        self.assertEqual(sorted(list(test.unique())), self.int_data)

        # Test an interesting sequence
        interesting_ints = [
            4654,
            4352436,
            5453,
            7556,
            45435,
            4654,
            5453,
            4654,
            5453,
            1,
            1,
            1,
            5,
            5,
            5,
            8,
            66,
            7,
            7,
            77,
            90,
            -34,
        ]
        test = SArray(interesting_ints)
        u = test.unique()
        self.assertEqual(len(u), 13)

        # We do not preserve order
        self.assertEqual(sorted(list(u)), sorted(np.unique(interesting_ints)))

        # Test other types
        self.__generic_unique_test(self.string_data[0:6])

        # only works reliably because these are values that floats can perform
        # reliable equality tests
        self.__generic_unique_test(self.float_data)

        self.__generic_unique_test(self.list_data)
        self.__generic_unique_test(self.vec_data)

        with self.assertRaises(TypeError):
            SArray(self.dict_data).unique()

    def test_item_len(self):
        # empty SArray
        test = SArray([])
        with self.assertRaises(TypeError):
            self.assertEqual(test.item_length())

        # wrong type
        test = SArray([1, 2, 3])
        with self.assertRaises(TypeError):
            self.assertEqual(test.item_length())

        test = SArray(["1", "2", "3"])
        with self.assertRaises(TypeError):
            self.assertEqual(test.item_length())

        # vector type
        test = SArray([[], [1], [1, 2], [1, 2, 3], None])
        item_length = test.item_length()
        self.assertEqual(list(item_length), list([0, 1, 2, 3, None]))

        # dict type
        test = SArray([{}, {"key1": 1}, {"key2": 1, "key1": 2}, None])
        self.assertEqual(list(test.item_length()), list([0, 1, 2, None]))

        # list type
        test = SArray([[], [1, 2], ["str", "str2"], None])
        self.assertEqual(list(test.item_length()), list([0, 2, 2, None]))

    def test_random_access(self):
        t = list(range(0, 100000))
        s = SArray(t)
        # simple slices
        self.__test_equal(s[1:10000], t[1:10000], int)
        self.__test_equal(s[0:10000:3], t[0:10000:3], int)
        self.__test_equal(s[1:10000:3], t[1:10000:3], int)
        self.__test_equal(s[2:10000:3], t[2:10000:3], int)
        self.__test_equal(s[3:10000:101], t[3:10000:101], int)
        # negative slices
        self.__test_equal(s[-5:], t[-5:], int)
        self.__test_equal(s[-1:], t[-1:], int)
        self.__test_equal(s[-100:-10], t[-100:-10], int)
        self.__test_equal(s[-100:-10:2], t[-100:-10:2], int)
        # single element reads
        self.assertEqual(s[511], t[511])
        self.assertEqual(s[1912], t[1912])
        self.assertEqual(s[-1], t[-1])
        self.assertEqual(s[-10], t[-10])

        # A cache boundary
        self.assertEqual(s[32 * 1024 - 1], t[32 * 1024 - 1])
        self.assertEqual(s[32 * 1024], t[32 * 1024])

        # totally different
        self.assertEqual(s[19312], t[19312])

        # edge case oddities
        self.__test_equal(s[10:100:100], t[10:100:100], int)
        self.__test_equal(s[-100 : len(s) : 10], t[-100 : len(t) : 10], int)
        self.__test_equal(s[-1:-2], t[-1:-2], int)
        self.__test_equal(s[-1:-1000:2], t[-1:-1000:2], int)
        with self.assertRaises(IndexError):
            s[len(s)]

        # with caching abilities; these should be fast, as 32K
        # elements are cached.
        for i in range(0, 100000, 100):
            self.assertEqual(s[i], t[i])
        for i in range(0, 100000, 100):
            self.assertEqual(s[-i], t[-i])

    def test_sort(self):
        test = SArray([1, 2, 3, 5, 1, 4])
        ascending = SArray([1, 1, 2, 3, 4, 5])
        descending = SArray([5, 4, 3, 2, 1, 1])
        result = test.sort()
        self.assertEqual(list(result), list(ascending))
        result = test.sort(ascending=False)
        self.assertEqual(list(result), list(descending))

        with self.assertRaises(TypeError):
            SArray([[1, 2], [2, 3]]).sort()

    def test_unicode_encode_should_not_fail(self):
        g = SArray([{"a": u"\u2019"}])
        g = SArray([u"123", u"\u2019"])
        g = SArray(["123", u"\u2019"])

    def test_from_const(self):
        g = SArray.from_const("a", 100)
        self.assertEqual(len(g), 100)
        self.assertEqual(list(g), ["a"] * 100)
        g = SArray.from_const(dt.datetime(2013, 5, 7, 10, 4, 10), 10)
        self.assertEqual(len(g), 10)
        self.assertEqual(list(g), [dt.datetime(2013, 5, 7, 10, 4, 10)] * 10)
        g = SArray.from_const(0, 0)
        self.assertEqual(len(g), 0)

        g = SArray.from_const(None, 100)
        self.assertEqual(list(g), [None] * 100)
        self.assertEqual(g.dtype, float)

        g = SArray.from_const(None, 100, str)
        self.assertEqual(list(g), [None] * 100)
        self.assertEqual(g.dtype, str)

        g = SArray.from_const(0, 100, float)
        self.assertEqual(list(g), [0.0] * 100)
        self.assertEqual(g.dtype, float)

        g = SArray.from_const(0.0, 100, int)
        self.assertEqual(list(g), [0] * 100)
        self.assertEqual(g.dtype, int)

        g = SArray.from_const(None, 100, float)
        self.assertEqual(list(g), [None] * 100)
        self.assertEqual(g.dtype, float)

        g = SArray.from_const(None, 100, int)
        self.assertEqual(list(g), [None] * 100)
        self.assertEqual(g.dtype, int)

        g = SArray.from_const(None, 100, list)
        self.assertEqual(list(g), [None] * 100)
        self.assertEqual(g.dtype, list)

        g = SArray.from_const([1], 100, list)
        self.assertEqual(list(g), [[1]] * 100)
        self.assertEqual(g.dtype, list)

    def test_from_sequence(self):
        with self.assertRaises(TypeError):
            g = SArray.from_sequence()
        g = SArray.from_sequence(100)
        self.assertEqual(list(g), list(range(100)))
        g = SArray.from_sequence(10, 100)
        self.assertEqual(list(g), list(range(10, 100)))
        g = SArray.from_sequence(100, 10)
        self.assertEqual(list(g), list(range(100, 10)))

    def test_datetime(self):
        sa = SArray(self.datetime_data)
        self.__test_equal(sa, self.datetime_data, dt.datetime)
        sa = SArray(self.datetime_data2)
        self.__test_equal(sa, self.datetime_data2, dt.datetime)

        ret = sa.split_datetime(
            limit=[
                "year",
                "month",
                "day",
                "hour",
                "minute",
                "second",
                "us",
                "weekday",
                "isoweekday",
                "tmweekday",
            ]
        )
        self.assertEqual(ret.num_columns(), 10)
        self.__test_equal(ret["X.year"], [2013, 1902, None], int)
        self.__test_equal(ret["X.month"], [5, 10, None], int)
        self.__test_equal(ret["X.day"], [7, 21, None], int)
        self.__test_equal(ret["X.hour"], [10, 10, None], int)
        self.__test_equal(ret["X.minute"], [4, 34, None], int)
        self.__test_equal(ret["X.second"], [10, 10, None], int)
        self.__test_equal(ret["X.us"], [109321, 991111, None], int)
        self.__test_equal(ret["X.weekday"], [1, 1, None], int)
        self.__test_equal(ret["X.isoweekday"], [2, 2, None], int)
        self.__test_equal(ret["X.tmweekday"], [2, 2, None], int)

    def test_datetime_difference(self):
        sa = SArray(self.datetime_data)
        sa2 = SArray(self.datetime_data2)
        res = sa2 - sa
        expected = [
            float(x.microsecond) / 1000000.0 if x is not None else x
            for x in self.datetime_data2
        ]
        self.assertEqual(len(res), len(expected))
        for i in range(len(res)):
            if res[i] is None:
                self.assertEqual(res[i], expected[i])
            else:
                self.assertAlmostEqual(res[i], expected[i], places=6)

    def test_datetime_lambda(self):
        data = [
            dt.datetime(2013, 5, 7, 10, 4, 10, 109321),
            dt.datetime(1902, 10, 21, 10, 34, 10, 991111, tzinfo=GMT(1)),
        ]
        g = SArray(data)
        gstr = g.apply(lambda x: str(x))
        self.__test_equal(gstr, [str(x) for x in g], str)
        gident = g.apply(lambda x: x)
        self.__test_equal(gident, list(g), dt.datetime)

    def test_datetime_to_str(self):
        sa = SArray(self.datetime_data)
        sa_string_back = sa.datetime_to_str()

        self.__test_equal(
            sa_string_back,
            ["2013-05-07T10:04:10", "1902-10-21T10:34:10GMT+00", None],
            str,
        )

        sa = SArray([None, None, None], dtype=dt.datetime)
        sa_string_back = sa.datetime_to_str()

        self.__test_equal(sa_string_back, [None, None, None], str)

        sa = SArray(dtype=dt.datetime)
        sa_string_back = sa.datetime_to_str()

        self.__test_equal(sa_string_back, [], str)

        sa = SArray([None, None, None])
        self.assertRaises(TypeError, sa.datetime_to_str)

        sa = SArray()
        self.assertRaises(TypeError, sa.datetime_to_str)

    def test_str_to_datetime(self):
        sa_string = SArray(["2013-05-07T10:04:10", "1902-10-21T10:34:10GMT+00", None])
        sa_datetime_back = sa_string.str_to_datetime()

        expected = self.datetime_data

        self.__test_equal(sa_datetime_back, expected, dt.datetime)

        sa_string = SArray([None, None, None], str)
        sa_datetime_back = sa_string.str_to_datetime()

        self.__test_equal(sa_datetime_back, [None, None, None], dt.datetime)

        sa_string = SArray(dtype=str)
        sa_datetime_back = sa_string.str_to_datetime()

        self.__test_equal(sa_datetime_back, [], dt.datetime)

        sa = SArray([None, None, None])
        self.assertRaises(TypeError, sa.str_to_datetime)

        sa = SArray()
        self.assertRaises(TypeError, sa.str_to_datetime)

        # hour without leading zero
        sa = SArray(["10/30/2014 9:01"])
        sa = sa.str_to_datetime("%m/%d/%Y %H:%M")
        expected = [dt.datetime(2014, 10, 30, 9, 1)]
        self.__test_equal(sa, expected, dt.datetime)

        # without delimiters
        sa = SArray(["10302014 0901", "10302014 2001"])
        sa = sa.str_to_datetime("%m%d%Y %H%M")
        expected = [dt.datetime(2014, 10, 30, 9, 1), dt.datetime(2014, 10, 30, 20, 1)]
        self.__test_equal(sa, expected, dt.datetime)

        # another without delimiter test
        sa = SArray(["20110623T191001"])
        sa = sa.str_to_datetime("%Y%m%dT%H%M%S%F%q")
        expected = [dt.datetime(2011, 6, 23, 19, 10, 1)]
        self.__test_equal(sa, expected, dt.datetime)

        # am pm
        sa = SArray(["10/30/2014 9:01am", "10/30/2014 9:01pm"])
        sa = sa.str_to_datetime("%m/%d/%Y %H:%M%p")
        expected = [dt.datetime(2014, 10, 30, 9, 1), dt.datetime(2014, 10, 30, 21, 1)]
        self.__test_equal(sa, expected, dt.datetime)

        sa = SArray(["10/30/2014 9:01AM", "10/30/2014 9:01PM"])
        sa = sa.str_to_datetime("%m/%d/%Y %H:%M%P")
        expected = [dt.datetime(2014, 10, 30, 9, 1), dt.datetime(2014, 10, 30, 21, 1)]
        self.__test_equal(sa, expected, dt.datetime)

        # failure 13pm
        sa = SArray(["10/30/2014 13:01pm"])
        with self.assertRaises(RuntimeError):
            sa.str_to_datetime("%m/%d/%Y %H:%M%p")

        # failure hour 13 when %l should only have up to hour 12
        sa = SArray(["10/30/2014 13:01"])
        with self.assertRaises(RuntimeError):
            sa.str_to_datetime("%m/%d/%Y %l:%M")

        with self.assertRaises(RuntimeError):
            sa.str_to_datetime("%m/%d/%Y %L:%M")

        sa = SArray(["2013-05-07T10:04:10", "1902-10-21T10:34:10UTC+05:45"])
        expected = [
            dt.datetime(2013, 5, 7, 10, 4, 10),
            dt.datetime(1902, 10, 21, 10, 34, 10).replace(tzinfo=GMT(5.75)),
        ]
        self.__test_equal(sa.str_to_datetime(), expected, dt.datetime)

    def test_apply_with_partial(self):
        sa = SArray([1, 2, 3, 4, 5])

        def concat_fn(character, number):
            return "%s%d" % (character, number)

        my_partial_fn = functools.partial(concat_fn, "x")
        sa_transformed = sa.apply(my_partial_fn)
        self.assertEqual(list(sa_transformed), ["x1", "x2", "x3", "x4", "x5"])

    def test_apply_with_functor(self):
        sa = SArray([1, 2, 3, 4, 5])

        class Concatenator(object):
            def __init__(self, character):
                self.character = character

            def __call__(self, number):
                return "%s%d" % (self.character, number)

        concatenator = Concatenator("x")
        sa_transformed = sa.apply(concatenator)
        self.assertEqual(list(sa_transformed), ["x1", "x2", "x3", "x4", "x5"])

    def test_argmax_argmin(self):
        sa = SArray([1, 4, -1, 10, 3, 5, 8])
        index = [sa.argmax(), sa.argmin()]
        expected = [3, 2]
        self.assertEqual(index, expected)

        sa = SArray([1, 4.3, -1.4, 0, 3, 5.6, 8.9])
        index = [sa.argmax(), sa.argmin()]
        expected = [6, 2]
        self.assertEqual(index, expected)

        # empty case
        sa = SArray([])
        index = [sa.argmax(), sa.argmin()]
        expected = [None, None]
        self.assertEqual(index, expected)

        # non-numeric type
        sa = SArray(["434", "43"])
        with self.assertRaises(TypeError):
            sa.argmax()

        with self.assertRaises(TypeError):
            sa.argmin()

    def test_apply_with_recursion(self):
        sa = SArray(range(1000))
        sastr = sa.astype(str)
        rets = sa.apply(lambda x: sastr[x])
        self.assertEqual(list(rets), list(sastr))

    def test_save_sarray(self):
        """save lazily evaluated SArray should not materialize to target folder
        """
        data = SArray(range(1000))
        data = data[data > 50]
        # lazy and good
        tmp_dir = tempfile.mkdtemp()
        data.save(tmp_dir)
        shutil.rmtree(tmp_dir)
        print(data)

    def test_to_numpy(self):
        X = SArray(range(100))
        import numpy as np
        import numpy.testing as nptest

        Y = np.array(range(100))
        nptest.assert_array_equal(X.to_numpy(), Y)

        X = X.astype(str)
        Y = np.array([str(i) for i in range(100)])
        nptest.assert_array_equal(X.to_numpy(), Y)

    def test_rolling_mean(self):
        data = SArray(range(1000))
        neg_data = SArray(range(-100, 100, 2))

        ### Small backward window including current
        res = data.rolling_mean(-3, 0)
        expected = [None for i in range(3)] + [i + 0.5 for i in range(1, 998)]
        self.__test_equal(res, expected, float)

        # Test float inputs as well
        res = data.astype(float).rolling_mean(-3, 0)
        self.__test_equal(res, expected, float)

        # Test min observations
        res = data.rolling_mean(-3, 0, min_observations=5)
        self.__test_equal(res, expected, float)

        res = data.rolling_mean(-3, 0, min_observations=4)
        self.__test_equal(res, expected, float)

        res = data.rolling_mean(-3, 0, min_observations=3)
        expected[2] = 1.0
        self.__test_equal(res, expected, float)

        res = data.rolling_mean(-3, 0, min_observations=2)
        expected[1] = 0.5
        self.__test_equal(res, expected, float)

        res = data.rolling_mean(-3, 0, min_observations=1)
        expected[0] = 0.0
        self.__test_equal(res, expected, float)

        res = data.rolling_mean(-3, 0, min_observations=0)
        self.__test_equal(res, expected, float)

        with self.assertRaises(ValueError):
            res = data.rolling_mean(-3, 0, min_observations=-1)

        res = neg_data.rolling_mean(-3, 0)
        expected = [None for i in range(3)] + [float(i) for i in range(-97, 96, 2)]
        self.__test_equal(res, expected, float)

        # Test float inputs as well
        res = neg_data.astype(float).rolling_mean(-3, 0)
        self.__test_equal(res, expected, float)

        # Test vector input
        res = SArray(self.vec_data).rolling_mean(-3, 0)
        expected = [None for i in range(3)] + [
            array.array("d", [i + 0.5, i + 1.5]) for i in range(2, 9)
        ]
        self.__test_equal(res, expected, array.array)

        ### Small forward window including current
        res = data.rolling_mean(0, 4)
        expected = [float(i) for i in range(2, 998)] + [None for i in range(4)]
        self.__test_equal(res, expected, float)

        res = neg_data.rolling_mean(0, 4)
        expected = [float(i) for i in range(-96, 95, 2)] + [None for i in range(4)]
        self.__test_equal(res, expected, float)

        ### Small backward window not including current
        res = data.rolling_mean(-5, -1)
        expected = [None for i in range(5)] + [float(i) for i in range(2, 997)]
        self.__test_equal(res, expected, float)

        res = neg_data.rolling_mean(-5, -1)
        expected = [None for i in range(5)] + [float(i) for i in range(-96, 94, 2)]
        self.__test_equal(res, expected, float)

        ### Small forward window not including current
        res = data.rolling_mean(1, 5)
        expected = [float(i) for i in range(3, 998)] + [None for i in range(5)]
        self.__test_equal(res, expected, float)

        res = neg_data.rolling_mean(1, 5)
        expected = [float(i) for i in range(-94, 96, 2)] + [None for i in range(5)]
        self.__test_equal(res, expected, float)

        ### "Centered" rolling aggregate
        res = data.rolling_mean(-2, 2)
        expected = (
            [None for i in range(2)]
            + [float(i) for i in range(2, 998)]
            + [None for i in range(2)]
        )
        self.__test_equal(res, expected, float)

        res = neg_data.rolling_mean(-2, 2)
        expected = (
            [None for i in range(2)]
            + [float(i) for i in range(-96, 96, 2)]
            + [None for i in range(2)]
        )
        self.__test_equal(res, expected, float)

        ### Lopsided rolling aggregate
        res = data.rolling_mean(-2, 1)
        expected = (
            [None for i in range(2)]
            + [i + 0.5 for i in range(1, 998)]
            + [None for i in range(1)]
        )
        self.__test_equal(res, expected, float)

        res = neg_data.rolling_mean(-2, 1)
        expected = (
            [None for i in range(2)]
            + [float(i) for i in range(-97, 97, 2)]
            + [None for i in range(1)]
        )
        self.__test_equal(res, expected, float)

        ### A very forward window
        res = data.rolling_mean(500, 502)
        expected = [float(i) for i in range(501, 999)] + [None for i in range(502)]
        self.__test_equal(res, expected, float)

        res = neg_data.rolling_mean(50, 52)
        expected = [float(i) for i in range(2, 98, 2)] + [None for i in range(52)]
        self.__test_equal(res, expected, float)

        ### A very backward window
        res = data.rolling_mean(-502, -500)
        expected = [None for i in range(502)] + [float(i) for i in range(1, 499)]
        self.__test_equal(res, expected, float)

        res = neg_data.rolling_mean(-52, -50)
        expected = [None for i in range(52)] + [float(i) for i in range(-98, -2, 2)]
        self.__test_equal(res, expected, float)

        ### A window size much larger than anticipated segment size
        res = data.rolling_mean(0, 749)
        expected = [i + 0.5 for i in range(374, 625)] + [None for i in range(749)]
        self.__test_equal(res, expected, float)

        ### A window size larger than the array
        res = data.rolling_mean(0, 1000)
        expected = [None for i in range(1000)]
        self.__test_equal(res, expected, type(None))

        ### A window size of 1
        res = data.rolling_mean(0, 0)
        self.__test_equal(res, list(data), float)

        res = data.rolling_mean(-2, -2)
        expected = [None for i in range(2)] + list(data[0:998])
        self.__test_equal(res, expected, float)

        res = data.rolling_mean(3, 3)
        expected = list(data[3:1000]) + [None for i in range(3)]
        self.__test_equal(res, expected, float)

        ### A negative window size
        with self.assertRaises(RuntimeError):
            res = data.rolling_mean(4, 2)

        ### Non-numeric
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.string_data).rolling_mean(0, 1)

        ### Empty SArray
        sa = SArray()
        res = sa.rolling_mean(0, 1)
        self.__test_equal(res, [], type(None))

        ### Small SArray
        sa = SArray([1, 2, 3])
        res = sa.rolling_mean(0, 1)
        self.__test_equal(res, [1.5, 2.5, None], float)

    def test_rolling_sum(self):
        data = SArray(range(1000))
        neg_data = SArray(range(-100, 100, 2))

        ### Small backward window including current
        res = data.rolling_sum(-3, 0)
        expected = [None for i in range(3)] + [i for i in range(6, 3994, 4)]
        self.__test_equal(res, expected, int)

        # Test float inputs as well
        res = data.astype(float).rolling_sum(-3, 0)
        self.__test_equal(res, expected, float)

        # Test min observations
        res = data.rolling_sum(-3, 0, min_observations=5)
        self.__test_equal(res, expected, int)

        res = data.rolling_sum(-3, 0, min_observations=4)
        self.__test_equal(res, expected, int)

        res = data.rolling_sum(-3, 0, min_observations=3)
        expected[2] = 3
        self.__test_equal(res, expected, int)

        res = data.rolling_sum(-3, 0, min_observations=2)
        expected[1] = 1
        self.__test_equal(res, expected, int)

        res = data.rolling_sum(-3, 0, min_observations=1)
        expected[0] = 0
        self.__test_equal(res, expected, int)

        res = data.rolling_sum(-3, 0, min_observations=0)
        self.__test_equal(res, expected, int)

        with self.assertRaises(ValueError):
            res = data.rolling_sum(-3, 0, min_observations=-1)

        res = neg_data.rolling_sum(-3, 0)
        expected = [None for i in range(3)] + [i for i in range(-388, 388, 8)]
        self.__test_equal(res, expected, int)

        # Test float inputs as well
        res = neg_data.astype(float).rolling_sum(-3, 0)
        self.__test_equal(res, expected, float)

        # Test vector input
        res = SArray(self.vec_data).rolling_sum(-3, 0)
        expected = [None for i in range(3)] + [
            array.array("d", [i, i + 4]) for i in range(10, 38, 4)
        ]
        self.__test_equal(res, expected, array.array)

        ### Small forward window including current
        res = data.rolling_sum(0, 4)
        expected = [i for i in range(10, 4990, 5)] + [None for i in range(4)]
        self.__test_equal(res, expected, int)

        res = neg_data.rolling_sum(0, 4)
        expected = [i for i in range(-480, 480, 10)] + [None for i in range(4)]
        self.__test_equal(res, expected, int)

        ### Small backward window not including current
        res = data.rolling_sum(-5, -1)
        expected = [None for i in range(5)] + [i for i in range(10, 4985, 5)]
        self.__test_equal(res, expected, int)

        res = neg_data.rolling_sum(-5, -1)
        expected = [None for i in range(5)] + [i for i in range(-480, 470, 10)]
        self.__test_equal(res, expected, int)

        ### Small forward window not including current
        res = data.rolling_sum(1, 5)
        expected = [i for i in range(15, 4990, 5)] + [None for i in range(5)]
        self.__test_equal(res, expected, int)

        res = neg_data.rolling_sum(1, 5)
        expected = [i for i in range(-470, 480, 10)] + [None for i in range(5)]
        self.__test_equal(res, expected, int)

        ### "Centered" rolling aggregate
        res = data.rolling_sum(-2, 2)
        expected = (
            [None for i in range(2)]
            + [i for i in range(10, 4990, 5)]
            + [None for i in range(2)]
        )
        self.__test_equal(res, expected, int)

        res = neg_data.rolling_sum(-2, 2)
        expected = (
            [None for i in range(2)]
            + [i for i in range(-480, 480, 10)]
            + [None for i in range(2)]
        )
        self.__test_equal(res, expected, int)

        ### Lopsided rolling aggregate
        res = data.rolling_sum(-2, 1)
        expected = (
            [None for i in range(2)]
            + [i for i in range(6, 3994, 4)]
            + [None for i in range(1)]
        )
        self.__test_equal(res, expected, int)

        res = neg_data.rolling_sum(-2, 1)
        expected = (
            [None for i in range(2)]
            + [i for i in range(-388, 388, 8)]
            + [None for i in range(1)]
        )
        self.__test_equal(res, expected, int)

        ### A very forward window
        res = data.rolling_sum(500, 502)
        expected = [i for i in range(1503, 2997, 3)] + [None for i in range(502)]
        self.__test_equal(res, expected, int)

        res = neg_data.rolling_sum(50, 52)
        expected = [i for i in range(6, 294, 6)] + [None for i in range(52)]
        self.__test_equal(res, expected, int)

        ### A very backward window
        res = data.rolling_sum(-502, -500)
        expected = [None for i in range(502)] + [i for i in range(3, 1497, 3)]
        self.__test_equal(res, expected, int)

        res = neg_data.rolling_sum(-52, -50)
        expected = [None for i in range(52)] + [i for i in range(-294, -6, 6)]
        self.__test_equal(res, expected, int)

        ### A window size much larger than anticipated segment size
        res = data.rolling_sum(0, 749)
        expected = [i for i in range(280875, 469125, 750)] + [None for i in range(749)]
        self.__test_equal(res, expected, int)

        ### A window size larger than the array
        res = data.rolling_sum(0, 1000)
        expected = [None for i in range(1000)]
        self.__test_equal(res, expected, type(None))

        ### A window size of 1
        res = data.rolling_sum(0, 0)
        self.__test_equal(res, list(data), int)

        res = data.rolling_sum(-2, -2)
        expected = [None for i in range(2)] + list(data[0:998])
        self.__test_equal(res, expected, int)

        res = data.rolling_sum(3, 3)
        expected = list(data[3:1000]) + [None for i in range(3)]
        self.__test_equal(res, expected, int)

        ### A negative window size
        with self.assertRaises(RuntimeError):
            res = data.rolling_sum(4, 2)

        ### Non-numeric
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.string_data).rolling_sum(0, 1)

        ### Empty SArray
        sa = SArray()
        res = sa.rolling_sum(0, 1)
        self.__test_equal(res, [], type(None))

        ### Small SArray
        sa = SArray([1, 2, 3])
        res = sa.rolling_sum(0, 1)
        self.__test_equal(res, [3, 5, None], int)

    def test_rolling_max(self):
        data = SArray(range(1000))

        ### Small backward window including current
        res = data.rolling_max(-3, 0)
        expected = [None for i in range(3)] + [i for i in range(3, 1000)]
        self.__test_equal(res, expected, int)

        # Test float inputs as well
        res = data.astype(float).rolling_max(-3, 0)
        self.__test_equal(res, expected, float)

        # Test min observations
        res = data.rolling_max(-3, 0, min_observations=5)
        self.__test_equal(res, expected, int)

        res = data.rolling_max(-3, 0, min_observations=4)
        self.__test_equal(res, expected, int)

        res = data.rolling_max(-3, 0, min_observations=3)
        expected[2] = 2
        self.__test_equal(res, expected, int)

        with self.assertRaises(ValueError):
            res = data.rolling_max(-3, 0, min_observations=-1)

        # Test vector input
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.vec_data).rolling_max(-3, 0)

        ### Small forward window including current
        res = data.rolling_max(0, 4)
        expected = [float(i) for i in range(4, 1000)] + [None for i in range(4)]
        self.__test_equal(res, expected, int)

        ### A window size of 1
        res = data.rolling_max(0, 0)
        self.__test_equal(res, list(data), int)

        res = data.rolling_max(-2, -2)
        expected = [None for i in range(2)] + list(data[0:998])
        self.__test_equal(res, expected, int)

        res = data.rolling_max(3, 3)
        expected = list(data[3:1000]) + [None for i in range(3)]
        self.__test_equal(res, expected, int)

        ### A negative window size
        with self.assertRaises(RuntimeError):
            res = data.rolling_max(4, 2)

        ### Non-numeric
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.string_data).rolling_max(0, 1)

        ### Empty SArray
        sa = SArray()
        res = sa.rolling_max(0, 1)
        self.__test_equal(res, [], type(None))

        ### Small SArray
        sa = SArray([1, 2, 3])
        res = sa.rolling_max(0, 1)
        self.__test_equal(res, [2, 3, None], int)

    def test_rolling_min(self):
        data = SArray(range(1000))

        ### Small backward window including current
        res = data.rolling_min(-3, 0)
        expected = [None for i in range(3)] + [i for i in range(0, 997)]
        self.__test_equal(res, expected, int)

        # Test float inputs as well
        res = data.astype(float).rolling_min(-3, 0)
        self.__test_equal(res, expected, float)

        # Test min observations
        res = data.rolling_min(-3, 0, min_observations=5)
        self.__test_equal(res, expected, int)

        res = data.rolling_min(-3, 0, min_observations=4)
        self.__test_equal(res, expected, int)

        res = data.rolling_min(-3, 0, min_observations=3)
        expected[2] = 0
        self.__test_equal(res, expected, int)

        with self.assertRaises(ValueError):
            res = data.rolling_min(-3, 0, min_observations=-1)

        # Test vector input
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.vec_data).rolling_min(-3, 0)

        ### Small forward window including current
        res = data.rolling_min(0, 4)
        expected = [float(i) for i in range(0, 996)] + [None for i in range(4)]
        self.__test_equal(res, expected, int)

        ### A window size of 1
        res = data.rolling_min(0, 0)
        self.__test_equal(res, list(data), int)

        res = data.rolling_min(-2, -2)
        expected = [None for i in range(2)] + list(data[0:998])
        self.__test_equal(res, expected, int)

        res = data.rolling_min(3, 3)
        expected = list(data[3:1000]) + [None for i in range(3)]
        self.__test_equal(res, expected, int)

        ### A negative window size
        with self.assertRaises(RuntimeError):
            res = data.rolling_min(4, 2)

        ### Non-numeric
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.string_data).rolling_min(0, 1)

        ### Empty SArray
        sa = SArray()
        res = sa.rolling_min(0, 1)
        self.__test_equal(res, [], type(None))

        ### Small SArray
        sa = SArray([1, 2, 3])
        res = sa.rolling_min(0, 1)
        self.__test_equal(res, [1, 2, None], int)

    def test_rolling_var(self):
        data = SArray(range(1000))

        ### Small backward window including current
        res = data.rolling_var(-3, 0)
        expected = [None for i in range(3)] + [1.25 for i in range(997)]
        self.__test_equal(res, expected, float)

        # Test float inputs as well
        res = data.astype(float).rolling_var(-3, 0)
        self.__test_equal(res, expected, float)

        # Test min observations
        res = data.rolling_var(-3, 0, min_observations=5)
        self.__test_equal(res, expected, float)

        res = data.rolling_var(-3, 0, min_observations=4)
        self.__test_equal(res, expected, float)

        res = data.rolling_var(-3, 0, min_observations=3)
        expected[2] = 2.0 / 3.0
        self.__test_equal(res, expected, float)

        with self.assertRaises(ValueError):
            res = data.rolling_var(-3, 0, min_observations=-1)

        # Test vector input
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.vec_data).rolling_var(-3, 0)

        ### Small forward window including current
        res = data.rolling_var(0, 4)
        expected = [2 for i in range(996)] + [None for i in range(4)]
        self.__test_equal(res, expected, float)

        ### A window size of 1
        res = data.rolling_var(0, 0)
        self.__test_equal(res, [0 for i in range(1000)], float)

        res = data.rolling_var(-2, -2)
        self.__test_equal(res, [None, None] + [0 for i in range(998)], float)

        ### A negative window size
        with self.assertRaises(RuntimeError):
            res = data.rolling_var(4, 2)

        ### Non-numeric
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.string_data).rolling_var(0, 1)

        ### Empty SArray
        sa = SArray()
        res = sa.rolling_var(0, 1)
        self.__test_equal(res, [], type(None))

        ### Small SArray
        sa = SArray([1, 2, 3])
        res = sa.rolling_var(0, 1)
        self.__test_equal(res, [0.25, 0.25, None], float)

    def test_rolling_stdv(self):
        data = SArray(range(1000))

        ### Small backward window including current
        res = data.rolling_stdv(-3, 0)
        expected = [None for i in range(3)] + [1.118033988749895 for i in range(997)]
        self.__test_equal(res, expected, float)

        # Test float inputs as well
        res = data.astype(float).rolling_stdv(-3, 0)
        self.__test_equal(res, expected, float)

        # Test min observations
        res = data.rolling_stdv(-3, 0, min_observations=5)
        self.__test_equal(res, expected, float)

        res = data.rolling_stdv(-3, 0, min_observations=4)
        self.__test_equal(res, expected, float)

        res = data.rolling_stdv(-3, 0, min_observations=3)
        expected[2] = math.sqrt(2.0 / 3.0)
        self.__test_equal(res, expected, float)

        with self.assertRaises(ValueError):
            res = data.rolling_stdv(-3, 0, min_observations=-1)

        # Test vector input
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.vec_data).rolling_stdv(-3, 0)

        ### Small forward window including current
        res = data.rolling_stdv(0, 4)
        expected = [math.sqrt(2) for i in range(996)] + [None for i in range(4)]
        self.__test_equal(res, expected, float)

        ### A window size of 1
        res = data.rolling_stdv(0, 0)
        self.__test_equal(res, [0 for i in range(1000)], float)

        res = data.rolling_stdv(-2, -2)
        self.__test_equal(res, [None, None] + [0 for i in range(998)], float)

        ### A negative window size
        with self.assertRaises(RuntimeError):
            res = data.rolling_stdv(4, 2)

        ### Non-numeric
        with self.assertRaisesRegexp(RuntimeError, ".*support.*type.*"):
            res = SArray(self.string_data).rolling_stdv(0, 1)

        ### Empty SArray
        sa = SArray()
        res = sa.rolling_stdv(0, 1)
        self.__test_equal(res, [], type(None))

        ### Small SArray
        sa = SArray([1, 2, 3])
        res = sa.rolling_stdv(0, 1)
        self.__test_equal(res, [0.5, 0.5, None], float)

    def test_rolling_count(self):
        data = SArray(range(100))

        ### Small backward window including current
        res = data.rolling_count(-3, 0)
        expected = [1, 2, 3] + [4 for i in range(97)]
        self.__test_equal(res, expected, int)

        # Test float inputs
        res = data.astype(float).rolling_count(-3, 0)
        self.__test_equal(res, expected, int)

        # Test vector input
        res = SArray(self.vec_data).rolling_count(-3, 0)
        expected = [1, 2, 3] + [4 for i in range(7)]
        self.__test_equal(res, expected, int)

        ### Test string input
        res = SArray(self.string_data).rolling_count(-3, 0)
        self.__test_equal(res, expected[0:8], int)

        ### Small forward window including current
        res = data.rolling_count(0, 4)
        expected = [5 for i in range(0, 96)] + [4, 3, 2, 1]
        self.__test_equal(res, expected, int)

        ### A window size of 1
        res = data.rolling_count(0, 0)
        self.__test_equal(res, [1 for i in range(100)], int)

        res = data.rolling_count(-2, -2)
        self.__test_equal(res, [0, 0] + [1 for i in range(98)], int)

        ### A negative window size
        with self.assertRaises(RuntimeError):
            res = data.rolling_count(4, 2)

        ### Empty SArray
        sa = SArray()
        res = sa.rolling_count(0, 1)
        self.__test_equal(res, [], type(None))

        ### Small SArray
        sa = SArray([1, 2, 3])
        res = sa.rolling_count(0, 1)
        self.__test_equal(res, [2, 2, 1], int)

        sa = SArray([1, 2, None])
        res = sa.rolling_count(0, 1)
        self.__test_equal(res, [2, 1, 0], int)

    def cumulative_aggregate_comparison(self, out, ans):
        import array

        self.assertEqual(out.dtype, ans.dtype)
        self.assertEqual(len(out), len(ans))
        for i in range(len(out)):
            if out[i] is None:
                self.assertTrue(ans[i] is None)
            if ans[i] is None:
                self.assertTrue(out[i] is None)

            if type(out[i]) != array.array:
                self.assertAlmostEqual(out[i], ans[i])
            else:
                self.assertEqual(len(out[i]), len(ans[i]))
                oi = out[i]
                ansi = ans[i]
                for j in range(len(oi)):
                    self.assertAlmostEqual(oi, ansi)

    def test_cumulative_sum(self):
        def single_test(src, ans):
            out = src.cumulative_sum()
            self.cumulative_aggregate_comparison(out, ans)

        with self.assertRaises(RuntimeError):
            sa = SArray(["foo"]).cumulative_sum()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], ["foo"]]).cumulative_sum()
        with self.assertRaises(RuntimeError):
            sa = SArray([{"bar": 1}]).cumulative_sum()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1, 1], [1], [1]]).cumulative_sum()

        single_test(
            SArray([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
            SArray([0, 1, 3, 6, 10, 15, 21, 28, 36, 45, 55]),
        )
        single_test(
            SArray([0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1]),
            SArray([0.1, 1.2, 3.3, 6.4, 10.5, 15.6, 21.7, 28.8]),
        )
        single_test(
            SArray([[11.0, 2.0], [22.0, 1.0], [3.0, 4.0], [4.0, 4.0]]),
            SArray([[11.0, 2.0], [33.0, 3.0], [36.0, 7.0], [40.0, 11.0]]),
        )
        single_test(
            SArray([None, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
            SArray([None, 1, 3, 6, 10, 15, 21, 28, 36, 45, 55]),
        )
        single_test(SArray([None, 1, None, 3, None, 5]), SArray([None, 1, 1, 4, 4, 9]))
        single_test(
            SArray([None, [33.0, 3.0], [3.0, 4.0], [4.0, 4.0]]),
            SArray([None, [33.0, 3.0], [36.0, 7.0], [40.0, 11.0]]),
        )
        single_test(
            SArray([None, [33.0, 3.0], None, [4.0, 4.0]]),
            SArray([None, [33.0, 3.0], [33.0, 3.0], [37.0, 7.0]]),
        )

    def test_cumulative_mean(self):
        def single_test(src, ans):
            out = src.cumulative_mean()
            self.cumulative_aggregate_comparison(out, ans)

        with self.assertRaises(RuntimeError):
            sa = SArray(["foo"]).cumulative_mean()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], ["foo"]]).cumulative_mean()
        with self.assertRaises(RuntimeError):
            sa = SArray([{"bar": 1}]).cumulative_mean()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1, 1], [1], [1]]).cumulative_mean()

        single_test(
            SArray([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
            SArray([0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0]),
        )
        single_test(
            SArray([0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1]),
            SArray([0.1, 0.6, 1.1, 1.6, 2.1, 2.6, 3.1, 3.6]),
        )
        single_test(
            SArray([[11.0, 22.0], [33.0, 66.0], [4.0, 2.0], [4.0, 2.0]]),
            SArray([[11.0, 22.0], [22.0, 44.0], [16.0, 30.0], [13.0, 23.0]]),
        )
        single_test(
            SArray([None, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
            SArray([None, 0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0]),
        )
        single_test(
            SArray([None, 1, None, 3, None, 5]), SArray([None, 1, 1.0, 2.0, 2.0, 3.0])
        )
        single_test(
            SArray([None, [11.0, 22.0], [33.0, 66.0], [4.0, 2.0]]),
            SArray([None, [11.0, 22.0], [22.0, 44.0], [16.0, 30.0]]),
        )
        single_test(
            SArray([None, [11.0, 22.0], None, [33.0, 66.0], [4.0, 2.0]]),
            SArray([None, [11.0, 22.0], [11.0, 22.0], [22.0, 44.0], [16.0, 30.0]]),
        )

    def test_cumulative_min(self):
        def single_test(src, ans):
            out = src.cumulative_min()
            self.cumulative_aggregate_comparison(out, ans)

        with self.assertRaises(RuntimeError):
            sa = SArray(["foo"]).cumulative_min()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], ["foo"]]).cumulative_min()
        with self.assertRaises(RuntimeError):
            sa = SArray([{"bar": 1}]).cumulative_min()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1, 1], [1], [1]]).cumulative_min()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1], [1], [1]]).cumulative_min()

        single_test(
            SArray([0, 1, 2, 3, 4, 5, -1, 7, 8, -2, 10]),
            SArray([0, 0, 0, 0, 0, 0, -1, -1, -1, -2, -2]),
        )
        single_test(
            SArray([7.1, 6.1, 3.1, 3.9, 4.1, 2.1, 2.9, 0.1]),
            SArray([7.1, 6.1, 3.1, 3.1, 3.1, 2.1, 2.1, 0.1]),
        )
        single_test(
            SArray([None, 8, 6, 3, 4, None, 6, 2, 8, 9, 1]),
            SArray([None, 8, 6, 3, 3, 3, 3, 2, 2, 2, 1]),
        )
        single_test(SArray([None, 5, None, 3, None, 10]), SArray([None, 5, 5, 3, 3, 3]))

    def test_cumulative_max(self):
        def single_test(src, ans):
            out = src.cumulative_max()
            self.cumulative_aggregate_comparison(out, ans)

        with self.assertRaises(RuntimeError):
            sa = SArray(["foo"]).cumulative_max()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], ["foo"]]).cumulative_max()
        with self.assertRaises(RuntimeError):
            sa = SArray([{"bar": 1}]).cumulative_max()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1, 1], [1], [1]]).cumulative_max()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1], [1], [1]]).cumulative_max()

        single_test(
            SArray([0, 1, 0, 3, 5, 4, 1, 7, 6, 2, 10]),
            SArray([0, 1, 1, 3, 5, 5, 5, 7, 7, 7, 10]),
        )
        single_test(
            SArray([2.1, 6.1, 3.1, 3.9, 2.1, 8.1, 8.9, 10.1]),
            SArray([2.1, 6.1, 6.1, 6.1, 6.1, 8.1, 8.9, 10.1]),
        )
        single_test(
            SArray([None, 1, 6, 3, 4, None, 4, 2, 8, 9, 1]),
            SArray([None, 1, 6, 6, 6, 6, 6, 6, 8, 9, 9]),
        )
        single_test(
            SArray([None, 2, None, 3, None, 10]), SArray([None, 2, 2, 3, 3, 10])
        )

    def test_cumulative_std(self):
        def single_test(src, ans):
            out = src.cumulative_std()
            self.cumulative_aggregate_comparison(out, ans)

        with self.assertRaises(RuntimeError):
            sa = SArray(["foo"]).cumulative_std()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], ["foo"]]).cumulative_std()
        with self.assertRaises(RuntimeError):
            sa = SArray([{"bar": 1}]).cumulative_std()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1, 1], [1], [1]]).cumulative_std()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1], [1], [1]]).cumulative_std()

        single_test(
            SArray([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
            SArray(
                [
                    0.0,
                    0.5,
                    0.816496580927726,
                    1.118033988749895,
                    1.4142135623730951,
                    1.707825127659933,
                    2.0,
                    2.29128784747792,
                    2.581988897471611,
                    2.8722813232690143,
                    3.1622776601683795,
                ]
            ),
        )
        single_test(
            SArray([0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1]),
            SArray(
                [
                    0.0,
                    0.5,
                    0.81649658092772603,
                    1.1180339887498949,
                    1.4142135623730949,
                    1.707825127659933,
                    1.9999999999999998,
                    2.2912878474779195,
                ]
            ),
        )
        single_test(
            SArray([None, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
            SArray(
                [
                    None,
                    0.0,
                    0.5,
                    0.816496580927726,
                    1.118033988749895,
                    1.4142135623730951,
                    1.707825127659933,
                    2.0,
                    2.29128784747792,
                    2.581988897471611,
                    2.8722813232690143,
                    3.1622776601683795,
                ]
            ),
        )
        single_test(
            SArray([None, 1, None, 3, None, 5]),
            SArray([None, 0.0, 0.0, 1.0, 1.0, 1.6329931618554521]),
        )

    def test_cumulative_var(self):
        def single_test(src, ans):
            out = src.cumulative_var()
            self.cumulative_aggregate_comparison(out, ans)

        with self.assertRaises(RuntimeError):
            sa = SArray(["foo"]).cumulative_var()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], ["foo"]]).cumulative_var()
        with self.assertRaises(RuntimeError):
            sa = SArray([{"bar": 1}]).cumulative_var()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1, 1], [1], [1]]).cumulative_var()
        with self.assertRaises(RuntimeError):
            sa = SArray([[1], [1], [1], [1]]).cumulative_var()

        single_test(
            SArray([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
            SArray(
                [
                    0.0,
                    0.25,
                    0.6666666666666666,
                    1.25,
                    2.0,
                    2.9166666666666665,
                    4.0,
                    5.25,
                    6.666666666666667,
                    8.25,
                    10.0,
                ]
            ),
        )
        single_test(
            SArray([0.1, 1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1]),
            SArray(
                [
                    0.0,
                    0.25000000000000006,
                    0.6666666666666666,
                    1.25,
                    1.9999999999999996,
                    2.916666666666666,
                    3.999999999999999,
                    5.249999999999998,
                ]
            ),
        )
        single_test(
            SArray([None, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
            SArray(
                [
                    None,
                    0.0,
                    0.25,
                    0.6666666666666666,
                    1.25,
                    2.0,
                    2.9166666666666665,
                    4.0,
                    5.25,
                    6.666666666666667,
                    8.25,
                    10.0,
                ]
            ),
        )
        single_test(
            SArray([None, 1, None, 3, None, 5]),
            SArray([None, 0.0, 0.0, 1.0, 1.0, 2.6666666666666665]),
        )

    def test_numpy_datetime64(self):
        # Make all datetimes naive
        expected = [
            i.replace(tzinfo=GMT(0.0)) if i is not None and i.tzinfo is None else i
            for i in self.datetime_data
        ]

        # A regular list
        iso_str_list = [
            np.datetime64("2013-05-07T10:04:10Z"),
            np.datetime64("1902-10-21T10:34:10Z"),
            None,
        ]
        sa = SArray(iso_str_list)
        self.__test_equal(sa, expected, dt.datetime)

        iso_str_list[2] = np.datetime64("NaT")
        sa = SArray(iso_str_list)
        self.__test_equal(sa, expected, dt.datetime)

        # A numpy array
        np_ary = np.array(iso_str_list)
        sa = SArray(np_ary)
        self.__test_equal(sa, expected, dt.datetime)

        ### Every possible type of datetime64
        test_str = "1969-12-31T23:59:56Z"
        available_time_units = ["h", "m", "s", "ms", "us", "ns", "ps", "fs", "as"]
        expected = [
            dt.datetime(1969, 12, 31, 23, 59, 56, tzinfo=GMT(0.0)) for i in range(7)
        ]
        expected.insert(0, dt.datetime(1969, 12, 31, 23, 59, 0, tzinfo=GMT(0.0)))
        expected.insert(0, dt.datetime(1969, 12, 31, 23, 0, 0, tzinfo=GMT(0.0)))
        for i in range(len(available_time_units)):
            sa = SArray([np.datetime64(test_str, available_time_units[i])])
            self.__test_equal(sa, [expected[i]], dt.datetime)

        test_str = "1908-06-01"
        available_date_units = ["Y", "M", "W", "D"]
        expected = [dt.datetime(1908, 6, 1, 0, 0, 0, tzinfo=GMT(0.0)) for i in range(4)]
        expected[2] = dt.datetime(
            1908, 5, 28, 0, 0, 0, tzinfo=GMT(0.0)
        )  # weeks start on Thursday?
        expected[0] = dt.datetime(1908, 1, 1, 0, 0, 0, tzinfo=GMT(0.0))
        for i in range(len(available_date_units)):
            sa = SArray([np.datetime64(test_str, available_date_units[i])])
            self.__test_equal(sa, [expected[i]], dt.datetime)

        # Daylight savings time (Just to be safe. datetime64 deals in UTC, and
        # we store times in UTC by default, so this shouldn't affect anything)
        sa = SArray([np.datetime64("2015-03-08T02:38:00-08")])
        expected = [dt.datetime(2015, 3, 8, 10, 38, tzinfo=GMT(0.0))]
        self.__test_equal(sa, expected, dt.datetime)

        # timezone considerations
        sa = SArray([np.datetime64("2016-01-01T05:45:00+0545")])
        expected = [dt.datetime(2016, 1, 1, 0, 0, 0, tzinfo=GMT(0.0))]
        self.__test_equal(sa, expected, dt.datetime)

        ### Out of our datetime range
        with self.assertRaises(TypeError):
            sa = SArray([np.datetime64("1066-10-14T09:00:00Z")])

    def test_pandas_timestamp(self):
        iso_str_list = [
            pd.Timestamp("2013-05-07T10:04:10"),
            pd.Timestamp("1902-10-21T10:34:10Z"),
            None,
        ]
        sa = SArray(iso_str_list)
        self.__test_equal(sa, self.datetime_data, dt.datetime)

        iso_str_list[2] = pd.NaT
        sa = SArray(iso_str_list)
        self.__test_equal(sa, self.datetime_data, dt.datetime)

        sa = SArray([pd.Timestamp("2015-03-08T02:38:00-08")])
        expected = [dt.datetime(2015, 3, 8, 2, 38, tzinfo=GMT(-8.0))]
        self.__test_equal(sa, expected, dt.datetime)

        sa = SArray([pd.Timestamp("2016-01-01 05:45:00", tz=GMT(5.75))])
        expected = [dt.datetime(2016, 1, 1, 5, 45, 0, tzinfo=GMT(5.75))]
        self.__test_equal(sa, expected, dt.datetime)

    def test_decimal(self):
        import decimal

        test_val = decimal.Decimal(3.0)
        sa = SArray([test_val])
        expected = [3.0]
        self.__test_equal(sa, expected, float)

    def test_timedelta(self):
        test_val = dt.timedelta(1, 1)
        sa = SArray([test_val])
        expected = [86401.0]
        self.__test_equal(sa, expected, float)

    def test_materialize(self):
        sa = SArray(range(100))
        sa = sa[sa > 10]
        self.assertFalse(sa.is_materialized())
        sa.materialize()
        self.assertTrue(sa.is_materialized())

    def test_ternary(self):
        lista = range(1000)
        a = SArray(lista)

        # identity
        self.__test_equal(SArray.where(a > 10, a, a), lista, int)

        # clip lower
        self.__test_equal(
            SArray.where(a > 10, a, 10), [i if i > 10 else 10 for i in lista], int
        )

        # clip upper
        self.__test_equal(
            SArray.where(a > 10, 10, a), [10 if i > 10 else i for i in lista], int
        )

        # constants
        self.__test_equal(
            SArray.where(a > 10, 10, 9), [10 if i > 10 else 9 for i in lista], int
        )

        # constant float
        self.__test_equal(
            SArray.where(a > 10, 10.0, 9.0),
            [10.0 if i > 10 else 9.0 for i in lista],
            float,
        )

        # constant str
        self.__test_equal(
            SArray.where(a > 10, "10", "9"),
            ["10" if i > 10 else "9" for i in lista],
            str,
        )

        # inconsistent types
        with self.assertRaises(TypeError):
            SArray.where(a > 10, 10, "9")  # 10 and "9" different types

        # inconsistent types
        with self.assertRaises(TypeError):
            SArray.where(a > 10, a, "9")  # expecting an integer for "a"

        # technically different types but type coercion happened
        self.__test_equal(
            SArray.where(a > 10, a, 10.0), [i if i > 10 else 10 for i in lista], int
        )

        # list types
        self.__test_equal(
            SArray.where(a > 10, [], [1], list),
            [[] if i > 10 else [1] for i in lista],
            list,
        )

        # really the same as the above, but using an SArray in place
        # of a constant in istrue. And hoping the type coercion
        # will take care of [1]
        b = SArray([[] for i in range(1000)])
        self.__test_equal(
            SArray.where(a > 10, b, [1]), [[] if i > 10 else [1] for i in lista], list
        )

    def test_shape(self):
        sa = SArray()
        self.assertEqual(sa.shape, (0,))
        for i in [0, 1, 2, 10, 345]:
            sa = SArray(range(i))
            self.assertEqual(sa.shape, (i,))

    def test_random_split(self):
        sa = SArray(range(10))
        (train, test) = sa.random_split(0.8, seed=12423)
        self.assertEqual(list(train), [0, 1, 2, 3, 5, 7, 8, 9])
        self.assertEqual(list(test), [4, 6])

    def test_copy(self):
        from copy import copy

        sa = SArray(range(1000))
        sa_copy = copy(sa)

        assert sa is not sa_copy

        assert (sa == sa_copy).all()

    def test_deepcopy(self):
        from copy import deepcopy

        sa = SArray(range(1000))
        sa_copy = deepcopy(sa)

        assert sa is not sa_copy

        assert (sa == sa_copy).all()

    def test_value_counts(self):
        sa = SArray([1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3])
        c = sa.value_counts()
        self.assertEqual(c.column_names(), ["value", "count"])
        self.__test_equal(c["value"], [3, 2, 1], int)
        self.__test_equal(c["count"], [7, 4, 2], int)
        sa = SArray()
        c = sa.value_counts()
        self.assertEqual(len(c), 0)

    def test_ndarray_shape(self):
        a1 = np.array([[1, 2, 3, 4], [5, 6, 7, 8]], "d")
        a2 = a1.reshape(4, 2)
        a3 = a1.transpose()
        a4 = a3.reshape(2, 4)

        b1 = a1[:2, :2]
        b2 = a2[:2, :2]
        b3 = a3[:2, :2]
        b4 = a4[:2, :2]

        c1 = b1.transpose()
        c2 = b2.transpose()
        c3 = b3.transpose()
        c4 = b4.transpose()

        d1 = a1[:2, 2:4]
        d2 = a2[2:4, :2]
        d3 = a3[2:4, :2]
        d4 = a4[:2, 2:4]

        originals = [a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4]
        sa = SArray(originals)
        l = list(sa)

        # check roundtriping of SArray ndarray type
        for i in range(len(l)):
            self.assertTrue(np.array_equal(l[i], originals[i]))

        # check roundtriping again because the ndarray type SArray
        # returned is slightly odd (it uses a custom bufferprotocol
        # to share memory with C++)
        sb = SArray(l)
        l2 = list(sb)
        for i in range(len(l)):
            self.assertTrue(np.array_equal(l[i], l2[i]))

        # test slicing
        slice_true = [x[1:] for x in originals]

        slice_test = list(sa.apply(lambda x: x[1:]))
        for i in range(len(l)):
            self.assertTrue(np.array_equal(slice_test[i], slice_true[i]))
        # test slice round tripping
        # test SArray(slice_true)

    def test_ndarray_ops(self):
        a1 = np.array([[1, 2, 3, 4], [5, 6, 7, 8]], "d")
        a2 = a1.reshape(4, 2)
        sa = SArray([a1, a2])

        b1 = np.array([[2, 1, 4, 3], [6, 5, 8, 7]], "d")
        b2 = a1.reshape(4, 2)
        sb = SArray([b1, b2])

        res = sa + sb
        self.assertTrue(np.array_equal(res[0], a1 + b1))
        self.assertTrue(np.array_equal(res[1], a2 + b2))

        res = sa + 1
        self.assertTrue(np.array_equal(res[0], a1 + 1))
        self.assertTrue(np.array_equal(res[1], a2 + 1))

        res = 1 + sa
        self.assertTrue(np.array_equal(res[0], 1 + a1))
        self.assertTrue(np.array_equal(res[1], 1 + a2))

        res = sa - sb
        self.assertTrue(np.array_equal(res[0], a1 - b1))
        self.assertTrue(np.array_equal(res[1], a2 - b2))

        res = sa - 1
        self.assertTrue(np.array_equal(res[0], a1 - 1))
        self.assertTrue(np.array_equal(res[1], a2 - 1))

        res = 1 - sa
        self.assertTrue(np.array_equal(res[0], 1 - a1))
        self.assertTrue(np.array_equal(res[1], 1 - a2))

        res = sa * sb
        self.assertTrue(np.array_equal(res[0], a1 * b1))
        self.assertTrue(np.array_equal(res[1], a2 * b2))

        res = sa * 2
        self.assertTrue(np.array_equal(res[0], a1 * 2))
        self.assertTrue(np.array_equal(res[1], a2 * 2))

        res = 2 * sa
        self.assertTrue(np.array_equal(res[0], 2 * a1))
        self.assertTrue(np.array_equal(res[1], 2 * a2))

        res = sa / sb
        self.assertTrue(np.array_equal(res[0], a1 / b1))
        self.assertTrue(np.array_equal(res[1], a2 / b2))

        res = sa / 2
        self.assertTrue(np.array_equal(res[0], a1 / 2.0))
        self.assertTrue(np.array_equal(res[1], a2 / 2.0))

        res = sa / 2.0
        self.assertTrue(np.array_equal(res[0], a1 / 2.0))
        self.assertTrue(np.array_equal(res[1], a2 / 2.0))

        res = 2.0 / sa
        self.assertTrue(np.array_equal(res[0], 2.0 / a1))
        self.assertTrue(np.array_equal(res[1], 2.0 / a2))

        # misshappen
        with self.assertRaises(RuntimeError):
            res.sum()

        self.assertTrue(np.array_equal(SArray([a1, b1]).sum(), a1 + b1))

    def test_type_casting(self):
        x = SFrame({"a": [[1, 2], None, [3, 4], None]})
        x["a"] = SArray(x["a"], list)
        self.assertTrue(x["a"].dtype == list)

    def test_filter_by(self):

        # integer example
        x = SArray([1, 2, 3, 4, 5, 6, 7])
        self.assertEqual(sorted(x.filter_by([11, 7, 2, 8, 4])), [2, 4, 7])
        self.assertEqual(
            sorted(x.filter_by([11, 7, 2, 8, 4, 3], exclude=True)), [1, 5, 6]
        )

        # empty SArray
        self.assertEqual(sorted(x.filter_by([77, 22, 18, 42])), [])
        self.assertEqual(sorted(x.filter_by([77, 22, 18, 42], exclude=True)), list(x))

        # duplicates
        self.assertEqual(sorted(x.filter_by([2, 2, 3, 44])), [2, 3])
        x = SArray([1, 2, 2, 3, 4, 5, 6, 7])
        self.assertEqual(sorted(x.filter_by([2, 2, 3, 44])), [2, 2, 3])

        # strings
        x = SArray(["dog", "cat", "cow", "horse"])
        self.assertEqual(
            sorted(x.filter_by(["cat", "hamster", "dog", "fish", "bird", "snake"])),
            ["cat", "dog"],
        )
        self.assertEqual(
            sorted(
                x.filter_by(
                    ["cat", "hamster", "dog", "fish", "bird", "snake"], exclude=True
                )
            ),
            ["cow", "horse"],
        )
        self.assertEqual(sorted(x.filter_by("dog")), ["dog"])

    def test_abs(self):
        sa = SArray([-1.10, 2, -3.33, 4])
        result = sa.abs()
        expected = [1.1, 2.0, 3.33, 4.0]
        self.__test_equal(result, expected, float)

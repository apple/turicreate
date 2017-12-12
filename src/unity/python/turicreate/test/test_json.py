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

import array
import datetime
import json # Python built-in JSON module
import math
import os
import pytz
import sys
import unittest

from . import util
from .. import _json # turicreate._json

if sys.version_info.major == 3:
    long = int

class image_info:
    def __init__(self, url):
        self.url = url
        if 'png' in url:
            self.format = 'PNG'
        elif 'jpg' in url:
            self.format = 'JPG'
        if 'grey' in url:
            self.channels = 1
        else:
            self.channels = 3

current_file_dir = os.path.dirname(os.path.realpath(__file__))
image_urls = [current_file_dir + x for x in [
    '/images/nested/sample_grey.jpg',
    '/images/nested/sample_grey.png',
    '/images/sample.jpg',
    '/images/sample.png'
]]
image_info = [image_info(u) for u in image_urls]

_SFrameComparer = util.SFrameComparer()

class JSONTest(unittest.TestCase):
    def _assertEqual(self, x, y):
        from ..data_structures.sarray import SArray
        from ..data_structures.sframe import SFrame
        from ..data_structures.sgraph import SGraph
        if type(x) in [long,int]:
            self.assertTrue(type(y) in [long,int])
        else:
            self.assertEqual(type(x), type(y))
        if isinstance(x, SArray):
            _SFrameComparer._assert_sarray_equal(x, y)
        elif isinstance(x, SFrame):
            _SFrameComparer._assert_sframe_equal(x, y)
        elif isinstance(x, SGraph):
            _SFrameComparer._assert_sgraph_equal(x, y)
        elif isinstance(x, dict):
            for (k1,v1),(k2,v2) in zip(sorted(x.items()), sorted(y.items())):
                self._assertEqual(k1, k2)
                self._assertEqual(v1, v2)
        elif isinstance(x, list):
            for v1,v2 in zip(x, y):
                self._assertEqual(v1, v2)
        else:
            self.assertEqual(x, y)

    def _run_test_case(self, value):
        # test that JSON serialization is invertible with respect to both
        # value and type.
        (data, schema) = _json.to_serializable(value)

        # ensure that resulting value is actually naively serializable
        data = json.loads(json.dumps(data, allow_nan=False))
        schema = json.loads(json.dumps(schema, allow_nan=False))

        #print("----------------------------------")
        #print("Value: %s" % value)
        #print("Serializable Data: %s" % data)
        #print("Serializable Schema: %s" % schema)
        result = _json.from_serializable(data, schema)
        #print("Deserialized Result: %s" % result)
        #print("----------------------------------")
        self._assertEqual(result, value)
        # test that JSON serialization gives expected result
        serialized = _json.dumps(value)
        deserialized = _json.loads(serialized)
        self._assertEqual(deserialized, value)

    @unittest.skipIf(sys.platform == 'win32', "Windows long issue")
    def test_int(self):
        [self._run_test_case(value) for value in [
            0,
            1,
            -2147483650,
            -2147483649, # boundary of accurate representation in JS 64-bit float
            2147483648, # boundary of accurate representation in JS 64-bit float
            2147483649,
        ]]

    def test_float(self):
        [self._run_test_case(value) for value in [
            -1.1,
            -1.0,
            0.0,
            1.0,
            1.1,
            float('-inf'),
            float('inf'),
        ]]
        self.assertTrue(
            math.isnan(
                _json.from_serializable(*_json.to_serializable(float('nan')))))

    def test_string_to_json(self):
        [self._run_test_case(value) for value in [
            "hello",
            "a'b",
            "a\"b",
            "ɖɞɫɷ",
        ]]

    def test_vec_to_json(self):
        [self._run_test_case(value) for value in [
            array.array('d'),
            array.array('d', [1.5]),
            array.array('d', [2.1,2.5,3.1]),
            array.array('d', [float('-inf'), float('inf')]),
        ]]

    def test_list_to_json(self):
        # TODO -- we can't test lists of numbers, due to
        # Python<->flexible_type not being reversible for lists of numbers.
        # if `list` of `int` goes into C++, the flexible_type representation
        # becomes flex_vec (vector<flex_float>). This is a lossy representation.
        # known issue, can't resolve here.
        [self._run_test_case(value) for value in [
            [],
            ["hello", "world"],
            ["hello", 3, None],
            [3.14159, None],
            [{}, {'x': 1, 'y': 2}],
            ["hello", float('-inf'), float('inf')],
        ]]

    def test_dict_to_json(self):
        [self._run_test_case(value) for value in [
            {},
            {
                "x": 1,
                "y": 2
            },
        ]]

    def test_date_time_to_json(self):
        d = datetime.datetime(year=2016, month=3, day=5)
        [self._run_test_case(value) for value in [
            d,
            pytz.utc.localize(d),
            pytz.timezone('US/Arizona').localize(d),
        ]]

    def test_image_to_json(self):
        from .. import Image
        [self._run_test_case(value) for value in [
            Image(path=item.url, format=item.format) for item in image_info
        ]]

    def test_sarray_to_json(self):
        from ..data_structures.sarray import SArray
        from ..data_structures.sframe import SFrame
        from .. import Image
        d = datetime.datetime(year=2016, month=3, day=5)
        [self._run_test_case(value) for value in [
            SArray(),
            SArray([1,2,3]),
            SArray([1.0,2.0,3.0]),
            SArray([None, 3, None]),
            SArray(["hello", "world"]),
            SArray(array.array('d', [2.1,2.5,3.1])),
            SArray([
                ["hello", None, "world"],
                ["hello", 3, None],
                [3.14159, None],
            ]),
            SArray([
                {
                    "x": 1,
                    "y": 2
                }, {
                    "x": 5,
                    "z": 3
                },
            ]),
            SArray([
                d,
                pytz.utc.localize(d),
                pytz.timezone('US/Arizona').localize(d),
            ]),
            SArray([
                Image(path=item.url, format=item.format) for item in image_info
            ]),
        ]]

    def test_sframe_to_json(self):
        from ..data_structures.sframe import SFrame
        [self._run_test_case(value) for value in [
            SFrame(),
            SFrame({'foo': [1,2,3,4], 'bar': [None, "Hello", None, "World"]}),
        ]]

    def test_sgraph_to_json(self):
        from ..data_structures.sframe import SFrame
        from ..data_structures.sgraph import SGraph, Vertex, Edge

        sg = SGraph()
        self._run_test_case(sg)

        sg = sg.add_vertices([Vertex(x) for x in [1,2,3,4]])
        sg = sg.add_edges([Edge(x, x+1) for x in [1,2,3]])
        self._run_test_case(sg)

    def test_nested_to_json(self):
        # not tested in the cases above: nested data, nested schema
        # (but all flexible_type compatible)
        [self._run_test_case(value) for value in [
            {'foo': ['a','b','c'], 'bar': array.array('d', [0.0, float('inf'), float('-inf')])},
            [['a','b','c'], array.array('d', [0.0, float('inf'), float('-inf')])],
            {
                'baz': {'foo': ['a','b','c'], 'bar': array.array('d', [0.0, float('inf'), float('-inf')])},
                'qux': [['a','b','c'], array.array('d', [0.0, float('inf'), float('-inf')])],
            }
        ]]

    def test_variant_to_json(self):
        # not tested in the cases above: variant_type other than SFrame-like
        # but containing SFrame-like (so cannot be a flexible_type)
        from ..data_structures.sarray import SArray
        from ..data_structures.sframe import SFrame
        sf = SFrame({'col1': [1,2], 'col2': ['hello','world']})
        sa = SArray([5.0,6.0,7.0])
        [self._run_test_case(value) for value in [
            {'foo': sf, 'bar': sa},
            [sf, sa],
        ]]

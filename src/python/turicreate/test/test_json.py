# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
# This software may be modified and distributed under the terms
# of the BSD license. See the LICENSE file for details.

# This file tests invertibility (serializing to/from) the "serializable" format
# of variant_type (produced by extensions.json). This extension results in a
# naively-JSON-serializable flexible_type that should retain all necessary
# information to be rehydrated into the original variant_type.

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import array
import datetime
import hypothesis
import json  # Python built-in JSON module
import math
import os
import pandas
import pytest
import pytz
import six
import string
import sys
import unittest
import tempfile

from . import util
from .. import _json  # turicreate._json
from ..data_structures.sarray import SArray
from ..data_structures.sframe import SFrame
from ..data_structures.sgraph import SGraph, Vertex, Edge

if sys.version_info.major == 3:
    long = int


class image_info:
    def __init__(self, url):
        self.url = url
        if "png" in url:
            self.format = "PNG"
        elif "jpg" in url:
            self.format = "JPG"
        if "grey" in url:
            self.channels = 1
        else:
            self.channels = 3


current_file_dir = os.path.dirname(os.path.realpath(__file__))
image_urls = [
    current_file_dir + x
    for x in [
        "/images/nested/sample_grey.jpg",
        "/images/nested/sample_grey.png",
        "/images/sample.jpg",
        "/images/sample.png",
    ]
]
image_info = [image_info(u) for u in image_urls]

_SFrameComparer = util.SFrameComparer()


class JSONTest(unittest.TestCase):
    # Only generate lists of dicts, but allow nearly-arbitrary JSON inside those.
    # However, limit to length 1 to make sure the keys are the same in all rows.

    # Known bug #1: escaped chars give different behavior in SFrame JSON parsing
    # vs Python's built-in JSON module. Not sure which is correct:
    """
    Original JSON:  [{"": [{"\f": 0}]}]
    Expected:  +---------------+
    |       X1      |
    +---------------+
    | [{'\x0c': 0}] |
    +---------------+
    [1 rows x 1 columns]

    Actual:  +--------------+
    |      X1      |
    +--------------+
    | [{'\\f': 0}] |
    +--------------+
    [1 rows x 1 columns]
    """
    # In the meantime, let's use `string.ascii_letters + string.digits` instead of
    # `string.printable` (which contains the problematic characters).
    hypothesis_json = hypothesis.strategies.lists(
        hypothesis.strategies.dictionaries(
            keys=hypothesis.strategies.text(string.ascii_letters + string.digits),
            values=hypothesis.strategies.recursive(
                # Known bug #2: [{"": null}] parses as "null" in SFrame, and should be None
                # Once this is fixed, uncomment the line below.
                # hypothesis.strategies.none() |
                # Known bug #3: [{"": false}] parses as "false" in SFrame, and should be 0
                # Once this is fixed, uncomment the line below.
                # hypothesis.strategies.booleans() |
                hypothesis.strategies.integers(
                    min_value=-(2 ** 53) + 1, max_value=(2 ** 53) - 1
                )
                | hypothesis.strategies.floats()
                | hypothesis.strategies.text(string.ascii_letters + string.digits),
                lambda children: hypothesis.strategies.lists(children, 1)
                | hypothesis.strategies.dictionaries(
                    hypothesis.strategies.text(string.ascii_letters + string.digits),
                    children,
                    min_size=1,
                ),
            ),
            min_size=1,
            max_size=1,
        ),
        min_size=1,
        max_size=1,
    )

    def _assertEqual(self, x, y):
        if type(x) in [long, int]:
            self.assertTrue(type(y) in [long, int])
        elif isinstance(x, six.string_types):
            self.assertTrue(isinstance(y, six.string_types))
        else:
            self.assertEqual(type(x), type(y))
        if isinstance(x, six.string_types):
            self.assertEqual(str(x), str(y))
        elif isinstance(x, SArray):
            _SFrameComparer._assert_sarray_equal(x, y)
        elif isinstance(x, SFrame):
            _SFrameComparer._assert_sframe_equal(x, y)
        elif isinstance(x, SGraph):
            _SFrameComparer._assert_sgraph_equal(x, y)
        elif isinstance(x, dict):
            for (k1, v1), (k2, v2) in zip(sorted(x.items()), sorted(y.items())):
                self._assertEqual(k1, k2)
                self._assertEqual(v1, v2)
        elif isinstance(x, list):
            for v1, v2 in zip(x, y):
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

        # print("----------------------------------")
        # print("Value: %s" % value)
        # print("Serializable Data: %s" % data)
        # print("Serializable Schema: %s" % schema)
        result = _json.from_serializable(data, schema)
        # print("Deserialized Result: %s" % result)
        # print("----------------------------------")
        self._assertEqual(result, value)
        # test that JSON serialization gives expected result
        serialized = _json.dumps(value)
        deserialized = _json.loads(serialized)
        self._assertEqual(deserialized, value)

    @unittest.skipIf(sys.platform == "win32", "Windows long issue")
    def test_int(self):
        [
            self._run_test_case(value)
            for value in [
                0,
                1,
                -2147483650,
                -2147483649,  # boundary of accurate representation in JS 64-bit float
                2147483648,  # boundary of accurate representation in JS 64-bit float
                2147483649,
            ]
        ]

    def test_float(self):
        [
            self._run_test_case(value)
            for value in [-1.1, -1.0, 0.0, 1.0, 1.1, float("-inf"), float("inf"),]
        ]
        self.assertTrue(
            math.isnan(_json.from_serializable(*_json.to_serializable(float("nan"))))
        )

    def test_string_to_json(self):
        [self._run_test_case(value) for value in ["hello", "a'b", 'a"b', "ɖɞɫɷ",]]

    def test_vec_to_json(self):
        [
            self._run_test_case(value)
            for value in [
                array.array("d"),
                array.array("d", [1.5]),
                array.array("d", [2.1, 2.5, 3.1]),
                array.array("d", [float("-inf"), float("inf")]),
            ]
        ]

    def test_list_to_json(self):
        # TODO -- we can't test lists of numbers, due to
        # Python<->flexible_type not being reversible for lists of numbers.
        # if `list` of `int` goes into C++, the flexible_type representation
        # becomes flex_vec (vector<flex_float>). This is a lossy representation.
        # known issue, can't resolve here.
        [
            self._run_test_case(value)
            for value in [
                [],
                ["hello", "world"],
                ["hello", 3, None],
                [3.14159, None],
                [{}, {"x": 1, "y": 2}],
                ["hello", float("-inf"), float("inf")],
            ]
        ]

    def test_dict_to_json(self):
        [self._run_test_case(value) for value in [{}, {"x": 1, "y": 2},]]

    def test_date_time_to_json(self):
        d = datetime.datetime(year=2016, month=3, day=5)
        [
            self._run_test_case(value)
            for value in [
                d,
                pytz.utc.localize(d),
                pytz.timezone("US/Arizona").localize(d),
            ]
        ]

    def test_image_to_json(self):
        from .. import Image

        [
            self._run_test_case(value)
            for value in [
                Image(path=item.url, format=item.format) for item in image_info
            ]
        ]

    def test_sarray_to_json(self):
        from .. import Image

        d = datetime.datetime(year=2016, month=3, day=5)
        [
            self._run_test_case(value)
            for value in [
                SArray(),
                SArray([1, 2, 3]),
                SArray([1.0, 2.0, 3.0]),
                SArray([None, 3, None]),
                SArray(["hello", "world"]),
                SArray(array.array("d", [2.1, 2.5, 3.1])),
                SArray(
                    [["hello", None, "world"], ["hello", 3, None], [3.14159, None],]
                ),
                SArray([{"x": 1, "y": 2}, {"x": 5, "z": 3},]),
                SArray(
                    [d, pytz.utc.localize(d), pytz.timezone("US/Arizona").localize(d),]
                ),
                SArray(
                    [Image(path=item.url, format=item.format) for item in image_info]
                ),
            ]
        ]

    def test_sframe_to_json(self):
        [
            self._run_test_case(value)
            for value in [
                SFrame(),
                SFrame({"foo": [1, 2, 3, 4], "bar": [None, "Hello", None, "World"]}),
            ]
        ]

    def test_sgraph_to_json(self):
        sg = SGraph()
        self._run_test_case(sg)

        sg = sg.add_vertices([Vertex(x) for x in [1, 2, 3, 4]])
        sg = sg.add_edges([Edge(x, x + 1) for x in [1, 2, 3]])
        self._run_test_case(sg)

    def test_nested_to_json(self):
        # not tested in the cases above: nested data, nested schema
        # (but all flexible_type compatible)
        [
            self._run_test_case(value)
            for value in [
                {
                    "foo": ["a", "b", "c"],
                    "bar": array.array("d", [0.0, float("inf"), float("-inf")]),
                },
                [["a", "b", "c"], array.array("d", [0.0, float("inf"), float("-inf")])],
                {
                    "baz": {
                        "foo": ["a", "b", "c"],
                        "bar": array.array("d", [0.0, float("inf"), float("-inf")]),
                    },
                    "qux": [
                        ["a", "b", "c"],
                        array.array("d", [0.0, float("inf"), float("-inf")]),
                    ],
                },
            ]
        ]

    def test_variant_to_json(self):
        # not tested in the cases above: variant_type other than SFrame-like
        # but containing SFrame-like (so cannot be a flexible_type)
        sf = SFrame({"col1": [1, 2], "col2": ["hello", "world"]})
        sa = SArray([5.0, 6.0, 7.0])
        [self._run_test_case(value) for value in [{"foo": sf, "bar": sa}, [sf, sa],]]

    def test_malformed_json(self):
        out = """
[
  {
  "text": "["I", "have", "an", "atlas"]",
  "label": ["NONE", "NONE", "NONE", "NONE"]
  },
  {
  "text": ["These", "are", "my", "dogs"],
  "label": ["NONE", "NONE", "NONE", "PLN"]
  },
  {
  "text": ["The", "sheep", "are", "fluffy"],
  "label": ["NONE","PLN","NONE","NONE"]
  },
  {
  "text": ["Billiards", "is", "my", "favourite", "game"],
  "label": ["NONE", "NONE", "NONE", "NONE", "NONE"]
  },
  {
  "text": ["I", "went", "to", "five", "sessions", "today"],
  "label": ["NONE", "NONE", "NONE", "NONE", "PLN", "NONE"]
  }
 ]
 """
        with tempfile.NamedTemporaryFile("w") as f:
            f.write(out)
            f.flush()

            self.assertRaises(RuntimeError, SArray.read_json, f.name)
            self.assertRaises(RuntimeError, SFrame.read_json, f.name)

    def test_nonexistant_json(self):
        self.assertRaises(IOError, SArray.read_json, "/nonexistant.json")
        self.assertRaises(IOError, SFrame.read_json, "/nonexistant.json")

    def test_strange_128_char_corner_case(self):
        json_text = """
{"foo":[{"bar":"Lorem ipsum dolor sit amet, consectetur adipiscing elit. In eget odio velit. Suspendisse potenti. Vivamus a urna feugiat nullam."}]}
"""
        with tempfile.NamedTemporaryFile("w") as f:
            f.write(json_text)
            f.flush()

            df = pandas.read_json(f.name, lines=True)
            sf_actual = SFrame.read_json(f.name, orient="lines")
            sf_expected = SFrame(df)
            _SFrameComparer._assert_sframe_equal(sf_expected, sf_actual)

    @pytest.mark.xfail(
        reason="Non-deterministic test failure tracked in https://github.com/apple/turicreate/issues/2934"
    )
    # deterministic across runs, and may take a while
    @hypothesis.settings(
        derandomize=True, suppress_health_check=[hypothesis.HealthCheck.too_slow]
    )
    @hypothesis.given(hypothesis_json)
    def test_arbitrary_json(self, json_obj):
        # Known bug #1: escaped chars give different behavior in SFrame JSON parsing
        # vs Python's built-in JSON module. Not sure which is correct.
        # Workaround captured in definition of `hypothesis_json`

        # Known bug #2: [{"": null}] parses as "null" in SFrame, and should be None
        # Workaround captured in definition of `hypothesis_json`

        # Known bug #3: [{"": false}] parses as "false" in SFrame, and should be 0
        # Workaround captured in definition of `hypothesis_json`

        try:
            json_text = json.dumps(json_obj, allow_nan=False)
        except:
            # not actually valid JSON - skip this example
            return

        try:
            expected = SFrame(json_obj).unpack("X1", column_name_prefix="")
        except TypeError:
            # something like TypeError: A common type cannot be infered from types integer, string.
            # TC enforces all list items have the same type, which
            # JSON does not necessarily enforce. Let's skip those examples.
            return
        with tempfile.NamedTemporaryFile("w") as f:
            f.write(json_text)
            f.flush()

            try:
                actual = SFrame.read_json(f.name)
            except TypeError:
                # something like TypeError: A common type cannot be infered from types integer, string.
                # TC enforces all list items have the same type, which
                # JSON does not necessarily enforce. Let's skip those examples.
                return

            _SFrameComparer._assert_sframe_equal(expected, actual)

    def test_true_false_substitutions(self):
        expecteda = [["a", "b", "c"], ["a", "b", "c"]]
        expectedb = [["d", "false", "e", 0, "true", 1, "a"], ["d", "e", "f"]]

        records_json_file = """
[{"a" : ["a", "b", "c"],
  "b" : ["d", "false", "e", false, "true", true, "a"]},
 {"a" : ["a", "b", "c"],
  "b" : ["d", "e", "f"]}]
"""
        lines_json_file = """
{"a" : ["a", "b", "c"], "b" : ["d", "false", "e", false, "true", true, "a"]}
{"a" : ["a", "b", "c"], "b" : ["d", "e", "f"]}
"""

        with tempfile.NamedTemporaryFile("w") as f:
            f.write(records_json_file)
            f.flush()
            records = SFrame.read_json(f.name, orient="records")
        self.assertEqual(list(records["a"]), expecteda)
        self.assertEqual(list(records["b"]), expectedb)

        with tempfile.NamedTemporaryFile("w") as f:
            f.write(lines_json_file)
            f.flush()
            lines = SFrame.read_json(f.name, orient="lines")

        self.assertEqual(list(lines["a"]), expecteda)
        self.assertEqual(list(lines["b"]), expectedb)

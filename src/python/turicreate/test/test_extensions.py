# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
from ..data_structures.sarray import SArray
from ..data_structures.sframe import SFrame

import sys

if sys.version_info.major > 2:
    long = int
import random

from .._cython.cy_variant import _debug_is_flexible_type_encoded


class VariantCheckTest(unittest.TestCase):
    def identical(self, reference, b):
        if type(reference) in [int, long]:
            self.assertIn(type(b), [int, long])
        else:
            self.assertEqual(type(reference), type(b))
        if isinstance(reference, list):
            self.assertEqual(len(reference), len(b))
            for i in range(len(reference)):
                self.identical(reference[i], b[i])
        if isinstance(reference, dict):
            self.assertEqual(sorted(reference.keys()), sorted(b.keys()))
            for i in reference:
                self.identical(reference[i], b[i])
        if isinstance(reference, SArray):
            self.identical(list(reference), list(b))
        if isinstance(reference, SFrame):
            self.identical(list(reference), list(b))

    def variant_turnaround(self, reference, expected_result=None):
        if expected_result is None:
            expected_result = reference
        from ..extensions import _demo_identity

        self.identical(expected_result, _demo_identity(reference))

    def test_variant_check(self):
        sa = SArray([1, 2, 3, 4, 5])
        sf = SFrame({"a": sa})
        import array

        self.variant_turnaround(1)
        self.variant_turnaround(1.0)
        self.variant_turnaround(array.array("d", [1.0, 2.0, 3.0]))
        # numeric lists currently converts to array
        self.variant_turnaround([1, 2, 3], array.array("d", [1, 0.0, 2.0, 3.0]))
        self.variant_turnaround("abc")
        self.variant_turnaround(["abc", "def"])
        self.variant_turnaround({"a": 1, "b": "c"})
        self.variant_turnaround({"a": [1, 2, "d"], "b": ["a", "b", "c"]})
        # numeric lists currently converts to array
        self.variant_turnaround(
            {"a": [1, 2, 3], "b": ["a", "b", "c"]},
            {"a": array.array("d", [1, 2, 3]), "b": ["a", "b", "c"]},
        )
        self.variant_turnaround(sa)
        self.variant_turnaround(sf)
        self.variant_turnaround([sa, sf])
        self.variant_turnaround([sa, sa])
        self.variant_turnaround([sf, sf])
        self.variant_turnaround({"a": sa, "b": sf, "c": ["a", "b", "c", "d"]})
        self.variant_turnaround({"a": [{"a": 1, "b": 2}], "b": [{"a": 3}]})
        self.variant_turnaround({"a": [{"a": sa, "b": sa}], "b": [{"a": sa}]})
        self.variant_turnaround({"a": [sa, {"c": sa, "d": sa}], "e": [{"f": sa}]})
        self.variant_turnaround({"a": [sa, {"a": sa}]})
        self.variant_turnaround({"a": [{"a": sa, "b": "c"}]})
        self.variant_turnaround({"a": [sa, {"a": sa, "b": "c"}]})
        self.variant_turnaround(
            {"a": [sa, sf, {"a": sa, "b": "c"}], "b": sf, "c": ["a", "b", "c", "d"]}
        )

    def test_stress(self):

        random.seed(0)

        class A:
            pass

        A.flextype_encodable = True

        def _make(depth):

            if depth == 0:
                s = random.randint(0, 3)
            else:
                s = random.randint(0, 5)

            if s == 0:
                return str(random.randint(0, 100))
            elif s == 1:
                return random.randint(0, 100000)
            elif s == 2:
                A.flextype_encodable = False
                return SArray([random.randint(0, 100000) for i in range(2)])
            elif s == 3:
                A.flextype_encodable = False
                return SFrame(
                    {
                        "a": [random.randint(0, 100000) for i in range(2)],
                        "b": [str(random.randint(0, 100000)) for i in range(2)],
                    }
                )

            elif s == 4:
                length = random.randint(3, 8)
                # The ['a'] needed so it doesn't get translated to a string.
                return ["a"] + [_make(depth - 1) for i in range(length)]
            elif s == 5:
                length = random.randint(3, 8)
                return {
                    str(random.randint(0, 100)): _make(depth - 1) for i in range(length)
                }

        for depth in [2, 3, 4, 5, 10]:
            for i in range(10):

                A.flextype_encodable = True

                obj = _make(depth)

                # Test that it's losslessly encoded and decoded
                self.variant_turnaround(obj)

                # Test that if it can be
                if _debug_is_flexible_type_encoded(obj):
                    self.assertTrue(A.flextype_encodable)
                else:
                    self.assertFalse(A.flextype_encodable)

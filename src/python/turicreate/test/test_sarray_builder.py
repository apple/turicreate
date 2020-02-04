# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ..data_structures.sarray_builder import SArrayBuilder
import unittest
import array
import datetime as dt
from .._cython.cy_flexible_type import GMT


class SArrayBuilderTest(unittest.TestCase):
    def __test_equal(self, _sarray, _data, _type):
        self.assertEqual(_sarray.dtype, _type)
        self.assertEqual(len(_sarray), len(_data))
        self.assertSequenceEqual(list(_sarray.head(len(_sarray))), _data)

    def __test_append(self, sb, data, dtype):
        for i in data:
            sb.append(i)
        self.assertEqual(sb.get_type(), dtype)
        sa = sb.close()
        self.__test_equal(sa, data, dtype)

    def __test_append_multiple(self, sb, data, dtype):
        sb.append_multiple(data)
        self.assertEqual(sb.get_type(), dtype)
        sa = sb.close()
        self.__test_equal(sa, data, dtype)

    def test_basic(self):
        data_to_test = [
            ([1, -1, None, 2], int),
            ([i for i in range(20000)], int),
            ([None, 1.0, -1.0, 2.3], float),
            (["hi", None, "hello", "None"], str),
            (
                [
                    dt.datetime(2013, 5, 7, 10, 4, 10),
                    dt.datetime(1902, 10, 21, 10, 34, 10).replace(tzinfo=GMT(0.0)),
                    None,
                ],
                dt.datetime,
            ),
            ([["hi", 1], None, ["hi", 2, 3], ["hello"]], list),
            (
                [array.array("d", [1.0, 2.0]), array.array("d", [3.0, 4.0]), None],
                array.array,
            ),
            ([{"a": 1, "b": 2}, {"c": 3, "d": 4}, None], dict),
        ]
        for i in data_to_test:
            sb = SArrayBuilder(i[1])
            self.__test_append(sb, i[0], i[1])

            sb = SArrayBuilder(i[1])
            self.__test_append_multiple(sb, i[0], i[1])

    def test_history(self):
        sb = SArrayBuilder(int, history_size=10)
        sb.append_multiple((i for i in range(8)))
        hist = sb.read_history(3)
        self.assertEqual(hist, [5, 6, 7])

        hist = sb.read_history(20)
        self.assertEqual(hist, [i for i in range(8)])
        hist = sb.read_history()
        self.assertEqual(hist, [i for i in range(8)])

        sb.append_multiple((i for i in range(5)))
        hist = sb.read_history(10)
        self.assertEqual(hist, [3, 4, 5, 6, 7, 0, 1, 2, 3, 4])

        sb.append(50)
        hist = sb.read_history(10)
        self.assertEqual(hist, [4, 5, 6, 7, 0, 1, 2, 3, 4, 50])

        hist = sb.read_history(-1)
        self.assertEqual(hist, [])
        hist = sb.read_history(0)
        self.assertEqual(hist, [])

        sa = sb.close()
        self.__test_equal(sa, [i for i in range(8)] + [i for i in range(5)] + [50], int)

    def test_segments(self):
        sb = SArrayBuilder(int, num_segments=4)

        sb.append_multiple((i for i in range(20, 30)), segment=2)
        sb.append_multiple((i for i in range(10, 20)), segment=1)
        sb.append_multiple((i for i in range(30, 40)), segment=3)
        sb.append_multiple((i for i in range(0, 10)), segment=0)

        hist = sb.read_history(3, segment=0)
        self.assertSequenceEqual(hist, [7, 8, 9])
        hist = sb.read_history(3, segment=1)
        self.assertSequenceEqual(hist, [17, 18, 19])
        hist = sb.read_history(3, segment=2)
        self.assertSequenceEqual(hist, [27, 28, 29])
        hist = sb.read_history(3, segment=3)
        self.assertSequenceEqual(hist, [37, 38, 39])

        with self.assertRaises(RuntimeError):
            sb.read_history(3, segment=99)

        sa = sb.close()
        self.__test_equal(sa, range(40), int)

# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as tc
import unittest
from pickle import PicklingError
import pickle
import turicreate.util._cloudpickle as cloudpickle


class CloudPickleTest(unittest.TestCase):
    def test_pickle_unity_object_exception(self):
        sa = tc.SArray()
        sf = tc.SFrame()
        g = tc.SGraph()
        sk = sa.summary()
        m = tc.pagerank.create(g)
        for obj in [sa, sf, g, sk, m]:
            self.assertRaises(PicklingError, lambda: cloudpickle.dumps(obj))

    def test_memoize_subclass(self):
        class A(object):
            def __init__(self):
                self.name = "A"

        class B(A):
            def __init__(self):
                super(B, self).__init__()
                self.name2 = "B"

        b = B()
        self.assertEqual(b.name, "A")
        self.assertEqual(b.name2, "B")

        b2 = pickle.loads(cloudpickle.dumps(b))
        self.assertEqual(b.name, b2.name)
        self.assertEqual(b.name2, b2.name2)

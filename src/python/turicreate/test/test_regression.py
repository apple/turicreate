# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import turicreate as tc
import numpy as np


class RegressionCreateTest(unittest.TestCase):
    """
    Unit test class for testing a regression model.
    """

    """
       Creation test helper function.
    """

    def _test_create(self, n, d, validation_set="auto"):

        # Simulate test data
        np.random.seed(42)
        sf = tc.SFrame()

        for i in range(d):
            sf.add_column(tc.SArray(np.random.rand(n)), inplace=True)

        target = np.random.rand(n)
        sf["target"] = target
        model = tc.regression.create(
            sf, target="target", features=None, validation_set=validation_set
        )
        self.assertTrue(model is not None)

        features = sf.column_names()
        features.remove("target")
        model = tc.regression.create(
            sf, target="target", features=features, validation_set=validation_set
        )
        self.assertTrue(model is not None)

    """
       Test create.
    """

    def test_create(self):

        self._test_create(99, 10)
        self._test_create(100, 100)
        self._test_create(20000, 10)
        self._test_create(99, 10, validation_set=None)
        self._test_create(100, 100, validation_set=None)
        self._test_create(20000, 10, validation_set=None)

# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..util import generate_random_sframe
from ..util import generate_random_regression_sframe
from ..util import generate_random_classification_sframe

import unittest
import array


class SFrameGeneration(unittest.TestCase):
    def test_data_types(self):
        column_codes = {
            "n": float,
            "N": float,
            "r": float,
            "R": float,
            "b": int,
            "z": int,
            "Z": int,
            "c": str,
            "C": str,
            "s": str,
            "S": str,
            "x": str,
            "X": str,
            "h": str,
            "H": str,
            "v": array.array,
            "V": array.array,
            "l": list,
            "L": list,
            "m": list,
            "M": list,
            "d": dict,
            "D": dict,
        }

        test_codes = "".join(column_codes.keys())
        X = generate_random_sframe(10, test_codes)
        column_names = X.column_names()

        for c, n in zip(test_codes, column_names):
            self.assertEqual(X[n].dtype, column_codes[c])

    def test_regression_result(self):

        for L in range(1, 10):
            X = generate_random_regression_sframe(100, "n" * L, target_noise_level=0)
            X["target_2"] = X.apply(
                lambda d: sum(v for k, v in d.items() if k != "target")
            )
            X["target_2"] = X["target_2"] - X["target_2"].min()
            X["target_2"] = X["target_2"] / X["target_2"].max()

            self.assertAlmostEqual((X["target_2"] - X["target"]).std(), 0, delta=0.001)

    def test_classification_result(self):

        for L in range(1, 10):
            X = generate_random_classification_sframe(
                100,
                "n" * L,
                misclassification_spread=0,
                num_classes=2,
                num_extra_class_bins=0,
            )
            X["target_2"] = X.apply(
                lambda d: sum(v for k, v in d.items() if k != "target")
            )
            X["target_2"] = X["target_2"] - X["target_2"].min()
            X["target_2"] = X["target_2"] / X["target_2"].max()

            x_1 = X["target_2"][X["target"] == 0]
            x_2 = X["target_2"][X["target"] == 1]

            self.assertTrue(
                (x_1.max() - 1e-4 <= x_2.min() + 1e-4)
                or (x_2.max() - 1e-4 <= x_1.min() + 1e-4)
            )

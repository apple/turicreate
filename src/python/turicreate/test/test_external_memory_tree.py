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
from array import array


def _get_data(n):
    t = [1] * (n - n // 2) + [0] * (n // 2)
    sf = tc.SFrame({"target": t})
    sf = sf.add_row_number()
    sf["id"] = sf["id"].apply(lambda x: {x: 1} if x != 0 else {i: 1 for i in range(n)})
    return sf


class TreeExtractFeaturesTest(unittest.TestCase):
    def _run_test(self, train_sf, test_sf, target="target"):

        for model in [
            tc.classifier.decision_tree_classifier,
            tc.classifier.random_forest_classifier,
            tc.classifier.boosted_trees_classifier,
        ]:
            m = model.create(train_sf, target=target, validation_set=None)
            for leaf in m.extract_features(test_sf)[-1]:
                self.assertTrue(leaf > 1e-5)  # leaf_id should not be zero.

    def test_multiple_cache_files_in_memory(self):
        N = 10000
        sf = _get_data(N)
        self._run_test(sf, sf, "target")

    def test_multiple_cache_files_external_memory(self):
        N = 20000
        sf = _get_data(N)
        self._run_test(sf, sf, "target")

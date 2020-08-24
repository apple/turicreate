# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import coremltools.models._feature_management as fm
import coremltools.models.datatypes as dt
import six
import unittest
from coremltools._deps import _HAS_SKLEARN


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class FeatureManagementTests(unittest.TestCase):
    def test_all_strings(self):
        features = ["a", "b", "c"]
        processed_features = [
            ("a", dt.Double()),
            ("b", dt.Double()),
            ("c", dt.Double()),
        ]
        out = fm.process_or_validate_features(features)
        self.assertEquals(out, processed_features)
        self.assertTrue(fm.is_valid_feature_list(out))

    def test_single_array(self):
        # test both int and long as input to num_dimensions
        for t in six.integer_types:
            self.assertEquals(
                fm.process_or_validate_features("a", num_dimensions=t(10)),
                [("a", dt.Array(10))],
            )

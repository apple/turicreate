# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
import numpy as np
from coremltools._deps import _HAS_SKLEARN

if _HAS_SKLEARN:
    from coremltools.converters import sklearn as converter
    from sklearn.preprocessing import Imputer


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class ImputerTestCase(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        from sklearn.datasets import load_boston

        scikit_data = load_boston()
        scikit_model = Imputer(strategy="most_frequent", axis=0)
        scikit_data["data"][1, 8] = np.NaN

        input_data = scikit_data["data"][:, 8].reshape(-1, 1)
        scikit_model.fit(input_data, scikit_data["target"])

        # Save the data and the model
        self.scikit_data = scikit_data
        self.scikit_model = scikit_model

    def test_conversion(self):
        spec = converter.convert(self.scikit_model, "data", "out").get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)

        # Test the interface
        self.assertTrue(spec.pipeline.models[-1].HasField("imputer"))

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(Exception):
            model = Imputer()
            spec = converter.convert(model, "data", "out")

        # Check the expected class during covnersion.
        with self.assertRaises(Exception):
            from sklearn.linear_model import LinearRegression

            model = LinearRegression()
            spec = converter.convert(model, "data", "out")

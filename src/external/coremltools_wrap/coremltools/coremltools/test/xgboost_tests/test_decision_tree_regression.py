# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
import tempfile

import json
from coremltools.proto import Model_pb2
from coremltools.proto import FeatureTypes_pb2
from coremltools._deps import _HAS_XGBOOST
from coremltools._deps import _HAS_SKLEARN

if _HAS_XGBOOST:
    import xgboost
    from coremltools.converters import xgboost as xgb_converter

if _HAS_SKLEARN:
    from coremltools.converters import sklearn as skl_converter
    from sklearn.tree import DecisionTreeRegressor


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class DecisionTreeRegressorScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        from sklearn.datasets import load_boston
        from sklearn.tree import DecisionTreeRegressor

        scikit_data = load_boston()
        scikit_model = DecisionTreeRegressor(random_state=1)
        scikit_model.fit(scikit_data["data"], scikit_data["target"])

        # Save the data and the model
        self.scikit_data = scikit_data
        self.scikit_model = scikit_model

    def test_conversion(self):
        feature_names = self.scikit_data.feature_names
        output_name = "target"
        spec = skl_converter.convert(
            self.scikit_model, feature_names, "target"
        ).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertIsNotNone(spec.treeEnsembleRegressor)

        # Test the interface class
        self.assertEqual(spec.description.predictedFeatureName, "target")

        # Test the inputs and outputs
        self.assertEqual(len(spec.description.output), 1)
        self.assertEqual(spec.description.output[0].name, "target")
        self.assertEqual(
            spec.description.output[0].type.WhichOneof("Type"), "doubleType"
        )
        for input_type in spec.description.input:
            self.assertEqual(input_type.type.WhichOneof("Type"), "doubleType")
        self.assertEqual(
            sorted(feature_names), sorted(map(lambda x: x.name, spec.description.input))
        )

        # Test the linear regression parameters.
        tr = spec.pipelineRegressor.pipeline.models[
            1
        ].treeEnsembleRegressor.treeEnsemble
        self.assertIsNotNone(tr)
        self.assertEqual(len(tr.nodes), 935)

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(Exception):
            model = DecisionTreeRegressor()
            spec = skl_converter.convert(model, "data", "out")

        # Check the expected class during covnersion.
        from sklearn.preprocessing import OneHotEncoder

        with self.assertRaises(Exception):
            model = OneHotEncoder()
            spec = skl_converter.convert(model, "data", "out")

# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
from sklearn.ensemble import RandomForestRegressor
from coremltools._deps import _HAS_SKLEARN
from coremltools.proto import Model_pb2
from coremltools.proto import FeatureTypes_pb2

if _HAS_SKLEARN:
    from sklearn.ensemble import RandomForestRegressor
    from coremltools.converters import sklearn as skl_converter


@unittest.skipIf(not _HAS_SKLEARN, "Missing scikit-learn. Skipping tests.")
class RandomForestRegressorScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        from sklearn.datasets import load_boston
        from sklearn.ensemble import RandomForestRegressor

        scikit_data = load_boston()
        scikit_model = RandomForestRegressor(random_state=1)
        scikit_model.fit(scikit_data["data"], scikit_data["target"])

        # Save the data and the model
        self.scikit_data = scikit_data
        self.scikit_model = scikit_model

    def test_conversion(self):
        input_names = self.scikit_data.feature_names
        output_name = "target"
        spec = skl_converter.convert(
            self.scikit_model, input_names, "target"
        ).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)

        # Test the interface class
        self.assertEquals(spec.description.predictedFeatureName, "target")

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.output), 1)
        self.assertEquals(spec.description.output[0].name, "target")
        self.assertEquals(
            spec.description.output[0].type.WhichOneof("Type"), "doubleType"
        )
        for input_type in spec.description.input:
            self.assertEquals(input_type.type.WhichOneof("Type"), "doubleType")
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )

        # Test the linear regression parameters.
        self.assertEquals(len(spec.pipelineRegressor.pipeline.models), 2)
        tr = spec.pipelineRegressor.pipeline.models[
            -1
        ].treeEnsembleRegressor.treeEnsemble
        self.assertIsNotNone(tr)
        self.assertEquals(len(tr.nodes), 5996)

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(Exception):
            model = RandomForestRegressor()
            spec = skl_converter.convert(model, "data", "out")

        # Check the expected class during covnersion.
        from sklearn.preprocessing import OneHotEncoder

        with self.assertRaises(Exception):
            model = OneHotEncoder()
            spec = skl_converter.convert(model, "data", "out")

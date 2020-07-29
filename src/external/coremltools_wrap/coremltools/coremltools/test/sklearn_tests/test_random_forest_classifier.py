# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
from coremltools._deps import _HAS_SKLEARN
from coremltools.proto import Model_pb2
from coremltools.proto import FeatureTypes_pb2

if _HAS_SKLEARN:
    from sklearn.ensemble import RandomForestClassifier
    from coremltools.converters import sklearn as skl_converter


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class RandomForestBinaryClassifierScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        from sklearn.datasets import load_boston
        from sklearn.ensemble import RandomForestClassifier

        scikit_data = load_boston()
        scikit_model = RandomForestClassifier(random_state=1)
        target = 1 * (scikit_data["target"] > scikit_data["target"].mean())
        scikit_model.fit(scikit_data["data"], target)

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
        self.assertEquals(len(spec.description.output), 2)
        self.assertEquals(spec.description.output[0].name, "target")
        self.assertEquals(
            spec.description.output[0].type.WhichOneof("Type"), "int64Type"
        )
        for input_type in spec.description.input:
            self.assertEquals(input_type.type.WhichOneof("Type"), "doubleType")
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )

        self.assertEquals(len(spec.pipelineClassifier.pipeline.models), 2)
        tr = spec.pipelineClassifier.pipeline.models[
            -1
        ].treeEnsembleClassifier.treeEnsemble
        self.assertIsNotNone(tr)
        self.assertEquals(len(tr.nodes), 1048)

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(Exception):
            model = RandomForestClassifier()
            spec = skl_converter.convert(model, "data", "out")

        # Check the expected class during covnersion.
        from sklearn.preprocessing import OneHotEncoder

        with self.assertRaises(Exception):
            model = OneHotEncoder()
            spec = skl_converter.convert(model, "data", "out")


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class RandomForestMultiClassClassifierScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        from sklearn.datasets import load_boston
        from sklearn.ensemble import RandomForestClassifier
        import numpy as np

        scikit_data = load_boston()
        scikit_model = RandomForestClassifier(random_state=1)
        t = scikit_data.target
        target = np.digitize(t, np.histogram(t)[1]) - 1
        scikit_model.fit(scikit_data.data, target)

        # Save the data and the model
        self.scikit_data = scikit_data
        self.target = target
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
        self.assertIsNotNone(spec.treeEnsembleClassifier)

        # Test the interface class
        self.assertEquals(spec.description.predictedFeatureName, "target")

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.output), 2)
        self.assertEquals(spec.description.output[0].name, "target")
        self.assertEquals(
            spec.description.output[0].type.WhichOneof("Type"), "int64Type"
        )

        for input_type in spec.description.input:
            self.assertEquals(input_type.type.WhichOneof("Type"), "doubleType")
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )

        self.assertEquals(len(spec.pipelineClassifier.pipeline.models), 2)
        tr = spec.pipelineClassifier.pipeline.models[
            -1
        ].treeEnsembleClassifier.treeEnsemble
        self.assertIsNotNone(tr)
        self.assertEquals(len(tr.nodes), 2970)

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(Exception):
            model = RandomForestClassifier()
            spec = skl_converter.convert(model, "data", "out")

        # Check the expected class during covnersion.
        with self.assertRaises(Exception):
            from sklearn.preprocessing import OneHotEncoder

            model = OneHotEncoder()
            spec = skl_converter.convert(model, "data", "out")

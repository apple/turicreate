# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
from coremltools._deps import _HAS_XGBOOST
from coremltools._deps import _HAS_SKLEARN
from coremltools.proto import Model_pb2
from coremltools.proto import FeatureTypes_pb2

if _HAS_SKLEARN:
    from sklearn.tree import DecisionTreeClassifier
    from coremltools.converters.sklearn import convert as skl_converter

if _HAS_XGBOOST:
    from coremltools.converters import xgboost as xgb_converter


@unittest.skipIf(not _HAS_SKLEARN, "Missing scikit-learn. Skipping tests.")
class DecisionTreeBinaryClassifierScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        from sklearn.datasets import load_boston
        from sklearn.tree import DecisionTreeClassifier

        scikit_data = load_boston()
        scikit_model = DecisionTreeClassifier(random_state=1)
        target = scikit_data["target"] > scikit_data["target"].mean()
        scikit_model.fit(scikit_data["data"], target)

        # Save the data and the model
        self.scikit_data = scikit_data
        self.scikit_model = scikit_model

    def test_conversion(self):
        output_name = "target"
        spec = skl_converter(self.scikit_model, "data", "target").get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertIsNotNone(spec.treeEnsembleClassifier)

        # Test the interface class
        self.assertEqual(spec.description.predictedFeatureName, "target")

        # Test the inputs and outputs
        self.assertEqual(len(spec.description.output), 2)
        self.assertEqual(spec.description.output[0].name, "target")
        self.assertEqual(
            spec.description.output[0].type.WhichOneof("Type"), "int64Type"
        )
        self.assertEqual(len(spec.description.input), 1)

        input_type = spec.description.input[0]

        self.assertEqual(input_type.type.WhichOneof("Type"), "multiArrayType")
        self.assertEqual(input_type.name, "data")

        # Test the linear regression parameters.
        tr = spec.treeEnsembleClassifier.treeEnsemble
        self.assertIsNotNone(tr)
        self.assertEqual(len(tr.nodes), 111)

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(Exception):
            model = DecisionTreeClassifier()
            spec = skl_converter(model, "data", "out")

        # Check the expected class during covnersion.
        from sklearn.preprocessing import OneHotEncoder

        with self.assertRaises(Exception):
            model = OneHotEncoder()
            spec = skl_converter(model, "data", "out")


@unittest.skipIf(not _HAS_SKLEARN, "Missing scikit-learn. Skipping tests.")
class DecisionTreeMultiClassClassifierScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        from sklearn.datasets import load_boston
        from sklearn.tree import DecisionTreeClassifier
        from sklearn.preprocessing import MultiLabelBinarizer
        import numpy as np

        scikit_data = load_boston()
        scikit_model = DecisionTreeClassifier(random_state=1)
        t = scikit_data.target
        target = np.digitize(t, np.histogram(t)[1]) - 1
        scikit_model.fit(scikit_data.data, target)

        # Save the data and the model
        self.scikit_data = scikit_data
        self.target = target
        self.scikit_model = scikit_model

    def test_conversion(self):
        output_name = "target"
        spec = skl_converter(self.scikit_model, "data", "target").get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertIsNotNone(spec.treeEnsembleClassifier)

        # Test the interface class
        self.assertEqual(spec.description.predictedFeatureName, "target")

        # Test the inputs and outputs
        self.assertEqual(len(spec.description.output), 2)
        self.assertEqual(spec.description.output[0].name, "target")
        self.assertEqual(
            spec.description.output[0].type.WhichOneof("Type"), "int64Type"
        )
        self.assertEqual(spec.description.input[0].name, "data")
        self.assertEqual(
            spec.description.input[0].type.WhichOneof("Type"), "multiArrayType"
        )

        tr = spec.treeEnsembleClassifier.treeEnsemble
        self.assertIsNotNone(tr)
        self.assertEqual(len(tr.nodes), 315)

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(Exception):
            model = DecisionTreeClassifier()
            spec = skl_converter(model, "data", "out")

        # Check the expected class during covnersion.
        from sklearn.preprocessing import OneHotEncoder

        with self.assertRaises(Exception):
            model = OneHotEncoder()
            spec = skl_converter(model, "data", "out")

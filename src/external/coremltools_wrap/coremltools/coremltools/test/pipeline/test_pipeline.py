# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
import tempfile
from coremltools.proto import FeatureTypes_pb2
from coremltools._deps import _HAS_SKLEARN, _HAS_LIBSVM
from coremltools.models.pipeline import PipelineRegressor, PipelineClassifier
from coremltools.models.utils import evaluate_transformer
import coremltools.models.datatypes as datatypes
from coremltools.models.feature_vectorizer import create_feature_vectorizer

if _HAS_SKLEARN:
    from sklearn.preprocessing import OneHotEncoder
    from sklearn.datasets import load_boston
    from sklearn.linear_model import LinearRegression
    from sklearn.pipeline import Pipeline
    from coremltools.converters import sklearn as converter

if _HAS_LIBSVM:
    from libsvm import svm
    from libsvm import svmutil
    from coremltools.converters import libsvm as libsvm_converter


@unittest.skipIf(not _HAS_SKLEARN, "Missing scikit-learn. Skipping tests.")
@unittest.skipIf(not _HAS_LIBSVM, "Missing libsvm. Skipping tests.")
class LinearRegressionPipelineCreationTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """

        if not (_HAS_SKLEARN):
            return

        scikit_data = load_boston()
        feature_names = scikit_data.feature_names

        scikit_model = LinearRegression()
        scikit_model.fit(scikit_data["data"], scikit_data["target"])
        scikit_spec = converter.convert(
            scikit_model, feature_names, "target"
        ).get_spec()

        # Save the data and the model
        self.scikit_data = scikit_data
        self.scikit_model = scikit_model
        self.scikit_spec = scikit_spec

    def test_pipeline_regression_creation(self):

        input_names = self.scikit_data.feature_names
        output_name = "target"
        p_regressor = PipelineRegressor(input_names, "target")
        p_regressor.add_model(self.scikit_spec)

        self.assertIsNotNone(p_regressor.spec)
        self.assertEqual(len(p_regressor.spec.pipelineRegressor.pipeline.models), 1)

        # Test the model class of the linear regressor model
        spec = p_regressor.spec.pipelineRegressor.pipeline.models[0]
        self.assertIsNotNone(spec.description)

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
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )


@unittest.skipIf(not _HAS_SKLEARN, "Missing scikit-learn. Skipping tests.")
@unittest.skipIf(not _HAS_LIBSVM, "Missing libsvm. Skipping tests.")
class LibSVMPipelineCreationTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        if not _HAS_SKLEARN:
            return
        if not _HAS_LIBSVM:
            return

        scikit_data = load_boston()
        prob = svmutil.svm_problem(
            scikit_data["target"] > scikit_data["target"].mean(),
            scikit_data["data"].tolist(),
        )
        param = svmutil.svm_parameter()
        param.svm_type = svmutil.C_SVC
        param.kernel_type = svmutil.LINEAR
        param.eps = 1

        libsvm_model = svmutil.svm_train(prob, param)
        libsvm_spec = libsvm_converter.convert(
            libsvm_model, scikit_data.feature_names, "target"
        ).get_spec()

        # Save the data and the model
        self.scikit_data = scikit_data
        self.libsvm_spec = libsvm_spec

    def test_pipeline_classifier_creation(self):

        input_names = self.scikit_data.feature_names
        p_classifier = PipelineClassifier(input_names, [1, 0])
        p_classifier.add_model(self.libsvm_spec)

        self.assertIsNotNone(p_classifier.spec)
        self.assertEqual(len(p_classifier.spec.pipelineClassifier.pipeline.models), 1)

        # Test the model class of the svm model
        spec = p_classifier.spec.pipelineClassifier.pipeline.models[0]
        self.assertIsNotNone(spec.description)

        # Test the interface class
        self.assertEqual(spec.description.predictedFeatureName, "target")

        # Test the inputs and outputs
        self.assertEqual(len(spec.description.output), 1)
        self.assertEqual(spec.description.output[0].name, "target")
        self.assertEqual(
            spec.description.output[0].type.WhichOneof("Type"), "int64Type"
        )

        for input_type in spec.description.input:
            self.assertEqual(input_type.type.WhichOneof("Type"), "doubleType")
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )


@unittest.skipIf(not _HAS_SKLEARN, "Missing scikit-learn. Skipping tests.")
class LinearRegressionPipeline(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        if not _HAS_SKLEARN:
            return
        scikit_data = load_boston()
        feature_names = scikit_data.feature_names

        scikit_model = Pipeline(steps=[("linear", LinearRegression())])
        scikit_model.fit(scikit_data["data"], scikit_data["target"])

        # Save the data and the model
        self.scikit_data = scikit_data
        self.scikit_model = scikit_model

    def test_pipeline_regression_creation(self):
        input_names = self.scikit_data.feature_names
        output_name = "target"

        p_regressor = converter.convert(
            self.scikit_model, input_names, "target"
        ).get_spec()
        self.assertIsNotNone(p_regressor)
        self.assertEqual(len(p_regressor.pipelineRegressor.pipeline.models), 2)

        # Test the model class of the linear regressor model
        spec = p_regressor.pipelineRegressor.pipeline.models[-1]
        self.assertIsNotNone(spec.description)

        # Test the interface class
        self.assertEqual(spec.description.predictedFeatureName, "target")

        # Test the inputs and outputs
        self.assertEqual(len(spec.description.output), 1)
        self.assertEqual(spec.description.output[0].name, "target")
        self.assertEqual(
            spec.description.output[0].type.WhichOneof("Type"), "doubleType"
        )

        for input_type in p_regressor.description.input:
            self.assertEqual(input_type.type.WhichOneof("Type"), "doubleType")
        self.assertEqual(
            sorted(input_names),
            sorted(map(lambda x: x.name, p_regressor.description.input)),
        )

    def test_conversion_bad_inputs(self):
        """
        Failure testing for bad conversion.
        """
        # Error on converting an untrained model
        with self.assertRaises(TypeError):
            model = OneHotEncoder()
            spec = converter.convert(model, "data", "out", "regressor")

# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import os,shutil
import numpy as _np
import coremltools.models.datatypes as datatypes
import unittest
import tempfile
from coremltools.models.utils import save_spec
from coremltools.models import MLModel
from coremltools.models.neural_network import NeuralNetworkBuilder, AdamParams, SgdParams
from coremltools.models.pipeline import PipelineRegressor, PipelineClassifier


class MLModelUpdatableTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        self.model_dir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(self):
        if os.path.exists(self.model_dir):
            shutil.rmtree(self.model_dir)

    def create_base_builder(self):
        self.input_features = [('input', datatypes.Array(3))]
        self.output_features = [('output', None)]
        self.output_names = ["output"]

        builder = NeuralNetworkBuilder(self.input_features, self.output_features)

        W1 = _np.random.uniform(-0.5, 0.5, (3, 3))
        W2 = _np.random.uniform(-0.5, 0.5, (3, 3))
        builder.add_inner_product(name='ip1',
                                  W=W1,
                                  b=None,
                                  input_channels=3,
                                  output_channels=3,
                                  has_bias=False,
                                  input_name='input',
                                  output_name='hidden')
        builder.add_inner_product(name='ip2',
                                  W=W2,
                                  b=None,
                                  input_channels=3,
                                  output_channels=3,
                                  has_bias=False,
                                  input_name='hidden',
                                  output_name='output')

        builder.make_updatable(['ip1', 'ip2'])  # or a dict for weightParams
        return builder

    def test_updatable_model_creation_ce_sgd(self):

        builder = self.create_base_builder()

        builder.add_softmax(name='softmax', input_name='output', output_name='softmax_output')

        builder.set_categorical_cross_entropy_loss(name='cross_entropy', input='softmax_output')

        builder.set_sgd_optimizer(SgdParams(lr=1e-2, batch=10, momentum=0.0))
        builder.set_epochs(20, allowed_set=[10, 20, 30, 40])

        model_path = os.path.join(self.model_dir, 'updatable_creation.mlmodel')
        print(model_path)
        save_spec(builder.spec, model_path)

        mlmodel = MLModel(model_path)
        self.assertTrue(mlmodel is not None)
        spec = mlmodel.get_spec()
        self.assertTrue(spec.isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[0].isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[0].innerProduct.weights.isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[1].isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[1].innerProduct.weights.isUpdatable)

        self.assertTrue(spec.neuralNetwork.updateParams.lossLayers[0].categoricalCrossEntropyLossLayer is not None)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer is not None)

        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.learningRate.defaultValue, 1e-2, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.miniBatchSize.defaultValue, 10, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.momentum.defaultValue, 0, atol=1e-8))

        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.epochs.defaultValue, 20, atol=1e-4))

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.learningRate.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.learningRate.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.miniBatchSize.set.values == [10])

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.momentum.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.momentum.range.maxValue == 1)

    def test_updatable_model_creation_ce_adam(self):

        builder = self.create_base_builder()

        builder.add_softmax(name='softmax', input_name='output', output_name='softmax_output')

        builder.set_categorical_cross_entropy_loss(name='cross_entropy', input='softmax_output')

        adam_params = AdamParams()
        adam_params.set_batch(value=10, allowed_set=[10, 20])
        builder.set_adam_optimizer(adam_params)
        builder.set_epochs(20)

        model_path = os.path.join(self.model_dir, 'updatable_creation.mlmodel')
        print(model_path)
        save_spec(builder.spec, model_path)

        mlmodel = MLModel(model_path)
        self.assertTrue(mlmodel is not None)
        spec = mlmodel.get_spec()
        self.assertTrue(spec.isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[0].isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[0].innerProduct.weights.isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[1].isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[1].innerProduct.weights.isUpdatable)

        self.assertTrue(spec.neuralNetwork.updateParams.lossLayers[0].categoricalCrossEntropyLossLayer is not None)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer is not None)

        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.learningRate.defaultValue, 1e-2, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.miniBatchSize.defaultValue, 10, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta1.defaultValue, 0.9, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta2.defaultValue, 0.999, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.eps.defaultValue, 1e-8, atol=1e-8))

        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.epochs.defaultValue, 20, atol=1e-4))

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.learningRate.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.learningRate.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.miniBatchSize.set.values == [10, 20])

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta1.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta1.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta2.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta2.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.eps.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.eps.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.epochs.set.values == [20])

    def test_updatable_model_creation_mse_sgd(self):

        builder = self.create_base_builder()

        builder.set_mean_squared_error_loss(name='mse', input_feature=('output', datatypes.Array(3)))

        builder.set_sgd_optimizer(SgdParams(lr=1e-2, batch=10, momentum=0.0))

        builder.set_epochs(20)

        model_path = os.path.join(self.model_dir, 'updatable_creation.mlmodel')
        print(model_path)
        save_spec(builder.spec, model_path)

        mlmodel = MLModel(model_path)
        self.assertTrue(mlmodel is not None)
        spec = mlmodel.get_spec()
        self.assertTrue(spec.isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[0].isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[0].innerProduct.weights.isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[1].isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[1].innerProduct.weights.isUpdatable)

        self.assertTrue(spec.neuralNetwork.updateParams.lossLayers[0].categoricalCrossEntropyLossLayer is not None)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer is not None)

        self.assertTrue(
            _np.isclose(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.learningRate.defaultValue, 1e-2,
                        atol=1e-4))
        self.assertTrue(
            _np.isclose(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.miniBatchSize.defaultValue, 10,
                        atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.momentum.defaultValue, 0, atol=1e-8))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.epochs.defaultValue, 20, atol=1e-4))

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.learningRate.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.learningRate.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.miniBatchSize.set.values == [10])

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.momentum.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.sgdOptimizer.momentum.range.maxValue == 1)


    def test_updatable_model_creation_mse_adam(self):

        builder = self.create_base_builder()

        builder.set_mean_squared_error_loss(name='mse', input_feature=('output', datatypes.Array(3)))

        builder.set_adam_optimizer(AdamParams(lr=1e-2, batch=10,
                                   beta1=0.9, beta2=0.999, eps=1e-8))
        builder.set_epochs(20, allowed_set=[10, 20, 30])

        model_path = os.path.join(self.model_dir, 'updatable_creation.mlmodel')
        print(model_path)
        save_spec(builder.spec, model_path)

        mlmodel = MLModel(model_path)
        self.assertTrue(mlmodel is not None)
        spec = mlmodel.get_spec()
        self.assertTrue(spec.isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[0].isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[0].innerProduct.weights.isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[1].isUpdatable)
        self.assertTrue(spec.neuralNetwork.layers[1].innerProduct.weights.isUpdatable)

        self.assertTrue(spec.neuralNetwork.updateParams.lossLayers[0].categoricalCrossEntropyLossLayer is not None)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer is not None)

        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.learningRate.defaultValue, 1e-2, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.miniBatchSize.defaultValue, 10, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta1.defaultValue, 0.9, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta2.defaultValue, 0.999, atol=1e-4))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.eps.defaultValue, 1e-8, atol=1e-8))
        self.assertTrue(_np.isclose(spec.neuralNetwork.updateParams.epochs.defaultValue, 20, atol=1e-4))

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.learningRate.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.learningRate.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.miniBatchSize.set.values == [10])

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta1.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta1.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta2.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.beta2.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.eps.range.minValue == 0)
        self.assertTrue(spec.neuralNetwork.updateParams.optimizer.adamOptimizer.eps.range.maxValue == 1)

        self.assertTrue(spec.neuralNetwork.updateParams.epochs.set.values == [10, 20, 30])

    def test_nn_set_cce_without_softmax_fail(self):

        nn_builder = self.create_base_builder()

        # fails since adding CCE without softmax must raise error
        with self.assertRaises(ValueError):
            nn_builder.set_categorical_cross_entropy_loss(name='cross_entropy', input='output')

    def test_nn_set_cce_invalid(self):
        nn_builder = self.create_base_builder()
        nn_builder.add_softmax(name='softmax', input_name='output', output_name='softmax_output')

        # fails since CCE input must be softmax output
        with self.assertRaises(ValueError):
            nn_builder.set_categorical_cross_entropy_loss(name='cross_entropy', input='output')

    def test_nn_set_softmax_updatable_invalid(self):
        nn_builder = self.create_base_builder()
        nn_builder.add_softmax(name='softmax', input_name='output', output_name='softmax_output')

        # fails since marking softmax as updatable layer is not allowed
        with self.assertRaises(ValueError):
            nn_builder.make_updatable(['softmax'])

    def test_nn_set_training_input(self):

        builder = self.create_base_builder()

        builder.set_mean_squared_error_loss(name='mse', input_feature=('output', datatypes.Array(3)))

        builder.set_adam_optimizer(AdamParams(lr=1e-2, batch=10,
                                   beta1=0.9, beta2=0.999, eps=1e-8))
        builder.set_epochs(20, allowed_set=[10, 20, 30])

        model_path = os.path.join(self.model_dir, 'updatable_creation.mlmodel')
        print(model_path)
        save_spec(builder.spec, model_path)

        mlmodel = MLModel(model_path)
        self.assertTrue(mlmodel is not None)
        spec = mlmodel.get_spec()
        self.assertEqual(spec.description.trainingInput[0].name, 'input')
        self.assertEqual(spec.description.trainingInput[0].type.WhichOneof('Type'), 'multiArrayType')
        self.assertEqual(spec.description.trainingInput[1].name, 'output_true')
        self.assertEqual(spec.description.trainingInput[1].type.WhichOneof('Type'), 'multiArrayType')

    def test_nn_builder_with_training_features(self):

        input_features = [('input', datatypes.Array(3))]
        output_features = [('output', datatypes.Array(3))]
        builder = NeuralNetworkBuilder(input_features, output_features)

        W1 = _np.random.uniform(-0.5, 0.5, (3, 3))
        W2 = _np.random.uniform(-0.5, 0.5, (3, 3))
        builder.add_inner_product(name='ip1',
                                  W=W1,
                                  b=None,
                                  input_channels=3,
                                  output_channels=3,
                                  has_bias=False,
                                  input_name='input',
                                  output_name='hidden')
        builder.add_inner_product(name='ip2',
                                  W=W2,
                                  b=None,
                                  input_channels=3,
                                  output_channels=3,
                                  has_bias=False,
                                  input_name='hidden',
                                  output_name='output')

        builder.make_updatable(['ip1', 'ip2'])  # or a dict for weightParams

        builder.set_mean_squared_error_loss(name='mse', input_feature=('output', datatypes.Array(3)))

        builder.set_adam_optimizer(AdamParams(lr=1e-2, batch=10,
                                   beta1=0.9, beta2=0.999, eps=1e-8))
        builder.set_epochs(20, allowed_set=[10, 20, 30])

        model_path = os.path.join(self.model_dir, 'updatable_creation.mlmodel')
        print(model_path)
        save_spec(builder.spec, model_path)

        mlmodel = MLModel(model_path)
        self.assertTrue(mlmodel is not None)
        spec = mlmodel.get_spec()
        self.assertEqual(spec.description.trainingInput[0].name, 'input')
        self.assertEqual(spec.description.trainingInput[0].type.WhichOneof('Type'), 'multiArrayType')
        self.assertEqual(spec.description.trainingInput[1].name, 'output_true')
        self.assertEqual(spec.description.trainingInput[1].type.WhichOneof('Type'), 'multiArrayType')

    def test_pipeline_regressor_make_updatable(self):
        builder = self.create_base_builder()
        builder.spec.isUpdatable = False

        training_input = [('input', datatypes.Array(3)), ('target', 'Double')]

        # fails due to missing sub-models
        p_regressor = PipelineRegressor(self.input_features, self.output_names, training_input)
        with self.assertRaises(ValueError):
            p_regressor.make_updatable()
        self.assertEqual(p_regressor.spec.isUpdatable, False)

        # fails due to sub-model being not updatable
        p_regressor.add_model(builder.spec)
        with self.assertRaises(ValueError):
            p_regressor.make_updatable()
        self.assertEqual(p_regressor.spec.isUpdatable, False)

        builder.spec.isUpdatable = True
        p_regressor.add_model(builder.spec)

        self.assertEqual(p_regressor.spec.isUpdatable, False)
        p_regressor.make_updatable();
        self.assertEqual(p_regressor.spec.isUpdatable, True)
        self.assertEqual(p_regressor.spec.description.trainingInput[0].name, 'input')
        self.assertEqual(p_regressor.spec.description.trainingInput[0].type.WhichOneof('Type'), 'multiArrayType')
        self.assertEqual(p_regressor.spec.description.trainingInput[1].name, 'target')
        self.assertEqual(p_regressor.spec.description.trainingInput[1].type.WhichOneof('Type'), 'doubleType')

        # fails since once updatable does not allow adding new models
        with self.assertRaises(ValueError):
            p_regressor.add_model(builder.spec)
        self.assertEqual(p_regressor.spec.isUpdatable, True)

    def test_pipeline_classifier_make_updatable(self):
        builder = self.create_base_builder()
        builder.spec.isUpdatable = False
        training_input = [('input', datatypes.Array(3)), ('target', 'String')]

        # fails due to missing sub-models
        p_classifier = PipelineClassifier(self.input_features, self.output_names, training_features=training_input)
        with self.assertRaises(ValueError):
            p_classifier.make_updatable()
        self.assertEqual(p_classifier.spec.isUpdatable, False)

        # fails due to sub-model being not updatable
        p_classifier.add_model(builder.spec)
        with self.assertRaises(ValueError):
            p_classifier.make_updatable()
        self.assertEqual(p_classifier.spec.isUpdatable, False)

        builder.spec.isUpdatable = True
        p_classifier.add_model(builder.spec)

        self.assertEqual(p_classifier.spec.isUpdatable, False)
        p_classifier.make_updatable();
        self.assertEqual(p_classifier.spec.isUpdatable, True)
        self.assertEqual(p_classifier.spec.description.trainingInput[0].name, 'input')
        self.assertEqual(p_classifier.spec.description.trainingInput[0].type.WhichOneof('Type'), 'multiArrayType')
        self.assertEqual(p_classifier.spec.description.trainingInput[1].name, 'target')
        self.assertEqual(p_classifier.spec.description.trainingInput[1].type.WhichOneof('Type'), 'stringType')

        # fails since once updatable does not allow adding new models
        with self.assertRaises(ValueError):
            p_classifier.add_model(builder.spec)
        self.assertEqual(p_classifier.spec.isUpdatable, True)


    def test_pipeline_classifier_set_training_inputs(self):
        builder = self.create_base_builder()
        builder.spec.isUpdatable = False
        training_input = [('input', datatypes.Array(3)), ('target', 'String')]

        # fails due to missing sub-models
        p_classifier = PipelineClassifier(self.input_features, self.output_names)
        p_classifier.set_training_input(training_input)
        with self.assertRaises(ValueError):
            p_classifier.make_updatable()
        self.assertEqual(p_classifier.spec.isUpdatable, False)

        # fails due to sub-model being not updatable
        p_classifier.add_model(builder.spec)
        with self.assertRaises(ValueError):
            p_classifier.make_updatable()
        self.assertEqual(p_classifier.spec.isUpdatable, False)

        builder.spec.isUpdatable = True
        p_classifier.add_model(builder.spec)

        self.assertEqual(p_classifier.spec.isUpdatable, False)
        p_classifier.make_updatable();
        self.assertEqual(p_classifier.spec.isUpdatable, True)
        self.assertEqual(p_classifier.spec.description.trainingInput[0].name, 'input')
        self.assertEqual(p_classifier.spec.description.trainingInput[0].type.WhichOneof('Type'), 'multiArrayType')
        self.assertEqual(p_classifier.spec.description.trainingInput[1].name, 'target')
        self.assertEqual(p_classifier.spec.description.trainingInput[1].type.WhichOneof('Type'), 'stringType')

        # fails since once updatable does not allow adding new models
        with self.assertRaises(ValueError):
            p_classifier.add_model(builder.spec)
        self.assertEqual(p_classifier.spec.isUpdatable, True)

    def test_shuffle_on_by_default(self):
        builder = self.create_base_builder()

        # base builder already marks two layers as updatable
        self.assertTrue(builder.nn_spec.updateParams.shuffle.defaultValue, "Shuffle not turned on by default for updatable models")

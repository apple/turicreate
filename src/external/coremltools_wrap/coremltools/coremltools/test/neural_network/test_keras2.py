import unittest

from coremltools._deps import _HAS_KERAS2_TF
from coremltools.proto import Model_pb2
from coremltools.proto import FeatureTypes_pb2
from coremltools.proto import NeuralNetwork_pb2
import pytest

if _HAS_KERAS2_TF:
    import tensorflow as tf
    from keras.models import Sequential, Model
    from coremltools.converters import keras


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
class KerasSingleLayerTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading common utilities.
        """

    def test_dense(self):
        """
        Test the conversion of Dense layer.
        """
        from keras.layers import Dense

        # Create a simple Keras model
        model = Sequential()
        model.add(Dense(32, input_dim=16))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.innerProduct)

    def test_activations(self):
        """
        Test the conversion for a Dense + Activation('something')
        """
        from keras.layers import Dense, Activation

        # Create a simple Keras model
        keras_activation_options = [
            "elu",
            "tanh",
            "softplus",
            "softsign",
            "relu",
            "sigmoid",
            "hard_sigmoid",
            "linear",
        ]
        coreml_activation_options = [
            "ELU",
            "tanh",
            "softplus",
            "softsign",
            "ReLU",
            "sigmoid",
            "sigmoidHard",
            "linear",
        ]

        for i, k_act in enumerate(keras_activation_options):
            c_act = coreml_activation_options[i]
            model = Sequential()
            model.add(Dense(32, input_dim=16))
            model.add(Activation(k_act))

            input_names = ["input"]
            output_names = ["output"]
            spec = keras.convert(model, input_names, output_names).get_spec()
            self.assertIsNotNone(spec)

            # Test the model class
            self.assertIsNotNone(spec.description)
            self.assertTrue(spec.HasField("neuralNetwork"))

            # Test the inputs and outputs
            self.assertEquals(len(spec.description.input), len(input_names))
            self.assertEqual(
                sorted(input_names),
                sorted(map(lambda x: x.name, spec.description.input)),
            )
            self.assertEquals(len(spec.description.output), len(output_names))
            self.assertEqual(
                sorted(output_names),
                sorted(map(lambda x: x.name, spec.description.output)),
            )

            # Test the layer parameters.
            layers = spec.neuralNetwork.layers
            self.assertIsNotNone(layers[0].innerProduct)
            self.assertIsNotNone(layers[1].activation)
            self.assertTrue(layers[1].activation.HasField(c_act))

    def test_activation_softmax(self):
        """
        Test the conversion for a Dense + Activation('softmax')
        """
        from keras.layers import Dense, Activation

        # Create a simple Keras model
        model = Sequential()
        model.add(Dense(32, input_dim=16))
        model.add(Activation("softmax"))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.innerProduct)
        layer_1 = layers[1]
        self.assertIsNotNone(layer_1.softmax)

    def test_dropout(self):
        """
        Test the conversion for a Dense + Dropout
        """
        from keras.layers import Dense, Dropout

        # Create a simple Keras model
        model = Sequential()
        model.add(Dense(32, input_shape=(16,)))
        model.add(Dropout(0.5))
        model.add(Dense(32, input_shape=(16,)))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.innerProduct)
        self.assertEquals(len(layers), 2)

    def test_convolution(self, with_dilations=False):
        """
        Test the conversion of 2D convolutional layer.
        """
        from keras.layers import Conv2D

        dilation_rate = [1, 1]
        if with_dilations:
            dilation_rate = [2, 2]

        # Create a simple Keras model
        model = Sequential()
        model.add(
            Conv2D(
                input_shape=(64, 64, 3),
                filters=32,
                kernel_size=(5, 5),
                activation=None,
                padding="valid",
                strides=(1, 1),
                use_bias=True,
                dilation_rate=dilation_rate,
            )
        )

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.convolution)
        self.assertEqual(layer_0.convolution.dilationFactor, dilation_rate)

    def test_convolution_dilated(self):
        """
        Test the conversion of 2D convolutional layer with dilated kernels
        """
        self.test_convolution(with_dilations=True)

    def test_separable_convolution(self, with_dilations=False, activation=None):
        """
        Test the conversion of 2D depthwise separable convolutional layer.
        """
        from keras.layers import SeparableConv2D

        dilation_rate = [1, 1]
        if with_dilations:
            dilation_rate = [2, 2]

        # Create a simple Keras model
        model = Sequential()
        model.add(
            SeparableConv2D(
                input_shape=(64, 64, 3),
                filters=32,
                kernel_size=(5, 5),
                activation=activation,
                padding="valid",
                strides=(1, 1),
                use_bias=True,
                dilation_rate=dilation_rate,
            )
        )

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_depthwise, layer_pointwise = layers[0], layers[1]

        self.assertIsNotNone(layer_depthwise.convolution)
        self.assertIsNotNone(layer_pointwise.convolution)
        self.assertEqual(layer_depthwise.convolution.dilationFactor, dilation_rate)
        if activation is not None:
            self.assertIsNotNone(layers[2].activation)
            self.assertTrue(layers[2].activation.HasField("ELU"))

    def test_separable_convolution_dilated(self):
        """
        Test the conversion of 2D depthwise separable convolutional layer with dilated kernels.
        """
        self.test_separable_convolution(with_dilations=True)

    def test_separable_convolution_with_nonlinearity(self):
        """
        Test the conversion of 2D depthwise separable convolutional layer with nonlinearity.
        """
        self.test_separable_convolution(activation="elu")

    def test_upsample(self):
        """
        Test the conversion of 2D convolutional layer + upsample
        """
        from keras.layers import Conv2D, UpSampling2D

        # Create a simple Keras model
        model = Sequential()
        model.add(Conv2D(input_shape=(64, 64, 3), filters=32, kernel_size=(5, 5)))
        model.add(UpSampling2D(size=(2, 2)))
        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.convolution)
        layer_1 = layers[1]
        self.assertIsNotNone(layer_1.upsample)
        self.assertEquals(
            layer_1.upsample.mode,
            NeuralNetwork_pb2.UpsampleLayerParams.InterpolationMode.Value("NN"),
        )

        # Test if BILINEAR mode works as well
        model = Sequential()
        model.add(Conv2D(input_shape=(64, 64, 3), filters=32, kernel_size=(5, 5)))
        try:
            model.add(UpSampling2D(size=(2, 2), interpolation="bilinear"))
        except TypeError:  # Early version of Keras, no support for 'interpolation'
            return

        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)
        layers = spec.neuralNetwork.layers
        layer_1 = layers[1]
        self.assertIsNotNone(layer_1.upsample)
        self.assertEquals(
            layer_1.upsample.mode,
            NeuralNetwork_pb2.UpsampleLayerParams.InterpolationMode.Value("BILINEAR"),
        )

    def test_pooling(self):
        """
        Test the conversion of pooling layer.
        """
        from keras.layers import Conv2D, MaxPooling2D

        # Create a simple Keras model
        model = Sequential()
        model.add(
            Conv2D(
                input_shape=(64, 64, 3),
                filters=32,
                kernel_size=(5, 5),
                strides=(1, 1),
                activation=None,
                padding="valid",
                use_bias=True,
            )
        )
        model.add(MaxPooling2D(pool_size=(2, 2)))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].pooling)

    def test_permute(self):
        """
        Test the conversion of pooling layer.
        """
        from keras.layers.core import Permute

        # Create a simple Keras model
        model = Sequential()
        model.add(Permute((3, 2, 1), input_shape=(10, 64, 3)))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.permute)

    def test_lstm(self):
        """
        Test the conversion of an LSTM layer.
        """
        from keras.layers import LSTM

        # Create a simple Keras model
        model = Sequential()
        model.add(LSTM(32, input_shape=(10, 24)))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()

        print(spec)

        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names) + 2)

        self.assertEquals(32, spec.description.input[1].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.input[2].type.multiArrayType.shape[0])

        self.assertEquals(len(spec.description.output), len(output_names) + 2)
        self.assertEquals(output_names[0], spec.description.output[0].name)
        self.assertEquals(32, spec.description.output[0].type.multiArrayType.shape[0])

        self.assertEquals(32, spec.description.output[1].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.output[2].type.multiArrayType.shape[0])

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.uniDirectionalLSTM)
        self.assertEquals(len(layer_0.input), 3)
        self.assertEquals(len(layer_0.output), 3)

    def test_simple_rnn(self):
        """
        Test the conversion of a simple RNN layer.
        """
        from keras.layers import SimpleRNN

        # Create a simple Keras model
        model = Sequential()
        model.add(SimpleRNN(32, input_shape=(10, 32)))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names) + 1)
        self.assertEquals(input_names[0], spec.description.input[0].name)

        self.assertEquals(32, spec.description.input[1].type.multiArrayType.shape[0])

        self.assertEquals(len(spec.description.output), len(output_names) + 1)
        self.assertEquals(output_names[0], spec.description.output[0].name)
        self.assertEquals(32, spec.description.output[0].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.output[1].type.multiArrayType.shape[0])

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.simpleRecurrent)
        self.assertEquals(len(layer_0.input), 2)
        self.assertEquals(len(layer_0.output), 2)

    def test_gru(self):
        """
        Test the conversion of a GRU layer.
        """
        from keras.layers import GRU

        # Create a simple Keras model
        model = Sequential()
        model.add(GRU(32, input_shape=(32, 10)))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names) + 1)
        self.assertEquals(input_names[0], spec.description.input[0].name)

        self.assertEquals(32, spec.description.input[1].type.multiArrayType.shape[0])

        self.assertEquals(len(spec.description.output), len(output_names) + 1)
        self.assertEquals(output_names[0], spec.description.output[0].name)
        self.assertEquals(32, spec.description.output[0].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.output[1].type.multiArrayType.shape[0])

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.gru)
        self.assertEquals(len(layer_0.input), 2)
        self.assertEquals(len(layer_0.output), 2)

    def test_bidir(self):
        """
        Test the conversion of a bidirectional layer
        """
        from keras.layers import LSTM
        from keras.layers.wrappers import Bidirectional

        # Create a simple Keras model
        model = Sequential()
        model.add(Bidirectional(LSTM(32, input_shape=(10, 32)), input_shape=(10, 32)))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names) + 4)
        self.assertEquals(input_names[0], spec.description.input[0].name)

        self.assertEquals(32, spec.description.input[1].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.input[2].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.input[3].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.input[4].type.multiArrayType.shape[0])

        self.assertEquals(len(spec.description.output), len(output_names) + 4)
        self.assertEquals(output_names[0], spec.description.output[0].name)
        self.assertEquals(64, spec.description.output[0].type.multiArrayType.shape[0])

        self.assertEquals(32, spec.description.output[1].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.output[2].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.output[3].type.multiArrayType.shape[0])
        self.assertEquals(32, spec.description.output[4].type.multiArrayType.shape[0])

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.biDirectionalLSTM)
        self.assertEquals(len(layer_0.input), 5)
        self.assertEquals(len(layer_0.output), 5)

    def test_embedding(self):
        from keras.layers import Embedding

        model = Sequential()
        num_inputs = 10
        num_outputs = 3
        model.add(Embedding(num_inputs, num_outputs, input_length=5))

        input_names = ["input"]
        output_names = ["output"]

        spec = keras.convert(model, input_names, output_names).get_spec()

        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        layer_0 = layers[0]
        self.assertIsNotNone(layer_0.embedding)

        self.assertEquals(layer_0.embedding.inputDim, num_inputs)
        self.assertEquals(layer_0.embedding.outputChannels, num_outputs)

        self.assertEquals(
            len(layer_0.embedding.weights.floatValue), num_inputs * num_outputs
        )

    def test_sentiment_analysis(self):
        """
        Test the conversion for a Embedding + LSTM + Dense layer
        """
        from keras.layers import Dense, Embedding, LSTM

        # Create a simple Keras model
        max_features = 50
        embedded_dim = 32
        sequence_length = 10

        model = Sequential()
        # Embedding layer example:
        # Embedding(1000, 64, input_length=10) input_dim=index(0~999), 64-dimensional vector, sequence length = 10
        # If we have Dense/Flatten layer upstream, input_length, a.k.a sequence_length is required

        model.add(Embedding(max_features, embedded_dim, input_length=sequence_length))
        # output_dim = 32
        model.add(LSTM(32))
        model.add(Dense(1, activation="sigmoid"))

        # Input/output
        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        # We're giving state input and output so expect description to differ.
        self.assertEquals(len(spec.description.input), len(input_names) + 2)
        self.assertEquals(len(spec.description.output), len(output_names) + 2)

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[0].embedding)
        self.assertIsNotNone(layers[1].uniDirectionalLSTM)
        self.assertIsNotNone(layers[2].innerProduct)

    def test_conv1d_lstm(self):
        from keras.layers import Conv1D, LSTM, Dense

        model = Sequential()
        # input_shape = (time_step, dimensions)
        model.add(Conv1D(32, 3, padding="same", input_shape=(10, 8)))
        # conv1d output shape = (None, 10, 32)
        model.add(LSTM(24))
        model.add(Dense(1, activation="sigmoid"))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()

        self.assertIsNotNone(spec)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names) + 2)
        self.assertEquals(len(spec.description.output), len(output_names) + 2)

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[0].convolution)
        self.assertIsNotNone(layers[1].simpleRecurrent)
        self.assertIsNotNone(layers[2].innerProduct)

    def test_batchnorm(self):
        """
        Test the conversion for a Convoultion2D + Batchnorm layer
        """
        from keras.layers import Conv2D
        from keras.layers.normalization import BatchNormalization

        # Create a simple Keras model
        model = Sequential()
        model.add(
            Conv2D(
                input_shape=(64, 64, 3),
                filters=32,
                kernel_size=(5, 5),
                strides=(1, 1),
                activation=None,
                padding="valid",
                use_bias=True,
            )
        )
        # epsilon in CoreML is currently fixed at 1e-5
        model.add(BatchNormalization(epsilon=1e-5))
        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )

        # Test the layer parameters.
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[0].convolution)
        self.assertIsNotNone(layers[1].batchnorm)

    def test_repeat_vector(self):
        from keras.layers import RepeatVector

        model = Sequential()
        model.add(RepeatVector(3, input_shape=(5,)))

        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(model, input_names, output_names).get_spec()
        self.assertIsNotNone(spec)
        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))
        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(output_names))
        self.assertEqual(
            sorted(output_names), sorted(map(lambda x: x.name, spec.description.output))
        )
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[0].sequenceRepeat)

    @pytest.mark.xfail(raises=ValueError)
    def test_unsupported_variational_deconv(self):
        from keras.layers import Input, Lambda, Conv2D, Flatten, Dense

        x = Input(shape=(8, 8, 3))
        conv_1 = Conv2D(4, (2, 2), padding="same", activation="relu")(x)
        flat = Flatten()(conv_1)
        hidden = Dense(10, activation="relu")(flat)
        z_mean = Dense(10)(hidden)
        z_log_var = Dense(10)(hidden)

        def sampling(args):
            z_mean, z_log_var = args
            return z_mean + z_log_var

        z = Lambda(sampling, output_shape=(10,))([z_mean, z_log_var])
        model = Model([x], [z])
        spec = keras.convert(model, ["input"], ["output"]).get_spec()

    def test_image_processing(self):
        """
        Test the image-processing parameters.
        """
        from keras.layers import Conv2D

        # Create a simple Keras model
        model = Sequential()
        model.add(
            Conv2D(
                input_shape=(64, 64, 3),
                filters=32,
                kernel_size=(5, 5),
                activation=None,
                padding="valid",
                strides=(1, 1),
                use_bias=True,
            )
        )
        input_names = ["input"]
        output_names = ["output"]
        spec = keras.convert(
            model,
            input_names,
            output_names,
            image_input_names=["input"],
            red_bias=110.0,
            blue_bias=117.0,
            green_bias=120.0,
            is_bgr=True,
            image_scale=1.0,
        ).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))
        self.assertEquals(
            spec.description.input[0].type.WhichOneof("Type"), "imageType"
        )
        self.assertEquals(
            spec.description.input[0].type.imageType.colorSpace,
            FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value("BGR"),
        )

        # Test the layer parameters.
        preprocessing = spec.neuralNetwork.preprocessing[0]
        self.assertTrue(preprocessing.HasField("scaler"))
        pr_0 = preprocessing.scaler
        print("pr_0.channelScale = ", pr_0.channelScale)
        print("pr_0.redBias = ", pr_0.redBias)
        print("pr_0.blueBias = ", pr_0.blueBias)
        print("pr_0.greenBias = ", pr_0.greenBias)
        self.assertIsNotNone(pr_0.redBias)
        self.assertIsNotNone(pr_0.greenBias)
        self.assertIsNotNone(pr_0.blueBias)
        self.assertIsNotNone(pr_0.channelScale)
        self.assertEqual(pr_0.channelScale, 1.0)
        self.assertEqual(pr_0.redBias, 110.0)
        self.assertEqual(pr_0.blueBias, 117.0)
        self.assertEqual(pr_0.greenBias, 120.0)

        # Configuration 2: isbgr = False
        spec = keras.convert(
            model,
            input_names,
            output_names,
            image_input_names=["input"],
            red_bias=110.0,
            blue_bias=117.0,
            green_bias=120.0,
            is_bgr=False,
            image_scale=1.0,
        ).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))
        self.assertEquals(
            spec.description.input[0].type.WhichOneof("Type"), "imageType"
        )
        self.assertEquals(
            spec.description.input[0].type.imageType.colorSpace,
            FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value("RGB"),
        )

        # Test the layer parameters.
        preprocessing = spec.neuralNetwork.preprocessing[0]
        self.assertTrue(preprocessing.HasField("scaler"))
        pr_0 = preprocessing.scaler
        self.assertIsNotNone(pr_0.redBias)
        self.assertIsNotNone(pr_0.greenBias)
        self.assertIsNotNone(pr_0.blueBias)
        self.assertIsNotNone(pr_0.channelScale)
        self.assertEqual(pr_0.channelScale, 1.0)
        self.assertEqual(pr_0.redBias, 110.0)
        self.assertEqual(pr_0.blueBias, 117.0)
        self.assertEqual(pr_0.greenBias, 120.0)

        # Configuration 3: Defaults
        spec = keras.convert(
            model,
            input_names,
            output_names,
            image_input_names=["input"],
            is_bgr=False,
            image_scale=1.0,
        ).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))
        self.assertEquals(
            spec.description.input[0].type.WhichOneof("Type"), "imageType"
        )
        self.assertEquals(
            spec.description.input[0].type.imageType.colorSpace,
            FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value("RGB"),
        )

        # Test the layer parameters.
        preprocessing = spec.neuralNetwork.preprocessing[0]
        self.assertTrue(preprocessing.HasField("scaler"))
        pr_0 = preprocessing.scaler
        self.assertIsNotNone(pr_0.redBias)
        self.assertIsNotNone(pr_0.greenBias)
        self.assertIsNotNone(pr_0.blueBias)
        self.assertIsNotNone(pr_0.channelScale)
        self.assertEqual(pr_0.channelScale, 1.0)
        self.assertEqual(pr_0.redBias, 0.0)
        self.assertEqual(pr_0.blueBias, 0.0)
        self.assertEqual(pr_0.greenBias, 0.0)

    def test_classifier_string_classes(self):
        from keras.layers import Dense
        from keras.layers import Activation

        # Create a simple Keras model
        model = Sequential()
        model.add(Dense(32, input_shape=(16,)))
        model.add(Activation("softmax"))
        classes = ["c%s" % i for i in range(32)]

        input_names = ["input"]
        output_names = ["prob_output"]
        expected_output_names = ["prob_output", "classLabel"]
        spec = keras.convert(
            model, input_names, output_names, class_labels=classes
        ).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetworkClassifier"))
        self.assertFalse(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(expected_output_names))
        self.assertEquals(
            expected_output_names, list(map(lambda x: x.name, spec.description.output))
        )

        # Check the types
        self.assertEquals(
            spec.description.output[0].type.WhichOneof("Type"), "dictionaryType"
        )
        self.assertEquals(
            spec.description.output[0].type.dictionaryType.WhichOneof("KeyType"),
            "stringKeyType",
        )
        self.assertEquals(
            spec.description.output[1].type.WhichOneof("Type"), "stringType"
        )
        self.assertTrue(spec.description.predictedFeatureName, "classLabel")
        self.assertTrue(spec.description.predictedProbabilitiesName, "prob_output")

        # Test the class parameters
        self.assertEqual(
            spec.WhichOneof("Type"),
            "neuralNetworkClassifier",
            "Expected a NN classifier model",
        )
        self.assertEqual(
            spec.neuralNetworkClassifier.WhichOneof("ClassLabels"), "stringClassLabels"
        )
        class_from_proto = list(spec.neuralNetworkClassifier.stringClassLabels.vector)
        self.assertEqual(sorted(classes), sorted(class_from_proto))

    def test_classifier_file(self):
        from keras.layers import Dense
        from keras.layers import Activation
        import os
        import tempfile

        # Create a simple Keras model
        model = Sequential()
        model.add(Dense(32, input_shape=(16,)))
        model.add(Activation("softmax"))
        classes = ["c%s" % i for i in range(32)]
        classes_file = tempfile.mktemp()
        with open(classes_file, "w") as f:
            f.write("\n".join(classes))

        input_names = ["input"]
        output_names = ["prob_output"]
        expected_output_names = ["prob_output", "classLabel"]
        spec = keras.convert(
            model, input_names, output_names, class_labels=classes
        ).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetworkClassifier"))
        self.assertFalse(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(expected_output_names))
        self.assertEquals(
            expected_output_names, list(map(lambda x: x.name, spec.description.output))
        )

        # Check the types
        self.assertEquals(
            spec.description.output[0].type.WhichOneof("Type"), "dictionaryType"
        )
        self.assertEquals(
            spec.description.output[0].type.dictionaryType.WhichOneof("KeyType"),
            "stringKeyType",
        )
        self.assertEquals(
            spec.description.output[1].type.WhichOneof("Type"), "stringType"
        )
        self.assertTrue(spec.description.predictedFeatureName, "classLabel")
        self.assertTrue(spec.description.predictedProbabilitiesName, "prob_output")

        # cleanup
        os.remove(classes_file)

    def test_classifier_integer_classes(self):
        from keras.layers import Dense
        from keras.layers import Activation

        # Create a simple Keras model
        model = Sequential()
        model.add(Dense(32, input_shape=(16,)))
        model.add(Activation("softmax"))
        classes = list(range(32))

        input_names = ["input"]
        output_names = ["prob_output"]
        expected_output_names = ["prob_output", "classLabel"]
        spec = keras.convert(
            model, input_names, output_names, class_labels=classes
        ).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetworkClassifier"))
        self.assertFalse(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(expected_output_names))
        self.assertEquals(
            expected_output_names, list(map(lambda x: x.name, spec.description.output))
        )

        # Check the types
        self.assertEquals(
            spec.description.output[0].type.WhichOneof("Type"), "dictionaryType"
        )
        self.assertEquals(
            spec.description.output[0].type.dictionaryType.WhichOneof("KeyType"),
            "int64KeyType",
        )
        self.assertEquals(
            spec.description.output[1].type.WhichOneof("Type"), "int64Type"
        )
        self.assertTrue(spec.description.predictedFeatureName, "classLabel")
        self.assertTrue(spec.description.predictedProbabilitiesName, "prob_output")

        # Test the class parameters
        self.assertEqual(
            spec.WhichOneof("Type"),
            "neuralNetworkClassifier",
            "Expected a NN classifier model",
        )
        self.assertEqual(
            spec.neuralNetworkClassifier.WhichOneof("ClassLabels"), "int64ClassLabels"
        )
        class_from_proto = list(spec.neuralNetworkClassifier.int64ClassLabels.vector)
        self.assertEqual(sorted(classes), sorted(class_from_proto))

    def test_classifier_custom_class_name(self):
        from keras.layers import Dense
        from keras.layers import Activation

        # Create a simple Keras model
        model = Sequential()
        model.add(Dense(32, input_shape=(16,)))
        model.add(Activation("softmax"))
        classes = ["c%s" % i for i in range(32)]

        input_names = ["input"]
        output_names = ["prob_output"]
        expected_output_names = ["prob_output", "my_foo_bar_class_output"]
        spec = keras.convert(
            model,
            input_names,
            output_names,
            class_labels=classes,
            predicted_feature_name="my_foo_bar_class_output",
        ).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetworkClassifier"))
        self.assertFalse(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(input_names))
        self.assertEqual(
            sorted(input_names), sorted(map(lambda x: x.name, spec.description.input))
        )
        self.assertEquals(len(spec.description.output), len(expected_output_names))
        self.assertEquals(
            expected_output_names, list(map(lambda x: x.name, spec.description.output))
        )

        # Check the types
        self.assertEquals(
            spec.description.output[0].type.WhichOneof("Type"), "dictionaryType"
        )
        self.assertEquals(
            spec.description.output[0].type.dictionaryType.WhichOneof("KeyType"),
            "stringKeyType",
        )
        self.assertEquals(
            spec.description.output[1].type.WhichOneof("Type"), "stringType"
        )
        self.assertTrue(
            spec.description.predictedFeatureName, "my_foo_bar_class_output"
        )
        self.assertTrue(spec.description.predictedProbabilitiesName, "prob_output")

        # Test the class parameters
        self.assertEqual(
            spec.WhichOneof("Type"),
            "neuralNetworkClassifier",
            "Expected a NN classifier model",
        )
        self.assertEqual(
            spec.neuralNetworkClassifier.WhichOneof("ClassLabels"), "stringClassLabels"
        )
        class_from_proto = list(spec.neuralNetworkClassifier.stringClassLabels.vector)
        self.assertEqual(sorted(classes), sorted(class_from_proto))

    def test_default_interface_names(self):
        from keras.layers import Dense
        from keras.layers import Activation

        # Create a simple Keras model
        model = Sequential()
        model.add(Dense(32, input_shape=(16,)))
        model.add(Activation("softmax"))

        expected_input_names = ["input1"]
        expected_output_names = ["output1"]
        spec = keras.convert(model).get_spec()
        self.assertIsNotNone(spec)

        # Test the model class
        self.assertIsNotNone(spec.description)
        self.assertTrue(spec.HasField("neuralNetwork"))

        # Test the inputs and outputs
        self.assertEquals(len(spec.description.input), len(expected_input_names))
        self.assertEqual(
            sorted(expected_input_names),
            sorted(map(lambda x: x.name, spec.description.input)),
        )
        self.assertEquals(len(spec.description.output), len(expected_output_names))
        self.assertEquals(
            sorted(expected_output_names),
            sorted(map(lambda x: x.name, spec.description.output)),
        )

    def test_updatable_model_flag_off(self):
        """
        Test to ensure that when respect_trainable is off, then we will ignore
        any 'trainable' layers of the original network.
        """
        import coremltools
        from keras.layers import Dense
        from keras.losses import categorical_crossentropy
        from keras.optimizers import SGD

        input = ["data"]
        output = ["output"]
        # First, set respect_trainable to False and then check to make sure the
        # converted model is NOT updatable.
        not_updatable = Sequential()
        not_updatable.add(Dense(128, input_shape=(16,)))
        # layer is updatable, but the flag during convert is false, so that bit
        # must get dropped on the floor.
        not_updatable.add(Dense(10, name="foo", activation="softmax", trainable=True))
        not_updatable.compile(
            loss=categorical_crossentropy, optimizer=SGD(lr=0.01), metrics=["accuracy"]
        )
        cml = coremltools.converters.keras.convert(
            not_updatable, input, output, respect_trainable=False
        )
        spec = cml.get_spec()
        self.assertFalse(spec.isUpdatable)
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].innerProduct)
        self.assertTrue(layers[1].innerProduct)
        self.assertFalse(layers[1].isUpdatable)

    def test_updatable_model_flag_cce_sgd(self):
        """
        Test to ensure that respect_trainable is honored during convert of a
        model with categorical cross entropy loss and SGD optimizer.
        """
        import coremltools
        from keras.layers import Dense
        from keras.losses import categorical_crossentropy
        from keras.optimizers import SGD

        input = ["data"]
        output = ["output"]

        # This should result in an updatable model.
        updatable = Sequential()
        updatable.add(Dense(128, input_shape=(16,)))
        updatable.add(Dense(10, name="foo", activation="softmax", trainable=True))
        updatable.compile(
            loss=categorical_crossentropy, optimizer=SGD(lr=1.0), metrics=["accuracy"]
        )
        cml = coremltools.converters.keras.convert(
            updatable, input, output, respect_trainable=True
        )
        spec = cml.get_spec()
        self.assertTrue(spec.isUpdatable)
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].innerProduct)
        self.assertTrue(layers[1].innerProduct)
        self.assertTrue(layers[1].isUpdatable)
        self.assertEqual(len(spec.neuralNetwork.updateParams.lossLayers), 1)
        sgdopt = spec.neuralNetwork.updateParams.optimizer.sgdOptimizer
        self.assertEqual(sgdopt.learningRate.defaultValue, 1.0)
        self.assertEqual(sgdopt.miniBatchSize.defaultValue, 16)
        self.assertEqual(sgdopt.momentum.defaultValue, 0.0)

    def test_updatable_model_flag_functional(self):
        """
        Test to ensure that respect_trainable is honored during convert of a
        Keras model defined via the Keras functional API.
        """
        import coremltools
        from keras.layers import Dense, Input
        from keras.losses import categorical_crossentropy
        from keras.optimizers import SGD

        input = ["data"]
        output = ["output"]

        # This should result in an updatable model.
        inputs = Input(shape=(16,))
        d1 = Dense(128)(inputs)
        d2 = Dense(10, name="foo", activation="softmax", trainable=True)(d1)
        kmodel = Model(inputs=inputs, outputs=d2)
        kmodel.compile(
            loss=categorical_crossentropy, optimizer=SGD(lr=1.0), metrics=["accuracy"]
        )
        cml = coremltools.converters.keras.convert(
            kmodel, input, output, respect_trainable=True
        )
        spec = cml.get_spec()
        self.assertTrue(spec.isUpdatable)
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].innerProduct)
        self.assertTrue(layers[1].innerProduct)
        self.assertTrue(layers[1].isUpdatable)
        self.assertEqual(len(spec.neuralNetwork.updateParams.lossLayers), 1)
        sgdopt = spec.neuralNetwork.updateParams.optimizer.sgdOptimizer
        self.assertEqual(sgdopt.learningRate.defaultValue, 1.0)
        self.assertEqual(sgdopt.miniBatchSize.defaultValue, 16)
        self.assertEqual(sgdopt.momentum.defaultValue, 0.0)

    def test_updatable_model_flag_mse_adam(self):
        """
        Test to ensure that respect_trainable is honored during convert of a
        model with mean squared error loss and the Adam optimizer.
        """
        import coremltools
        from keras.layers import Dense
        from keras.losses import mean_squared_error
        from keras.optimizers import Adam

        input = ["data"]
        output = ["output"]

        # Again, this should give an updatable model.
        updatable = Sequential()
        updatable.add(Dense(128, input_shape=(16,)))
        updatable.add(Dense(10, name="foo", activation="softmax", trainable=True))
        updatable.compile(
            loss=mean_squared_error,
            optimizer=Adam(lr=1.0, beta_1=0.5, beta_2=0.75, epsilon=0.25),
            metrics=["accuracy"],
        )
        cml = coremltools.converters.keras.convert(
            updatable, input, output, respect_trainable=True
        )
        spec = cml.get_spec()
        self.assertTrue(spec.isUpdatable)
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].innerProduct)
        self.assertTrue(layers[1].innerProduct)
        self.assertTrue(layers[1].isUpdatable)
        self.assertEqual(len(spec.neuralNetwork.updateParams.lossLayers), 1)
        adopt = spec.neuralNetwork.updateParams.optimizer.adamOptimizer
        self.assertEqual(adopt.learningRate.defaultValue, 1.0)
        self.assertEqual(adopt.beta1.defaultValue, 0.5)
        self.assertEqual(adopt.beta2.defaultValue, 0.75)
        self.assertEqual(adopt.eps.defaultValue, 0.25)

    def test_updatable_model_flag_no_loss_optimizer(self):
        """
        Tests the 'respect_trainable' flag on models that have not been
        compiled, and thus do not have a loss function or optimizer.
        """
        import coremltools
        from keras.layers import Dense

        updatable = Sequential()
        updatable.add(Dense(128, input_shape=(16,)))
        updatable.add(Dense(10, name="foo", activation="softmax", trainable=True))
        input = ["data"]
        output = ["output"]
        cml = coremltools.converters.keras.convert(
            updatable, input, output, respect_trainable=True
        )
        spec = cml.get_spec()
        self.assertTrue(spec.isUpdatable)
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].innerProduct)
        self.assertTrue(layers[1].innerProduct)
        self.assertTrue(layers[1].isUpdatable)

    # <rdar://problem/53688606>
    # when loss was specified as a string the converter had failed to work.
    def test_updatable_model_flag_mse_string_adam(self):
        """
        Tests the 'respect_trainable' flag when used along with string
        for the loss(here mse), conversion is successful
        """
        import coremltools
        from keras.layers import Dense
        from keras.optimizers import Adam

        updatable = Sequential()
        updatable.add(Dense(128, input_shape=(16,)))
        updatable.add(Dense(10, name="foo", activation="relu", trainable=True))
        updatable.compile(
            loss="mean_squared_error",
            optimizer=Adam(lr=1.0, beta_1=0.5, beta_2=0.75, epsilon=0.25),
            metrics=["accuracy"],
        )
        input = ["data"]
        output = ["output"]
        cml = coremltools.converters.keras.convert(
            updatable, input, output, respect_trainable=True
        )
        spec = cml.get_spec()
        self.assertTrue(spec.isUpdatable)
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].innerProduct)
        self.assertTrue(layers[1].innerProduct)
        self.assertTrue(layers[1].isUpdatable)

        self.assertEqual(len(spec.neuralNetwork.updateParams.lossLayers), 1)
        # check that mean squared error input name and output name is set
        # check length is non-zero for mse
        self.assertTrue(
            len(
                spec.neuralNetwork.updateParams.lossLayers[
                    0
                ].meanSquaredErrorLossLayer.input
            )
        )
        self.assertTrue(
            len(
                spec.neuralNetwork.updateParams.lossLayers[
                    0
                ].meanSquaredErrorLossLayer.target
            )
        )
        # check length is 0 for cce
        self.assertFalse(
            len(
                spec.neuralNetwork.updateParams.lossLayers[
                    0
                ].categoricalCrossEntropyLossLayer.input
            )
        )
        self.assertFalse(
            len(
                spec.neuralNetwork.updateParams.lossLayers[
                    0
                ].categoricalCrossEntropyLossLayer.target
            )
        )

        adopt = spec.neuralNetwork.updateParams.optimizer.adamOptimizer
        # verify default values
        self.assertEqual(adopt.learningRate.defaultValue, 1.0)
        self.assertEqual(adopt.beta1.defaultValue, 0.5)
        self.assertEqual(adopt.beta2.defaultValue, 0.75)
        self.assertEqual(adopt.eps.defaultValue, 0.25)

    # <rdar://problem/53688606>
    def test_updatable_model_flag_cce_string_sgd(self):
        """
        Tests the 'respect_trainable' flag when used along with string
        for the loss(here cce), conversion is successful
        """
        import coremltools
        from keras.layers import Dense
        from keras.optimizers import SGD

        updatable = Sequential()
        updatable.add(Dense(128, input_shape=(16,)))
        updatable.add(Dense(10, name="foo", activation="softmax", trainable=True))
        updatable.compile(
            loss="categorical_crossentropy", optimizer=SGD(lr=1.0), metrics=["accuracy"]
        )
        input = ["data"]
        output = ["output"]
        cml = coremltools.converters.keras.convert(
            updatable, input, output, respect_trainable=True
        )
        spec = cml.get_spec()
        self.assertTrue(spec.isUpdatable)
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].innerProduct)
        self.assertTrue(layers[1].innerProduct)
        self.assertTrue(layers[1].isUpdatable)
        self.assertEqual(len(spec.neuralNetwork.updateParams.lossLayers), 1)

        # check that cce input name and output name is set
        # check length is non-zero for cce
        self.assertTrue(
            len(
                spec.neuralNetwork.updateParams.lossLayers[
                    0
                ].categoricalCrossEntropyLossLayer.input
            )
        )
        self.assertTrue(
            len(
                spec.neuralNetwork.updateParams.lossLayers[
                    0
                ].categoricalCrossEntropyLossLayer.target
            )
        )
        # check length is 0 for mse
        self.assertFalse(
            len(
                spec.neuralNetwork.updateParams.lossLayers[
                    0
                ].meanSquaredErrorLossLayer.input
            )
        )
        self.assertFalse(
            len(
                spec.neuralNetwork.updateParams.lossLayers[
                    0
                ].meanSquaredErrorLossLayer.target
            )
        )

        sgdopt = spec.neuralNetwork.updateParams.optimizer.sgdOptimizer
        self.assertEqual(sgdopt.learningRate.defaultValue, 1.0)
        self.assertEqual(sgdopt.miniBatchSize.defaultValue, 16)
        self.assertEqual(sgdopt.momentum.defaultValue, 0.0)

    def test_updatable_model_flag_cce_sgd_string(self):
        """
        Tests the 'respect_trainable' flag when used along with string
        for the optimizer(keras internally creates an instance, here sgd),
        conversion is successful
        """
        import coremltools
        from keras.layers import Dense, Input
        from keras.losses import categorical_crossentropy

        input = ["data"]
        output = ["output"]

        # This should result in an updatable model.
        inputs = Input(shape=(16,))
        d1 = Dense(128)(inputs)
        d2 = Dense(10, name="foo", activation="softmax", trainable=True)(d1)
        kmodel = Model(inputs=inputs, outputs=d2)
        kmodel.compile(
            loss=categorical_crossentropy, optimizer="sgd", metrics=["accuracy"]
        )
        cml = coremltools.converters.keras.convert(
            kmodel, input, output, respect_trainable=True
        )
        spec = cml.get_spec()
        self.assertTrue(spec.isUpdatable)
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].innerProduct)
        self.assertTrue(layers[1].innerProduct)
        self.assertTrue(layers[1].isUpdatable)
        self.assertEqual(len(spec.neuralNetwork.updateParams.lossLayers), 1)
        sgdopt = spec.neuralNetwork.updateParams.optimizer.sgdOptimizer
        # use almost equal for default verification with at least 5 decimal
        # places of closeness
        self.assertAlmostEqual(sgdopt.learningRate.defaultValue, 0.01, places=5)
        self.assertEqual(sgdopt.miniBatchSize.defaultValue, 16)
        self.assertEqual(sgdopt.momentum.defaultValue, 0.0)

    def test_updatable_model_flag_cce_adam_string(self):
        """
        Tests the 'respect_trainable' flag when used along with string
        for the optimizer(keras internally creates an instance, here adam),
        conversion is successful
        """
        import coremltools
        from keras.layers import Dense, Input
        from keras.losses import categorical_crossentropy

        input = ["data"]
        output = ["output"]

        # This should result in an updatable model.
        inputs = Input(shape=(16,))
        d1 = Dense(128)(inputs)
        d2 = Dense(10, name="foo", activation="softmax", trainable=True)(d1)
        kmodel = Model(inputs=inputs, outputs=d2)
        kmodel.compile(
            loss=categorical_crossentropy, optimizer="adam", metrics=["accuracy"]
        )
        cml = coremltools.converters.keras.convert(
            kmodel, input, output, respect_trainable=True
        )
        spec = cml.get_spec()
        self.assertTrue(spec.isUpdatable)
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[1].innerProduct)
        self.assertTrue(layers[1].innerProduct)
        self.assertTrue(layers[1].isUpdatable)
        self.assertEqual(len(spec.neuralNetwork.updateParams.lossLayers), 1)
        adopt = spec.neuralNetwork.updateParams.optimizer.adamOptimizer
        # use almost equal for default verification with at least 5 decimal
        # places of closeness
        self.assertAlmostEqual(adopt.learningRate.defaultValue, 0.001, places=5)
        self.assertAlmostEqual(adopt.miniBatchSize.defaultValue, 16)
        self.assertAlmostEqual(adopt.beta1.defaultValue, 0.90, places=5)
        self.assertAlmostEqual(adopt.beta2.defaultValue, 0.999, places=5)

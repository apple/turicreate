# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import itertools
import os
import shutil
import tempfile
import unittest

import numpy as np
import pytest

from coremltools._deps import _HAS_KERAS_TF
from coremltools.models.utils import _macos_version, _is_macos

if _HAS_KERAS_TF:
    from keras.models import Sequential, Model
    from keras.layers import (
        Dense,
        Activation,
        Convolution2D,
        AtrousConvolution2D,
        LSTM,
        ZeroPadding2D,
        Deconvolution2D,
        Permute,
        Convolution1D,
        AtrousConvolution1D,
        MaxPooling2D,
        AveragePooling2D,
        Flatten,
        Dropout,
        UpSampling2D,
        merge,
        Merge,
        Input,
        GRU,
        GlobalMaxPooling2D,
        GlobalMaxPooling1D,
        GlobalAveragePooling2D,
        GlobalAveragePooling1D,
        Cropping1D,
        Cropping2D,
        Reshape,
        AveragePooling1D,
        MaxPooling1D,
        RepeatVector,
        ELU,
        SimpleRNN,
        BatchNormalization,
        Embedding,
        ZeroPadding1D,
        UpSampling1D,
    )
    from keras.layers.wrappers import Bidirectional, TimeDistributed


def _keras_transpose(x, is_sequence=False):
    if len(x.shape) == 4:
        # Keras input shape = [Batch, Height, Width, Channels]
        x = np.transpose(x, [0, 3, 1, 2])
        return np.expand_dims(x, axis=0)
    elif len(x.shape) == 3:
        # Keras input shape = [Batch, (Sequence) Length, Channels]
        return np.transpose(x, [1, 0, 2])
    elif len(x.shape) == 2:
        if is_sequence:  # (N,S) --> (S,N,1,)
            return x.reshape(x.shape[::-1] + (1,))
        else:  # (N,C) --> (N,C,1,1)
            return x.reshape((1,) + x.shape)  # Dense
    elif len(x.shape) == 1:
        if is_sequence:  # (S) --> (S,N,1,1,1)
            return x.reshape((x.shape[0], 1, 1))
        else:
            return x
    else:
        return x


def _get_coreml_model(model, model_path, input_names, output_names):
    """
    Get the coreml model from the Keras model.
    """
    # Convert the model
    from coremltools.converters import keras as keras_converter

    model = keras_converter.convert(model, input_names, output_names)
    return model


def _generate_data(input_shape, mode="random"):
    """
    Generate some random data according to a shape.
    """
    if mode == "zeros":
        X = np.zeros(input_shape)
    elif mode == "ones":
        X = np.ones(input_shape)
    elif mode == "linear":
        X = np.array(range(np.product(input_shape))).reshape(input_shape)
    elif mode == "random":
        X = np.random.rand(*input_shape)
    elif mode == "random_zero_mean":
        X = np.random.rand(*input_shape) - 0.5
    return X


def conv2d_bn(
    x, nb_filter, nb_row, nb_col, border_mode="same", subsample=(1, 1), name=None
):
    """
    Utility function to apply conv + BN.
    """
    if name is not None:
        bn_name = name + "_bn"
        conv_name = name + "_conv"
    else:
        bn_name = None
        conv_name = None
    bn_axis = 3
    x = Convolution2D(
        nb_filter,
        nb_row,
        nb_col,
        subsample=subsample,
        activation="relu",
        border_mode=border_mode,
        name=conv_name,
    )(x)
    x = BatchNormalization(axis=bn_axis, name=bn_name)(x)
    return x


@unittest.skipIf(not _HAS_KERAS_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras1
class KerasNumericCorrectnessTest(unittest.TestCase):
    """
    Unit test class for testing the Keras converter.
    """

    def _test_keras_model(
        self,
        model,
        num_samples=1,
        mode="random",
        input_blob="data",
        output_blob="output",
        delta=1e-2,
        model_dir=None,
        transpose_keras_result=True,
        one_dim_seq_flags=None,
    ):

        # transpose_keras_result: if true, compare the transposed Keras result
        # one_dim_seq_flags: a list of same length as the number of inputs in
        # the model; if None, treat all 1D input (if any) as non-sequence
        # if one_dim_seq_flags[i] is True, it means the ith input, with shape
        # (X,) is in fact a sequence of length X.

        # Get the CoreML model
        use_tmp_folder = False
        if model_dir is None:
            use_tmp_folder = True
            model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, "keras.mlmodel")

        # Generate data
        nb_inputs = len(model.inputs)
        if nb_inputs > 1:
            input_names = []
            input_data = []
            coreml_input = {}
            for i in range(nb_inputs):
                input_shape = [1 if a is None else a for a in model.input_shape[i]]
                X = _generate_data(input_shape, mode)
                feature_name = "data_%s" % i
                input_names.append(feature_name)
                input_data.append(X)
                if one_dim_seq_flags is None:
                    coreml_input[feature_name] = _keras_transpose(X).astype("f").copy()
                else:
                    coreml_input[feature_name] = (
                        _keras_transpose(X, one_dim_seq_flags[i]).astype("f").copy()
                    )
        else:
            input_shape = [1 if a is None else a for a in model.input_shape]
            input_names = ["data"]
            input_data = _generate_data(input_shape, mode)
            if one_dim_seq_flags is None:
                coreml_input = {"data": _keras_transpose(input_data).astype("f").copy()}
            else:
                coreml_input = {
                    "data": _keras_transpose(input_data, one_dim_seq_flags[0])
                    .astype("f")
                    .copy()
                }

        # Compile the model
        output_names = ["output" + str(i) for i in range(len(model.outputs))]
        coreml_model = _get_coreml_model(model, model_path, input_names, output_names)

        if _is_macos() and _macos_version() >= (10, 13):
            # Assuming coreml model output names are in the same order as Keras
            # Output list, put predictions into a list, sorted by output name
            coreml_preds = coreml_model.predict(coreml_input)
            c_preds = [coreml_preds[name] for name in output_names]

            # Run Keras predictions
            keras_preds = model.predict(input_data)
            k_preds = keras_preds if type(keras_preds) is list else [keras_preds]

            # Compare each output blob
            for idx, k_pred in enumerate(k_preds):
                if transpose_keras_result:
                    kp = _keras_transpose(k_pred).flatten()
                else:
                    kp = k_pred.flatten()
                cp = c_preds[idx].flatten()
                # Compare predictions
                self.assertEquals(len(kp), len(cp))
                for i in range(len(kp)):
                    max_den = max(1.0, kp[i], cp[i])
                    self.assertAlmostEquals(
                        kp[i] / max_den, cp[i] / max_den, delta=delta
                    )

        # Cleanup files - models on disk no longer useful
        if use_tmp_folder and os.path.exists(model_dir):
            shutil.rmtree(model_dir)


@unittest.skipIf(not _HAS_KERAS_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras1
class KerasBasicNumericCorrectnessTest(KerasNumericCorrectnessTest):
    def test_tiny_inner_product_zero_input(self):
        np.random.seed(1988)
        input_dim = 2
        num_channels = 2

        # Define a model
        model = Sequential()
        model.add(Dense(num_channels, input_dim=input_dim))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, mode="zeros")

    def test_tiny_inner_product_ones(self):
        np.random.seed(1988)
        input_dim = 2
        num_channels = 2

        # Define a model
        model = Sequential()
        model.add(Dense(num_channels, input_dim=input_dim))

        # Set some random weights
        model.set_weights([np.ones(w.shape) for w in model.get_weights()])

        # test the keras model
        self._test_keras_model(model, mode="ones")

    def test_tiny_inner_product_random(self):
        np.random.seed(1988)
        input_dim = 2
        num_channels = 2

        # Define a model
        model = Sequential()
        model.add(Dense(num_channels, input_dim=input_dim))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_inner_product_random(self):
        np.random.seed(1988)
        input_dim = 100
        input_shape = (input_dim,)
        num_channels = 100

        # Define a model
        model = Sequential()
        model.add(Dense(num_channels, input_dim=input_dim))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv_ones(self):
        np.random.seed(1988)
        input_dim = 10
        input_shape = (input_dim, input_dim, 1)
        num_kernels = 3
        kernel_height = 5
        kernel_width = 5

        # Define a model
        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=input_shape,
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
            )
        )

        # Set some random weights
        model.set_weights([np.ones(w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_shape = (input_dim, input_dim, 1)
        num_kernels = 3
        kernel_height = 5
        kernel_width = 5

        # Define a model
        model = Sequential()
        model.add(
            AtrousConvolution2D(
                num_kernels, kernel_height, kernel_width, input_shape=input_shape
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_atrous_conv_random(self):
        np.random.seed(1988)
        input_dim = 8
        input_shape = (input_dim, input_dim, 1)
        num_kernels = 2
        kernel_height = 3
        kernel_width = 3

        # Define a model
        model = Sequential()
        model.add(
            AtrousConvolution2D(
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
                input_shape=input_shape,
                atrous_rate=(2, 2),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_atrous_conv_rect_random(self):
        np.random.seed(1988)
        input_shape = (32, 20, 1)
        num_kernels = 2
        kernel_height = 3
        kernel_width = 3

        # Define a model
        model = Sequential()
        model.add(
            AtrousConvolution2D(
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
                input_shape=input_shape,
                atrous_rate=(3, 3),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv_rect_kernel_x(self):
        np.random.seed(1988)
        input_dim = 10
        input_shape = (input_dim, input_dim, 1)
        num_kernels = 3
        kernel_height = 1
        kernel_width = 5

        # Define a model
        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=input_shape,
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
                border_mode="same",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv_rect_kernel_y(self):
        np.random.seed(1988)
        input_dim = 10
        input_shape = (input_dim, input_dim, 1)
        num_kernels = 3
        kernel_height = 5
        kernel_width = 1

        # Define a model
        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=input_shape,
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
                border_mode="valid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv_rect_kernel_xy(self):
        np.random.seed(1988)
        input_dim = 10
        input_shape = (input_dim, input_dim, 1)
        num_kernels = 3
        kernel_height = 5
        kernel_width = 3

        # Define a model
        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=input_shape,
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
                border_mode="valid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv_pseudo_1d_x(self):
        np.random.seed(1988)
        input_dim = 2
        input_length = 5
        filter_length = 1  # 3
        nb_filters = 1
        # Define a model
        model = Sequential()
        model.add(
            Convolution2D(
                nb_filters,
                1,
                filter_length,
                input_shape=(1, input_length, input_dim),
                border_mode="valid",
            )
        )
        # Set some random weights
        model.set_weights([np.ones(w.shape) for w in model.get_weights()])
        self._test_keras_model(model, mode="linear")

    def test_tiny_conv1d_same_random(self):
        np.random.seed(1988)
        input_dim = 2
        input_length = 10
        filter_length = 3
        nb_filters = 4
        model = Sequential()
        model.add(
            Convolution1D(
                nb_filters,
                filter_length,
                border_mode="same",
                input_shape=(input_length, input_dim),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv1d_valid_random(self):
        np.random.seed(1988)
        input_dim = 2
        input_length = 10
        filter_length = 3
        nb_filters = 4
        model = Sequential()
        model.add(
            Convolution1D(
                nb_filters,
                filter_length,
                border_mode="valid",
                input_shape=(input_length, input_dim),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_atrous_conv1d_random(self):
        np.random.seed(1988)
        input_dim = 8
        input_shape = (input_dim, 1)
        num_kernels = 2
        kernel_length = 3

        # Define a model
        model = Sequential()
        model.add(
            AtrousConvolution1D(
                nb_filter=num_kernels,
                filter_length=kernel_length,
                input_shape=input_shape,
                atrous_rate=2,
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_deconv_random(self):
        np.random.seed(1988)
        input_dim = 13
        output_dim = 28
        input_shape = (input_dim, input_dim, 3)
        num_kernels = 16
        kernel_height = 3
        kernel_width = 3
        output_shape = (None, output_dim, output_dim, num_kernels)

        # Define a model
        model = Sequential()
        model.add(
            Deconvolution2D(
                num_kernels,
                kernel_width,
                kernel_height,
                input_shape=input_shape,
                output_shape=output_shape,
                border_mode="valid",
                subsample=(2, 2),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_deconv_random_same_padding(self):
        np.random.seed(1988)
        input_dim = 14
        output_dim = 28
        input_shape = (input_dim, input_dim, 3)
        num_kernels = 16
        kernel_height = 3
        kernel_width = 3
        output_shape = (None, output_dim, output_dim, num_kernels)

        # Define a model
        model = Sequential()
        model.add(
            Deconvolution2D(
                num_kernels,
                kernel_width,
                kernel_height,
                input_shape=input_shape,
                output_shape=output_shape,
                border_mode="same",
                subsample=(2, 2),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv_upsample_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_shape = (input_dim, input_dim, 1)
        num_kernels = 3
        kernel_height = 5
        kernel_width = 5

        # Define a model
        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=input_shape,
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
            )
        )
        model.add(UpSampling2D(size=(2, 2)))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_housenet_random(self):
        np.random.seed(1988)
        num_hidden = 2
        num_features = 3

        # Define a model
        model = Sequential()
        model.add(Dense(num_hidden, input_dim=num_features))
        model.add(Activation("relu"))
        model.add(Dense(1, input_dim=num_features))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_no_sequence_lstm_zeros(self):
        np.random.seed(1988)
        input_dim = 1
        input_length = 1
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                consume_less="cpu",
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(
            model, mode="zeros", input_blob="data", output_blob="output"
        )

    def test_tiny_no_sequence_lstm_zeros_gpu(self):
        np.random.seed(1988)
        input_dim = 1
        input_length = 1
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                consume_less="gpu",
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(
            model, mode="zeros", input_blob="data", output_blob="output"
        )

    def test_tiny_no_sequence_lstm_ones(self):
        np.random.seed(1988)
        input_dim = 1
        input_length = 1
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                consume_less="cpu",
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(
            model, mode="ones", input_blob="data", output_blob="output"
        )

    def test_small_no_sequence_lstm_zeros(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                consume_less="gpu",
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(
            model, mode="zeros", input_blob="data", output_blob="output"
        )

    def test_small_no_sequence_lstm_ones(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                consume_less="gpu",
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(
            model, mode="ones", input_blob="data", output_blob="output"
        )

    def test_tiny_no_sequence_simple_rnn_random(self):
        np.random.seed(1988)
        input_dim = 1
        input_length = 1
        num_channels = 1
        num_samples = 1

        # Define a model
        model = Sequential()
        model.add(
            SimpleRNN(num_channels, input_dim=input_dim, input_length=input_length)
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_tiny_no_sequence_gru_random(self):
        np.random.seed(1988)
        input_dim = 1
        input_length = 1
        num_channels = 1
        num_samples = 1

        # Define a model
        model = Sequential()
        model.add(
            GRU(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_tiny_no_sequence_bidir_random(self):
        np.random.seed(1988)
        input_dim = 1
        input_length = 1
        num_channels = 1
        num_samples = 1

        # Define a model
        model = Sequential()
        model.add(
            Bidirectional(
                LSTM(
                    num_channels,
                    input_dim=input_dim,
                    input_length=input_length,
                    consume_less="cpu",
                    inner_activation="sigmoid",
                ),
                input_shape=(input_length, input_dim),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_tiny_no_sequence_bidir_random_gpu(self):
        np.random.seed(1988)
        input_dim = 1
        input_length = 1
        num_channels = 1
        num_samples = 1

        # Define a model
        model = Sequential()
        model.add(
            Bidirectional(
                LSTM(
                    num_channels,
                    input_dim=input_dim,
                    input_length=input_length,
                    consume_less="gpu",
                    inner_activation="sigmoid",
                ),
                input_shape=(input_length, input_dim),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_small_no_sequence_lstm_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                consume_less="gpu",
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_small_no_sequence_gru_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            GRU(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_small_no_sequence_bidir_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            Bidirectional(
                LSTM(
                    num_channels,
                    input_dim=input_dim,
                    input_length=input_length,
                    consume_less="gpu",
                    inner_activation="sigmoid",
                ),
                input_shape=(input_length, input_dim),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_small_no_sequence_simple_rnn_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            SimpleRNN(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                consume_less="gpu",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_medium_no_sequence_lstm_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 10

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_medium_no_sequence_bidir_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 10

        # Define a model
        model = Sequential()
        model.add(
            Bidirectional(
                LSTM(
                    num_channels,
                    input_dim=input_dim,
                    input_length=input_length,
                    consume_less="gpu",
                    inner_activation="sigmoid",
                ),
                input_shape=(input_length, input_dim),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_medium_bidir_random_return_seq_false(self):
        np.random.seed(1988)
        input_dim = 7
        input_length = 5
        num_channels = 10

        # Define a model
        model = Sequential()
        model.add(
            Bidirectional(
                LSTM(
                    num_channels,
                    input_dim=input_dim,
                    input_length=input_length,
                    return_sequences=False,
                    consume_less="gpu",
                    inner_activation="sigmoid",
                ),
                input_shape=(input_length, input_dim),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_medium_bidir_random_return_seq_true(self):
        np.random.seed(1988)
        input_dim = 7
        input_length = 5
        num_channels = 10

        # Define a model
        model = Sequential()
        model.add(
            Bidirectional(
                LSTM(
                    num_channels,
                    input_dim=input_dim,
                    input_length=input_length,
                    return_sequences=True,
                    consume_less="gpu",
                    inner_activation="sigmoid",
                ),
                input_shape=(input_length, input_dim),
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_lstm_seq(self):
        np.random.seed(1988)

        input_dim = 11
        input_length = 5

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                20,
                input_dim=input_dim,
                input_length=input_length,
                return_sequences=False,
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_lstm_seq_dense(self):
        np.random.seed(1988)

        input_dim = 5
        num_hidden = 12
        num_classes = 6
        input_length = 3

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                num_hidden,
                input_dim=input_dim,
                input_length=input_length,
                return_sequences=False,
            )
        )
        model.add(Dense(num_classes, activation="softmax"))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_lstm_seq_backwards(self):
        np.random.seed(1988)

        input_dim = 11
        input_length = 5

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                20,
                input_dim=input_dim,
                input_length=input_length,
                return_sequences=False,
                go_backwards=True,
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_rnn_seq(self):
        np.random.seed(1988)

        input_dim = 11
        input_length = 5

        # Define a model
        model = Sequential()
        model.add(
            SimpleRNN(
                20,
                input_dim=input_dim,
                input_length=input_length,
                return_sequences=False,
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_rnn_seq_backwards(self):
        np.random.seed(1988)

        input_dim = 11
        input_length = 5

        # Define a model
        model = Sequential()
        model.add(
            SimpleRNN(
                20,
                input_dim=input_dim,
                input_length=input_length,
                return_sequences=False,
                go_backwards=True,
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_gru_seq(self):
        np.random.seed(1988)

        input_dim = 11
        input_length = 5

        # Define a model
        model = Sequential()
        model.add(
            GRU(
                20,
                input_dim=input_dim,
                input_length=input_length,
                return_sequences=False,
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_gru_seq_backwards(self):
        np.random.seed(1988)

        input_dim = 11
        input_length = 5

        # Define a model
        model = Sequential()
        model.add(
            GRU(
                20,
                input_dim=input_dim,
                input_length=input_length,
                return_sequences=False,
                go_backwards=True,
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_medium_no_sequence_simple_rnn_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 10

        # Define a model
        model = Sequential()
        model.add(
            SimpleRNN(num_channels, input_dim=input_dim, input_length=input_length)
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model, input_blob="data", output_blob="output")

    def test_medium_no_sequence_gru_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_length = 1
        num_channels = 10

        # Define a model
        model = Sequential()
        model.add(
            GRU(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model

    def test_medium_conv_batchnorm_random(self):
        np.random.seed(1988)
        input_dim = 10
        input_shape = (input_dim, input_dim, 3)
        num_kernels = 3
        kernel_height = 5
        kernel_width = 5
        data_mean = 2
        data_var = 1

        # Define a model
        from keras.layers.normalization import BatchNormalization

        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=input_shape,
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
            )
        )
        model.add(BatchNormalization(epsilon=1e-5))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_conv_elu_random(self):
        np.random.seed(1988)

        # Define a model
        from keras.layers.advanced_activations import ELU

        model = Sequential()
        model.add(
            Convolution2D(input_shape=(10, 10, 3), nb_filter=3, nb_row=5, nb_col=5)
        )
        model.add(ELU(alpha=0.8))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_conv_prelu_random(self):
        np.random.seed(1988)

        # Define a model
        from keras.layers.advanced_activations import PReLU

        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=(10, 10, 3),
                nb_filter=3,
                nb_row=5,
                nb_col=5,
                border_mode="same",
            )
        )
        model.add(PReLU(shared_axes=[1, 2]))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_conv_leaky_relu_random(self):
        np.random.seed(1988)

        # Define a model
        from keras.layers.advanced_activations import LeakyReLU

        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=(10, 10, 3),
                nb_filter=3,
                nb_row=5,
                nb_col=5,
                border_mode="same",
            )
        )
        model.add(LeakyReLU(alpha=0.3))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_parametric_softplus_random(self):
        input_shape = (8, 8, 3)  # (10,10,3)
        # Define a model
        from keras.layers.advanced_activations import ParametricSoftplus

        model = Sequential()
        model.add(ParametricSoftplus(input_shape=input_shape))

        alpha_per_channel = np.random.rand(3)
        beta_per_channel = np.random.rand(3)
        alphas = (
            np.repeat(alpha_per_channel, input_shape[0] * input_shape[1])
            .reshape(input_shape[::-1])
            .transpose((2, 1, 0))
        )
        betas = (
            np.repeat(beta_per_channel, input_shape[0] * input_shape[1])
            .reshape(input_shape[::-1])
            .transpose((2, 1, 0))
        )

        model.layers[0].set_weights([alphas, betas])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_conv_parametric_softplus_random(self):
        np.random.seed(1988)
        input_shape = (8, 8, 3)  # (10,10,3)
        nb_filters = 2
        output_shape = (8, 8, 2)
        # Define a model
        from keras.layers.advanced_activations import ParametricSoftplus

        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=(8, 8, 3),
                nb_filter=nb_filters,
                nb_row=3,
                nb_col=3,
                border_mode="same",
            )
        )
        model.add(ParametricSoftplus())

        # CoreML only takes 1-param per channel, so weights are set differently
        # model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])
        alpha_per_channel = np.random.rand(nb_filters)
        beta_per_channel = np.random.rand(nb_filters)
        alphas = (
            np.repeat(alpha_per_channel, output_shape[0] * output_shape[1])
            .reshape(output_shape[::-1])
            .transpose((2, 1, 0))
        )
        betas = (
            np.repeat(beta_per_channel, output_shape[0] * output_shape[1])
            .reshape(output_shape[::-1])
            .transpose((2, 1, 0))
        )
        model.layers[1].set_weights([alphas, betas])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_dense_parametric_softplus_random(self):
        np.random.seed(1988)

        # Define a model
        from keras.layers.advanced_activations import ParametricSoftplus

        model = Sequential()
        model.add(Dense(10, input_shape=(4,)))
        model.add(ParametricSoftplus())

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_conv_thresholded_relu_random(self):
        np.random.seed(1988)

        # Define a model
        from keras.layers.advanced_activations import ThresholdedReLU

        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=(10, 10, 3),
                nb_filter=3,
                nb_row=5,
                nb_col=5,
                border_mode="same",
            )
        )
        model.add(ThresholdedReLU(theta=0.8))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_concat_random(self):
        np.random.seed(1988)
        input_dim = 10
        num_channels = 6

        # Define a model
        input_tensor = Input(shape=(input_dim,))
        x1 = Dense(num_channels)(input_tensor)
        x2 = Dense(num_channels)(x1)
        x3 = Dense(num_channels)(x1)
        x4 = merge([x2, x3], mode="concat")
        x5 = Dense(num_channels)(x4)

        model = Model(input=[input_tensor], output=[x5])

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_concat_seq_random(self):
        np.random.seed(1988)
        max_features = 10
        embedding_dims = 4
        seq_len = 5
        num_channels = 6

        # Define a model
        input_tensor = Input(shape=(seq_len,))
        x1 = Embedding(max_features, embedding_dims)(input_tensor)
        x2 = Embedding(max_features, embedding_dims)(input_tensor)
        x3 = merge([x1, x2], mode="concat", concat_axis=1)

        model = Model(input=[input_tensor], output=[x3])

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model, one_dim_seq_flags=[True])

    def test_tiny_add_random(self):
        np.random.seed(1988)
        input_dim = 10
        num_channels = 6

        # Define a model
        input_tensor = Input(shape=(input_dim,))
        x1 = Dense(num_channels)(input_tensor)
        x2 = Dense(num_channels)(x1)
        x3 = Dense(num_channels)(x1)
        x4 = merge([x2, x3], mode="sum")
        x5 = Dense(num_channels)(x4)

        model = Model(input=[input_tensor], output=[x5])

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_mul_random(self):
        np.random.seed(1988)
        input_dim = 10
        num_channels = 6

        # Define a model
        input_tensor = Input(shape=(input_dim,))
        x1 = Dense(num_channels)(input_tensor)
        x2 = Dense(num_channels)(x1)
        x3 = Dense(num_channels)(x1)
        x4 = merge([x2, x3], mode="mul")
        x5 = Dense(num_channels)(x4)

        model = Model(input=[input_tensor], output=[x5])

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_cos_random(self):
        np.random.seed(1988)
        input_dim = 10
        num_channels = 6

        # Define a model
        input_tensor = Input(shape=(input_dim,))
        x1 = Dense(num_channels)(input_tensor)
        x2 = Dense(num_channels)(x1)
        x3 = Dense(num_channels)(x1)
        x4 = merge([x2, x3], mode="cos")
        x5 = Dense(num_channels)(x4)

        model = Model(input=[input_tensor], output=[x5])

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_zeropad_simple(self):
        input_shape = (48, 48, 3)
        model = Sequential()
        model.add(ZeroPadding2D((1, 1), input_shape=input_shape))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_zeropad_fancy(self):
        input_shape = (48, 48, 3)
        model = Sequential()
        model.add(ZeroPadding2D((2, 5), input_shape=input_shape))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_crop_simple(self):
        input_shape = (48, 48, 3)
        model = Sequential()
        model.add(Cropping2D(cropping=((2, 5), (2, 5)), input_shape=input_shape))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_permute(self):
        model = Sequential()
        model.add(Permute((3, 2, 1), input_shape=(4, 3, 2)))

        # When input blob is 3D array (D1, D2, D3), Keras assumes the axes' meaning is
        # (D1=H,D2=W,D3=C), while CoreML assumes (D1=C,D2=H,D3=W). However,
        # it's unclear after permutation, what the axes' meaning is for the output blob.
        # Since permutation done on (H,W,C) blobs usually is usually followed by
        # recurrent layers / Dense, we choose that the ouput axis order of CoreML is
        # the same as Keras after permutation.
        self._test_keras_model(model, transpose_keras_result=False)

    def test_max_pooling_no_overlap(self):
        # no_overlap: pool_size = strides
        model = Sequential()
        model.add(
            MaxPooling2D(
                input_shape=(16, 16, 3),
                pool_size=(2, 2),
                strides=None,
                border_mode="valid",
            )
        )
        self._test_keras_model(model)

    def test_max_pooling_overlap_multiple(self):
        # input shape is multiple of pool_size, strides != pool_size
        model = Sequential()
        model.add(
            MaxPooling2D(
                input_shape=(18, 18, 3),
                pool_size=(3, 3),
                strides=(2, 2),
                border_mode="valid",
            )
        )
        self._test_keras_model(model)

    def test_max_pooling_overlap_odd(self):
        model = Sequential()
        model.add(
            MaxPooling2D(
                input_shape=(16, 16, 3),
                pool_size=(3, 3),
                strides=(2, 2),
                border_mode="valid",
            )
        )
        self._test_keras_model(model)

    def test_max_pooling_overlap_same(self):
        model = Sequential()
        model.add(
            MaxPooling2D(
                input_shape=(16, 16, 3),
                pool_size=(3, 3),
                strides=(2, 2),
                border_mode="same",
            )
        )
        self._test_keras_model(model)

    def test_global_max_pooling(self):
        model = Sequential()
        model.add(GlobalMaxPooling2D(input_shape=(16, 16, 3)))
        self._test_keras_model(model)

    def test_max_pooling_1d(self):
        model = Sequential()
        model.add(MaxPooling1D(input_shape=(16, 3), pool_length=4))
        self._test_keras_model(model)

    def test_global_max_pooling_1d(self):
        np.random.seed(1988)
        input_dim = 2
        input_length = 10
        filter_length = 3
        nb_filters = 4
        model = Sequential()
        model.add(
            Convolution1D(
                nb_filters,
                filter_length,
                border_mode="same",
                input_shape=(input_length, input_dim),
            )
        )
        model.add(GlobalMaxPooling1D())
        self._test_keras_model(model)

    def test_average_pooling_no_overlap(self):
        # no_overlap: pool_size = strides
        model = Sequential()
        model.add(
            AveragePooling2D(
                input_shape=(16, 16, 3),
                pool_size=(2, 2),
                strides=None,
                border_mode="valid",
            )
        )
        self._test_keras_model(model, delta=1e-2)

    def test_average_pooling_inception_config_1(self):
        # no_overlap: pool_size = strides
        model = Sequential()
        model.add(
            AveragePooling2D(
                input_shape=(16, 16, 3),
                pool_size=(3, 3),
                strides=(1, 1),
                border_mode="same",
            )
        )
        self._test_keras_model(model, delta=1e-2)

    def test_global_average_pooling(self):
        model = Sequential()
        model.add(GlobalAveragePooling2D(input_shape=(16, 16, 3)))
        self._test_keras_model(model)

    def test_average_pooling_1d(self):
        np.random.seed(1988)
        input_dim = 2
        input_length = 10
        filter_length = 3
        nb_filters = 4
        model = Sequential()
        model.add(
            Convolution1D(
                nb_filters,
                filter_length,
                border_mode="same",
                input_shape=(input_length, input_dim),
            )
        )
        model.add(AveragePooling1D(pool_length=2))
        self._test_keras_model(model)

    def test_global_average_pooling_1d(self):
        np.random.seed(1988)
        input_dim = 2
        input_length = 10
        filter_length = 3
        nb_filters = 4
        model = Sequential()
        model.add(
            Convolution1D(
                nb_filters,
                filter_length,
                border_mode="same",
                input_shape=(input_length, input_dim),
            )
        )
        model.add(GlobalAveragePooling1D())
        self._test_keras_model(model)

    def test_tiny_conv_dense_random(self):
        np.random.seed(1988)
        num_samples = 1
        input_dim = 8
        input_shape = (input_dim, input_dim, 3)
        num_kernels = 2
        kernel_height = 5
        kernel_width = 5
        hidden_dim = 4

        # Define a model
        from keras.layers import Flatten

        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=input_shape,
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
            )
        )
        model.add(Flatten())
        model.add(Dense(hidden_dim))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_dense_tanh_fused_random(self):
        np.random.seed(1988)
        num_samples = 1
        input_dim = 3
        hidden_dim = 4

        # Define a model
        model = Sequential()
        model.add(Dense(hidden_dim, input_shape=(input_dim,), activation="tanh"))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_tiny_conv_relu_fused_random(self):
        np.random.seed(1988)
        num_samples = 1
        input_dim = 8
        input_shape = (input_dim, input_dim, 3)
        num_kernels = 2
        kernel_height = 5
        kernel_width = 5
        hidden_dim = 4

        # Define a model
        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=input_shape,
                nb_filter=num_kernels,
                nb_row=kernel_height,
                nb_col=kernel_width,
                activation="relu",
            )
        )

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Get the coreml model
        self._test_keras_model(model)

    def test_flatten(self):
        model = Sequential()
        model.add(Flatten(input_shape=(2, 2, 2)))
        self._test_keras_model(model, mode="linear")

    def test_reshape_3d(self):
        model = Sequential()
        model.add(Reshape((10, 1, 6), input_shape=(5, 4, 3)))
        self._test_keras_model(model, mode="linear")

    def test_embedding(self):
        model = Sequential()
        num_inputs = 10
        num_outputs = 3
        model.add(Embedding(num_inputs, num_outputs))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        self._test_keras_model(model)

    def test_embedding_seq(self):
        model = Sequential()
        num_inputs = 10
        num_outputs = 3
        model.add(Embedding(num_inputs, num_outputs, input_length=7))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        self._test_keras_model(model, one_dim_seq_flags=[True])

    def test_tiny_time_distrbuted(self):
        # as the first layer in a model
        model = Sequential()
        model.add(TimeDistributed(Dense(8), input_shape=(10, 16)))

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        self._test_keras_model(model)

    def test_tiny_sequence_lstm(self):
        np.random.seed(1988)
        input_dim = 1
        input_length = 2
        num_channels = 1

        # Define a model
        model = Sequential()
        model.add(
            LSTM(
                num_channels,
                input_dim=input_dim,
                input_length=input_length,
                consume_less="cpu",
                inner_activation="sigmoid",
            )
        )

        # Set some random weights
        model.set_weights(
            [(np.random.rand(*w.shape) - 0.5) / 5.0 for w in model.get_weights()]
        )

        # Test the keras model
        self._test_keras_model(
            model, input_blob="data", output_blob="output", delta=1e-4
        )

    def test_tiny_spatial_bn(self):
        np.random.seed(1988)
        x_in = Input(shape=(7, 7, 2))
        x = ZeroPadding2D(padding=(1, 1))(x_in)
        x = BatchNormalization(axis=2)(x)
        model = Model(x_in, x)

        self._test_keras_model(
            model, input_blob="data", output_blob="output", delta=1e-2
        )

    def test_dense_fused_act_in_td(self):
        np.random.seed(1988)
        x_in = Input(shape=(10, 2))
        x = TimeDistributed(Dense(6, activation="softmax"))(x_in)
        model = Model(x_in, x)

        self._test_keras_model(
            model, input_blob="data", output_blob="output", delta=1e-2
        )

    def test_tiny_conv_upsample_1d_random(self):
        np.random.seed(1988)
        input_dim = 2
        input_length = 10
        filter_length = 3
        nb_filters = 4
        model = Sequential()
        model.add(
            Convolution1D(
                nb_filters,
                filter_length=filter_length,
                border_mode="same",
                input_shape=(input_length, input_dim),
            )
        )
        model.add(UpSampling1D(length=2))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv_crop_1d_random(self):
        np.random.seed(1988)
        input_dim = 2
        input_length = 10
        filter_length = 3
        nb_filters = 4
        model = Sequential()
        model.add(
            Convolution1D(
                nb_filters,
                filter_length=filter_length,
                border_mode="same",
                input_shape=(input_length, input_dim),
            )
        )
        model.add(Cropping1D(cropping=(2, 2)))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_tiny_conv_pad_1d_random(self):
        np.random.seed(1988)
        input_dim = 2
        input_length = 10
        filter_length = 3
        nb_filters = 4
        model = Sequential()
        model.add(
            Convolution1D(
                nb_filters,
                filter_length=filter_length,
                border_mode="same",
                input_shape=(input_length, input_dim),
            )
        )
        model.add(ZeroPadding1D(padding=(2, 2)))

        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        # Test the keras model
        self._test_keras_model(model)

    def test_conv_batch_1d(self):
        vocabulary_size = 4
        embedding_dimension = 6
        input_length = 10

        model = Sequential()
        model.add(
            Embedding(
                vocabulary_size,
                embedding_dimension,
                input_length=input_length,
                trainable=True,
            )
        )

        model.add(Convolution1D(5, 2))
        model.add(BatchNormalization())
        model.add(Activation("relu"))

        model.add(MaxPooling1D(2))

        self._test_keras_model(model, one_dim_seq_flags=[True])

    # Making sure that giant channel sizes get handled correctly
    def test_large_channel_gpu(self):
        input_shape = (20, 20, 3)
        num_channels = 2049
        kernel_size = 3

        model = Sequential()
        model.add(
            Convolution2D(
                input_shape=input_shape,
                nb_filter=num_channels,
                nb_row=kernel_size,
                nb_col=kernel_size,
            )
        )

        model.set_weights(
            [(np.random.rand(*w.shape) - 0.5) / 5.0 for w in model.get_weights()]
        )

        self._test_keras_model(
            model, input_blob="data", output_blob="output", delta=1e-2
        )

    @pytest.mark.xfail(raises=Exception)
    def test_large_batch_gpu(self):
        batch_size = 2049
        num_channels = 4
        kernel_size = 3

        model = Sequential()
        model.add(
            TimeDistributed(Dense(num_channels), input_shape=(batch_size, kernel_size))
        )

        model.set_weights(
            [(np.random.rand(*w.shape) - 0.5) / 5.0 for w in model.get_weights()]
        )

        self._test_keras_model(
            model, input_blob="data", output_blob="output", delta=1e-2
        )


@unittest.skipIf(not _HAS_KERAS_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras1
class KerasTopologyCorrectnessTest(KerasNumericCorrectnessTest):
    def test_tiny_sequential_merge(self):
        np.random.seed(1988)

        # Define a model
        model1 = Sequential()
        model1.add(Dense(4, input_dim=3))
        model1.add(Dense(4))
        model2 = Sequential()
        model2.add(Dense(4, input_dim=3))
        model2.add(Dense(4))
        model3 = Sequential()
        model3.add(Merge([model1, model2], mode="concat"))

        # Set some random weights
        model3.set_weights([np.random.rand(*w.shape) for w in model3.get_weights()])

        # Test the keras model
        self._test_keras_model(model3)

    def test_dangling_merge_left(self):
        x1 = Input(shape=(4,), name="input1")
        x2 = Input(shape=(5,), name="input2")
        y1 = Dense(6, name="dense")(x2)
        z = merge([x1, y1], mode="concat")
        model = Model([x1, x2], [z])

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        self._test_keras_model(model)

    def test_dangling_merge_right(self):
        x1 = Input(shape=(4,), name="input1")
        x2 = Input(shape=(5,), name="input2")
        y1 = Dense(6, name="dense")(x2)
        z = merge([y1, x1], mode="concat")
        model = Model([x1, x2], [z])

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])

        self._test_keras_model(model)

    def test_shared_vision(self):
        digit_input = Input(shape=(27, 27, 1))
        x = Convolution2D(64, 3, 3)(digit_input)
        x = Convolution2D(64, 3, 3)(x)
        out = Flatten()(x)

        vision_model = Model(digit_input, out)

        # then define the tell-digits-apart model
        digit_a = Input(shape=(27, 27, 1))
        digit_b = Input(shape=(27, 27, 1))

        # the vision model will be shared, weights and all
        out_a = vision_model(digit_a)
        out_b = vision_model(digit_b)

        concatenated = merge([out_a, out_b], mode="concat")
        out = Dense(1, activation="sigmoid")(concatenated)
        model = Model([digit_a, digit_b], out)
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])
        self._test_keras_model(model)

    def test_tiny_weight_sharing(self):
        #     - Dense1 -----------
        # x - |                   |- Merge
        #     - Dense1 - Dense2 --

        x = Input(shape=(3,))
        dense = Dense(4)
        y1 = dense(x)
        y2 = dense(x)
        y3 = Dense(4)(y2)
        z = merge([y1, y3], mode="concat")
        model = Model(x, z)

        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])
        self._test_keras_model(model, mode="random", delta=1e-2)


@unittest.skipIf(not _HAS_KERAS_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras1
class KerasInceptionCorrectnessTest(KerasNumericCorrectnessTest):
    def test_inception_conv_stage(self):

        input_shape = (299, 299, 3)
        img_input = Input(shape=input_shape)
        channel_axis = 3
        inputs = img_input

        x = conv2d_bn(img_input, 32, 3, 3, subsample=(2, 2), border_mode="valid")
        x = conv2d_bn(x, 32, 3, 3, border_mode="valid")
        x = conv2d_bn(x, 64, 3, 3)
        x = MaxPooling2D((3, 3), strides=(2, 2))(x)

        x = conv2d_bn(x, 80, 1, 1, border_mode="valid")
        x = conv2d_bn(x, 192, 3, 3, border_mode="valid")
        x = MaxPooling2D((3, 3), strides=(2, 2))(x)

        model = Model(inputs, x, name="inception_v3")

        # Set some random weights
        # use small weights for numerical correctness
        model.set_weights(
            [np.random.rand(*w.shape) * 1e-3 for w in model.get_weights()]
        )

        # Get the coreml model
        self._test_keras_model(model)

    def test_inception_first_branch(self):

        input_shape = (299, 299, 3)
        img_input = Input(shape=input_shape)

        channel_axis = 3
        inputs = img_input

        x = conv2d_bn(img_input, 32, 3, 3, subsample=(2, 2), border_mode="valid")
        x = conv2d_bn(x, 32, 3, 3, border_mode="valid")
        x = conv2d_bn(x, 64, 3, 3)
        x = MaxPooling2D((3, 3), strides=(2, 2))(x)

        x = conv2d_bn(x, 80, 1, 1, border_mode="valid")
        x = conv2d_bn(x, 192, 3, 3, border_mode="valid")
        x = MaxPooling2D((3, 3), strides=(2, 2))(x)

        # mixed 0, 1, 2: 35 x 35 x 256
        for i in range(3):
            branch1x1 = conv2d_bn(x, 64, 1, 1)

            branch5x5 = conv2d_bn(x, 48, 1, 1)
            branch5x5 = conv2d_bn(branch5x5, 64, 5, 5)

            branch3x3dbl = conv2d_bn(x, 64, 1, 1)
            branch3x3dbl = conv2d_bn(branch3x3dbl, 96, 3, 3)
            branch3x3dbl = conv2d_bn(branch3x3dbl, 96, 3, 3)

            branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(
                x
            )
            branch_pool = conv2d_bn(branch_pool, 32, 1, 1)
            x = merge(
                [branch1x1, branch5x5, branch3x3dbl, branch_pool],
                mode="concat",
                concat_axis=channel_axis,
                name="mixed" + str(i),
            )

        model = Model(inputs, x, name="inception_v3")

        # Set some random weights
        # use small weights for numerical correctness
        model.set_weights(
            [np.random.rand(*w.shape) * 1e-3 for w in model.get_weights()]
        )

        # Get the coreml model
        self._test_keras_model(model)

    def test_inception_second_branch(self):

        input_shape = (299, 299, 3)
        img_input = Input(shape=input_shape)

        channel_axis = 3
        inputs = img_input

        x = conv2d_bn(img_input, 32, 3, 3, subsample=(2, 2), border_mode="valid")
        x = conv2d_bn(x, 32, 3, 3, border_mode="valid")
        x = conv2d_bn(x, 64, 3, 3)
        x = MaxPooling2D((3, 3), strides=(2, 2))(x)

        x = conv2d_bn(x, 80, 1, 1, border_mode="valid")
        x = conv2d_bn(x, 192, 3, 3, border_mode="valid")
        x = MaxPooling2D((3, 3), strides=(2, 2))(x)

        # mixed 0, 1, 2: 35 x 35 x 256
        for i in range(3):
            branch1x1 = conv2d_bn(x, 64, 1, 1)

            branch5x5 = conv2d_bn(x, 48, 1, 1)
            branch5x5 = conv2d_bn(branch5x5, 64, 5, 5)

            branch3x3dbl = conv2d_bn(x, 64, 1, 1)
            branch3x3dbl = conv2d_bn(branch3x3dbl, 96, 3, 3)
            branch3x3dbl = conv2d_bn(branch3x3dbl, 96, 3, 3)

            branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(
                x
            )
            branch_pool = conv2d_bn(branch_pool, 32, 1, 1)
            x = merge(
                [branch1x1, branch5x5, branch3x3dbl, branch_pool],
                mode="concat",
                concat_axis=channel_axis,
                name="mixed" + str(i),
            )

        # mixed 3: 17 x 17 x 768
        branch3x3 = conv2d_bn(x, 384, 3, 3, subsample=(2, 2), border_mode="valid")

        branch3x3dbl = conv2d_bn(x, 64, 1, 1)
        branch3x3dbl = conv2d_bn(branch3x3dbl, 96, 3, 3)
        branch3x3dbl = conv2d_bn(
            branch3x3dbl, 96, 3, 3, subsample=(2, 2), border_mode="valid"
        )

        branch_pool = MaxPooling2D((3, 3), strides=(2, 2))(x)
        x = merge(
            [branch3x3, branch3x3dbl, branch_pool],
            mode="concat",
            concat_axis=channel_axis,
            name="mixed3",
        )

        # mixed 4: 17 x 17 x 768
        branch1x1 = conv2d_bn(x, 192, 1, 1)

        branch7x7 = conv2d_bn(x, 128, 1, 1)
        branch7x7 = conv2d_bn(branch7x7, 128, 1, 7)
        branch7x7 = conv2d_bn(branch7x7, 192, 7, 1)

        branch7x7dbl = conv2d_bn(x, 128, 1, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 128, 7, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 128, 1, 7)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 128, 7, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 1, 7)

        branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(x)
        branch_pool = conv2d_bn(branch_pool, 192, 1, 1)
        x = merge(
            [branch1x1, branch7x7, branch7x7dbl, branch_pool],
            mode="concat",
            concat_axis=channel_axis,
            name="mixed4",
        )

        # mixed 5, 6: 17 x 17 x 768
        for i in range(2):
            branch1x1 = conv2d_bn(x, 192, 1, 1)

            branch7x7 = conv2d_bn(x, 160, 1, 1)
            branch7x7 = conv2d_bn(branch7x7, 160, 1, 7)
            branch7x7 = conv2d_bn(branch7x7, 192, 7, 1)

            branch7x7dbl = conv2d_bn(x, 160, 1, 1)
            branch7x7dbl = conv2d_bn(branch7x7dbl, 160, 7, 1)
            branch7x7dbl = conv2d_bn(branch7x7dbl, 160, 1, 7)
            branch7x7dbl = conv2d_bn(branch7x7dbl, 160, 7, 1)
            branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 1, 7)

            branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(
                x
            )
            branch_pool = conv2d_bn(branch_pool, 192, 1, 1)
            x = merge(
                [branch1x1, branch7x7, branch7x7dbl, branch_pool],
                mode="concat",
                concat_axis=channel_axis,
                name="mixed" + str(5 + i),
            )

        # mixed 7: 17 x 17 x 768
        branch1x1 = conv2d_bn(x, 192, 1, 1)

        branch7x7 = conv2d_bn(x, 192, 1, 1)
        branch7x7 = conv2d_bn(branch7x7, 192, 1, 7)
        branch7x7 = conv2d_bn(branch7x7, 192, 7, 1)

        branch7x7dbl = conv2d_bn(x, 160, 1, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 7, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 1, 7)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 7, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 1, 7)

        branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(x)
        branch_pool = conv2d_bn(branch_pool, 192, 1, 1)
        x = merge(
            [branch1x1, branch7x7, branch7x7dbl, branch_pool],
            mode="concat",
            concat_axis=channel_axis,
            name="mixed7",
        )

        model = Model(inputs, x, name="inception_v3")

        # Set some random weights
        # use small weights for numerical correctness
        model.set_weights(
            [np.random.rand(*w.shape) * 1e-3 for w in model.get_weights()]
        )

        # Get the coreml model
        self._test_keras_model(model)

    def test_inception_no_top(self):

        input_shape = (299, 299, 3)
        img_input = Input(shape=input_shape)
        channel_axis = 3
        inputs = img_input

        x = conv2d_bn(img_input, 32, 3, 3, subsample=(2, 2), border_mode="valid")
        x = conv2d_bn(x, 32, 3, 3, border_mode="valid")
        x = conv2d_bn(x, 64, 3, 3)
        x = MaxPooling2D((3, 3), strides=(2, 2))(x)

        x = conv2d_bn(x, 80, 1, 1, border_mode="valid")
        x = conv2d_bn(x, 192, 3, 3, border_mode="valid")
        x = MaxPooling2D((3, 3), strides=(2, 2))(x)

        # mixed 0, 1, 2: 35 x 35 x 256
        for i in range(3):
            branch1x1 = conv2d_bn(x, 64, 1, 1)

            branch5x5 = conv2d_bn(x, 48, 1, 1)
            branch5x5 = conv2d_bn(branch5x5, 64, 5, 5)

            branch3x3dbl = conv2d_bn(x, 64, 1, 1)
            branch3x3dbl = conv2d_bn(branch3x3dbl, 96, 3, 3)
            branch3x3dbl = conv2d_bn(branch3x3dbl, 96, 3, 3)

            branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(
                x
            )
            branch_pool = conv2d_bn(branch_pool, 32, 1, 1)
            x = merge(
                [branch1x1, branch5x5, branch3x3dbl, branch_pool],
                mode="concat",
                concat_axis=channel_axis,
                name="mixed" + str(i),
            )

        # mixed 3: 17 x 17 x 768
        branch3x3 = conv2d_bn(x, 384, 3, 3, subsample=(2, 2), border_mode="valid")

        branch3x3dbl = conv2d_bn(x, 64, 1, 1)
        branch3x3dbl = conv2d_bn(branch3x3dbl, 96, 3, 3)
        branch3x3dbl = conv2d_bn(
            branch3x3dbl, 96, 3, 3, subsample=(2, 2), border_mode="valid"
        )

        branch_pool = MaxPooling2D((3, 3), strides=(2, 2))(x)
        x = merge(
            [branch3x3, branch3x3dbl, branch_pool],
            mode="concat",
            concat_axis=channel_axis,
            name="mixed3",
        )

        # mixed 4: 17 x 17 x 768
        branch1x1 = conv2d_bn(x, 192, 1, 1)

        branch7x7 = conv2d_bn(x, 128, 1, 1)
        branch7x7 = conv2d_bn(branch7x7, 128, 1, 7)
        branch7x7 = conv2d_bn(branch7x7, 192, 7, 1)

        branch7x7dbl = conv2d_bn(x, 128, 1, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 128, 7, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 128, 1, 7)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 128, 7, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 1, 7)

        branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(x)
        branch_pool = conv2d_bn(branch_pool, 192, 1, 1)
        x = merge(
            [branch1x1, branch7x7, branch7x7dbl, branch_pool],
            mode="concat",
            concat_axis=channel_axis,
            name="mixed4",
        )

        # mixed 5, 6: 17 x 17 x 768
        for i in range(2):
            branch1x1 = conv2d_bn(x, 192, 1, 1)

            branch7x7 = conv2d_bn(x, 160, 1, 1)
            branch7x7 = conv2d_bn(branch7x7, 160, 1, 7)
            branch7x7 = conv2d_bn(branch7x7, 192, 7, 1)

            branch7x7dbl = conv2d_bn(x, 160, 1, 1)
            branch7x7dbl = conv2d_bn(branch7x7dbl, 160, 7, 1)
            branch7x7dbl = conv2d_bn(branch7x7dbl, 160, 1, 7)
            branch7x7dbl = conv2d_bn(branch7x7dbl, 160, 7, 1)
            branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 1, 7)

            branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(
                x
            )
            branch_pool = conv2d_bn(branch_pool, 192, 1, 1)
            x = merge(
                [branch1x1, branch7x7, branch7x7dbl, branch_pool],
                mode="concat",
                concat_axis=channel_axis,
                name="mixed" + str(5 + i),
            )

        # mixed 7: 17 x 17 x 768
        branch1x1 = conv2d_bn(x, 192, 1, 1)

        branch7x7 = conv2d_bn(x, 192, 1, 1)
        branch7x7 = conv2d_bn(branch7x7, 192, 1, 7)
        branch7x7 = conv2d_bn(branch7x7, 192, 7, 1)

        branch7x7dbl = conv2d_bn(x, 160, 1, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 7, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 1, 7)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 7, 1)
        branch7x7dbl = conv2d_bn(branch7x7dbl, 192, 1, 7)

        branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(x)
        branch_pool = conv2d_bn(branch_pool, 192, 1, 1)
        x = merge(
            [branch1x1, branch7x7, branch7x7dbl, branch_pool],
            mode="concat",
            concat_axis=channel_axis,
            name="mixed7",
        )

        # mixed 8: 8 x 8 x 1280
        branch3x3 = conv2d_bn(x, 192, 1, 1)
        branch3x3 = conv2d_bn(
            branch3x3, 320, 3, 3, subsample=(2, 2), border_mode="valid"
        )

        branch7x7x3 = conv2d_bn(x, 192, 1, 1)
        branch7x7x3 = conv2d_bn(branch7x7x3, 192, 1, 7)
        branch7x7x3 = conv2d_bn(branch7x7x3, 192, 7, 1)
        branch7x7x3 = conv2d_bn(
            branch7x7x3, 192, 3, 3, subsample=(2, 2), border_mode="valid"
        )

        branch_pool = AveragePooling2D((3, 3), strides=(2, 2))(x)
        x = merge(
            [branch3x3, branch7x7x3, branch_pool],
            mode="concat",
            concat_axis=channel_axis,
            name="mixed8",
        )

        # mixed 9: 8 x 8 x 2048
        for i in range(2):
            branch1x1 = conv2d_bn(x, 320, 1, 1)

            branch3x3 = conv2d_bn(x, 384, 1, 1)
            branch3x3_1 = conv2d_bn(branch3x3, 384, 1, 3)
            branch3x3_2 = conv2d_bn(branch3x3, 384, 3, 1)
            branch3x3 = merge(
                [branch3x3_1, branch3x3_2],
                mode="concat",
                concat_axis=channel_axis,
                name="mixed9_" + str(i),
            )

            branch3x3dbl = conv2d_bn(x, 448, 1, 1)
            branch3x3dbl = conv2d_bn(branch3x3dbl, 384, 3, 3)
            branch3x3dbl_1 = conv2d_bn(branch3x3dbl, 384, 1, 3)
            branch3x3dbl_2 = conv2d_bn(branch3x3dbl, 384, 3, 1)
            branch3x3dbl = merge(
                [branch3x3dbl_1, branch3x3dbl_2],
                mode="concat",
                concat_axis=channel_axis,
            )

            branch_pool = AveragePooling2D((3, 3), strides=(1, 1), border_mode="same")(
                x
            )
            branch_pool = conv2d_bn(branch_pool, 192, 1, 1)
            x = merge(
                [branch1x1, branch3x3, branch3x3dbl, branch_pool],
                mode="concat",
                concat_axis=channel_axis,
                name="mixed" + str(9 + i),
            )

        model = Model(inputs, x, name="inception_v3")

        # Set some random weights
        # use small weights for numerical correctness
        model.set_weights(
            [np.random.rand(*w.shape) * 1e-3 for w in model.get_weights()]
        )

        # Get the coreml model
        self._test_keras_model(model)


@unittest.skipIf(not _HAS_KERAS_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras1
@pytest.mark.slow
class KerasNumericCorrectnessStressTest(KerasNumericCorrectnessTest):
    """
    Unit test class for testing all combinations of a particular
    layer.
    """

    def _run_test(
        self,
        model,
        param,
        model_dir=None,
        delta=1e-2,
        transpose_keras_result=True,
        one_dim_seq_flags=None,
    ):
        """ Run a test on a particular model
        """
        use_tmp_folder = False
        if model_dir is None:
            use_tmp_folder = True
            model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, "keras.mlmodel")

        # Generate some random data
        nb_inputs = len(model.inputs)
        if nb_inputs > 1:
            input_names = []
            input_data = []
            coreml_input = {}
            for i in range(nb_inputs):
                input_shape = [1 if a is None else a for a in model.input_shape[i]]
                X = _generate_data(input_shape)
                feature_name = "data_%s" % i
                input_names.append(feature_name)
                input_data.append(X)
                if one_dim_seq_flags is None:
                    coreml_input[feature_name] = _keras_transpose(X).astype("f")
                else:
                    coreml_input[feature_name] = _keras_transpose(
                        X, one_dim_seq_flags[i]
                    ).astype("f")
        else:
            input_shape = [1 if a is None else a for a in model.input_shape]
            input_names = ["data"]
            input_data = _generate_data(input_shape)
            if one_dim_seq_flags is None:
                coreml_input = {"data": _keras_transpose(input_data).astype("f")}
            else:
                coreml_input = {
                    "data": _keras_transpose(input_data, one_dim_seq_flags[0]).astype(
                        "f"
                    )
                }

        # Make predictions
        if transpose_keras_result:
            keras_preds = _keras_transpose(model.predict(input_data)).flatten()
        else:
            keras_preds = model.predict(input_data).flatten()

        # Get the model
        coreml_model = _get_coreml_model(model, model_path, input_names, ["output"])
        if _is_macos() and _macos_version() >= (10, 13):
            # get prediction
            coreml_preds = coreml_model.predict(coreml_input)["output"].flatten()

            if use_tmp_folder:
                shutil.rmtree(model_dir)
            self.assertEquals(
                len(coreml_preds),
                len(keras_preds),
                msg="Failed test case %s. Lengths wrong (%s vs %s)"
                % (param, len(coreml_preds), len(keras_preds)),
            )
            for i in range(len(keras_preds)):
                max_den = max(1.0, keras_preds[i], coreml_preds[i])
                self.assertAlmostEquals(
                    keras_preds[i] / max_den,
                    coreml_preds[i] / max_den,
                    delta=delta,
                    msg="Failed test case %s. Predictions wrong (%s vs %s)"
                    % (param, coreml_preds[i], keras_preds[i]),
                )

    @pytest.mark.slow
    def test_activation_layer_params(self):
        options = dict(
            activation=["tanh", "relu", "sigmoid", "softmax", "softplus", "softsign"]
        )

        # Define a function that tests a model
        num_channels = 10
        input_dim = 10

        def build_model(x):
            model = Sequential()
            model.add(Dense(num_channels, input_dim=input_dim))
            model.add(Activation(**dict(zip(options.keys(), x))))
            return x, model

        # Iterate through all combinations
        product = itertools.product(*options.values())
        args = [build_model(p) for p in product]

        # Test the cases
        print("Testing a total of %s cases. This could take a while" % len(args))
        for param, model in args:
            model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])
            self._run_test(model, param)

    @pytest.mark.slow
    def test_dense_layer_params(self):
        options = dict(
            activation=["relu", "softmax", "tanh", "sigmoid"], bias=[True, False],
        )

        # Define a function that tests a model
        input_dim = 10
        num_channels = 10

        def build_model(x):
            kwargs = dict(zip(options.keys(), x))
            model = Sequential()
            model.add(Dense(num_channels, input_dim=input_dim, **kwargs))
            return x, model

        # Iterate through all combinations
        product = itertools.product(*options.values())
        args = [build_model(p) for p in product]

        # Test the cases
        print("Testing a total of %s cases. This could take a while" % len(args))
        for param, model in args:
            self._run_test(model, param)

    @pytest.mark.slow
    def test_upsample_layer_params(self):
        options = dict(size=[(2, 2), (3, 3), (4, 4), (5, 5)])

        np.random.seed(1988)
        input_dim = 10
        input_shape = (input_dim, input_dim, 1)
        X = np.random.rand(1, *input_shape)

        # Define a function that tests a model
        def build_model(x):
            kwargs = dict(zip(options.keys(), x))
            model = Sequential()
            model.add(
                Convolution2D(input_shape=input_shape, nb_row=7, nb_col=7, nb_filter=5)
            )
            model.add(UpSampling2D(**kwargs))
            return x, model

        # Iterate through all combinations
        product = itertools.product(*options.values())
        args = [build_model(p) for p in product]

        # Test the cases
        print("Testing a total of %s cases. This could take a while" % len(args))
        for param, model in args:
            self._run_test(model, param)

    @pytest.mark.slow
    def test_conv_layer_params(self):
        options = dict(
            activation=[
                "relu",
                "tanh",
                "sigmoid",
            ],  # keas does not support softmax on 4-D
            bias=[True, False],
            border_mode=["same", "valid"],
            nb_filter=[1, 3, 5],
            nb_row=[5],  # fails when sizes are different
            nb_col=[5],
        )

        # Define a function that tests a model
        input_shape = (10, 10, 1)

        def build_model(x):
            kwargs = dict(zip(options.keys(), x))
            model = Sequential()
            model.add(Convolution2D(input_shape=input_shape, **kwargs))
            return x, model

        # Iterate through all combinations
        product = itertools.product(*options.values())
        args = [build_model(p) for p in product]

        # Test the cases
        print("Testing a total of %s cases. This could take a while" % len(args))
        for param, model in args:
            self._run_test(model, param)

    @pytest.mark.slow
    def test_dense_elementwise_params(self):
        options = dict(modes=["sum", "mul", "concat", "ave", "cos", "dot", "max"])

        def build_model(mode):
            x1 = Input(shape=(3,))
            x2 = Input(shape=(3,))
            y1 = Dense(4)(x1)
            y2 = Dense(4)(x2)
            z = merge([y1, y2], mode=mode)
            model = Model([x1, x2], z)
            return mode, model

        product = itertools.product(*options.values())
        args = [build_model(p[0]) for p in product]
        print("Testing a total of %s cases. This could take a while" % len(args))
        for param, model in args:
            self._run_test(model, param)

    def test_vgg_16_tiny(self):

        input_shape = (48, 48, 3)
        model = Sequential()
        model.add(ZeroPadding2D((1, 1), input_shape=input_shape))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(Flatten())
        model.add(Dense(32, activation="relu"))
        # model.add(Dropout(0.5))
        model.add(Dense(32, activation="relu"))
        # model.add(Dropout(0.5))
        model.add(Dense(1000))  # activation='softmax'))

        # Set some random weights
        model.set_weights(
            [(np.random.rand(*w.shape) - 0.5) / 5.0 for w in model.get_weights()]
        )

        # Get the coreml model
        self._test_keras_model(model)

    def test_vgg_16_tiny_no_pooling(self):

        input_shape = (48, 48, 3)
        model = Sequential()
        model.add(ZeroPadding2D((1, 1), input_shape=input_shape))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(ZeroPadding2D((1, 1)))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(MaxPooling2D((2, 2), strides=(2, 2)))

        model.add(Flatten())
        model.add(Dense(32, activation="relu"))
        # model.add(Dropout(0.5))
        model.add(Dense(32, activation="relu"))
        # model.add(Dropout(0.5))
        model.add(Dense(1000))  # activation='softmax'))

        # Set some random weights
        model.set_weights(
            [(np.random.rand(*w.shape) - 0.5) / 5.0 for w in model.get_weights()]
        )

        # Get the coreml model
        self._test_keras_model(model)

    def test_vgg_16_tiny_no_pooling_no_padding(self):

        input_shape = (48, 48, 3)
        model = Sequential()
        model.add(Convolution2D(32, 3, 3, activation="relu", input_shape=input_shape))
        model.add(Convolution2D(32, 3, 3, activation="relu"))

        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(Convolution2D(32, 3, 3, activation="relu"))

        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(Convolution2D(32, 3, 3, activation="relu"))

        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(Convolution2D(32, 3, 3, activation="relu"))

        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(Convolution2D(32, 3, 3, activation="relu"))
        model.add(Convolution2D(32, 3, 3, activation="relu"))

        model.add(Flatten())
        model.add(Dense(32, activation="relu"))
        model.add(Dropout(0.5))
        model.add(Dense(32, activation="relu"))
        model.add(Dropout(0.5))
        model.add(Dense(1000, activation="softmax"))

        # Get the coreml model
        self._test_keras_model(model)

    def test_vgg_16_tiny_only_conv_dense(self):

        input_shape = (48, 48, 3)
        model = Sequential()
        model.add(Convolution2D(32, 3, 3, activation="relu", input_shape=input_shape))
        model.add(Flatten())
        model.add(Dense(10, activation="softmax"))

        # Get the coreml model
        self._test_keras_model(model)

    def test_imdb_fasttext_first_2(self):

        max_features = 10
        max_len = 6
        embedding_dims = 4
        pool_length = 2

        model = Sequential()
        model.add(Embedding(max_features, embedding_dims, input_length=max_len))
        # we add a AveragePooling1D, which will average the embeddings
        # of all words in the document
        model.add(AveragePooling1D(pool_length=pool_length))

        self._test_keras_model(model, one_dim_seq_flags=[True])

    def test_tiny_mcrnn_td(self):

        model = Sequential()
        model.add(Convolution2D(3, 1, 1, input_shape=(2, 4, 4), border_mode="same"))
        model.add(AveragePooling2D(pool_size=(2, 2)))
        model.add(Reshape((2, 3)))
        model.add(TimeDistributed(Dense(5)))

        self._test_keras_model(model)

    def test_tiny_mcrnn_recurrent(self):

        model = Sequential()
        model.add(Convolution2D(3, 1, 1, input_shape=(2, 4, 4), border_mode="same"))
        model.add(AveragePooling2D(pool_size=(2, 2)))
        model.add(Reshape((2, 3)))
        model.add(LSTM(5, inner_activation="sigmoid"))

        self._test_keras_model(model)

    def test_tiny_mcrnn_music_tagger(self):

        x_in = Input(shape=(4, 6, 1))
        x = ZeroPadding2D(padding=(0, 1))(x_in)
        x = BatchNormalization(axis=2, name="bn_0_freq")(x)
        # Conv block 1
        x = Convolution2D(2, 3, 3, border_mode="same", name="conv1")(x)
        x = BatchNormalization(axis=3, mode=0, name="bn1")(x)
        x = ELU()(x)
        x = MaxPooling2D(pool_size=(2, 2), strides=(2, 2), name="pool1")(x)
        # Conv block 2
        x = Convolution2D(4, 3, 3, border_mode="same", name="conv2")(x)
        x = BatchNormalization(axis=3, mode=0, name="bn2")(x)
        x = ELU()(x)
        x = MaxPooling2D(pool_size=(2, 2), strides=(2, 2), name="pool2")(x)

        # Should get you (1,1,2,4)
        x = Reshape((2, 4))(x)
        x = GRU(32, return_sequences=True, name="gru1")(x)
        x = GRU(32, return_sequences=False, name="gru2")(x)

        # Create model.
        model = Model(x_in, x)
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])
        self._test_keras_model(model, mode="random_zero_mean", delta=1e-2)

    def test_tiny_apple_manual(self):
        model = Sequential()
        model.add(LSTM(3, input_shape=(4, 5), inner_activation="sigmoid"))
        model.add(Dense(5))
        model.add(Activation("softmax"))

        self._test_keras_model(model)

    def test_tiny_image_captioning_image_branch(self):
        img_input_1 = Input(shape=(16, 16, 3))
        x = Convolution2D(2, 3, 3)(img_input_1)
        x = Flatten()(x)
        img_model = Model([img_input_1], [x])

        img_input = Input(shape=(16, 16, 3))
        x = img_model(img_input)
        x = Dense(8, name="cap_dense")(x)
        x = Reshape((1, 8), name="cap_reshape")(x)
        image_branch = Model([img_input], [x])
        self._test_keras_model(image_branch)

    def test_tiny_image_captioning_feature_merge(self):

        img_input_1 = Input(shape=(16, 16, 3))
        x = Convolution2D(2, 3, 3)(img_input_1)
        x = Flatten()(x)
        img_model = Model([img_input_1], [x])

        img_input = Input(shape=(16, 16, 3))
        x = img_model(img_input)
        x = Dense(8, name="cap_dense")(x)
        x = Reshape((1, 8), name="cap_reshape")(x)

        sentence_input = Input(shape=(5,))  # max_length = 5
        y = Embedding(8, 8, name="cap_embedding")(sentence_input)
        z = merge([x, y], mode="concat", concat_axis=1, name="cap_merge")

        combined_model = Model([img_input, sentence_input], [z])
        self._test_keras_model(combined_model, one_dim_seq_flags=[False, True])

    def test_tiny_image_captioning(self):
        # use a conv layer as a image feature branch
        img_input_1 = Input(shape=(16, 16, 3))
        x = Convolution2D(2, 3, 3)(img_input_1)
        x = Flatten()(x)
        img_model = Model([img_input_1], [x])

        img_input = Input(shape=(16, 16, 3))
        x = img_model(img_input)
        x = Dense(8, name="cap_dense")(x)
        x = Reshape((1, 8), name="cap_reshape")(x)

        sentence_input = Input(shape=(5,))  # max_length = 5
        y = Embedding(8, 8, name="cap_embedding")(sentence_input)
        z = merge([x, y], mode="concat", concat_axis=1, name="cap_merge")
        z = LSTM(4, return_sequences=True, name="cap_lstm")(z)
        z = TimeDistributed(Dense(8), name="cap_timedistributed")(z)

        combined_model = Model([img_input, sentence_input], [z])
        self._test_keras_model(combined_model, one_dim_seq_flags=[False, True])

    def test_tiny_babi_rnn(self):
        vocab_size = 10
        embed_hidden_size = 8
        story_maxlen = 5
        query_maxlen = 5

        sentrnn = Sequential()
        sentrnn.add(Embedding(vocab_size, embed_hidden_size, input_length=story_maxlen))
        sentrnn.add(Dropout(0.3))

        qrnn = Sequential()
        qrnn.add(Embedding(vocab_size, embed_hidden_size, input_length=query_maxlen))
        qrnn.add(Dropout(0.3))
        qrnn.add(LSTM(embed_hidden_size, return_sequences=False))
        qrnn.add(RepeatVector(story_maxlen))

        model = Sequential()
        model.add(Merge([sentrnn, qrnn], mode="sum"))
        model.add(LSTM(embed_hidden_size, return_sequences=False))
        model.add(Dropout(0.3))
        model.add(Dense(vocab_size, activation="softmax"))

        self._test_keras_model(model, one_dim_seq_flags=[True, True])

    def test_clickbait_cnn(self):
        # from: https://github.com/saurabhmathur96/clickbait-detector
        vocabulary_size = 500
        embedding_dimension = 30
        input_length = 20

        model = Sequential()
        model.add(
            Embedding(
                vocabulary_size,
                embedding_dimension,
                input_length=input_length,
                trainable=True,
            )
        )

        model.add(Convolution1D(32, 2))
        model.add(BatchNormalization())
        model.add(Activation("relu"))

        model.add(Convolution1D(32, 2))
        model.add(BatchNormalization())
        model.add(Activation("relu"))

        model.add(Convolution1D(32, 2))
        model.add(BatchNormalization())
        model.add(Activation("relu"))

        model.add(MaxPooling1D(17))
        model.add(Flatten())

        model.add(Dense(1, bias=True))
        model.add(BatchNormalization())
        model.add(Activation("sigmoid"))

        self._test_keras_model(model, one_dim_seq_flags=[True])

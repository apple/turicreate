"""Module containing unit tests for verifying various quantization."""
import os
import shutil
import tempfile
import unittest
import numpy as np
import pytest

import coremltools
import \
    coremltools.models.neural_network.quantization_utils as quantization_utils
from coremltools._deps import HAS_KERAS2_TF
from coremltools.models import (
    _MLMODEL_FULL_PRECISION,
    _QUANTIZATION_MODE_LINEAR_QUANTIZATION,
    _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS,
    _QUANTIZATION_MODE_CUSTOM_LOOKUP_TABLE
)


@unittest.skipIf(coremltools.utils.macos_version() < (10, 14),
                 'Missing macOS 10.14+. Skipping tests.')
@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
class QuantizationNumericalCorrectnessTests(unittest.TestCase):

    def runTest(self):
        pass

    def setUp(self):
        self.qbits = 8  # n-bit quantization for tests
        self.qmode = _QUANTIZATION_MODE_LINEAR_QUANTIZATION
        self.custom_lut = None
        from test_keras2_numeric import KerasBasicNumericCorrectnessTest
        self.keras_tester = KerasBasicNumericCorrectnessTest()
        self.keras_tester._test_model = self._test_model

    def _run_quantized_test(
            self, input_, full_precision_model, quantized_model, delta):
        # Output from both models should be the same
        full_output = full_precision_model.predict(input_)
        quantized_output = quantized_model.predict(input_)
        self.assertEqual(full_output.keys(), quantized_output.keys())

        for key in full_output.keys():
            full_output_flatten = full_output[key].flatten()
            quantized_output_flatten = quantized_output[key].flatten()

            self.assertTrue(
                len(full_output_flatten) == len(quantized_output_flatten)
            )

            norm_factor = np.maximum(
                full_output_flatten, quantized_output_flatten)
            norm_factor = np.maximum(norm_factor, 1.0)
            f_out = full_output_flatten / norm_factor
            q_out = quantized_output_flatten / norm_factor

            for idx, full_value in enumerate(f_out):
                quantized_value = q_out[idx]
                self.assertAlmostEqual(
                    full_value, quantized_value, delta=delta)

    def _test_model(self, model, num_samples=1, mode='random', delta=1e-2,
                    model_dir=None, transpose_keras_result=True,
                    one_dim_seq_flags=None,
                    model_precision=_MLMODEL_FULL_PRECISION):
        # Get the model path
        use_tmp_folder = False
        if model_dir is None:
            use_tmp_folder = True
            model_dir = tempfile.mkdtemp()
        _ = os.path.join(model_dir, 'keras.mlmodel')

        # Get converted coreml model and sample input
        (
            input_names,
            output_names,
            _,
            coreml_input
        ) = self.keras_tester._get_coreml_model_params_and_test_input(
            model, mode, one_dim_seq_flags)
        from test_keras2_numeric import _get_coreml_model
        coreml_model = _get_coreml_model(model, input_names, output_names,
                                         model_precision=model_precision)

        # Now we quantize the model and dequantize it. We then use this model
        # as our full precision model since quantizing this model again will
        # result in 0 quantization error.

        coreml_spec = coreml_model.get_spec()
        quantization_utils.quantize_spec_weights(
            spec=coreml_spec,
            nbits=self.qbits,
            quantization_mode=self.qmode,
            lut_function=self.custom_lut
        )

        # De-quantize model
        quantization_utils._dequantize_nn_spec(spec=coreml_spec.neuralNetwork)
        full_precision_model_spec = coreml_spec

        # Quantize model from another copy
        quantized_model_spec = quantization_utils.quantize_spec_weights(
            spec=coreml_model.get_spec(),
            nbits=self.qbits,
            quantization_mode=self.qmode,
            lut_function=self.custom_lut
        )

        full_precision_model = coremltools.models.MLModel(
            full_precision_model_spec)
        quantized_model = coremltools.models.MLModel(quantized_model_spec)
        self._run_quantized_test(coreml_input, full_precision_model,
                                 quantized_model, delta)

        # Clean up after ourselves
        if use_tmp_folder and os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_quantized_tiny_inner_product(self):
        self.keras_tester.test_tiny_inner_product()

    def test_quantized_conv_batchnorm_random(self):
        self.keras_tester.test_conv_batchnorm_random()

    def test_quantized_conv_batchnorm_no_gamma_no_beta(self):
        self.keras_tester.test_conv_batchnorm_no_gamma_no_beta()

    def test_quantized_tiny_deconv_random(self):
        self.keras_tester.test_tiny_deconv_random()

    def test_quantized_tiny_deconv_random_same_padding(self):
        self.keras_tester.test_tiny_deconv_random_same_padding()

    def test_quantized_tiny_depthwise_conv_valid_pad(self):
        self.keras_tester.test_tiny_depthwise_conv_valid_pad()

    def test_quantized_tiny_separable_conv_valid_depth_multiplier(self):
        self.keras_tester.test_tiny_separable_conv_valid_depth_multiplier()

    def test_quantized_max_pooling_no_overlap(self):
        self.keras_tester.test_max_pooling_no_overlap()

    def test_quantized_dense_softmax(self):
        self.keras_tester.test_dense_softmax()

    def test_quantized_housenet_random(self):
        self.keras_tester.test_housenet_random()

    def test_quantized_large_input_length_conv1d_same_random(self):
        self.keras_tester.test_large_input_length_conv1d_same_random()

    def test_quantized_conv_dense(self):
        self.keras_tester.test_conv_dense()

    def test_quantized_tiny_conv_crop_1d_random(self):
        self.keras_tester.test_tiny_conv_crop_1d_random()

    def test_quantized_embedding(self):
        self.keras_tester.test_embedding()

    def test_quantized_tiny_conv_elu_random(self):
        self.keras_tester.test_tiny_conv_elu_random()

    def test_quantized_tiny_concat_random(self):
        self.keras_tester.test_tiny_concat_random()

    def test_quantized_tiny_dense_tanh_fused_random(self):
        self.keras_tester.test_tiny_dense_tanh_fused_random()

    def test_quantized_conv1d_flatten(self):
        # Softmax after quantization appears to have a bigger error margin
        self.keras_tester.test_conv1d_flatten(delta=2e-2)

    def test_quantized_tiny_conv_dropout_random(self):
        self.keras_tester.test_tiny_conv_dropout_random()

    def test_quantized_tiny_mul_random(self):
        self.keras_tester.test_tiny_mul_random()

    def test_quantized_tiny_conv_thresholded_relu_random(self):
        self.keras_tester.test_tiny_conv_thresholded_relu_random()

    def test_quantized_tiny_seq2seq_rnn_random(self):
        self.keras_tester.test_tiny_seq2seq_rnn_random()

    def test_quantized_rnn_seq(self):
        self.keras_tester.test_rnn_seq()

    def test_quantized_medium_no_sequence_simple_rnn_random(self):
        self.keras_tester.test_medium_no_sequence_simple_rnn_random()

    def test_quantized_tiny_no_sequence_lstm_zeros(self):
        self.keras_tester.test_tiny_no_sequence_lstm_zeros()

    def test_quantized_tiny_no_sequence_lstm_ones(self):
        self.keras_tester.test_tiny_no_sequence_lstm_ones()

    def test_quantized_lstm_seq(self):
        self.keras_tester.test_lstm_seq()

    def test_quantized_medium_no_sequence_lstm_random(self):
        self.keras_tester.test_medium_no_sequence_lstm_random()

    def test_quantized_tiny_no_sequence_gru_random(self):
        self.keras_tester.test_tiny_no_sequence_gru_random()

    def test_quantized_gru_seq_backwards(self):
        self.keras_tester.test_gru_seq_backwards()

    def test_quantized_tiny_no_sequence_bidir_random(self):
        self.keras_tester.test_tiny_no_sequence_bidir_random()

    def test_quantized_tiny_no_sequence_bidir_random_gpu(self):
        self.keras_tester.test_tiny_no_sequence_bidir_random_gpu()

    def test_quantized_small_no_sequence_bidir_random(self):
        self.keras_tester.test_small_no_sequence_bidir_random()

    def test_quantized_medium_no_sequence_bidir_random(self):
        self.keras_tester.test_medium_no_sequence_bidir_random()

    def test_quantized_medium_bidir_random_return_seq_false(self):
        self.keras_tester.test_medium_bidir_random_return_seq_false()

    def test_quantized_tiny_sequence_lstm(self):
        self.keras_tester.test_tiny_sequence_lstm()

    def test_quantized__lstm_td(self):
        self.keras_tester.test_lstm_td()

    def test_quantized_large_channel_gpu(self):
        self.keras_tester.test_large_channel_gpu()

    def test_quantized_tiny_seq2seq_rnn_random(self):
        self.keras_tester.test_tiny_seq2seq_rnn_random()

    def test_quantized_lstm_seq_backwards(self):
        self.keras_tester.test_lstm_seq_backwards()


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class SevenBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(SevenBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 7


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class SixBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(SixBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 6


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class FiveBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(FiveBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 5


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
class FourBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(FourBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 4


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class ThreeBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(ThreeBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 3


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class TwoBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(TwoBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 2


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
class OneBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(OneBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 1


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
class LUTQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 8
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS

    def test_quantized_custom_lut(self):
        pass


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class LUTSevenBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTSevenBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 7
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class LUTSixBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTSixBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 6
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class LUTFiveBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTFiveBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 5
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
class LUTFourBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTFourBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 4
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class LUTThreeBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTThreeBitQuantizationNumericalCorrectnessTests,
              self).setUp()
        self.qbits = 3
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
@pytest.mark.slow
class LUTTwoBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTTwoBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 2
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
class LUTOneBitQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTOneBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 1
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
class LUTCustomQuantizationNumericalCorrectnessTests(
        QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTCustomQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 8
        self.qmode = _QUANTIZATION_MODE_CUSTOM_LOOKUP_TABLE
        self.custom_lut = quantization_utils._get_linear_lookup_table_and_weight


from coremltools.converters import keras as keras_converter

@unittest.skipIf(coremltools.utils.macos_version() < (10, 14),
                 'Missing macOS 10.14+. Skipping tests.')
@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
class AdvancedQuantizationNumericalCorrectnessTests(unittest.TestCase):
    """ Quantization tests for advanced settings
    """
    def test_8bit_symmetric_and_skips(self):
        from keras.models import Sequential
        from keras.layers import Conv2D

        def stable_rel_error(x, ref):
            err = x - ref
            denom = np.maximum(np.abs(ref), np.ones_like(ref))
            return np.abs(err) / denom

        np.random.seed(1988)
        input_dim = 16
        num_kernels, kernel_height, kernel_width, input_channels = 64, 3, 3, 32

        # Define a model
        model = Sequential()
        model.add(Conv2D(input_shape=(input_dim, input_dim, input_channels),
            filters=num_kernels, kernel_size=(kernel_height, kernel_width)))

        # Set some random weights
        weight, bias = model.layers[0].get_weights()
        num_filters = weight.shape[-1]
        filter_shape = weight.shape[:-1]

        new_weight = np.stack([4.0 * np.random.rand(*filter_shape) - 2 for
            i in range(num_filters)], axis=-1)
        model.layers[0].set_weights([new_weight, bias])

        mlmodel = keras_converter.convert(model, ['data'], ['output_0'])
        selector = quantization_utils.AdvancedQuantizedLayerSelector(
                skip_layer_types=['batchnorm', 'bias', 'depthwiseConv'],
                minimum_conv_kernel_channels=4,
                minimum_conv_weight_count=4096)

        q_mlmodel = quantization_utils.quantize_weights(mlmodel, 8,
                selector=selector)

        input_shape = (1,1,input_channels,input_dim,input_dim)
        input_val = 2 * np.random.rand(*input_shape) - 1

        coreml_input = {'data' : input_val}
        coreml_output = mlmodel.predict(coreml_input)
        q_coreml_output = q_mlmodel.predict(coreml_input)

        val = coreml_output['output_0']
        q_val = q_coreml_output['output_0']
        rel_err = stable_rel_error(q_val, val)
        max_rel_err, mean_rel_err = np.max(rel_err), np.mean(rel_err)
        self.assertTrue(max_rel_err < 0.25)
        self.assertTrue(max_rel_err > 0.01)
        self.assertTrue(mean_rel_err < 0.02)

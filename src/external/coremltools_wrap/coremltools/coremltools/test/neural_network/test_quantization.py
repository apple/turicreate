"""Module containing unit tests for verifying various quantization."""
import os
import shutil
import tempfile
import unittest
import numpy as np
import pytest

import coremltools
import coremltools.models.datatypes as datatypes
from coremltools.models import neural_network
import coremltools.models.neural_network.quantization_utils as quantization_utils
from coremltools.models.neural_network.quantization_utils import (
    activate_int8_int8_matrix_multiplications,
    MatrixMultiplyLayerSelector,
    _quantize_spec_weights,
)

from coremltools._deps import _HAS_KERAS2_TF
from coremltools.models import (
    _MLMODEL_FULL_PRECISION,
    _QUANTIZATION_MODE_LINEAR_QUANTIZATION,
    _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS,
    _QUANTIZATION_MODE_CUSTOM_LOOKUP_TABLE,
)


@unittest.skipIf(
    not coremltools.utils._is_macos() or coremltools.utils._macos_version() < (10, 14),
    "Missing macOS 10.14+. Skipping tests.",
)
@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
class QuantizationNumericalCorrectnessTests(unittest.TestCase):
    def runTest(self):
        pass

    def setUp(self):
        self.qbits = 8  # n-bit quantization for tests
        self.qmode = _QUANTIZATION_MODE_LINEAR_QUANTIZATION
        self.custom_lut = None
        from .test_keras2_numeric import KerasBasicNumericCorrectnessTest

        self.keras_tester = KerasBasicNumericCorrectnessTest()
        self.keras_tester._test_model = self._test_model

    def _run_quantized_test(self, input_, full_precision_model, quantized_model, delta):
        # Output from both models should be the same
        full_output = full_precision_model.predict(input_)
        quantized_output = quantized_model.predict(input_)
        self.assertEqual(full_output.keys(), quantized_output.keys())

        for key in full_output.keys():
            full_output_flatten = full_output[key].flatten()
            quantized_output_flatten = quantized_output[key].flatten()

            self.assertTrue(len(full_output_flatten) == len(quantized_output_flatten))

            norm_factor = np.maximum(full_output_flatten, quantized_output_flatten)
            norm_factor = np.maximum(norm_factor, 1.0)
            f_out = full_output_flatten / norm_factor
            q_out = quantized_output_flatten / norm_factor

            for idx, full_value in enumerate(f_out):
                quantized_value = q_out[idx]
                self.assertAlmostEqual(full_value, quantized_value, delta=delta)

    def _test_model(
        self,
        model,
        num_samples=1,
        mode="random",
        delta=1e-2,
        model_dir=None,
        transpose_keras_result=True,
        one_dim_seq_flags=None,
        model_precision=_MLMODEL_FULL_PRECISION,
    ):
        # Get the model path
        use_tmp_folder = False
        if model_dir is None:
            use_tmp_folder = True
            model_dir = tempfile.mkdtemp()
        _ = os.path.join(model_dir, "keras.mlmodel")

        # Get converted coreml model and sample input
        (
            input_names,
            output_names,
            _,
            coreml_input,
        ) = self.keras_tester._get_coreml_model_params_and_test_input(
            model, mode, one_dim_seq_flags
        )
        from .test_keras2_numeric import _get_coreml_model

        coreml_model = _get_coreml_model(
            model, input_names, output_names, model_precision=model_precision
        )

        # Now we quantize the model and dequantize it. We then use this model
        # as our full precision model since quantizing this model again will
        # result in 0 quantization error.

        coreml_spec = coreml_model.get_spec()
        quantization_utils._quantize_spec_weights(
            spec=coreml_spec,
            nbits=self.qbits,
            quantization_mode=self.qmode,
            lut_function=self.custom_lut,
        )

        # De-quantize model
        quantization_utils._dequantize_nn_spec(spec=coreml_spec.neuralNetwork)
        full_precision_model_spec = coreml_spec

        # Quantize model from another copy
        quantized_model_spec = quantization_utils._quantize_spec_weights(
            spec=coreml_model.get_spec(),
            nbits=self.qbits,
            quantization_mode=self.qmode,
            lut_function=self.custom_lut,
        )

        full_precision_model = coremltools.models.MLModel(full_precision_model_spec)
        quantized_model = coremltools.models.MLModel(quantized_model_spec)
        self._run_quantized_test(
            coreml_input, full_precision_model, quantized_model, delta
        )

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


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class SevenBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(SevenBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 7


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class SixBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(SixBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 6


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class FiveBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(FiveBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 5


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
class FourBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(FourBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 4


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class ThreeBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(ThreeBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 3


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class TwoBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(TwoBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 2


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
class OneBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(OneBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 1


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
class LUTQuantizationNumericalCorrectnessTests(QuantizationNumericalCorrectnessTests):
    def setUp(self):
        super(LUTQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 8
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS

    def test_quantized_custom_lut(self):
        pass


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class LUTSevenBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(LUTSevenBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 7
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class LUTSixBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(LUTSixBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 6
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class LUTFiveBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(LUTFiveBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 5
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
class LUTFourBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(LUTFourBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 4
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class LUTThreeBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(LUTThreeBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 3
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
@pytest.mark.slow
class LUTTwoBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(LUTTwoBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 2
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
class LUTOneBitQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(LUTOneBitQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 1
        self.qmode = _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
class LUTCustomQuantizationNumericalCorrectnessTests(
    QuantizationNumericalCorrectnessTests
):
    def setUp(self):
        super(LUTCustomQuantizationNumericalCorrectnessTests, self).setUp()
        self.qbits = 8
        self.qmode = _QUANTIZATION_MODE_CUSTOM_LOOKUP_TABLE
        self.custom_lut = quantization_utils._get_linear_lookup_table_and_weight


from coremltools.converters import keras as keras_converter


@unittest.skipIf(
    not coremltools.utils._is_macos() or coremltools.utils._macos_version() < (10, 14),
    "Missing macOS 10.14+. Skipping tests.",
)
@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
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
        model.add(
            Conv2D(
                input_shape=(input_dim, input_dim, input_channels),
                filters=num_kernels,
                kernel_size=(kernel_height, kernel_width),
            )
        )

        # Set some random weights
        weight, bias = model.layers[0].get_weights()
        num_filters = weight.shape[-1]
        filter_shape = weight.shape[:-1]

        new_weight = np.stack(
            [4.0 * np.random.rand(*filter_shape) - 2 for i in range(num_filters)],
            axis=-1,
        )
        model.layers[0].set_weights([new_weight, bias])

        mlmodel = keras_converter.convert(model, ["data"], ["output_0"])
        selector = quantization_utils.AdvancedQuantizedLayerSelector(
            skip_layer_types=["batchnorm", "bias", "depthwiseConv"],
            minimum_conv_kernel_channels=4,
            minimum_conv_weight_count=4096,
        )

        q_mlmodel = quantization_utils.quantize_weights(mlmodel, 8, selector=selector)

        input_shape = (1, 1, input_channels, input_dim, input_dim)
        input_val = 2 * np.random.rand(*input_shape) - 1

        coreml_input = {"data": input_val}
        coreml_output = mlmodel.predict(coreml_input)
        q_coreml_output = q_mlmodel.predict(coreml_input)

        val = coreml_output["output_0"]
        q_val = q_coreml_output["output_0"]
        rel_err = stable_rel_error(q_val, val)
        max_rel_err, mean_rel_err = np.max(rel_err), np.mean(rel_err)
        self.assertTrue(max_rel_err < 0.25)
        self.assertTrue(max_rel_err > 0.01)
        self.assertTrue(mean_rel_err < 0.02)


@unittest.skipIf(
    not coremltools.utils._is_macos() or coremltools.utils._macos_version() < (10, 16),
    "Missing macOS 10.16+. Skipping tests.",
)
class DynamicQuantizedInt8Int8MatMul(unittest.TestCase):
    """
    Quantization tests for dynamic Int8 - Int8 matrix multiplications
    """

    def initialize(self):
        np.random.seed(1988)
        self.Cout, self.Cin = 16, 32
        self.W = np.random.rand(self.Cout, self.Cin) * 20.0 - 10.0
        self.b = np.random.rand(self.Cout) * 20.0 - 10.0
        self.input_shape = (5, self.Cin)
        input_features = [("data", datatypes.Array(*self.input_shape))]
        output_features = [("output", None)]
        self.builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        self.selector = MatrixMultiplyLayerSelector()

    def _test_predictions(
        self, np_preds, coreml_preds, SNR=30, PSNR=40,
    ):

        np_preds = np_preds.flatten()
        coreml_preds = coreml_preds.flatten()

        noise = np_preds - coreml_preds
        noise_var = np.sum(noise ** 2) / len(noise) + 1e-7
        signal_energy = np.sum(np_preds ** 2) / len(np_preds)
        max_signal_energy = np.amax(np_preds ** 2)
        snr = 10 * np.log10(signal_energy / noise_var)
        psnr = 10 * np.log10(max_signal_energy / noise_var)
        self.assertGreaterEqual(snr, SNR)
        self.assertGreaterEqual(psnr, PSNR)

    def compare(self, specification_modified=True):
        x = np.random.rand(*self.input_shape)

        def _get_preds(spec):
            mlmodel = coremltools.models.MLModel(spec)
            return mlmodel.predict({"data": x}, useCPUOnly=True)["output"]

        preds = _get_preds(self.builder.spec)
        self.assertEqual(self.builder.spec.specificationVersion, 4)

        quantized_spec = activate_int8_int8_matrix_multiplications(
            self.builder.spec, self.selector
        )

        layer = self.builder.spec.neuralNetwork.layers[0]
        layer_type = layer.WhichOneof("layer")
        if layer_type == "innerProduct":
            matmul_layer = layer.innerProduct

        elif layer_type == "batchedMatmul":
            matmul_layer = layer.batchedMatmul
        wp = matmul_layer.weights

        if specification_modified:
            self.assertEqual(self.builder.spec.specificationVersion, 5)
            quant_preds = _get_preds(quantized_spec)
            self._test_predictions(preds, quant_preds, SNR=40)
            self.assertEqual(len(wp.floatValue), 0)
        else:
            self.assertEqual(self.builder.spec.specificationVersion, 4)
            quant_preds = _get_preds(quantized_spec)
            np.testing.assert_array_almost_equal(preds, quant_preds)
            self.assertGreater(len(wp.floatValue), 0)

    def test_single_batched_matmul_no_bias(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.compare()

    def test_single_batched_matmul_with_bias(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
            bias=self.b,
        )
        self.compare()

    def test_single_inner_product_no_bias(self):

        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=None,
            has_bias=False,
        )
        self.compare()

    def test_single_inner_product_with_bias(self):

        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.compare()

    def test_inner_product_min_input_channels_valid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.minimum_input_channels = 31
        self.compare()

    def test_batched_matmul_min_input_channels_valid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.minimum_input_channels = 32
        self.compare()

    def test_inner_product_min_input_channels_invalid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.minimum_input_channels = 33
        self.compare(specification_modified=False)

    def test_batched_matmul_min_input_channels_invalid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.minimum_input_channels = 33
        self.compare(specification_modified=False)

    def test_batched_matmul_max_input_channels_valid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.maximum_input_channels = 32
        self.compare()

    def test_inner_product_max_input_channels_valid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.maximum_input_channels = 33
        self.compare()

    def test_batched_matmul_max_input_channels_invalid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.maximum_input_channels = 31
        self.compare(specification_modified=False)

    def test_inner_product_max_input_channels_invalid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.maximum_input_channels = 30
        self.compare(specification_modified=False)

    def test_inner_product_min_output_channels_valid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.minimum_output_channels = 16
        self.compare()

    def test_batched_matmul_min_output_channels_valid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.minimum_output_channels = 16
        self.compare()

    def test_inner_product_min_output_channels_invalid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.minimum_output_channels = 17
        self.compare(specification_modified=False)

    def test_batched_matmul_min_output_channels_invalid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.minimum_output_channels = 17
        self.compare(specification_modified=False)

    def test_batched_matmul_max_output_channels_valid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.maximum_output_channels = 17
        self.compare()

    def test_inner_product_max_output_channels_valid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.maximum_output_channels = 16
        self.compare()

    def test_batched_matmul_max_output_channels_invalid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.maximum_output_channels = 14
        self.compare(specification_modified=False)

    def test_inner_product_max_output_channels_invalid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.maximum_output_channels = 15
        self.compare(specification_modified=False)

    def test_inner_product_min_weight_count_valid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.minimum_weight_count = 512
        self.compare()

    def test_batched_matmul_min_weight_count_invalid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.minimum_weight_count = 513
        self.compare(specification_modified=False)

    def test_inner_product_layer_names_invalid(self):
        self.initialize()
        self.builder.add_inner_product(
            name="ip",
            input_name="data",
            output_name="output",
            input_channels=self.Cin,
            output_channels=self.Cout,
            W=self.W,
            b=self.b,
            has_bias=True,
        )
        self.selector.include_layers_with_names = ["ip1", "ip2"]
        self.compare(specification_modified=False)

    def test_batched_matmul_layer_names_valid(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        self.selector.include_layers_with_names = ["bm1", "batched_matmul"]
        self.compare()

    def test_batched_matmul_8bit_weight_quantized(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        _quantize_spec_weights(
            self.builder.spec, 8, _QUANTIZATION_MODE_LINEAR_QUANTIZATION
        )
        self.compare()

    def test_batched_matmul_4bit_weight_quantized(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        _quantize_spec_weights(
            self.builder.spec, 4, _QUANTIZATION_MODE_LINEAR_QUANTIZATION
        )
        self.compare()

    def test_batched_matmul_2bit_weight_quantized(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        _quantize_spec_weights(
            self.builder.spec, 2, _QUANTIZATION_MODE_LINEAR_QUANTIZATION
        )
        self.compare()

    def test_batched_matmul_1bit_weight_quantized(self):

        self.initialize()
        self.builder.add_batched_mat_mul(
            name="batched_matmul",
            input_names=["data"],
            output_name="output",
            weight_matrix_rows=self.Cin,
            weight_matrix_columns=self.Cout,
            W=self.W,
        )
        _quantize_spec_weights(
            self.builder.spec, 1, _QUANTIZATION_MODE_LINEAR_QUANTIZATION
        )
        self.compare()


@unittest.skipIf(
    not coremltools.utils._is_macos() or coremltools.utils._macos_version() < (10, 15),
    "Missing macOS 10.15+. Skipping tests.",
)
class QuantizeWeightsAPI(unittest.TestCase):
    def test_embeddingND_quantize(self):
        input_features = [("data", datatypes.Array(10, 1))]
        output_features = [("output", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )

        builder.add_embedding_nd(
            name="embedding_nd",
            input_name="data",
            output_name="output",
            vocab_size=300,
            embedding_size=20,
            W=np.random.rand(20, 300),
        )

        spec = builder.spec
        model_fp32 = coremltools.models.MLModel(spec)
        self.assertEqual(
            len(spec.neuralNetwork.layers[0].embeddingND.weights.floatValue), 6000
        )

        # quantize to FP16
        model_fp16 = quantization_utils.quantize_weights(model_fp32, nbits=16)
        spec_fp16 = model_fp16.get_spec()
        self.assertEqual(
            len(spec_fp16.neuralNetwork.layers[0].embeddingND.weights.floatValue), 0
        )
        self.assertEqual(
            len(spec_fp16.neuralNetwork.layers[0].embeddingND.weights.float16Value),
            2 * 6000,
        )

        # quantize to uint8
        model_uint8 = quantization_utils.quantize_weights(model_fp32, nbits=8)
        spec_uint8 = model_uint8.get_spec()
        self.assertEqual(
            len(spec_uint8.neuralNetwork.layers[0].embeddingND.weights.floatValue), 0
        )
        self.assertEqual(
            len(spec_uint8.neuralNetwork.layers[0].embeddingND.weights.float16Value), 0
        )
        self.assertEqual(
            len(spec_uint8.neuralNetwork.layers[0].embeddingND.weights.rawValue), 6000
        )

        # quantize to uint5
        model_uint5 = quantization_utils.quantize_weights(model_fp32, nbits=5)
        spec_uint5 = model_uint5.get_spec()
        self.assertEqual(
            len(spec_uint5.neuralNetwork.layers[0].embeddingND.weights.floatValue), 0
        )
        self.assertEqual(
            len(spec_uint5.neuralNetwork.layers[0].embeddingND.weights.float16Value), 0
        )
        self.assertEqual(
            len(spec_uint5.neuralNetwork.layers[0].embeddingND.weights.rawValue), 3750
        )  # 3750 = 5*6000/8

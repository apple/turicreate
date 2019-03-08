import unittest
import numpy as np
from coremltools.models import datatypes, MLModel
from coremltools.models.neural_network import NeuralNetworkBuilder
from coremltools.models.utils import macos_version
from coremltools.models.neural_network.quantization_utils import _convert_array_to_nbit_quantized_bytes

@unittest.skipIf(macos_version() < (10, 13), 'Only supported on macOS 10.13+')
class BasicNumericCorrectnessTest(unittest.TestCase):

    def test_undefined_shape_single_output(self):
        W = np.ones((3,3))
        input_features = [('data', datatypes.Array(3))]
        output_features = [('probs', None)]
        builder = NeuralNetworkBuilder(input_features, output_features)
        builder.add_inner_product(name = 'ip1',
                                  W = W,
                                  b = None,
                                  input_channels = 3,
                                  output_channels = 3,
                                  has_bias = False,
                                  input_name = 'data',
                                  output_name = 'probs')
        mlmodel = MLModel(builder.spec)
        data = np.ones((3,))
        data_dict = {'data': data}
        probs = mlmodel.predict(data_dict)['probs']
        self.assertTrue(np.allclose(probs, np.ones(3) * 3))

    def build_quant_conv_layer(self, W = None,
                          quantization_type = 'linear',
                          nbits = 8,
                          quant_scale = None,
                          quant_bias = None,
                          quant_lut = None):

        input_features = [('data', datatypes.Array(1, 2, 2))]
        output_features = [('out', datatypes.Array(2, 1, 1))]
        builder = NeuralNetworkBuilder(input_features, output_features)
        builder.add_convolution(name='conv',
                                kernel_channels=1,
                                output_channels=2,
                                height=2, width=2,
                                stride_height=1, stride_width=1,
                                border_mode='valid', groups=1,
                                W=W,
                                b=None, has_bias=False,
                                input_name='data', output_name='out',
                                quantization_type=quantization_type,
                                nbits=nbits,
                                quant_scale=quant_scale,
                                quant_bias=quant_bias,
                                quant_lut=quant_lut)
        return MLModel(builder.spec)

    def test_linear_quant_convolution_8bit(self):
        W = np.ones((2,2,1,2), dtype=np.uint8)
        W[:,:,:,1] = 2
        mlmodel = self.build_quant_conv_layer(W = W.flatten().tobytes(),
                          quantization_type = 'linear',
                          nbits = 8,
                          quant_scale = [4.0],
                          quant_bias = [-2.0])
        data = np.ones((1,2,2))
        data_dict = {'data': data}
        out = mlmodel.predict(data_dict, useCPUOnly=True)['out']
        expected_out = np.reshape(np.array([8, 24]), (2,1,1))
        self.assertTrue(np.allclose(out, expected_out))

    def test_linear_quant_convolution_8bit_vector_scalebias(self):
        W = np.ones((2,2,1,2), dtype=np.uint8)
        W[:,:,:,1] = 2
        mlmodel = self.build_quant_conv_layer(W = W.flatten().tobytes(),
                          quantization_type = 'linear',
                          nbits = 8,
                          quant_scale = [4.0, 5.0],
                          quant_bias = [-2.0, 1.0])
        data = np.ones((1,2,2))
        data_dict = {'data': data}
        out = mlmodel.predict(data_dict, useCPUOnly=True)['out']
        expected_out = np.reshape(np.array([8, 44]), (2,1,1))
        self.assertTrue(np.allclose(out, expected_out))

    def test_lut_quant_convolution_2bit(self):
        W = np.zeros((2,2,1,2), dtype=np.uint8)
        W[:,:,:,0] = 0
        W[:,:,:,1] = 2
        W = _convert_array_to_nbit_quantized_bytes(W.flatten(), 2).tobytes()
        mlmodel = self.build_quant_conv_layer(W = W,
                          quantization_type = 'lut',
                          nbits = 2,
                          quant_lut = [10.0, 11.0, -3.0, -1.0])
        data = np.ones((1,2,2))
        data_dict = {'data': data}
        out = mlmodel.predict(data_dict, useCPUOnly=True)['out']
        expected_out = np.reshape(np.array([40, -12]), (2,1,1))
        self.assertTrue(np.allclose(out, expected_out))

    def test_linear_quant_inner_product_3bit(self):
        W = np.reshape(np.arange(6), (2,3)).astype(np.uint8)
        input_features = [('data', datatypes.Array(3))]
        output_features = [('probs', None)]
        builder = NeuralNetworkBuilder(input_features, output_features)
        builder.add_inner_product(name = 'ip1',
                                  W = _convert_array_to_nbit_quantized_bytes(W.flatten(), 3).tobytes(),
                                  b = None,
                                  input_channels = 3,
                                  output_channels = 2,
                                  has_bias = False,
                                  input_name = 'data',
                                  output_name = 'probs',
                                  quantization_type = 'linear',
                                  nbits = 3,
                                  quant_scale = [11.0, 2.0],
                                  quant_bias = [-2.0, 10.0])
        mlmodel = MLModel(builder.spec)
        data = np.array([1.0, 3.0, 5.0])
        data_dict = {'data': data}
        probs = mlmodel.predict(data_dict)['probs']
        expected_out = np.array([125, 170])
        self.assertTrue(np.allclose(probs.flatten(), expected_out.flatten()))

    def test_lut_quant_inner_product_1bit(self):
        W = np.zeros((2,3), dtype=np.uint8)
        W[0,:] = [0,1,1]
        W[1,:] = [1,0,0]
        input_features = [('data', datatypes.Array(3))]
        output_features = [('probs', None)]
        builder = NeuralNetworkBuilder(input_features, output_features)
        builder.add_inner_product(name = 'ip1',
                                  W = _convert_array_to_nbit_quantized_bytes(W.flatten(), 1).tobytes(),
                                  b = None,
                                  input_channels = 3,
                                  output_channels = 2,
                                  has_bias = False,
                                  input_name = 'data',
                                  output_name = 'probs',
                                  quantization_type = 'lut',
                                  nbits = 1,
                                  quant_lut = [5.0, -3.0])
        mlmodel = MLModel(builder.spec)
        data = np.array([1.0, 3.0, 5.0])
        data_dict = {'data': data}
        probs = mlmodel.predict(data_dict)['probs']
        expected_out = np.array([-19, 37])
        self.assertTrue(np.allclose(probs.flatten(), expected_out.flatten()))

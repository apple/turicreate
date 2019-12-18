import unittest
import numpy as np
import os, shutil
import tempfile
import coremltools
from coremltools.models import datatypes, MLModel
from coremltools.models.neural_network import NeuralNetworkBuilder
from coremltools.models.utils import macos_version
from coremltools.models.neural_network.quantization_utils import \
    _convert_array_to_nbit_quantized_bytes, quantize_weights
import pytest

@unittest.skipIf(macos_version() < (10, 15), 'Only supported on macOS 10.15+')
class ControlFlowCorrectnessTest(unittest.TestCase):

    @classmethod
    def setup_class(cls):
        pass

    def runTest():
        pass

    def _test_model(self, model, input_dict, output_ref, delta=1e-2):
        preds = model.predict(input_dict)
        for name in output_ref:
            ref_val = output_ref[name]
            val = preds[name]
            self.assertTrue(np.allclose(val, ref_val, rtol=delta))

    def test_simple_branch(self):
        """ Test a simple if-else branch network
        """
        input_features = [('data', datatypes.Array(3)), ('cond', datatypes.Array(1))]
        output_features = [('output', None)]

        builder_top = NeuralNetworkBuilder(input_features, output_features, disable_rank5_shape_mapping=True)
        layer = builder_top.add_branch('branch_layer', 'cond')

        builder_ifbranch = NeuralNetworkBuilder(input_features=None, output_features=None, spec=None, nn_spec=layer.branch.ifBranch)
        builder_ifbranch.add_elementwise('mult_layer', input_names=['data'], output_name='output', mode='MULTIPLY', alpha=10)
        builder_elsebranch = NeuralNetworkBuilder(input_features=None, output_features=None, spec=None, nn_spec=layer.branch.elseBranch)
        builder_elsebranch.add_elementwise('add_layer', input_names=['data'], output_name='output', mode='ADD', alpha=10)
        coremltools.models.utils.save_spec(builder_top.spec, '/tmp/simple_branch.mlmodel')
        mlmodel = MLModel(builder_top.spec)

        # True branch case
        input_dict = {'data': np.array(range(1,4), dtype='float'), 'cond': np.array([1], dtype='float')}
        output_ref = {'output': input_dict['data'] * 10}
        self._test_model(mlmodel, input_dict, output_ref)

        # False branch case
        input_dict['cond'] = np.array([0], dtype='float')
        output_ref['output'] = input_dict['data'] + 10
        self._test_model(mlmodel, input_dict, output_ref)

    def test_simple_loop_fixed_iterations(self):
        input_features = [('data', datatypes.Array(1))]
        output_features = [('output', None)]

        builder_top = NeuralNetworkBuilder(input_features, output_features, disable_rank5_shape_mapping=True)
        builder_top.add_copy('copy_1', input_name='data', output_name='output')

        loop_layer = builder_top.add_loop('loop_layer')
        loop_layer.loop.maxLoopIterations = 5
        builder_body = NeuralNetworkBuilder(input_features=None, output_features=None, spec=None, nn_spec=loop_layer.loop.bodyNetwork)
        builder_body.add_elementwise('add', input_names=['output'], output_name='x', mode='ADD', alpha=2)

        builder_body.add_copy('copy_2', input_name='x', output_name='output')
        coremltools.models.utils.save_spec(builder_top.spec, '/tmp/simple_loop_fixed_iterations.mlmodel')
        mlmodel = MLModel(builder_top.spec)

        # True branch case
        input_dict = {'data': np.array([0], dtype='float')}
        output_ref = {'output': np.array([10], dtype='float')}
        self._test_model(mlmodel, input_dict, output_ref)


@unittest.skipIf(macos_version() < (10, 13), 'Only supported on macOS 10.13+')
class BasicNumericCorrectnessTest(unittest.TestCase):

    def _build_nn_with_one_ip_layer(self):
        input_features = [('data', datatypes.Array(3))]
        output_features = [('out', None)]
        builder = NeuralNetworkBuilder(input_features, output_features, disable_rank5_shape_mapping=True)
        w = np.random.uniform(-0.5, 0.5, (3, 3))
        builder.add_inner_product(name='ip1',
                                  W=w,
                                  b=None,
                                  input_channels=3,
                                  output_channels=3,
                                  has_bias=False,
                                  input_name='input',
                                  output_name='hidden')
        return builder

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

    def test_linear_quant_batchedmatmul_5bit(self):
        W = np.zeros((2, 3), dtype=np.uint8)
        W[0, :] = [31, 20, 11]
        W[1, :] = [1, 0, 8]
        quant_scale = np.reshape(np.array([10.0, 2.0, 3.0]), (1, 3))
        quant_bias = np.reshape(np.array([-2.0, -10.0, 6.0]), (1, 3))
        W_unquantized = np.broadcast_to(quant_scale, (2, 3)) * W + np.broadcast_to(quant_bias, (2, 3))
        bias = np.array([1.0, 2.0, 3.0])

        input_features = [('data', datatypes.Array(2, 2))]
        output_features = [('out', None)]
        builder = NeuralNetworkBuilder(input_features, output_features, disable_rank5_shape_mapping=True)
        builder.add_batched_mat_mul(name='batched_matmul',
                                    input_names=['data'], output_name='out',
                                    weight_matrix_rows=2, weight_matrix_columns=3,
                                    W=_convert_array_to_nbit_quantized_bytes(W.flatten(), 5).tobytes(),
                                    bias=bias,
                                    is_quantized_weight=True,
                                    quantization_type='linear',
                                    nbits=5,
                                    quant_scale=quant_scale.flatten(),
                                    quant_bias=quant_bias.flatten())
        mlmodel = MLModel(builder.spec)
        data = np.zeros((2, 2), dtype=np.float32)
        data[0, :] = [5, 6]
        data[1, :] = [10, 12]
        data_dict = {'data': data}
        out = mlmodel.predict(data_dict, useCPUOnly=True)['out']
        expected_out = np.matmul(data, W_unquantized) + bias
        self.assertTrue(out.shape == expected_out.shape)
        self.assertTrue(np.allclose(out.flatten(), expected_out.flatten()))

    def test_linear_quant_batchedmatmul_8bit(self):
        np.random.seed(1988)
        W = np.random.rand(32, 32) * 2.0 - 1
        bias = np.random.rand(32)

        input_features = [('data', datatypes.Array(2, 32))]
        output_features = [('out', None)]
        builder = NeuralNetworkBuilder(input_features, output_features,
                disable_rank5_shape_mapping=True)
        builder.add_batched_mat_mul(
                name='batched_matmul', input_names=['data'],
                output_name='out', weight_matrix_rows=32,
                weight_matrix_columns=32, W=W, bias=bias)
        mlmodel = MLModel(builder.spec)
        q_mlmodel = quantize_weights(mlmodel, 8)
        q_spec = q_mlmodel.get_spec()
        q_layer = q_spec.neuralNetwork.layers[0].batchedMatmul

        self.assertTrue(len(q_layer.weights.floatValue) == 0)
        self.assertTrue(len(q_layer.weights.rawValue) > 0)

        data = np.random.rand(2, 32)
        data_dict = {'data': data}
        out = q_mlmodel.predict(data_dict, useCPUOnly=True)['out']
        expected_out = np.matmul(data, W) + bias
        self.assertTrue(out.shape == expected_out.shape)
        self.assertTrue(np.allclose(out.flatten(), expected_out.flatten(),
                atol=0.1))

    def test_linear_quant_embedding_7bit(self):
        embed_size = 2
        vocab_size = 3
        W = np.zeros((embed_size, vocab_size), dtype=np.uint8)
        W[:, 0] = [100, 127]
        W[:, 1] = [20, 40]
        W[:, 2] = [90, 1]
        quant_scale = np.reshape(np.array([10.0, 2.0]), (2, 1))
        quant_bias = np.reshape(np.array([-2.0, -10.0]), (2, 1))
        W_unquantized = np.broadcast_to(quant_scale, (2, 3)) * W + np.broadcast_to(quant_bias, (2, 3))
        bias = np.reshape(np.array([1.0, 2.0]), (2, 1))
        W_unquantized = W_unquantized + np.broadcast_to(bias, (2, 3))

        input_features = [('data', datatypes.Array(4,1,1,1))]
        output_features = [('out', None)]
        builder = NeuralNetworkBuilder(input_features, output_features, disable_rank5_shape_mapping=True)
        builder.add_embedding(name='embed',
                              W = _convert_array_to_nbit_quantized_bytes(W.flatten(), 7).tobytes(),
                              b = bias,
                              input_dim = vocab_size,
                              output_channels = embed_size,
                              has_bias = True,
                              input_name = 'data', output_name = 'out',
                              is_quantized_weight= True,
                              quantization_type='linear',
                              nbits = 7,
                              quant_scale = quant_scale,
                              quant_bias = quant_bias)

        mlmodel = MLModel(builder.spec)
        data = np.reshape(np.array([2.0, 2.0, 1.0, 0.0]), (4, 1, 1, 1))
        data_dict = {'data': data}
        out = mlmodel.predict(data_dict, useCPUOnly=True)['out']
        self.assertTrue(out.shape == (4, embed_size, 1, 1))
        expected_out = np.zeros((4, embed_size), dtype=np.float32)
        expected_out[0, :] =  W_unquantized[:, 2].flatten()
        expected_out[1, :] = W_unquantized[:, 2].flatten()
        expected_out[2, :] = W_unquantized[:, 1].flatten()
        expected_out[3, :] = W_unquantized[:, 0].flatten()
        self.assertTrue(np.allclose(out.flatten(), expected_out.flatten()))

    def test_lut_quant_embedding_nd_2bit(self):
        embed_size = 2
        vocab_size = 3
        W = np.zeros((embed_size, vocab_size), dtype=np.uint8)
        W[:, 0] = [1, 0]
        W[:, 1] = [0, 1]
        W[:, 2] = [3, 2]
        bias = np.array([1.0, 2.0])
        quant_lut = np.array([34.0, 12.0, -6.0, 6.0])

        input_features = [('data', datatypes.Array(4, 1))]
        output_features = [('out', None)]
        builder = NeuralNetworkBuilder(input_features, output_features, disable_rank5_shape_mapping=True)
        builder.add_embedding_nd(name='embedding_nd',
                                 input_name='data',
                                 output_name='out',
                                 vocab_size=vocab_size, embedding_size=embed_size,
                                 W=_convert_array_to_nbit_quantized_bytes(W.flatten(), 2).tobytes(),
                                 b=bias,
                                 is_quantized_weight=True,
                                 quantization_type='lut',
                                 nbits=2,
                                 quant_lut=quant_lut)

        mlmodel = MLModel(builder.spec)
        data = np.reshape(np.array([2.0, 2.0, 1.0, 0.0]), (4, 1))
        data_dict = {'data': data}
        out = mlmodel.predict(data_dict, useCPUOnly=True)['out']
        expected_out = np.zeros((4, embed_size), dtype=np.float32)
        expected_out[0, :] = [quant_lut[W[0, 2]], quant_lut[W[1, 2]]] + bias
        expected_out[1, :] = [quant_lut[W[0, 2]], quant_lut[W[1, 2]]] + bias
        expected_out[2, :] = [quant_lut[W[0, 1]], quant_lut[W[1, 1]]] + bias
        expected_out[3, :] = [quant_lut[W[0, 0]], quant_lut[W[1, 0]]] + bias
        self.assertTrue(out.shape == expected_out.shape)
        self.assertTrue(np.allclose(out.flatten(), expected_out.flatten()))

    def test_set_input(self):
        builder = self._build_nn_with_one_ip_layer()
        builder.set_input(input_names=['data_renamed'], input_dims=[(2,)])

        self.assertEquals(builder.spec.description.input[0].type.multiArrayType.shape[0], 2)
        self.assertEquals(builder.spec.description.input[0].name, 'data_renamed')

    def test_set_input_fail(self):
        builder = self._build_nn_with_one_ip_layer()

        # fails since input_names and input_dims do not have same size
        with self.assertRaises(ValueError):
            builder.set_input(input_names=['data_1', 'data_2'], input_dims=[(3,)])

    def test_set_output(self):
        builder = self._build_nn_with_one_ip_layer()
        builder.set_output(output_names=['out_renamed'], output_dims=[(2,)])

        self.assertEquals(builder.spec.description.output[0].type.multiArrayType.shape[0], 2)
        self.assertEquals(builder.spec.description.output[0].name, 'out_renamed')

    def test_set_output_fail(self):
        builder = self._build_nn_with_one_ip_layer()

        # fails since output_names and output_dims do not have same size
        with self.assertRaises(ValueError):
            builder.set_output(output_names=['out_1', 'out_2'], output_dims=[(3,)])



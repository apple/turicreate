# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import os, sys
import tensorflow as tf
import numpy as np
import unittest

from test_base import TFNetworkTest


class TFSimpleNetworkTest(TFNetworkTest):

    # Allows you to override common test entry for this class
    # Backend - set use_cpu_only to be True when working on Intel GPU macs
    def _test_tf_model(
            self,
            graph,
            input_shapes,
            output_node_names,
            data_mode='random',
            input_refs=None,
            delta=1e-2,
            use_cpu_only=True,
            graph_optimizations="freeze",  # one of ["freeze", "convert_variables_to_constants", None]
            quantize_tf_model=False):

        super(TFSimpleNetworkTest, self)._test_tf_model(
            graph,
            input_shapes,
            output_node_names,
            data_mode=data_mode,
            input_refs=input_refs,
            delta=delta,
            use_cpu_only=use_cpu_only,
            graph_optimizations=graph_optimizations,
            quantize_tf_model=quantize_tf_model)

    def test_simple_matmul(self):
        graph = tf.Graph()
        with graph.as_default():
            matrix1 = tf.placeholder(tf.float32, shape=[1, 2], name='input')
            matrix2 = tf.Variable(tf.truncated_normal([2, 3]))
            product = tf.matmul(matrix1, matrix2, name='product')
        self._test_tf_model(graph, {'input': [1, 2]}, ['product'])

    def test_matmul_transposed_weight(self):
        graph = tf.Graph()
        with graph.as_default():
            matrix1 = tf.placeholder(tf.float32, shape=[1, 2], name='input')
            matrix2 = tf.Variable(tf.truncated_normal([3, 2]))
            product = tf.matmul(matrix1, matrix2, transpose_b=True, name='product')
            bias = tf.Variable(tf.truncated_normal([3]))
            y = tf.nn.bias_add(product, bias, name='y')

        self._test_tf_model(graph, {'input': [1, 2]}, ['y'])

    def test_variable_assign(self):
        graph = tf.Graph()
        with graph.as_default():
            x = tf.placeholder(tf.float32, shape=[1, ], name='input')
            y = tf.Variable(0.0, dtype=tf.float32, name='y')

            # We set our assign op
            assign_op = tf.assign(y, y + 10)

            with tf.control_dependencies([assign_op]):
                out = tf.multiply(x, y, name='output')

        self._test_tf_model(graph, {'input': [1, ]}, ['output', 'y'],
                            graph_optimizations=None)

    def test_control_dependency_with_no_op(self):
        graph = tf.Graph()
        with graph.as_default():
            x = tf.placeholder(tf.float32, shape=[1, ], name='input')
            y = tf.Variable(0.0, dtype=tf.float32, name='y')

            assign_op = tf.assign(y, y + 10)

            with tf.control_dependencies([assign_op]):
                c = tf.no_op()

            with tf.control_dependencies([c]):
                d = tf.no_op()

            with tf.control_dependencies([c, d]):
                e = tf.no_op()

            with tf.control_dependencies([e]):
                out = tf.multiply(x, y, name='output')

        self._test_tf_model(graph, {'input': [1, ]}, ['output', 'y'],
                            graph_optimizations=None)

    def test_matmul_biasadd_sub(self):
        graph = tf.Graph()
        with graph.as_default():
            x = tf.placeholder(tf.float32, shape=[None, 2], name='input')
            weight = tf.Variable(tf.truncated_normal([2, 3]))
            y = tf.matmul(x, weight)
            bias = tf.Variable(tf.truncated_normal([3]))
            z0 = tf.nn.bias_add(y, bias)
            c = tf.Variable(tf.truncated_normal([3]))
            z = tf.subtract(z0, c, name='output')
        self._test_tf_model(graph, {'input': [1, 2]}, ['output'])

    def test_matmul_transpose(self):
        graph = tf.Graph()
        with graph.as_default():
            matrix1 = tf.placeholder(tf.float32, shape=[1, 5], name='input')
            matrix2 = tf.Variable(tf.truncated_normal([5, 3]))
            product = tf.matmul(matrix1, matrix2, name='product')
            tp = tf.transpose(product, [0, 1], name='tp')
        self._test_tf_model(graph, {'input': [1, 5]}, ['tp'])

    def test_matmul_unstack(self):
        graph = tf.Graph()
        with graph.as_default():
            matrix1 = tf.placeholder(tf.float32, shape=[2, 5], name='input')
            matrix2 = tf.Variable(tf.truncated_normal([5, 3]))
            product = tf.matmul(matrix1, matrix2, name='product')
            y1, y2 = tf.unstack(product)
            y1 = tf.identity(y1, name='output_1')
            y2 = tf.identity(y2, name='output_2')
        self._test_tf_model(graph, {'input': [2, 5]}, ['output_1', 'output_2'])

    def test_dense_activations(self):
        # TODO - Add other activations
        for act_type in ['sigmoid', 'tanh']:
            graph = tf.Graph()
            with graph.as_default():
                matrix1 = tf.placeholder(tf.float32, shape=[1, 8], name='input')
                matrix2 = tf.Variable(tf.truncated_normal([8, 2]))
                product = tf.matmul(matrix1, matrix2, name='product')
                if act_type == 'sigmoid':
                    act = tf.sigmoid(product, name='act')
                elif act_type == 'tanh':
                    act = tf.tanh(product, name='act')
            self._test_tf_model(graph, {'input': [1, 8]}, ['act'])

    def test_extract_shape(self):
        dims = [2, 3, 4]
        for rank in range(1, len(dims) + 1):
            shape = [None] + dims[:rank]
            batched_shape = [1] + dims[:rank]
            graph = tf.Graph()
            with graph.as_default():
                x = tf.placeholder(tf.float32, shape=batched_shape, name='input')
                m = tf.Variable(tf.truncated_normal(tf.shape(x)))
                y = tf.identity(x + m, name='output')
            self._test_tf_model(graph, {'input': batched_shape}, ['output'])

    @unittest.skip
    def test_shape_slice(self):
        seqlen = 2
        graph = tf.Graph()
        with graph.as_default():
            data = tf.placeholder(
                tf.float32, [1, None, 1], name='input')  # (batch_size, seq_len, input_dim)
            m = tf.Variable(tf.truncated_normal([1, 1, 1]))
            data_t = tf.transpose(data + m, [1, 0, 2], name='tp')
            data_shape = tf.shape(data_t)
            output = tf.identity(data_shape[0], name='output')  # What is the slice here?
        self._test_tf_model(graph, {'input': [1, seqlen, 1]}, ['output'])

    @unittest.skip
    # "Backend exception: \"Invalid blob shape\": scatter_kernel_cpu: Invalid shape of input blob"
    def test_array_scatter(self):
        batch_size = 2
        graph = tf.Graph()
        with graph.as_default():
            data = tf.placeholder(
                tf.float32, shape=[batch_size, 3], name='input')  # (batch_size, input_dim)
            m = tf.Variable(tf.truncated_normal([batch_size, 3]))
            arr = tf.TensorArray(size=2, element_shape=[batch_size, 3], dtype=tf.float32)
            arr = arr.write(0, data)
            arr = arr.write(1, m)
            output = arr.gather([0, 1], name='output')
        self._test_tf_model(graph, {'input': [batch_size, 3]}, ['output'])

    def test_range(self):
        graph = tf.Graph()
        with graph.as_default():
            data = tf.placeholder(tf.int32, shape=(), name='input')  # input is a scalar
            m = tf.Variable(1)
            output = tf.range(0, data + m, 1, name='output')
        self._test_tf_model(graph, {'input': []}, ['output'], input_refs={'input': 1})

    def test_simple_loop(self):
        graph = tf.Graph()
        with graph.as_default():
            data = tf.placeholder(tf.float32, shape=[None, 2], name='data')
            i = tf.constant(0)
            # When providing placeholder directly into while loop structures,
            # placeholder must be the first one.
            c = lambda x, i, v: tf.less(i, 10)
            b = lambda x, i, v: (tf.add(x, v), i + 1, v)  # Dummy
            w = tf.Variable(2.0, dtype=tf.float32, name='weight')
            r = tf.while_loop(c, b, [data, i, w], name='output')

        self._test_tf_model(graph, {"data": [1, 2]}, ["output/Exit"])


    def test_simple_branch(self):
        graph = tf.Graph()
        with graph.as_default():
            data = tf.placeholder(tf.float32, shape=[None, 2], name='data')
            switch = tf.placeholder(tf.float32, shape=(), name='switch')
            m = tf.Variable(1.0)
            result = tf.cond(switch > 0,
                lambda: tf.add(data, m),
                lambda: tf.subtract(data, m),
                name='output')

        self._test_tf_model(graph=graph,
            input_shapes={"data": [1, 2], "switch": []},
            output_node_names=[result.op.name],
            input_refs={'data': np.array([0.1, 0.2]).reshape((1,2)),
                        'switch': 1.0})

        self._test_tf_model(graph=graph,
            input_shapes={"data": [1, 2], "switch": []},
            output_node_names=[result.op.name],
            input_refs={'data': np.array([0.1, 0.2]).reshape((1,2)),
                        'switch': -1.0})

    def test_onehot_matmul_encoding(self):
        seq_len = 6
        embedding_dim = 10  # depth
        out_channels = 4
        graph = tf.Graph()
        with graph.as_default():
            indices = tf.placeholder(tf.int32, shape=[None, seq_len], name='indices')
            onehot = tf.one_hot(indices, depth=embedding_dim)  # (batch_size, seq_len, embedding_dim)
            weight = tf.Variable(tf.truncated_normal([1, embedding_dim, out_channels]))
            y = tf.matmul(onehot, weight, name='output')

        self._test_tf_model(graph, {"indices": [1, seq_len]}, ["output"], data_mode='linear')

    def test_two_input_batch_matmul(self):
        test_cases = [
            {'r_x': 6, 'c_x': 10, 'r_y': 10, 'c_y': 4, 'transpose_x': False, 'transpose_y': False},
            {'r_x': 6, 'c_x': 10, 'r_y': 4, 'c_y': 10, 'transpose_x': False, 'transpose_y': True}
        ]
        # r_o, c_o = 6, 4
        for tc in test_cases:
            graph = tf.Graph()
            with graph.as_default():
                r_x, c_x, r_y, c_y, tp_x, tp_y = tc['r_x'], tc['c_x'], tc['r_y'], tc['c_y'], tc['transpose_x'], tc['transpose_y']
                data_shape = [1, r_x, c_x]
                weight_shape = [1, r_y, c_y]
                input_data = tf.placeholder(tf.float32, shape=data_shape, name='input_data')
                input_weight = tf.placeholder(tf.float32, shape=weight_shape, name='input_weight')
                y = tf.matmul(input_data, input_weight, name='output', transpose_a=tp_x, transpose_b=tp_y)
            self._test_tf_model(graph, {"input_data": data_shape, "input_weight": weight_shape}, ["output"],
                                graph_optimizations=None)

    def test_layer_norm(self):
        shapes = [(3, 4), (3, 4, 5), (3, 4, 5, 6)]
        for shape in shapes:
            graph = tf.Graph()
            with graph.as_default():
                x = tf.placeholder(tf.float32, shape=shape, name='input')
                y = tf.contrib.layers.layer_norm(x, begin_norm_axis=-1,
                                                 begin_params_axis=-1)
                z = tf.identity(y, name='output')
            self._test_tf_model(graph, {'input': shape}, ['output'])

    def test_gelu_tanh_approx(self):
        def gelu(x):
            cdf = 0.5 * (1.0 + tf.tanh(
                (np.sqrt(2 / np.pi) * (x + 0.044715 * tf.pow(x, 3)))))
            return x * cdf

        shapes = [(3, 4), (3, 4, 5), (3, 4, 5, 6)]
        for shape in shapes:
            graph = tf.Graph()
            with graph.as_default():
                x = tf.placeholder(tf.float32, shape=shape, name='input')
                y = gelu(x)
                z = tf.identity(y, name='output')
            self._test_tf_model_constant(graph, {'input': shape}, ['output'])


if __name__ == '__main__':
    # unittest.main()
    suite = unittest.TestSuite()
    suite.addTest(TFSimpleNetworkTest("test_simple_branch"))
    unittest.TextTestRunner().run(suite)

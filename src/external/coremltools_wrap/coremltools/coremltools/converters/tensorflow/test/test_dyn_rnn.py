# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import tensorflow as tf
import numpy as np
import unittest

from test_base import TFNetworkTest


class TFDynRNNTest(TFNetworkTest):

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
            quantize_tf_model=False,
            quantize_mlmodel=False,
            quantize_config={}):
        super(TFDynRNNTest, self)._test_tf_model(
            graph,
            input_shapes,
            output_node_names,
            data_mode=data_mode,
            input_refs=input_refs,
            delta=delta,
            use_cpu_only=use_cpu_only,
            graph_optimizations=graph_optimizations,
            quantize_tf_model=quantize_tf_model,
            quantize_mlmodel=quantize_mlmodel,
            quantize_config=quantize_config)

    def test_simple_lstm(self, **kwargs):
        # (batch_size, seq_len, input_dim)
        batch_size, sequence_length, hidden_size = 1, 5, 10
        input_shape = [batch_size, sequence_length, hidden_size]

        graph = tf.Graph()
        with graph.as_default() as g:
            lstm_initializer = tf.constant_initializer(0.8)

            data = tf.placeholder(tf.float32, input_shape, name='input')
            cell = tf.nn.rnn_cell.LSTMCell(
                hidden_size, state_is_tuple=True, initializer=lstm_initializer)

            init_state = cell.zero_state(batch_size, dtype=tf.float32)
            val, state = tf.nn.dynamic_rnn(cell, data, initial_state=init_state, dtype=tf.float32)

        self._test_tf_model(graph, {'input': input_shape},
            [val.op.name, state[0].op.name, state[1].op.name],
            quantize_mlmodel=kwargs.get('quantize_mlmodel',False),
            quantize_config=kwargs.get('quantize_config', {}))

    def test_simple_lstm_quantized_8bit_linear(self):
        np.random.seed(2019)
        quantize_config = {'nbits': 8, 'mode': 'linear'}
        self.test_simple_lstm(quantize_mlmodel=True,
                              quantize_config=quantize_config)

    def test_lstm_block_fused_cell(self):
        sequence_length, batch_size, input_dim, hidden_size = 5, 1, 24, 32
        input_shape = [sequence_length, batch_size, input_dim]
        forget_bias = np.random.rand()
        graph = tf.Graph()
        with graph.as_default():
            data = tf.placeholder(tf.float32, input_shape, name='input')
            lstm_block_cell = tf.contrib.rnn.LSTMBlockFusedCell(
                num_units=hidden_size, forget_bias=forget_bias)
            _, state = lstm_block_cell(data, dtype=tf.float32)
        self._test_tf_model(graph, {'input': input_shape}, [state[0].op.name, state[1].op.name])

    def test_simple_rnn(self):
        batch_size, sequence_length, hidden_size = 1, 5, 10
        input_shape = [batch_size, sequence_length, hidden_size]  # (batch_size, seq_len, input_dim)
        graph = tf.Graph()
        with graph.as_default() as g:
            rnn_initializer = tf.constant_initializer(0.8)

            data = tf.placeholder(tf.float32, input_shape, name='input')
            cell = tf.nn.rnn_cell.BasicRNNCell(hidden_size)

            init_state = cell.zero_state(batch_size, dtype=tf.float32)
            val, state = tf.nn.dynamic_rnn(cell, data, initial_state=init_state, dtype=tf.float32)

        self._test_tf_model(graph, {'input': input_shape}, [val.op.name, state.op.name])

    def test_simple_bilstm(self):
        batch_size, sequence_length, hidden_size = 1, 5, 10
        input_shape = [batch_size, sequence_length, hidden_size]  # (batch_size, seq_len, input_dim)
        graph = tf.Graph()
        with graph.as_default() as g:
            data = tf.placeholder(tf.float32, input_shape, name='input')
            fw_cell = tf.nn.rnn_cell.LSTMCell(hidden_size, state_is_tuple=True)
            bw_cell = tf.nn.rnn_cell.LSTMCell(hidden_size, state_is_tuple=True)

            init_state = fw_cell.zero_state(batch_size, dtype=tf.float32)
            val, states = tf.nn.bidirectional_dynamic_rnn(fw_cell, bw_cell, data, dtype=tf.float32)

        output_node_names = [
            x.op.name
            for x in [val[0], val[1], states[0][0], states[0][1], states[1][0], states[1][1]]
        ]
        self._test_tf_model(graph, {'input': input_shape}, output_node_names)

    @unittest.skip
    def test_batched_bilstm(self):
        batch_size, max_sequence_length, hidden_size = 4, 5, 10
        input_shape = [
            batch_size, max_sequence_length, hidden_size
        ]  # (batch_size, seq_len, input_dim)
        graph = tf.Graph()
        with graph.as_default() as g:
            data = tf.placeholder(tf.float32, input_shape, name='input')
            fw_cell = tf.nn.rnn_cell.LSTMCell(hidden_size, state_is_tuple=True)
            bw_cell = tf.nn.rnn_cell.LSTMCell(hidden_size, state_is_tuple=True)

            init_state = fw_cell.zero_state(batch_size, dtype=tf.float32)
            val, states = tf.nn.bidirectional_dynamic_rnn(
                fw_cell, bw_cell, data, sequence_length=[2, 3, 4, 5], dtype=tf.float32)

        output_node_names = [
            x.op.name
            for x in [val[0], val[1], states[0][0], states[0][1], states[1][0], states[1][1]]
        ]
        self._test_tf_model(graph, {'input': input_shape}, output_node_names)


if __name__ == '__main__':
    unittest.main()
    # suite = unittest.TestSuite()
    # suite.addTest(TFDynRNNTest('test_lstm_block_fused_cell'))
    # unittest.TextTestRunner().run(suite)

# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import turicreate.toolkits._tf_utils as _utils
import tensorflow.compat.v1 as _tf

_tf.disable_v2_behavior()
from .._tf_model import TensorFlowModel

import numpy as _np

# Constant parameters for the neural network
CONV_H = 64
LSTM_H = 200
DENSE_H = 128

class ActivityTensorFlowModel(TensorFlowModel):
    def __init__(
        self,
        net_params,
        batch_size,
        num_features,
        num_classes,
        prediction_window,
        seq_len,
        seed
    ):

        _utils.suppress_tensorflow_warnings()
        self.gpu_policy = _utils.TensorFlowGPUPolicy()
        self.gpu_policy.start()

        for key in net_params.keys():
            net_params[key] = _utils.convert_shared_float_array_to_numpy(
                net_params[key]
            )

        self.ac_graph = _tf.Graph()
        self.num_classes = num_classes
        self.batch_size = batch_size
        self.seq_len = seq_len
        self.sess = _tf.Session(graph=self.ac_graph)
        with self.ac_graph.as_default():
            self.init_activity_classifier_graph(
                net_params, num_features, prediction_window, seed
            )

    def init_activity_classifier_graph(
        self, net_params, num_features, prediction_window, seed
    ):
        # Vars
        self.data = _tf.placeholder(
            _tf.float32, [None, prediction_window * self.seq_len, num_features]
        )
        self.weight = _tf.placeholder(_tf.float32, [None, self.seq_len, 1])
        self.target = _tf.placeholder(_tf.int32, [None, self.seq_len, 1])
        self.is_training = _tf.placeholder(_tf.bool)

        # Reshaping weights
        reshaped_weight = _tf.reshape(self.weight, [self.batch_size, self.seq_len])

        # One hot encoding target
        reshaped_target = _tf.reshape(self.target, [self.batch_size, self.seq_len])
        one_hot_target = _tf.one_hot(reshaped_target, depth=self.num_classes, axis=-1)

        # Weights
        self.weights = {
            "conv_weight": _tf.Variable(
                _tf.zeros([prediction_window, num_features, CONV_H]), name="conv_weight"
            ),
            "dense0_weight": _tf.Variable(
                _tf.zeros([LSTM_H, DENSE_H]), name="dense0_weight"
            ),
            "dense1_weight": _tf.Variable(
                _tf.zeros([DENSE_H, self.num_classes]), name="dense1_weight"
            ),
        }

        # Biases
        self.biases = {
            "conv_bias": _tf.Variable(_tf.zeros([CONV_H]), name="conv_bias"),
            "dense0_bias": _tf.Variable(_tf.zeros([DENSE_H]), name="dense0_bias"),
            "dense1_bias": _tf.Variable(
                _tf.zeros([self.num_classes]), name="dense1_bias"
            ),
        }

        # Convolution
        conv = _tf.nn.conv1d(
            self.data,
            self.weights["conv_weight"],
            stride=prediction_window,
            padding="SAME",
        )
        conv = _tf.nn.bias_add(conv, self.biases["conv_bias"])
        conv = _tf.nn.relu(conv)

        dropout = _tf.layers.dropout(conv, rate=0.2, training=self.is_training, seed=seed)

        # Long Stem Term Memory
        lstm = self.load_lstm_weights_params(net_params)
        cells = _tf.nn.rnn_cell.LSTMCell(
            num_units=LSTM_H,
            reuse=_tf.AUTO_REUSE,
            forget_bias=0.0,
            initializer=_tf.initializers.constant(lstm, verify_shape=True),
        )
        init_state = cells.zero_state(self.batch_size, _tf.float32)
        rnn_outputs, final_state = _tf.nn.dynamic_rnn(
            cells, dropout, initial_state=init_state
        )

        # Dense
        dense = _tf.reshape(rnn_outputs, (-1, LSTM_H))
        dense = _tf.add(
            _tf.matmul(dense, self.weights["dense0_weight"]), self.biases["dense0_bias"]
        )
        dense = _tf.layers.batch_normalization(
            inputs=dense,
            beta_initializer=_tf.initializers.constant(
                net_params["bn_beta"], verify_shape=True
            ),
            gamma_initializer=_tf.initializers.constant(
                net_params["bn_gamma"], verify_shape=True
            ),
            moving_mean_initializer=_tf.initializers.constant(
                net_params["bn_running_mean"], verify_shape=True
            ),
            moving_variance_initializer=_tf.initializers.constant(
                net_params["bn_running_var"], verify_shape=True
            ),
            training=self.is_training,
        )

        dense = _tf.nn.relu(dense)
        dense = _tf.layers.dropout(dense, rate=0.5, training=self.is_training, seed=seed)

        # Output
        out = _tf.add(
            _tf.matmul(dense, self.weights["dense1_weight"]), self.biases["dense1_bias"]
        )
        out = _tf.reshape(out, (-1, self.seq_len, self.num_classes))
        self.probs = _tf.nn.softmax(out)

        # Weights
        seq_sum_weights = _tf.reduce_sum(reshaped_weight, axis=1)
        binary_seq_sum_weights = _tf.reduce_sum(
            _tf.cast(seq_sum_weights > 0, dtype=_tf.float32)
        )

        # Loss
        loss = _tf.losses.softmax_cross_entropy(
            logits=out,
            onehot_labels=one_hot_target,
            weights=reshaped_weight,
            reduction=_tf.losses.Reduction.NONE,
        )
        self.loss_per_seq = _tf.reduce_sum(loss, axis=1) / (seq_sum_weights + 1e-5)
        self.loss_op = _tf.reduce_sum(self.loss_per_seq) / (
            binary_seq_sum_weights + 1e-5
        )

        # Optimizer
        update_ops = _tf.get_collection(_tf.GraphKeys.UPDATE_OPS)
        self.set_learning_rate(1e-3)
        train_op = self.optimizer.minimize(self.loss_op)
        self.train_op = _tf.group([train_op, update_ops])

        # Initialize all variables
        self.sess.run(_tf.global_variables_initializer())
        self.sess.run(_tf.local_variables_initializer())

        self.load_weights(net_params)

    def __del__(self):
        self.sess.close()
        self.gpu_policy.stop()

    def load_lstm_weights_params(self, net_params):
        """
        Function to load lstm weights from the C++ implementation into TensorFlow

        Parameters
        ----------
        net_params: Dictionary
            Dict with weights from the C++ implementation and  its names

        Returns
        -------

        lstm: lstm weights in Tensorflow Format
        """
        i2h_i = net_params["lstm_i2h_i_weight"]
        i2h_f = net_params["lstm_i2h_f_weight"]
        i2h_c = net_params["lstm_i2h_c_weight"]
        i2h_o = net_params["lstm_i2h_o_weight"]
        h2h_i = net_params["lstm_h2h_i_weight"]
        h2h_f = net_params["lstm_h2h_f_weight"]
        h2h_c = net_params["lstm_h2h_c_weight"]
        h2h_o = net_params["lstm_h2h_o_weight"]
        lstm = _utils.convert_lstm_weight_coreml_to_tf(
            i2h_i, i2h_c, i2h_f, i2h_o, h2h_i, h2h_c, h2h_f, h2h_o
        )
        return lstm

    def load_weights(self, net_params):
        """
        Function to load weights from the C++ implementation into TensorFlow

        Parameters
        ----------
        net_params: Dictionary
            Dict with weights from the C++ implementation and  its names

        """
        for key in net_params.keys():
            if key in self.weights.keys():
                if key.startswith("conv"):
                    net_params[key] = _utils.convert_conv1d_coreml_to_tf(
                        net_params[key]
                    )
                    self.sess.run(
                        _tf.assign(
                            _tf.get_default_graph().get_tensor_by_name(key + ":0"),
                            net_params[key],
                        )
                    )
                elif key.startswith("dense"):
                    net_params[key] = _utils.convert_dense_coreml_to_tf(net_params[key])
                    self.sess.run(
                        _tf.assign(
                            _tf.get_default_graph().get_tensor_by_name(key + ":0"),
                            net_params[key],
                        )
                    )
            elif key in self.biases.keys():
                self.sess.run(
                    _tf.assign(
                        _tf.get_default_graph().get_tensor_by_name(key + ":0"),
                        net_params[key],
                    )
                )

        h2h_i_bias = net_params["lstm_h2h_i_bias"]
        h2h_c_bias = net_params["lstm_h2h_c_bias"]
        h2h_f_bias = net_params["lstm_h2h_f_bias"]
        h2h_o_bias = net_params["lstm_h2h_o_bias"]
        lstm_bias = _utils.convert_lstm_bias_coreml_to_tf(
            h2h_i_bias, h2h_c_bias, h2h_f_bias, h2h_o_bias
        )
        self.sess.run(
            _tf.assign(
                _tf.get_default_graph().get_tensor_by_name("rnn/lstm_cell/bias:0"),
                lstm_bias,
            )
        )

    def train(self, feed_dict):
        """
        Run session for training with new batch of data (inputs, labels and weights)

        Parameters
        ----------
        feed_dict: Dictionary
            Dictionary to store a batch of input data, corresponding labels and weights. This is currently
            passed from the ac_data_iterator.cpp file when a new batch of data is sent.

        Returns
        -------
        result: Dictionary
            Loss per batch and probabilities
        """
        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])
            feed_dict[key] = _np.squeeze(feed_dict[key], axis=1)
            feed_dict[key] = _np.reshape(
                feed_dict[key],
                (
                    feed_dict[key].shape[0],
                    feed_dict[key].shape[1],
                    feed_dict[key].shape[2],
                ),
            )

        _, loss, probs = self.sess.run(
            [self.train_op, self.loss_per_seq, self.probs],
            feed_dict={
                self.data: feed_dict["input"],
                self.target: feed_dict["labels"],
                self.weight: feed_dict["weights"],
                self.is_training: True,
            },
        )

        prob = _np.array(probs)
        probabilities = _np.reshape(
            prob, (prob.shape[0], prob.shape[1] * prob.shape[2])
        )
        result = {"loss": _np.array(loss), "output": probabilities}
        return result

    def predict(self, feed_dict):

        """
        Run session for predicting with new batch of validation data (inputs, labels and weights) as well as test data (inputs)

        Parameters
        ----------
        feed_dict: Dictionary
            Dictionary to store a batch of input data, corresponding labels and weights. This is currently
            passed from the ac_data_iterator.cpp file when a new batch of data is sent.

        Returns
        -------
        result: Dictionary
            Loss per batch and probabilities (in case of validation data)
            Probabilities (in case only inputs are provided)
        """

        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])
            feed_dict[key] = _np.squeeze(feed_dict[key], axis=1)
            feed_dict[key] = _np.reshape(
                feed_dict[key],
                (
                    feed_dict[key].shape[0],
                    feed_dict[key].shape[1],
                    feed_dict[key].shape[2],
                ),
            )

        if len(feed_dict.keys()) == 1:
            probs = self.sess.run(
                self.probs,
                feed_dict={self.data: feed_dict["input"], self.is_training: False},
            )
            prob = _np.array(probs)
            probabilities = _np.reshape(
                prob, (prob.shape[0], prob.shape[1] * prob.shape[2])
            )
            result = {"output": probabilities}
        else:
            loss, probs = self.sess.run(
                [self.loss_per_seq, self.probs],
                feed_dict={
                    self.data: feed_dict["input"],
                    self.target: feed_dict["labels"],
                    self.weight: feed_dict["weights"],
                    self.is_training: False,
                },
            )
            prob = _np.array(probs)
            probabilities = _np.reshape(
                prob, (prob.shape[0], prob.shape[1] * prob.shape[2])
            )
            result = {"loss": _np.array(loss), "output": probabilities}
        return result

    def export_weights(self):
        """
        Function to store TensorFlow weights back to into a dict in CoreML format to be used
        by the C++ implementation

        Returns
        -------
        tf_export_params: Dictionary
            Dictionary of weights from TensorFlow stored as {weight_name: weight_value}
        """
        tf_export_params = {}
        with self.ac_graph.as_default():
            tvars = _tf.trainable_variables()
            tvars_vals = self.sess.run(tvars)

        for var, val in zip(tvars, tvars_vals):
            if "weight" in var.name:
                if var.name.startswith("conv"):

                    tf_export_params[
                        var.name.split(":")[0]
                    ] = _utils.convert_conv1d_tf_to_coreml(val)
                elif var.name.startswith("dense"):
                    tf_export_params[
                        var.name.split(":")[0]
                    ] = _utils.convert_dense_tf_to_coreml(val)
            elif var.name.startswith("rnn/lstm_cell/kernel"):
                (
                    i2h_i,
                    i2h_c,
                    i2h_f,
                    i2h_o,
                    h2h_i,
                    h2h_c,
                    h2h_f,
                    h2h_o,
                ) = _utils.convert_lstm_weight_tf_to_coreml(val, CONV_H)
                tf_export_params["lstm_i2h_i_weight"] = i2h_i
                tf_export_params["lstm_i2h_c_weight"] = i2h_c
                tf_export_params["lstm_i2h_f_weight"] = i2h_f
                tf_export_params["lstm_i2h_o_weight"] = i2h_o
                tf_export_params["lstm_h2h_i_weight"] = h2h_i
                tf_export_params["lstm_h2h_c_weight"] = h2h_c
                tf_export_params["lstm_h2h_f_weight"] = h2h_f
                tf_export_params["lstm_h2h_o_weight"] = h2h_o
            elif var.name.startswith("rnn/lstm_cell/bias"):
                (
                    h2h_i_bias,
                    h2h_c_bias,
                    h2h_f_bias,
                    h2h_o_bias,
                ) = _utils.convert_lstm_bias_tf_to_coreml(val)
                tf_export_params["lstm_h2h_i_bias"] = h2h_i_bias
                tf_export_params["lstm_h2h_c_bias"] = h2h_c_bias
                tf_export_params["lstm_h2h_f_bias"] = h2h_f_bias
                tf_export_params["lstm_h2h_o_bias"] = h2h_o_bias
            elif var.name.startswith("batch_normalization"):
                tf_export_params["bn_" + var.name.split("/")[-1][0:-2]] = _np.array(val)
            else:
                tf_export_params[var.name.split(":")[0]] = _np.array(val)

        tvars = _tf.global_variables()
        tvars_vals = self.sess.run(tvars)
        for var, val in zip(tvars, tvars_vals):
            if "moving_mean" in var.name:
                tf_export_params["bn_running_mean"] = _np.array(val)
            if "moving_variance" in var.name:
                tf_export_params["bn_running_var"] = _np.array(val)
        for layer_name in tf_export_params.keys():
            tf_export_params[layer_name] = _np.ascontiguousarray(
                tf_export_params[layer_name]
            )
        return tf_export_params

    def set_learning_rate(self, lr):
        """
        Set the learning rate

        Parameters
        ----------
        lr: float32
            Learning rate

        """
        self.optimizer = _tf.train.AdamOptimizer(learning_rate=lr)

# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import turicreate.toolkits._tf_utils as _utils
from .._tf_model import TensorFlowModel

import numpy as _np

from turicreate._deps.minimal_package import _minimal_package_import_check


def _lazy_import_tensorflow():
    _tf = _minimal_package_import_check("tensorflow")
    return _tf

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
        seed,
    ):
        _utils.suppress_tensorflow_warnings()

        self.num_classes = num_classes
        self.batch_size = batch_size

        tf = _lazy_import_tensorflow()
        keras = tf.keras

        #############################################
        # Define the Neural Network
        #############################################
        inputs = keras.Input(shape=(prediction_window * seq_len, num_features))

        # First dense layer
        dense = keras.layers.Conv1D(
            filters=CONV_H,
            kernel_size=(prediction_window),
            padding='same',
            strides=prediction_window,
            use_bias=True,
            activation='relu',
        )
        cur_outputs = dense(inputs)

        # First dropout layer
        dropout = keras.layers.Dropout(
            rate=0.2,
            seed=seed,
        )
        cur_outputs = dropout(cur_outputs)

        # LSTM layer
        lstm = keras.layers.LSTM(
            units=LSTM_H,
            return_sequences=True,
            use_bias=True,
        )
        cur_outputs = lstm(cur_outputs)

        # Second dense layer
        dense2 = keras.layers.Dense(DENSE_H)
        cur_outputs = dense2(cur_outputs)

        # Batch norm layer
        batch_norm = keras.layers.BatchNormalization()
        cur_outputs = batch_norm(cur_outputs)

        # ReLU layer
        relu = keras.layers.ReLU()
        cur_outputs = relu(cur_outputs)

        # Final dropout layer
        dropout = keras.layers.Dropout(rate=0.5, seed=seed)
        cur_outputs = dropout(cur_outputs)

        # Final dense layer
        dense3 = keras.layers.Dense(num_classes, use_bias=False)
        cur_outputs = dense3(cur_outputs)

        # Softmax layer
        softmax = keras.layers.Softmax()
        cur_outputs = softmax(cur_outputs)

        self.model = keras.Model(inputs=inputs, outputs=cur_outputs)
        self.model.compile(
            loss=tf.losses.categorical_crossentropy,
            optimizer=keras.optimizers.Adam(learning_rate=1e-3),
            sample_weight_mode="temporal"
        )

        #############################################
        # Load the Weights of the Neural Network
        #############################################
        for key in net_params.keys():
            net_params[key] = _utils.convert_shared_float_array_to_numpy(net_params[key])

        # Set weight for first dense layer
        l = self.model.layers[1]
        l.set_weights(
            (_utils.convert_conv1d_coreml_to_tf(net_params["conv_weight"]),
             net_params["conv_bias"])
        )

        # Set LSTM weights
        i2h, h2h, bias = [], [], []
        for i in ('i', 'f', 'c', 'o'):
            i2h.append(eval('net_params["lstm_i2h_%s_weight"]' % i))
            h2h.append(eval('net_params["lstm_h2h_%s_weight"]' % i))
            bias.append(eval('net_params["lstm_h2h_%s_bias"]' % i))
        i2h = _np.concatenate(i2h, axis=0)
        h2h = _np.concatenate(h2h, axis=0)
        bias = _np.concatenate(bias, axis=0)
        i2h = _np.swapaxes(i2h, 1, 0)
        h2h = _np.swapaxes(h2h, 1, 0)
        l = self.model.layers[3]
        l.set_weights((i2h, h2h, bias))

        # Set weight for second dense layer
        l = self.model.layers[4]
        l.set_weights(
            (
                net_params['dense0_weight'].reshape(DENSE_H, LSTM_H).swapaxes(0, 1),
                net_params['dense0_bias']
            )
        )

        # Set batch Norm weights
        l = self.model.layers[5]
        l.set_weights(
            (
                net_params['bn_gamma'],
                net_params['bn_beta'],
                net_params['bn_running_mean'],
                net_params['bn_running_var']
            )
        )

        # Set weights for last dense layer
        l = self.model.layers[8]
        l.set_weights(
            (
                net_params['dense1_weight'].reshape((self.num_classes, DENSE_H)).swapaxes(0,1),
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


        keras = _lazy_import_tensorflow().keras
        loss = self.model.train_on_batch(
            x=feed_dict['input'],
            y=keras.utils.to_categorical(feed_dict['labels'], num_classes=self.num_classes),
            sample_weight=_np.reshape(feed_dict['weights'], (self.batch_size, 20))
        )

        prob = self.model.predict(feed_dict['input'])
        probabilities = _np.reshape(
            prob, (prob.shape[0], prob.shape[1] * prob.shape[2])
        )

        result = {"loss": _np.array(loss), "output": _np.array(probabilities)}
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
        # Convert input
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

        # Generate predictions
        prob = self.model.predict(feed_dict['input'])
        probabilities = _np.reshape(
            prob, (prob.shape[0], prob.shape[1] * prob.shape[2])
        )
        result = {"output": probabilities}

        if "labels" in feed_dict.keys():  # Validation data?
            keras = _lazy_import_tensorflow().keras
            labels = keras.utils.to_categorical(feed_dict['labels'], num_classes=self.num_classes)

            loss = self.model.loss(y_true=labels, y_pred=prob)
            loss = keras.backend.get_value(loss)

            weights = feed_dict["weights"].reshape(loss.shape)
            loss = loss * weights
            loss = _np.sum(loss, axis=1)

            result["loss"] = loss

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

        # First dense layer
        l = self.model.layers[1]
        tf_export_params["conv_weight"], tf_export_params["conv_bias"] = l.get_weights()
        tf_export_params["conv_weight"] = _utils.convert_conv1d_tf_to_coreml(
            tf_export_params["conv_weight"]
        )

        # LSTM layer
        l = self.model.layers[3]
        i2h, h2h, bias = l.get_weights()

        biases = _np.split(bias, 4)
        i2h = _np.swapaxes(i2h, 0, 1)
        i2h = _np.split(i2h, 4)
        h2h = _np.swapaxes(h2h, 0, 1)
        h2h = _np.split(h2h, 4)

        for i, c in enumerate(['i', 'f', 'c', 'o']):
            cur_bias_key = "lstm_h2h_%s_bias" % c
            tf_export_params[cur_bias_key] = biases[i]

            cur_i2h_key = "lstm_i2h_%s_weight" % c
            tf_export_params[cur_i2h_key] = i2h[i]

            cur_h2h_key = "lstm_h2h_%s_weight" % c
            tf_export_params[cur_h2h_key] = h2h[i]

        # Second dense layer
        l = self.model.layers[4]
        dense2_weights, tf_export_params['dense0_bias'] = l.get_weights()
        dense2_weights = dense2_weights.swapaxes(1, 0).reshape(DENSE_H, LSTM_H, 1, 1)
        tf_export_params['dense0_weight'] = dense2_weights

        # Batch Norm weights
        l = self.model.layers[5]
        (tf_export_params['bn_gamma'],
         tf_export_params['bn_beta'],
         tf_export_params['bn_running_mean'],
         tf_export_params['bn_running_var']) = l.get_weights()

        # Last dense layer
        l = self.model.layers[8]
        dense3 = l.get_weights()[0]
        dense3 = dense3.swapaxes(1, 0).reshape(self.num_classes, DENSE_H, 1, 1)
        tf_export_params['dense1_weight'] = dense3

        return tf_export_params

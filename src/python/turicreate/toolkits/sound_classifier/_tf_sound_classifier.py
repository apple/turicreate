# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .._tf_model import TensorFlowModel
import turicreate.toolkits._tf_utils as _utils

# Suppresses verbosity to only errors
_utils.suppress_tensorflow_warnings()
import tensorflow.compat.v1 as _tf

_tf.disable_v2_behavior()


class SoundClassifierTensorFlowModel(TensorFlowModel):
    def __init__(self, num_inputs, num_classes, custom_layer_sizes):
        """
        Defines the TensorFlow model, loss, optimisation and accuracy.

        """
        self.gpu_policy = _utils.TensorFlowGPUPolicy()
        self.gpu_policy.start()

        self.sc_graph = _tf.Graph()
        self.num_classes = num_classes
        self.sess = _tf.Session(graph=self.sc_graph)
        with self.sc_graph.as_default():
            self.init_sound_classifier_graph(num_inputs, custom_layer_sizes)

    def __del__(self):
        self.sess.close()
        self.gpu_policy.stop()

    def init_sound_classifier_graph(self, num_inputs, custom_layer_sizes):

        self.x = _tf.placeholder("float", [None, 12288])
        self.y = _tf.placeholder("float", [None, self.num_classes])

        # Xavier initialization
        initializer = _tf.keras.initializers.glorot_uniform()

        weights = {}
        biases = {}
        self.names_of_layers = []

        # Create variables for customized layers
        for i, cur_layer_size in enumerate(custom_layer_sizes):
            weight_name = "sound_dense{}_weight".format(i)
            bias_name = "sound_dense{}_bias".format(i)
            self.names_of_layers.append("dense{}".format(i))
            out_units = cur_layer_size
            if i == 0:
                in_units = num_inputs
            weights[weight_name] = _tf.Variable(
                initializer([in_units, out_units]), name=weight_name
            )
            biases[bias_name] = _tf.Variable(initializer([out_units]), name=bias_name)
            in_units = out_units

        i += 1
        weight_name = "sound_dense{}_weight".format(i)
        bias_name = "sound_dense{}_bias".format(i)
        self.names_of_layers.append("dense{}".format(i))
        weights[weight_name] = _tf.Variable(
            initializer([in_units, self.num_classes]), name=weight_name
        )
        biases[bias_name] = _tf.Variable(
            initializer([self.num_classes]), name=bias_name
        )

        # Add customized layers
        for i in range(len(weights.keys())):
            weight_name = "sound_dense{}_weight".format(i)
            bias_name = "sound_dense{}_bias".format(i)
            if i == 0:
                curr_dense = _tf.nn.xw_plus_b(
                    self.x, weights=weights[weight_name], biases=biases[bias_name]
                )
            else:
                curr_dense = _tf.nn.xw_plus_b(
                    curr_dense, weights=weights[weight_name], biases=biases[bias_name]
                )
            if i == (len(weights.keys()) - 1):
                out = _tf.nn.softmax(curr_dense)
            else:
                curr_dense = _tf.nn.relu(curr_dense)

        self.predictions = out

        # Loss
        self.cost = _tf.reduce_mean(
            _tf.nn.softmax_cross_entropy_with_logits_v2(
                logits=curr_dense, labels=self.y
            )
        )

        # Optimizer
        self.optimizer = _tf.train.MomentumOptimizer(
            learning_rate=0.01, momentum=0.9, use_nesterov=True
        ).minimize(self.cost)

        # Predictions
        correct_prediction = _tf.equal(
            _tf.argmax(self.predictions, 1), _tf.argmax(self.y, 1)
        )
        self.accuracy = _tf.reduce_mean(_tf.cast(correct_prediction, "float"))

        # Set variables to their initial values
        self.sess.run(_tf.global_variables_initializer())

    def train(self, data, label):
        data_shape = data.shape[0]
        _, final_train_loss, final_train_accuracy = self.sess.run(
            [self.optimizer, self.cost, self.accuracy],
            feed_dict={
                self.x: data.reshape((data_shape, 12288)),
                self.y: _tf.keras.utils.to_categorical(label, self.num_classes).reshape(
                    (data_shape, self.num_classes)
                ),
            },
        )
        result = {"accuracy": final_train_accuracy, "loss": final_train_loss}
        return result

    def evaluate(self, data, label):
        data_shape = data.shape[0]
        pred_probs, final_accuracy = self.sess.run(
            [self.predictions, self.accuracy],
            feed_dict={
                self.x: data.reshape((data_shape, 12288)),
                self.y: _tf.keras.utils.to_categorical(label, self.num_classes).reshape(
                    (data_shape, self.num_classes)
                ),
            },
        )
        result = {"accuracy": final_accuracy, "predictions": pred_probs}
        return result

    def predict(self, data):
        data_shape = data.shape[0]
        pred_probs = self.sess.run(
            self.predictions, feed_dict={self.x: data.reshape((data_shape, 12288))}
        )
        return pred_probs

    def export_weights(self):
        """
        Retrieve weights from the TF model, converts to the CoreML format
        and stores in a list of dictionaries.

        Returns
        -------
        layers : list
            List of dictionaries of weights and activations, where
            the key, for each element of the list, is `weight`, `bias`
            and `act` and the value is the respective weight of type
            `numpy.ndarray` converted to the CoreML format and the
            respective activation applied to the layer.
        """

        with self.sc_graph.as_default():
            layer_names = _tf.trainable_variables()
            layer_weights = self.sess.run(layer_names)

        layer_dict = {}
        for var, val in zip(layer_names, layer_weights):
            layer_dict[var.name] = val

        layers = []
        for i, name in enumerate(self.names_of_layers):
            weight_name = "sound_{}_weight:0".format(name)
            bias_name = "sound_{}_bias:0".format(name)
            layer = {}
            layer["weight"] = layer_dict[weight_name].transpose(1, 0)
            layer["bias"] = layer_dict[bias_name]
            if i == (len(self.names_of_layers) - 1):
                layer["act"] = None
            else:
                layer["act"] = "relu"
            layers.append(layer)
        return layers

    def get_weights(self):
        """
        Returns
        -------
                : dict
                Containing model weights and shapes
                {'data': weight data dict, 'shapes': weight shapes dict}
                Model is saved in CoreML format, hence dense weights and
                shapes are transposed.

        """

        with self.sc_graph.as_default():
            layer_names = _tf.trainable_variables()
            layer_weights = self.sess.run(layer_names)

        data = {}
        shapes = {}
        for var, val in zip(layer_names, layer_weights):
            layer_name = var.name[:-2]
            if "bias" in layer_name:
                data[layer_name] = val
            else:
                data[layer_name] = val.transpose(1, 0)
            shapes[layer_name] = val.shape[::-1]

        return {"data": data, "shapes": shapes}

    def load_weights(self, net_params):
        """
        TensorFlow model is assigned weights from `net_params` dictionary.
        `net_params` contains weights in CoreML format. The dense layers
        need to be transposed to match TF format.

        """
        layers = net_params["data"].keys()

        for layer_name in layers:
            new_layer_name = layer_name.replace("custom", "sound")
            if "bias" in layer_name:
                self.sess.run(
                    _tf.assign(
                        self.sc_graph.get_tensor_by_name(new_layer_name + ":0"),
                        net_params["data"][layer_name],
                    )
                )
            else:
                curr_shape = [int(x) for x in net_params["shapes"][layer_name]]
                self.sess.run(
                    _tf.assign(
                        self.sc_graph.get_tensor_by_name(new_layer_name + ":0"),
                        net_params["data"][layer_name]
                        .reshape(curr_shape)
                        .transpose(1, 0),
                    )
                )

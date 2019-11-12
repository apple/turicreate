# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from turicreate.util import _ProgressTablePrinter
import tensorflow as _tf
import numpy as _np
import time as _time
from .._tf_model import TensorFlowModel
import turicreate.toolkits._tf_utils as _utils

import tensorflow.compat.v1 as _tf
_tf.disable_v2_behavior()


class SoundClassifierTensorFlowModel(TensorFlowModel):

    def __init__(self, num_inputs, num_classes, custom_layer_sizes):
        """
        Defines the TensorFlow model, loss, optimisation and accuracy.

        """

        _tf.reset_default_graph()

        self.num_classes = num_classes

        self.x = _tf.placeholder("float", [None, 12288])
        self.y = _tf.placeholder("float", [None, self.num_classes])

        initializer = _tf.keras.initializers.glorot_uniform() #xavier initialization

        weights = {}
        biases = {}
        self.names_of_layers = []

        # Create variables for customized layers
        for i, cur_layer_size in enumerate(custom_layer_sizes):
            weight_name = 'sound_dense{}_weight'.format(i)
            bias_name = 'sound_dense{}_bias'.format(i)
            self.names_of_layers.append('dense{}'.format(i))
            out_units = cur_layer_size
            if i==0:
                in_units = num_inputs
            weights[weight_name] = _tf.Variable(initializer([in_units, out_units]), name=weight_name)
            biases[bias_name] = _tf.Variable(initializer([out_units]), name=bias_name)
            in_units = out_units

        i+=1
        weight_name = 'sound_dense{}_weight'.format(i)
        bias_name = 'sound_dense{}_bias'.format(i)
        self.names_of_layers.append('dense{}'.format(i))
        weights[weight_name] = _tf.Variable(initializer([in_units, num_classes]), name=weight_name)
        biases[bias_name] = _tf.Variable(initializer([num_classes]), name=bias_name)

        # Add customized layers
        for i in range(len(weights.keys())):
            weight_name = 'sound_dense{}_weight'.format(i)
            bias_name = 'sound_dense{}_bias'.format(i)
            if i==0:
                curr_dense = _tf.nn.xw_plus_b(self.x, weights=weights[weight_name], biases=biases[bias_name])
            else:
                curr_dense = _tf.nn.xw_plus_b(curr_dense, weights=weights[weight_name], biases=biases[bias_name])
            if i==(len(weights.keys())-1):
                out = _tf.nn.softmax(curr_dense)
            else:
                curr_dense = _tf.nn.relu(curr_dense)

        self.predictions = out

        # Loss
        self.cost = _tf.reduce_mean(_tf.nn.softmax_cross_entropy_with_logits_v2(logits=self.predictions,
            labels=self.y))

        # Optimizer
        self.optimizer = _tf.train.AdamOptimizer(learning_rate=0.01).minimize(self.cost)

        # Predictions
        correct_prediction = _tf.equal(_tf.argmax(self.predictions, 1), _tf.argmax(self.y, 1))
        self.accuracy = _tf.reduce_mean(_tf.cast(correct_prediction, "float"))

        self.sess = _tf.Session()
        self.sess.run(_tf.global_variables_initializer())


    def train(self, data, label):
        data_shape = data.shape[0]
        _, final_train_loss, final_train_accuracy = self.sess.run([self.optimizer, self.cost, self.accuracy],
                            feed_dict={
                                self.x: data.reshape((data_shape, 12288)),
                                self.y: _tf.keras.utils.to_categorical(label, self.num_classes).reshape((data_shape, self.num_classes))
                            })
        result = {'accuracy' : final_train_accuracy, 'loss' : final_train_loss}
        return result

    def evaluate(self, data, label):
        data_shape = data.shape[0]
        pred_probs, final_accuracy = self.sess.run([self.predictions, self.accuracy],
                             feed_dict={
                                 self.x: data.reshape((data_shape, 12288)),
                                 self.y: _tf.keras.utils.to_categorical(label, self.num_classes).reshape((data_shape, self.num_classes))
                             })
        result = {'accuracy' : final_accuracy, 'predictions' : pred_probs}
        return result

    def predict(self, data):
        data = data[0] #tuple
        data_shape = data.shape[0]
        pred_probs = self.sess.run([self.predictions],
                            feed_dict={
                                self.x: data.reshape((data_shape, 12288))
                            })
        result = {'predictions' : pred_probs}
        return result

    def export_weights(self):
        """
        Retrieve weights from the TF model, convert to the format MXNET
        expects and store in a dictionary.

        Returns
        -------
        net_params : dict
            Dictionary of weights, where the key is the name of the
            layer (e.g. `sound_conv0_weight`) and the value is the
            respective weight of type `numpy.ndarray` in MXNET format.
        """
        layer_names = _tf.trainable_variables()
        layer_weights = self.sess.run(layer_names)
        layer_dict = {}
        for var, val in zip(layer_names, layer_weights):
            layer_dict[var.name] = val

        layers = []
        for i, name in enumerate(self.names_of_layers):
            weight_name = 'sound_{}_weight:0'.format(name)
            bias_name = 'sound_{}_weight:0'.format(name)
            layer={}
            layer['weight'] = layer_dict[weight_name]#.transpose(1, 0)#.asnumpy() #### wait we don't need this!
            layer['bias'] = layer_dict[bias_name]#.asnumpy()
            if i==(len(self.names_of_layers)-1):
                layer['act']=None
            else:
                layer['act']='relu'
            layers.append(layer)
        return layers

    def get_weights(self):
        """
        Parameters
        ----------
        weights : dict
                Containing model weights and shapes
                {'data': weight data dict, 'shapes': weight shapes dict}

        """
        layer_names = _tf.trainable_variables()
        layer_weights = self.sess.run(layer_names)
        data = {}
        shapes = {}
        for var, val in zip(layer_names, layer_weights):
            layer_name = var.name[:-2]
            data[layer_name] = val
            shapes[layer_name] = val.shape

        return {'data': data, 'shapes': shapes}

    def load_weights(self, net_params):
        """
        Parameters
        ----------
        net_params : dict
                Containing model weights and shapes
                {'data': weight data dict, 'shapes': weight shapes dict}

        """
        layers = net_params['data'].keys()

        for layer_name in layers:
            new_layer_name = layer_name.replace("custom", "sound")
            if 'bias' in layer_name:
                self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(new_layer_name+":0"),
                    net_params['data'][layer_name]))
            else:
                curr_shape = [int(x) for x in net_params['shapes'][layer_name]]
                self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(new_layer_name+":0"),
                                        net_params['data'][layer_name].reshape(curr_shape)))#.transpose(1, 0)))


    def get_layer_activations(self, data, label):
        #TODO: remove this 
        data_shape = data.shape[0]
        dense0, dense1, dense2 = self.sess.run([self.dense0, self.dense1, self.out],
                            feed_dict={
                                self.x: data.reshape((data_shape, 12288)),
                                self.y: _tf.keras.utils.to_categorical(label, self.num_classes).reshape((data_shape, self.num_classes))
                            })
        return dense0, dense1, dense2




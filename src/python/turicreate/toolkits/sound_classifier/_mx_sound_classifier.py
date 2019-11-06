# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


import mxnet as mx
import time
import turicreate as _tc

from .._mxnet import _mxnet_utils

class MultiLayerPerceptronMXNetModel():


    def __init__(self, feature_output_length, num_labels, custom_layer_sizes, verbose):

        self.ctx = _mxnet_utils.get_mxnet_context()
        self.verbose = verbose
        self.custom_NN = self._build_custom_neural_network(feature_output_length, num_labels, custom_layer_sizes)
        self.custom_NN.initialize(mx.init.Xavier(), ctx=self.ctx)

        self.trainer = mx.gluon.Trainer(self.custom_NN.collect_params(), 'nag', {'learning_rate': 0.01, 'momentum': 0.9})
        self.softmax_cross_entropy_loss = mx.gluon.loss.SoftmaxCrossEntropyLoss()


    def train(self, data, label):
        """
        Parameters
        ----------
        data : NumPy Array
            `data` contains the input data features stored in the `deep features`
            column of the dataset.

        label : NumPy Array
            `label` contains the input data labels stored in the `labels`
            column of the dataset.
        """

        # Inside training scope
        batch_size = data.shape[0] # may be smaller than the specified batch_size in create()
        data = mx.gluon.utils.split_and_load(data, ctx_list=self.ctx, batch_axis=0, even_split=False)
        label = mx.gluon.utils.split_and_load(label, ctx_list=self.ctx, batch_axis=0, even_split=False)
        with mx.autograd.record():
            for x, y in zip(data, label):
                z = self.custom_NN(x)
                # Computes softmax cross entropy loss.
                loss = self.softmax_cross_entropy_loss(z, y)
                # Backpropagate the error for one iteration.
                loss.backward()
        # Make one step of parameter update. Trainer needs to know the
        # batch size of data to normalize the gradient by 1/batch_size.
        self.trainer.step(batch_size)


    def predict(self, data):
        """
        Parameters
        ----------
        data : NumPy Array
            `data` contains the input data features stored in the `deep features`
            column of the dataset.

        """

        data = mx.gluon.utils.split_and_load(data, ctx_list=self.ctx, batch_axis=0, even_split=False)
        outputs = [self.custom_NN(x).asnumpy() for x in data]
        soft_outputs = mx.nd.softmax(mx.nd.array(outputs[0]))
        return soft_outputs.asnumpy()

    @staticmethod
    def _build_custom_neural_network(num_inputs, num_labels, layer_sizes):
        from mxnet.gluon import nn

        net = nn.Sequential(prefix='custom_')
        with net.name_scope():
            for i, cur_layer_size in enumerate(layer_sizes):
                prefix = "dense%d_" % i
                if i == 0:
                    in_units = num_inputs
                else:
                    in_units = layer_sizes[i-1]
                net.add(nn.Dense(cur_layer_size, in_units=in_units, activation='relu', prefix=prefix))

            prefix = 'dense%d_' % len(layer_sizes)
            net.add(nn.Dense(num_labels, prefix=prefix))
        return net

    def get_weights(self):
        return _mxnet_utils.get_gluon_net_params_state(self.custom_NN.collect_params())

    def load_weights(self, weights):
        """
        Parameters
        ----------
        weights : dict
                Containing model weights and shapes
                {'data': weight data, 'shapes': weight shapes}

        """

        net_params = self.custom_NN.collect_params()
        _mxnet_utils.load_net_params_from_state(net_params, weights, ctx=self.ctx)

    def export_weights(self):
        layers = []
        for i, cur_layer in enumerate(self.custom_NN):
            layer ={}
            layer['weight'] = cur_layer.weight.data(self.ctx[0]).asnumpy()
            layer['bias'] = cur_layer.bias.data(self.ctx[0]).asnumpy()
            layer['act'] = cur_layer.act
            layers.append(layer)
        return layers


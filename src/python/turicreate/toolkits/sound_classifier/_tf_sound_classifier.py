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

    def __init__(self, num_classes):
        """
        Defines the TensorFlow model, loss, optimisation and accuracy.

        """
        #for key in net_params.keys():
        #    net_params[key] = _utils.convert_shared_float_array_to_numpy(net_params[key])

        _tf.reset_default_graph()

        user_num_layers = 3

        self.num_classes = num_classes
        #self.batch_size = batch_size

        self.x = _tf.placeholder("float", [None, 12288])
        self.y = _tf.placeholder("float", [None, self.num_classes])

        initializer = _tf.keras.initializers.glorot_uniform() #xavier initialization
        # Weights
        '''
        weights = {
        'sound_dense0_weight'  : _tf.Variable(initializer([12288, 100]), name='sound_dense0_weight'),
        'sound_dense1_weight'  : _tf.Variable(initializer([100, 100]), name='sound_dense1_weight'),
        'sound_dense2_weight'  : _tf.Variable(initializer([100, self.num_classes]), name='sound_dense2_weight')
        }

        # Biases
        biases = {
        'sound_dense0_bias'  : _tf.Variable(initializer([100]), name='sound_dense0_bias'),
        'sound_dense1_bias'  : _tf.Variable(initializer([100]), name='sound_dense1_bias'),
        'sound_dense2_bias'  : _tf.Variable(initializer([self.num_classes]), name='sound_dense2_bias')
        }

        self.names_of_layers = [x.split('_')[1] for x in weights.keys()]

        self.dense0 = _tf.nn.xw_plus_b(self.x, weights=weights["sound_dense0_weight"], biases=biases["sound_dense0_bias"])
        dense0 = _tf.nn.relu(self.dense0)

        self.dense1 = _tf.nn.xw_plus_b(dense0, weights=weights["sound_dense1_weight"], biases=biases["sound_dense1_bias"])
        dense1 = _tf.nn.relu(self.dense1)

        self.out = _tf.nn.xw_plus_b(dense1, weights=weights["sound_dense2_weight"], biases=biases["sound_dense2_bias"])
        out = _tf.nn.softmax(self.out)
        '''

        weights = {}
        biases = {}
        self.names_of_layers = []

        for i in range(user_num_layers):
            weight_name = 'sound_dense{}_weight'.format(i)
            bias_name = 'sound_dense{}_bias'.format(i)
            self.names_of_layers.append('dense{}'.format(i))
            if i==0:
                weights[weight_name] = _tf.Variable(initializer([12288, 100]), name=weight_name)
                biases[bias_name] = _tf.Variable(initializer([100]), name=bias_name)
            elif i==(user_num_layers-1):
                weights[weight_name] = _tf.Variable(initializer([100, self.num_classes]), name=weight_name)
                biases[bias_name] = _tf.Variable(initializer([self.num_classes]), name=bias_name)
            else:
                weights[weight_name] = _tf.Variable(initializer([100, 100]), name=weight_name)
                biases[bias_name] = _tf.Variable(initializer([100]), name=bias_name)


        for i in range(user_num_layers):
            weight_name = 'sound_dense{}_weight'.format(i)
            bias_name = 'sound_dense{}_bias'.format(i)
            if i==0:
                curr_dense = _tf.nn.xw_plus_b(self.x, weights=weights[weight_name], biases=biases[bias_name])
            else:
                curr_dense = _tf.nn.xw_plus_b(curr_dense, weights=weights[weight_name], biases=biases[bias_name])
            if i==(user_num_layers-1):
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

        '''
        # Assign the initialised weights from MXNet to tensorflow
        layers = ['sound_dense0_weight', 'sound_dense0_bias', 'sound_dense1_weight', 'sound_dense1_bias',
        'sound_dense2_weight', 'sound_dense2_bias']

        for key in layers:
            if 'bias' in key:
                self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(key+":0"),
                    net_params[key].data().asnumpy()))
            else:
                #if 'dense' in key:
                self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(key+":0"),
                                        net_params[key].data().asnumpy().transpose(1, 0)))
        '''

    def train(self, data, label):
        data_shape = data.shape[0]
        _, final_train_loss, final_train_accuracy = self.sess.run([self.optimizer, self.cost, self.accuracy],
                            feed_dict={
                                self.x: data.reshape((data_shape, 12288)),
                                self.y: _tf.keras.utils.to_categorical(label, self.num_classes).reshape((data_shape, self.num_classes))
                            })
        result = {'accuracy' : final_train_accuracy, 'loss' : final_train_loss}
        return result

    def predict_with_accuracy(self, data, label):
        '''
        data_shape = data.shape[0]
        pred_probs = self.sess.run([self.predictions],
                            feed_dict={
                                self.x: data.reshape((data_shape, 12288))
                            })
        pred_probs = pred_probs[0]
        return pred_probs
        '''
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
                                self.x: data#.reshape((data_shape, 12288))
                            })
        result = {'predictions' : pred_probs}
        return result

    def method_train(self, feed_dict):
        _, final_train_loss, final_train_accuracy = self.sess.run([self.optimizer, self.cost, self.accuracy],
                             feed_dict={
                                 self.x: feed_dict['x'],
                                 self.y: feed_dict['y']
                             })
        result = {'accuracy' : final_train_accuracy, 'loss' : final_train_loss}
        return result

    def method_predict(self, feed_dict):
        pred_probs, final_accuracy = self.sess.run([self.predictions, self.accuracy],
                             feed_dict={
                                 self.x: feed_dict['x'],
                                 self.y: feed_dict['y']
                             })
        result = {'accuracy' : final_accuracy, 'predictions' : pred_probs}
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
        data_shape = data.shape[0]
        dense0, dense1, dense2 = self.sess.run([self.dense0, self.dense1, self.out],
                            feed_dict={
                                self.x: data.reshape((data_shape, 12288)),
                                self.y: _tf.keras.utils.to_categorical(label, self.num_classes).reshape((data_shape, self.num_classes))
                            })
        return dense0, dense1, dense2

    def evaluate(self, train_loader):
        total_acc = 0.0
        count = 0
        for train_batch in train_loader:
            batch_x, batch_y = process_data(train_batch, self.batch_size, self.num_classes)
            import itertools
            for x_data, y_data in itertools.izip(batch_x, batch_y):
                x_data = _np.asarray(x_data).reshape((1, 1, 12288))
                y_data = _np.asarray(y_data).reshape((1, self.num_classes))
                acc = self.sess.run(self.accuracy,
                            feed_dict={
                                self.x: x_data,
                                self.y: y_data
                            })
                total_acc+= acc
                count+=1

        final_accuracy = total_acc/count
        return final_accuracy



def _tf_train_model(tf_model, train_loader, validation_loader, validation_set, batch_size, num_classes, verbose):
    """
    Trains the TensorFlow model.

    Returns
    -------

    final_train_accuracy : numpy.float32
        Training accuracy of the last iteration.

    final_val_accuracy : numpy.float32
        Validation accuracy of the last iteration.

    final_train_loss : numpy.float32
        The final loss recorded in training.

    total_train_time : float
        Time taken to complete the training
    """

    if verbose:
        column_names = ['iteration', 'train_loss', 'train_accuracy', 'time']
        column_titles = ['Iteration', 'Training Loss', 'Training Accuracy', 'Elapsed Time (seconds)']
        if validation_set is not None:
            column_names.insert(3, 'validation_accuracy')
            column_titles.insert(3, 'Validation Accuracy')
        table_printer = _ProgressTablePrinter(column_names, column_titles)

    num_iter = 0
    start_time = _time.time()

    for train_batch in train_loader:
        data_shape = train_batch.label[0].asnumpy().shape[0]
        result = tf_model.method_train( feed_dict={
                            'x': train_batch.data[0].asnumpy().reshape((data_shape, 12288)),
                            'y': _tf.keras.utils.to_categorical(train_batch.label[0].asnumpy(), num_classes).reshape((data_shape, num_classes))
                        })
        final_train_accuracy = result['accuracy']
        final_train_loss = result['loss']
        for val_batch in validation_loader:
            data_shape = val_batch.label[0].asnumpy().shape[0]
            result = tf_model.method_predict(feed_dict={
                            'x': val_batch.data[0].asnumpy().reshape((data_shape, 12288)),
                            'y': _tf.keras.utils.to_categorical(val_batch.label[0].asnumpy(), num_classes).reshape((data_shape, num_classes))
                        })
            val_acc = result['accuracy']

        num_iter+=1
        if verbose:
            kwargs = {  "iteration": num_iter,
                        "train_loss": "{:.3f}".format(final_train_loss),
                        "train_accuracy": "{:.3f}".format(final_train_accuracy),
                        "time": _time.time() - start_time}
            if validation_set is not None:
                kwargs["validation_accuracy"] = "{:.3f}".format(val_acc)
            #if num_iter%100==0:
            table_printer.print_row(**kwargs)

    final_val_accuracy = val_acc if validation_set else None
    total_train_time = _time.time() - start_time


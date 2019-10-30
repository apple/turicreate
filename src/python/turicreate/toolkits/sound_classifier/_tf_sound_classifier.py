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

    def __init__(self, batch_size, num_classes):
        """
        Defines the TensorFlow model, loss, optimisation and accuracy.

        """
        #for key in net_params.keys():
        #    net_params[key] = _utils.convert_shared_float_array_to_numpy(net_params[key])

        _tf.reset_default_graph()

        self.num_classes = num_classes
        self.batch_size = batch_size

        self.x = _tf.placeholder("float", [None, 1, 12288])
        self.y = _tf.placeholder("float", [None, self.num_classes])

        # Weights
        weights = {
        'sound_dense0_weight'  : _tf.Variable(_tf.random.uniform([12288, 100]), name='sound_dense0_weight'),
        'sound_dense1_weight'  : _tf.Variable(_tf.random.uniform([100, 100]), name='sound_dense1_weight'),
        'sound_dense2_weight'  : _tf.Variable(_tf.random.uniform([100, self.num_classes]), name='sound_dense2_weight')
        }

        # Biases
        biases = {
        'sound_dense0_bias'  : _tf.Variable(_tf.zeros([100]), name='sound_dense0_bias'),
        'sound_dense1_bias'  : _tf.Variable(_tf.zeros([100]), name='sound_dense1_bias'),
        'sound_dense2_bias'  : _tf.Variable(_tf.zeros([self.num_classes]), name='sound_dense2_bias')
        }

        dense0 = _tf.nn.xw_plus_b(self.x, weights=weights["sound_dense0_weight"], biases=biases["sound_dense0_bias"])
        dense0 = _tf.nn.relu(dense0)

        dense1 = _tf.nn.xw_plus_b(dense0, weights=weights["sound_dense1_weight"], biases=biases["sound_dense1_bias"])
        dense1 = _tf.nn.relu(dense1)

        out = _tf.nn.xw_plus_b(dense1, weights=weights["sound_dense2_weight"], biases=biases["sound_dense2_bias"])
        out = _tf.nn.softmax(out)

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


    def train(self, feed_dict):
        _, final_train_loss, final_train_accuracy = self.sess.run([self.optimizer, self.cost, self.accuracy],
                            feed_dict={
                                self.x: feed_dict['x'],
                                self.y: feed_dict['y']
                            })
        result = {'accuracy' : final_train_accuracy, 'loss' : final_train_loss}
        return result

    def predict(self, feed_dict):
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
        net_params = {}
        layer_names = _tf.trainable_variables()
        layer_weights = self.sess.run(layer_names)

        for var, val in zip(layer_names, layer_weights):
            if 'bias' in var.name:
                net_params.update({var.name.replace(":0", ""): val})
            else:
                #if 'dense' in var.name:
                net_params.update({var.name.replace(":0", ""): val.transpose(1, 0)})
        return net_params


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
        #batch_x, batch_y = process_data(train_batch, batch_size, num_classes)
        result = tf_model.train( feed_dict={
                            'x': train_batch['deep features'].reshape((1, 1, 12288)), #batch_x,
                            'y': _tf.keras.utils.to_categorical(train_batch['labels'], num_classes).reshape((1, num_classes)) #batch_y
                        })
        final_train_accuracy = result['accuracy']
        final_train_loss = result['loss']

        for val_batch in validation_loader:
            #val_x, val_y = process_data(val_batch)
            result = tf_model.predict(feed_dict={
                            'x': train_batch['deep features'].reshape((1, 1, 12288)), #val_x,
                            'y': _tf.keras.utils.to_categorical(train_batch['labels'], num_classes).reshape((1, num_classes)) #val_y
                        })
            val_acc = result['accuracy']
        #validation_loader.reset()

        num_iter+=1
        if verbose:
            kwargs = {  "iteration": num_iter,
                        "train_loss": "{:.3f}".format(final_train_loss),
                        "train_accuracy": "{:.3f}".format(final_train_accuracy),
                        "time": _time.time() - start_time}
            if validation_set is not None:
                kwargs["validation_accuracy"] = "{:.3f}".format(val_acc)
            if num_iter%100==0:
                table_printer.print_row(**kwargs)

    final_val_accuracy = val_acc if validation_set else None
    total_train_time = _time.time() - start_time

    return final_train_accuracy, final_val_accuracy, final_train_loss, total_train_time

'''
def process_data(batch_data, batch_size, num_classes):

    if batch_data.pad is not None:
        batch_x = batch_data.data[0].asnumpy()[0:batch_size-batch_data.pad]
        batch_y = _tf.keras.utils.to_categorical(batch_data.label[0].asnumpy()[0:batch_size-batch_data.pad],
                                                    num_classes)
    else:
        batch_x = batch_data.data[0].asnumpy()
        batch_y = _tf.keras.utils.to_categorical(batch_data['labels'], num_classes)
    return batch_x, batch_y
'''

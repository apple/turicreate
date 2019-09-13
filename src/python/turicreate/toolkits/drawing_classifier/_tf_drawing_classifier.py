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


class DrawingClassifierTensorFlowModel(TensorFlowModel):

    def __init__(self, validation_set, net_params, batch_size, num_classes, verbose):
        """
        Defines the TensorFlow model, loss, optimisation and accuracy. Then
        loads the MXNET weights into the model.

        """
        _tf.reset_default_graph()

        self.num_classes = num_classes
        self.batch_size = batch_size

        self.x = _tf.compat.v1.placeholder("float", [None, 28, 28, 1])
        self.y = _tf.compat.v1.placeholder("float", [None, self.num_classes])

        # Weights
        weights = {
        'drawing_conv0_weight' : _tf.Variable(_tf.zeros([3, 3, 1, 16]), name='drawing_conv0_weight'),
        'drawing_conv1_weight' : _tf.Variable(_tf.zeros([3, 3, 16, 32]), name='drawing_conv1_weight'),
        'drawing_conv2_weight' : _tf.Variable(_tf.zeros([3, 3, 32, 64]), name='drawing_conv2_weight'),
        'drawing_dense0_weight': _tf.Variable(_tf.zeros([576, 128]), name='drawing_dense0_weight'),
        'drawing_dense1_weight': _tf.Variable(_tf.zeros([128, self.num_classes]), name='drawing_dense1_weight')
        }

        # Biases
        biases = {
        'drawing_conv0_bias' : _tf.Variable(_tf.zeros([16]), name='drawing_conv0_bias'),
        'drawing_conv1_bias' : _tf.Variable(_tf.zeros([32]), name='drawing_conv1_bias'),
        'drawing_conv2_bias' : _tf.Variable(_tf.zeros([64]), name='drawing_conv2_bias'),
        'drawing_dense0_bias': _tf.Variable(_tf.zeros([128]), name='drawing_dense0_bias'),
        'drawing_dense1_bias': _tf.Variable(_tf.zeros([self.num_classes]), name='drawing_dense1_bias')
        }

        conv_1 = _tf.nn.conv2d(self.x, weights["drawing_conv0_weight"], strides=1, padding='SAME')
        conv_1 = _tf.nn.bias_add(conv_1, biases["drawing_conv0_bias"])
        relu_1 = _tf.nn.relu(conv_1)
        pool_1 = _tf.nn.max_pool2d(relu_1, ksize=[1, 2, 2, 1], strides=[1, 2, 2, 1], padding='SAME')

        conv_2 = _tf.nn.conv2d(pool_1, weights["drawing_conv1_weight"], strides=1, padding='SAME')
        conv_2 = _tf.nn.bias_add(conv_2, biases["drawing_conv1_bias"])
        relu_2 = _tf.nn.relu(conv_2)
        pool_2 = _tf.nn.max_pool2d(relu_2, ksize=[1, 2, 2, 1], strides=[1, 2, 2, 1], padding='SAME')

        conv_3 = _tf.nn.conv2d(pool_2, weights["drawing_conv2_weight"], strides=1, padding='SAME')
        conv_3 = _tf.nn.bias_add(conv_3, biases["drawing_conv2_bias"])
        relu_3 = _tf.nn.relu(conv_3)
        pool_3 = _tf.nn.max_pool2d(relu_3, ksize=[1, 2, 2, 1], strides=[1, 2, 2, 1], padding='VALID') 

        # Flatten the data to a 1-D vector for the fully connected layer
        fc1 = _tf.reshape(pool_3, (-1, 576))

        fc1 = _tf.compat.v1.nn.xw_plus_b(fc1, weights=weights["drawing_dense0_weight"],
            biases=biases["drawing_dense0_bias"])
        fc1 = _tf.nn.relu(fc1)

        out = _tf.compat.v1.nn.xw_plus_b(fc1, weights=weights["drawing_dense1_weight"],
            biases=biases["drawing_dense1_bias"])
        out = _tf.nn.softmax(out)

        self.predictions = out

        # Loss
        self.cost = _tf.reduce_mean(_tf.nn.softmax_cross_entropy_with_logits_v2(logits=self.predictions,
            labels=self.y))

        # Optimizer
        self.optimizer = _tf.compat.v1.train.AdamOptimizer(learning_rate=0.001).minimize(self.cost)

        # Predictions
        correct_prediction = _tf.equal(_tf.argmax(self.predictions, 1), _tf.argmax(self.y, 1))
        self.accuracy = _tf.reduce_mean(_tf.cast(correct_prediction, "float"))

        self.sess = _tf.compat.v1.Session()
        self.sess.run(_tf.compat.v1.global_variables_initializer())

        # Assign the initialised weights from MXNet to tensorflow
        layers = ['drawing_conv0_weight', 'drawing_conv0_bias', 'drawing_conv1_weight', 'drawing_conv1_bias',
        'drawing_conv2_weight', 'drawing_conv2_bias', 'drawing_dense0_weight', 'drawing_dense0_bias',
        'drawing_dense1_weight', 'drawing_dense1_bias']

        for key in layers:
            if 'bias' in key:
                self.sess.run(_tf.compat.v1.assign(_tf.compat.v1.get_default_graph().get_tensor_by_name(key+":0"),
                    net_params[key].data().asnumpy()))
            else:
                if 'dense' in key:
                    if 'drawing_dense0_weight' in key:
                        '''
                        To make output of MXNET pool3 (NCHW) compatible with TF (NHWC).
                        Decompose FC weights to NCHW. Transpose to NHWC. Reshape back to FC.
                        '''
                        mxnet_128_576 = net_params[key].data().asnumpy()
                        mxnet_128_576 = _np.reshape(mxnet_128_576, (128, 64, 3, 3))
                        mxnet_128_576 = mxnet_128_576.transpose(0, 2, 3, 1)
                        mxnet_128_576 = _np.reshape(mxnet_128_576, (128, 576))
                        self.sess.run(_tf.compat.v1.assign(_tf.compat.v1.get_default_graph().get_tensor_by_name(key+":0"),
                                        mxnet_128_576.transpose(1,0)))
                    else:
                        self.sess.run(_tf.compat.v1.assign(_tf.compat.v1.get_default_graph().get_tensor_by_name(key+":0"),
                                        net_params[key].data().asnumpy().transpose(1, 0)))
                else:
                    self.sess.run(_tf.compat.v1.assign(_tf.compat.v1.get_default_graph().get_tensor_by_name(key+":0"),
                                        net_params[key].data().asnumpy().transpose(2, 3, 1, 0)))


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
            layer (e.g. `drawing_conv0_weight`) and the value is the
            respective weight of type `numpy.ndarray` in MXNET format.
        """

        net_params = {}
        layer_names = _tf.compat.v1.trainable_variables()
        layer_weights = self.sess.run(layer_names)

        for var, val in zip(layer_names, layer_weights):
            if 'bias' in var.name:
                net_params.update({var.name.replace(":0", ""): val})
            else:
                if 'dense' in var.name:
                    if 'drawing_dense0_weight' in var.name:
                        '''
                        To make output of TF pool3 (NHWC) compatible with MXNET (NCHW).
                        Decompose FC weights to NHWC. Transpose to NCHW. Reshape back to FC.
                        '''
                        tf_576_128 = val
                        tf_576_128 = _np.reshape(tf_576_128, (3, 3, 64, 128))
                        tf_576_128 = tf_576_128.transpose(2, 0, 1, 3)
                        tf_576_128 = _np.reshape(tf_576_128, (576, 128))
                        net_params.update({var.name.replace(":0", ""): tf_576_128.transpose(1, 0)})
                    else:
                        net_params.update({var.name.replace(":0", ""): val.transpose(1, 0)})
                else:
                    net_params.update({var.name.replace(":0", ""): val.transpose(3, 2, 0, 1)})
        return net_params


    def evaluate(self, train_loader):
        total_acc = 0.0
        count = 0
        for train_batch in train_loader:
            batch_x, batch_y = process_data(train_batch, self.batch_size, self.num_classes)
            import itertools
            for x_data, y_data in itertools.izip(batch_x, batch_y):
                x_data = _np.asarray(x_data).reshape((1, 28, 28, 1))
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
        batch_x, batch_y = process_data(train_batch, batch_size, num_classes)
        result = tf_model.train( feed_dict={
                            'x': batch_x,
                            'y': batch_y
                        })
        final_train_accuracy = result['accuracy']
        final_train_loss = result['loss']

        for val_batch in validation_loader:
            val_x, val_y = process_data(val_batch)
            result = tf_model.predict(feed_dict={
                            'x': val_x,
                            'y': val_y
                        })
            val_acc = result['accuracy']
        validation_loader.reset()

        num_iter+=1
        if verbose:
            kwargs = {  "iteration": num_iter,
                        "train_loss": "{:.3f}".format(final_train_loss),
                        "train_accuracy": "{:.3f}".format(final_train_accuracy),
                        "time": _time.time() - start_time}
            if validation_set is not None:
                kwargs["validation_accuracy"] = "{:.3f}".format(val_acc)
            table_printer.print_row(**kwargs)

    final_val_accuracy = val_acc if validation_set else None
    total_train_time = _time.time() - start_time

    return final_train_accuracy, final_val_accuracy, final_train_loss, total_train_time


def process_data(batch_data, batch_size, num_classes):

    if batch_data.pad is not None:
        batch_x = batch_data.data[0].asnumpy().transpose(0, 2, 3, 1)[0:batch_size-batch_data.pad]
        batch_y = _tf.keras.utils.to_categorical(batch_data.label[0].asnumpy()[0:batch_size-batch_data.pad],
                                                    num_classes)
    else:
        batch_x = batch_data.data[0].asnumpy().transpose(0, 2, 3, 1)
        batch_y = _tf.keras.utils.to_categorical(batch_data.label[0].asnumpy(), num_classes)
    return batch_x, batch_y


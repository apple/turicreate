# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import tensorflow as tf 
import numpy as np
from .._tf_model import TensorFlowModel


_net_params = {
    'conv_h': 64,
    'lstm_h': 200,
    'dense_h': 128
}

class ActivityTensorFlowModel(TensorFlowModel):

    def __init__(self, net_params, batch_size, num_features, num_classes, prediction_window, seq_len):

        tf.reset_default_graph()

        self.is_train = tf.placeholder(tf.bool)
        self.num_classes = num_classes
        self.batch_size = batch_size
        self.seq_len = seq_len

        # Vars
        self.data = tf.compat.v1.placeholder(tf.float32, [None, prediction_window*seq_len, num_features])
        self.weight = tf.compat.v1.placeholder(tf.float32, [None, seq_len, 1])
        weight = tf.reshape(self.weight, [self.batch_size, seq_len])
        self.target = tf.compat.v1.placeholder(tf.int32, [None, seq_len, 1])
        target = tf.reshape(self.target, [self.batch_size, seq_len])
        one_hot = tf.one_hot(target, depth=self.num_classes, axis=-1)

        # Weights 
        self.weights = {
        'conv_weight' : tf.Variable(tf.random.normal([prediction_window, num_features, _net_params['conv_h']]), name='conv_weight'),
        'dense0_weight': tf.Variable(tf.random.normal([_net_params['lstm_h'], _net_params['dense_h']]), name='dense0_weight'),
        'dense1_weight'  : tf.Variable(tf.random.normal([_net_params['dense_h'], self.num_classes]), name='dense1_weight')
        }

        # Biases
        self.biases = {
        'conv_bias' : tf.Variable(tf.zeros([_net_params['conv_h']]), name='conv_bias'),
        'dense0_bias': tf.Variable(tf.zeros([_net_params['dense_h']]), name='dense0_bias'),
        'dense1_bias'  : tf.Variable(tf.zeros([num_classes]), name='dense1_bias')
        }

        # Batch Normalisation Variables
        self.bn_vars = {
        'bn_gamma' : tf.Variable(tf.ones([_net_params['dense_h']]), name='bn_gamma'),
        'bn_beta' : tf.Variable(tf.zeros([_net_params['dense_h']]), name='bn_beta'),
        'bn_moving_mean' : tf.Variable(tf.ones([_net_params['dense_h']]), name='bn_moving_mean'),
        'bn_moving_var' : tf.Variable(tf.ones([_net_params['dense_h']]), name='bn_moving_var')
        }

        # Convolution 
        conv = tf.nn.conv1d(self.data, self.weights['conv_weight'], stride=prediction_window, padding='SAME')
        conv = tf.nn.bias_add(conv, self.biases['conv_bias'])
        conv = tf.nn.relu(conv)

        # Long Stem Term Memory
        i2h_i, i2h_f, i2h_c, i2h_o = np.split(net_params['lstm_i2h_weight'].asnumpy(), 4)
        h2h_i, h2h_f, h2h_c, h2h_o = np.split(net_params['lstm_h2h_weight'].asnumpy(), 4)
        i2h = np.concatenate((i2h_i, i2h_c, i2h_f, i2h_o), axis=0)
        h2h = np.concatenate((h2h_i, h2h_c, h2h_f, h2h_o), axis=0)
        lstm = np.concatenate((i2h, h2h), axis=1)
        
        dropout = tf.nn.dropout(conv, rate=0.2)
        cells = tf.nn.rnn_cell.LSTMCell(num_units=_net_params['lstm_h'], reuse=tf.AUTO_REUSE, forget_bias=0.0, initializer=tf.initializers.constant(np.transpose(lstm), verify_shape=True))
        init_state = cells.zero_state(batch_size, tf.float32)
        rnn_outputs, final_state = tf.nn.dynamic_rnn(cells, dropout, initial_state=init_state)

        # Dense
        dense = tf.reshape(rnn_outputs, (-1, _net_params['lstm_h']))
        dense = tf.add(tf.matmul(dense, self.weights['dense0_weight']), self.biases['dense0_bias'])
        dense = tf.layers.batch_normalization(inputs=dense, beta_initializer=tf.initializers.constant(net_params['bn_beta'].asnumpy(), verify_shape=True), 
            gamma_initializer=tf.initializers.constant(net_params['bn_gamma'].asnumpy(), verify_shape=True),
            moving_mean_initializer=tf.initializers.constant(net_params['bn_moving_mean'].asnumpy(), verify_shape=True), 
            moving_variance_initializer=tf.initializers.constant(net_params['bn_moving_var'].asnumpy(), verify_shape=True), training=self.is_train)
        dense = tf.nn.relu(dense)
        dense = tf.nn.dropout(dense, rate=0.5)

        # Output
        out = tf.add(tf.matmul(dense, self.weights['dense1_weight']), self.biases['dense1_bias'])
        out = tf.reshape(out, (-1, self.seq_len, self.num_classes))
        probs = tf.nn.softmax(out)

        # Weights
        seq_sum_weights = tf.reduce_sum(weight, axis=1)
        binary_seq_sum_weights = tf.reduce_sum(tf.cast(seq_sum_weights > 0, dtype=tf.float32))

        # Loss
        self.loss_op = tf.losses.softmax_cross_entropy(logits=out, onehot_labels=one_hot, weights=weight)
        
        # Optimizer
        optimizer = tf.train.AdamOptimizer(learning_rate=self.set_learning_rate(1e-3))
        self.train_op = optimizer.minimize(self.loss_op)
        
        # Predictions
        correct_pred = tf.reduce_sum(tf.cast(tf.equal(tf.argmax(out, 2), tf.argmax(one_hot, 2)), dtype=tf.float32) * weight, axis=1)
        acc_per_seq = correct_pred / (seq_sum_weights + 1e-5)
        self.acc = tf.reduce_sum(acc_per_seq) / (binary_seq_sum_weights + 1e-5)

        # Session 
        self.sess = tf.compat.v1.Session()


    def train(self, feed_dict):

        _, loss, acc = self.sess.run([self.train_op, self.loss_op,  self.acc], feed_dict={self.data : feed_dict['input'], self.target : feed_dict['labels'], self.weight : feed_dict['weights'], self.is_train: True})
        result = {'loss' : loss, 'acc': acc}
        return result

    def predict(self, feed_dict):

        loss, acc = self.sess.run([self.loss_op, self.acc], feed_dict={self.data : feed_dict['input'], self.target : feed_dict['labels'], self.weight : feed_dict['weights'], self.is_train: False})
        result = {'loss' : loss, 'acc':acc }
        return result

    def export_weights(self):
        tf_export_params = {}
        tvars = tf.trainable_variables()
        tvars_vals = self.sess.run(tvars)
        for var, val in zip(tvars, tvars_vals):
            if 'weight' in var.name:
                if var.name.startswith('conv'):
                    tf_export_params.update(
                        {var.name.replace(":0", ""): np.transpose(val, (2,1,0))})
                elif var.name.startswith('dense'):
                    tf_export_params.update(
                        {var.name.replace(":0", ""): np.transpose(val)})
            elif var.name.startswith('rnn/lstm_cell/kernel'):
                lstm_i2h , lstm_h2h = np.split(val, [64])
                i2h_i, i2h_c, i2h_f, i2h_o = np.split(lstm_i2h, 4, axis=1)
                h2h_i, h2h_c, h2h_f, h2h_o = np.split(lstm_h2h, 4, axis=1)
                lstm_i2h = np.concatenate((i2h_i, i2h_f, i2h_c, i2h_o), axis=1)
                lstm_h2h = np.concatenate((h2h_i, h2h_f, h2h_c, h2h_o), axis=1)
                tf_export_params['lstm_i2h_weight'] =  np.transpose(lstm_i2h)
                tf_export_params['lstm_h2h_weight'] =  np.transpose(lstm_h2h)
            elif var.name.startswith('rnn/lstm_cell/bias'):
                h2h_i, h2h_c, h2h_f, h2h_o = np.split(val, 4)
                val = np.concatenate((h2h_c, h2h_f, h2h_i, h2h_o))
                tf_export_params.update({var.name.replace('rnn/lstm_cell/bias:0','lstm_h2h_bias'): val})
                tf_export_params.update({var.name.replace('rnn/lstm_cell/bias:0','lstm_i2h_bias'): val})
            else:
                tf_export_params.update({var.name.replace(":0", ""): val})

        return tf_export_params

    def set_learning_rate(self, lr):
        return lr

def _fit_model_tf(model, net_params, data_iter, valid_iter, max_iterations, verbose, lr):
    
    from time import time as _time
    init = tf.compat.v1.global_variables_initializer()
    
    model.sess.run(init)
    for key in net_params.keys():
        if key in model.weights.keys():
            if key.startswith('conv'):
                model.sess.run(tf.assign(tf.get_default_graph().get_tensor_by_name(key+":0"), np.transpose(net_params[key].asnumpy(), (2, 1, 0))))
            elif key.startswith('dense'):
                model.sess.run(tf.assign(tf.get_default_graph().get_tensor_by_name(key+":0"), np.transpose(net_params[key].asnumpy()) ))
        if key in model.biases.keys():
            model.sess.run(tf.assign(tf.get_default_graph().get_tensor_by_name(key+":0"), net_params[key].asnumpy()))
        if key in model.bn_vars.keys():
            model.sess.run(tf.assign(tf.get_default_graph().get_tensor_by_name(key+":0"), net_params[key].asnumpy()))

    if verbose:
         # Print progress table header
        column_names = ['Iteration', 'Train Accuracy', 'Train Loss']
        if valid_iter:
            column_names += ['Validation Accuracy', 'Validation Loss']
        column_names.append('Elapsed Time')
        num_columns = len(column_names)
        column_width = max(map(lambda x: len(x), column_names)) + 2
        hr = '+' + '+'.join(['-' * column_width] * num_columns) + '+'
        print(hr)
        print(('| {:<{width}}' * num_columns + '|').format(*column_names, width=column_width-1))
        print(hr)

    begin = _time()
    for iteration in range(max_iterations):
        log = {
            'train_loss': 0.,
            'train_acc': 0.
        }

        # Training iteration
        data_iter.reset()
        
        train_batches = float(data_iter.num_batches) 
        for batch in data_iter:
            feed_dict = {"input": batch.data, "labels": batch.labels, "weights": batch.weights }
            result = model.train(feed_dict)
            log['train_loss'] += result['loss'] 
            log['train_acc'] += result['acc'] 
    
        log['train_loss'] = log['train_loss']/train_batches
        log['train_acc'] = log['train_acc']/train_batches

        # Validation iteration
        if valid_iter is not None:
            log['valid_acc'] = 0
            log['valid_loss'] = 0
            
            valid_iter.reset()
            valid_batches = float(valid_iter.num_batches) 
            for val in valid_iter:
                feed_dict = {"input": val.data, "labels": batch.labels, "weights": batch.weights }
                val_results = model.predict(feed_dict)
                log['valid_loss'] += val_results['loss'] 
                log['valid_acc'] += val_results['acc'] 

            log['valid_loss'] = log['valid_loss'] / valid_batches
            log['valid_acc'] = log['valid_acc'] / valid_batches

        if verbose:
            elapsed_time = _time() - begin
            if valid_iter is None:
                # print progress row without validation info
                print("| {cur_iter:<{width}}| {train_acc:<{width}.3f}| {train_loss:<{width}.3f}| {time:<{width}.1f}|".format(
                          cur_iter = iteration + 1, train_acc = log['train_acc'], train_loss = log['train_loss'],
                          time = elapsed_time, width = column_width-1))
            else:
                # print progress row with validation info
                print("| {cur_iter:<{width}}| {train_acc:<{width}.3f}| {train_loss:<{width}.3f}"
                      "| {valid_acc:<{width}.3f}| {valid_loss:<{width}.3f}| {time:<{width}.1f}| ".format(
                          cur_iter = iteration + 1, train_acc = log['train_acc'], train_loss = log['train_loss'],
                          valid_acc = log['valid_acc'], valid_loss = log['valid_loss'], time = elapsed_time,
                          width = column_width-1))

    if verbose:
        print(hr)
        print('Training complete')
        end = _time()
        print('Total Time Spent: %gs' % (end - begin))

    return log
        
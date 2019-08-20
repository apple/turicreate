# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import tensorflow as tf 
import numpy as np

_net_params = {
    'conv_h': 64,
    'lstm_h': 200,
    'dense_h': 128
}

class ActivityTensorFlowModel():
    def __init__(self, batch_size, num_features, num_classes, prediction_window, seq_len):

        tf.compat.v1.get_default_graph()

        self.num_classes = num_classes
        self.batch_size = batch_size
        self.seq_len = seq_len

        # Initializer
        initializer = tf.compat.v1.glorot_uniform_initializer()

        # Vars
        self.data = tf.compat.v1.placeholder(tf.float32, [None, prediction_window*seq_len, num_features])
        self.weight = tf.compat.v1.placeholder(tf.float32, [None, seq_len])
        self.target = tf.compat.v1.placeholder(tf.float32, [None, seq_len, self.num_classes])

        # Weights 
        weights = {
        'conv_weight' : tf.Variable(initializer([prediction_window, num_features, _net_params['conv_h']]), name='conv_weight'),
        'lstm_i2h_weight' : tf.Variable(initializer([_net_params['conv_h'], _net_params['lstm_h']*4]), name='lstm_i2h_weight'),
        'lstm_h2h_weight': tf.Variable(initializer([_net_params['lstm_h'], _net_params['lstm_h']*4]), name='lstm_h2h_weight'),
        'dense0_weight': tf.Variable(initializer([_net_params['lstm_h'], _net_params['dense_h']]), name='dense0_weight'),
        'dense1_weight'  : tf.Variable(initializer([_net_params['dense_h'], num_classes]), name='dense1_weight')
        }

        # Biases
        biases = {
        'conv_bias' : tf.Variable(tf.zeros([_net_params['conv_h']]), name='conv_bias'),
        'lstm_i2h_bias' : tf.Variable(tf.zeros([_net_params['lstm_h']*4]), name='lstm_i2h_bias'),
        'lstm_h2h_bias' : tf.Variable(tf.zeros([_net_params['lstm_h']*4]), name='lstm_h2h_bias'),
        'dense0_bias': tf.Variable(tf.zeros([_net_params['dense_h']]), name='dense0_bias'),
        'dense1_bias'  : tf.Variable(tf.zeros([num_classes]), name='dense1_bias')
        }

        # Batch Normalisation
        scale = tf.Variable(tf.ones([_net_params['dense_h']]), name='bn_gamma') #gamma
        offset = tf.Variable(tf.zeros([_net_params['dense_h']]), name='bn_beta') #beta
        mean = tf.Variable(tf.ones([_net_params['dense_h']]), name='bn_moving_mean')
        variance = tf.Variable(tf.ones([_net_params['dense_h']]), name='bn_moving_var')

        # Convolution 
        conv = tf.nn.conv1d(self.data, weights['conv_weight'], stride=prediction_window, padding='SAME')
        conv = tf.nn.bias_add(conv, biases['conv_bias'])
        conv = tf.nn.relu(conv)

        # Long Stem Term Memory
        conv = tf.nn.dropout(conv, rate=0.2)
        cells = tf.nn.rnn_cell.LSTMCell(num_units=_net_params['lstm_h'], state_is_tuple=True, forget_bias=0.0, reuse=tf.compat.v1.AUTO_REUSE)
        init_state = cells.zero_state(batch_size, tf.float32)
        rnn_outputs, final_state = tf.nn.dynamic_rnn(cells, conv, initial_state=init_state)

        # Dense
        dense = tf.add(tf.matmul( rnn_outputs, weights['dense0_weight']), biases['dense0_bias'])
        dense = tf.nn.batch_normalization(dense, mean, variance, offset, scale, 1e-05)
        dense = tf.nn.relu(dense)
        dense = tf.nn.dropout(dense, rate=0.5)

        # Output
        out = tf.add(tf.matmul(dense, weights['dense1_weight']),biases['dense1_bias'])
        self.probs = tf.nn.softmax(out)

        seq_sum_weights = tf.reduce_sum(self.weight, axis=1)
        binary_seq_sum_weights = tf.reduce_sum(seq_sum_weights)

        self.loss_per_seq = tf.compat.v1.losses.softmax_cross_entropy(logits=self.probs, onehot_labels=self.target, weights=self.weight)
        self.loss_op = tf.reduce_sum(self.loss_per_seq) / (binary_seq_sum_weights + 1e-5)

        optimizer = tf.compat.v1.train.AdamOptimizer(learning_rate=self.set_learning_rate(1e-3))
        self.train_op = optimizer.minimize(self.loss_op)
        
        correct_pred = tf.cast(tf.equal(tf.argmax(out, 2), tf.argmax(self.target, 2)), dtype=tf.float32)
        correct_pred = tf.tensordot(correct_pred, tf.transpose(self.weight), axes=1)
        self.acc_per_seq = tf.reduce_sum(correct_pred) / (seq_sum_weights + 1e-5 )
        self.acc = tf.reduce_sum(self.acc_per_seq) / (binary_seq_sum_weights + 1e-5)

        # Session 
        self.sess = tf.compat.v1.Session()

    def train(self, feed_dict):
        labels = feed_dict["labels"].reshape((self.batch_size, self.seq_len))
        l = tf.one_hot(labels, self.num_classes, axis = -1)
        l = l.eval(session=self.sess)
        w = feed_dict["weights"].reshape((self.batch_size, self.seq_len))
        _, loss, probs, acc = self.sess.run([self.train_op, self.loss_op, self.probs, self.acc], feed_dict={self.data : feed_dict['input'], self.target : l, self.weight : w})
        result = {'loss' : loss, 'output': probs, 'acc': acc}
        return result

    def predict(self, feed_dict):
        loss, probs, acc = self.sess.run([self.loss_op, self.probs, self.acc], feed_dict={self.data : feed_dict['input']})
        result = {'loss' : loss, 'output': probs, 'acc':acc }
        return result

    def export_weights(self):
        vars = tf.compat.v1.trainable_variables()
        vars_vals = self.sess.run(vars)
        for var, val in zip(vars, vars_vals):
            result = {"var: {}, value: {}".format(var.name, val)}
            # print("var: {}, value: {}".format(var.name, val))
        return result

    def set_learning_rate(self, lr):
        self.learning_rate = lr
        return self.learning_rate

def fit_model_tf(model, data_iter, valid_iter, max_iterations, verbose, lr):
    from time import time as _time
    init = tf.compat.v1.global_variables_initializer()
    
    model.sess.run(init)
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
            # print(loss, probs)
            # print(loss_per_seq_value, loss_value, acc_per_seq_value, acc_value)

            log['train_loss'] += result['loss'] / train_batches
            log['train_acc'] += result['acc'] / train_batches
            

        # Validation iteration
        # if valid_iter is not None:
        #     valid_num_seq = valid_iter.num_rows
        #     valid_metrics = model.iter_predict(valid_iter)
        #     valid_metrics = [(_mx.nd.sum(m[0][2]).asscalar(),
        #                       _mx.nd.sum(m[0][3]).asscalar())
        #                      for m in valid_metrics]
        #     valid_loss, valid_acc = zip(*valid_metrics)
        #     log['valid_loss'] = sum(valid_loss) / valid_num_seq
        #     log['valid_acc'] = sum(valid_acc) / valid_num_seq

        if verbose:
            elapsed_time = _time() - begin
            if valid_iter is None:
                # print progress row without validation info
                print("| {cur_iter:<{width}}| {train_acc:<{width}.3f}| {train_loss:<{width}.3f}| {time:<{width}.1f}|".format(
                          cur_iter = iteration + 1, train_acc = log['train_acc'], train_loss = log['train_loss'],
                          time = elapsed_time, width = column_width-1))
            # else:
            #     # print progress row with validation info
            #     print("| {cur_iter:<{width}}| {train_acc:<{width}.3f}| {train_loss:<{width}.3f}"
            #           "| {valid_acc:<{width}.3f}| {valid_loss:<{width}.3f}| {time:<{width}.1f}| ".format(
            #               cur_iter = iteration + 1, train_acc = log['train_acc'], train_loss = log['train_loss'],
            #               valid_acc = log['valid_acc'], valid_loss = log['valid_loss'], time = elapsed_time,
            #               width = column_width-1))

    if verbose:
        print(hr)
        print('Training complete')
        end = _time()
        print('Total Time Spent: %gs' % (end - begin))

    return log
        
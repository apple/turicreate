# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import tensorflow as _tf 
import numpy as _np
from .._tf_model import TensorFlowModel


_net_params = {
    'conv_h': 64,
    'lstm_h': 200,
    'dense_h': 128
}


class ActivityTensorFlowModel(TensorFlowModel):
    
    def __init__(self, net_params, batch_size, num_features, num_classes, prediction_window, seq_len):

        _tf.reset_default_graph()

        self.num_classes = num_classes
        self.batch_size = batch_size
        self.seq_len = seq_len

        # Vars
        self.data = _tf.compat.v1.placeholder(_tf.float32, [None, prediction_window*seq_len, num_features])
        self.weight = _tf.compat.v1.placeholder(_tf.float32, [None, seq_len, 1])
        self.target = _tf.compat.v1.placeholder(_tf.int32, [None, seq_len, 1])
        self.is_training = _tf.compat.v1.placeholder(_tf.bool)

        # Reshaping weights 
        reshaped_weight = _tf.reshape(self.weight, [self.batch_size, seq_len])

        # One hot encoding target
        reshaped_target = _tf.reshape(self.target, [self.batch_size, seq_len])
        one_hot_target = _tf.one_hot(reshaped_target, depth=self.num_classes, axis=-1)

        # Weights 
        weights = {
        'conv_weight' : _tf.Variable(_tf.zeros([prediction_window, num_features, _net_params['conv_h']]), name='conv_weight'),
        'dense0_weight': _tf.Variable(_tf.zeros([_net_params['lstm_h'], _net_params['dense_h']]), name='dense0_weight'),
        'dense1_weight'  : _tf.Variable(_tf.zeros([_net_params['dense_h'], self.num_classes]), name='dense1_weight')
        }

        # Biases
        biases = {
        'conv_bias' : _tf.Variable(_tf.zeros([_net_params['conv_h']]), name='conv_bias'),
        'dense0_bias': _tf.Variable(_tf.zeros([_net_params['dense_h']]), name='dense0_bias'),
        'dense1_bias'  : _tf.Variable(_tf.zeros([num_classes]), name='dense1_bias')
        }

        # Convolution 
        conv = _tf.nn.conv1d(self.data, weights['conv_weight'], stride=prediction_window, padding='SAME')
        conv = _tf.nn.bias_add(conv, biases['conv_bias'])
        conv = _tf.nn.relu(conv)
        dropout = _tf.layers.dropout(conv, rate=0.2, training=self.is_training)

        # Long Stem Term Memory
        i2h_i, i2h_f, i2h_c, i2h_o = _np.split(net_params['lstm_i2h_weight'].asnumpy(), 4)
        h2h_i, h2h_f, h2h_c, h2h_o = _np.split(net_params['lstm_h2h_weight'].asnumpy(), 4)
        i2h = _np.concatenate((i2h_i, i2h_c, i2h_f, i2h_o), axis=0)
        h2h = _np.concatenate((h2h_i, h2h_c, h2h_f, h2h_o), axis=0)
        lstm = _np.concatenate((i2h, h2h), axis=1)
        cells = _tf.nn.rnn_cell.LSTMCell(num_units=_net_params['lstm_h'], reuse=_tf.AUTO_REUSE, forget_bias=0.0, 
            initializer=_tf.initializers.constant(_np.transpose(lstm), verify_shape=True))
        init_state = cells.zero_state(batch_size, _tf.float32)
        rnn_outputs, final_state = _tf.nn.dynamic_rnn(cells, dropout, initial_state=init_state)

        # Dense
        dense = _tf.reshape(rnn_outputs, (-1, _net_params['lstm_h']))
        dense = _tf.add(_tf.matmul(dense, weights['dense0_weight']), biases['dense0_bias'])
        dense = _tf.layers.batch_normalization(inputs=dense, 
            beta_initializer=_tf.initializers.constant(net_params['bn_beta'].asnumpy(), verify_shape=True), 
            gamma_initializer=_tf.initializers.constant(net_params['bn_gamma'].asnumpy(), verify_shape=True),
            moving_mean_initializer=_tf.initializers.constant(net_params['bn_moving_mean'].asnumpy(), verify_shape=True), 
            moving_variance_initializer=_tf.initializers.constant(net_params['bn_moving_var'].asnumpy(), verify_shape=True), training=self.is_training )
        dense = _tf.nn.relu(dense)
        dense = _tf.layers.dropout(dense, rate=0.5, training=self.is_training)
        
        # Output
        out = _tf.add(_tf.matmul(dense, weights['dense1_weight']), biases['dense1_bias'])
        out = _tf.reshape(out, (-1, self.seq_len, self.num_classes))
        self.probs = _tf.nn.softmax(out)

        # Weights
        seq_sum_weights = _tf.reduce_sum(reshaped_weight, axis=1)
        binary_seq_sum_weights = _tf.reduce_sum(_tf.cast(seq_sum_weights > 0, dtype=_tf.float32))

        # Loss
        self.loss_op = _tf.losses.softmax_cross_entropy(logits=out, onehot_labels=one_hot_target, weights=reshaped_weight)
        
        # Optimizer 
        update_ops = _tf.get_collection(_tf.GraphKeys.UPDATE_OPS)
        self.set_learning_rate(1e-3)
        train_op = self.optimizer.minimize(self.loss_op)
        self.train_op = _tf.group([train_op, update_ops])
        
        # Predictions
        correct_pred = _tf.reduce_sum(_tf.cast(_tf.equal(_tf.argmax(self.probs, 2), _tf.argmax(one_hot_target, 2)), dtype=_tf.float32) * reshaped_weight, axis=1)
        acc_per_seq = correct_pred / (seq_sum_weights + 1e-5)
        self.acc = _tf.reduce_sum(acc_per_seq) / (binary_seq_sum_weights + 1e-5)

        # Session 
        self.sess = _tf.compat.v1.Session()

        # Initialize all variables
        init = _tf.compat.v1.global_variables_initializer()
        self.sess.run(init)

        # Assign the initialised weights from MXNet to tensorflow 
        for key in net_params.keys():
            if key in weights.keys():
                if key.startswith('conv'):
                    self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(key+":0"), _np.transpose(net_params[key].asnumpy(), (2, 1, 0))))
                elif key.startswith('dense'):
                    self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(key+":0"), _np.transpose(net_params[key].asnumpy()) ))
            if key in biases.keys():
                self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(key+":0"), net_params[key].asnumpy()))


    def train(self, feed_dict):

        _, loss, acc, probs = self.sess.run([self.train_op, self.loss_op, self.acc, self.probs], 
            feed_dict={self.data : feed_dict['input'], self.target : feed_dict['labels'], self.weight : feed_dict['weights'], self.is_training : True})
        result = {'loss' : loss, 'acc': acc, 'probs': probs }
        return result


    def predict(self, feed_dict):

        loss, acc, probs= self.sess.run([self.loss_op, self.acc, self.probs], 
            feed_dict={self.data : feed_dict['input'], self.target : feed_dict['labels'], self.weight : feed_dict['weights'], self.is_training: False})
        result = {'loss' : loss, 'acc': acc, 'probs': probs }
        return result


    def export_weights(self):

        # Export weights in Core ML Spec
        tf_export_params = {}
        tvars = _tf.trainable_variables()
        tvars_vals = self.sess.run(tvars)
        for var, val in zip(tvars, tvars_vals):
            if 'weight' in var.name:
                if var.name.startswith('conv'):
                    tf_export_params[var.name.split(':')[0]] = _np.transpose(val, (2,1,0))
                elif var.name.startswith('dense'):
                    tf_export_params[var.name.split(':')[0]] = _np.transpose(val)
            elif var.name.startswith('rnn/lstm_cell/kernel'):
                lstm_i2h , lstm_h2h = _np.split(val, [64])
                i2h_i, i2h_c, i2h_f, i2h_o = _np.split(lstm_i2h, 4, axis=1)
                h2h_i, h2h_c, h2h_f, h2h_o = _np.split(lstm_h2h, 4, axis=1)
                tf_export_params['lstm_i2h_i_weight'] = _np.transpose(i2h_i)
                tf_export_params['lstm_i2h_c_weight'] = _np.transpose(i2h_c)
                tf_export_params['lstm_i2h_f_weight'] = _np.transpose(i2h_f)
                tf_export_params['lstm_i2h_o_weight'] = _np.transpose(i2h_o)
                tf_export_params['lstm_h2h_i_weight'] = _np.transpose(h2h_i)
                tf_export_params['lstm_h2h_c_weight'] = _np.transpose(h2h_c)
                tf_export_params['lstm_h2h_f_weight'] = _np.transpose(h2h_f)
                tf_export_params['lstm_h2h_o_weight'] = _np.transpose(h2h_o)
            elif var.name.startswith('rnn/lstm_cell/bias'):
                h2h_i, h2h_c, h2h_f, h2h_o = _np.split(val, 4)
                val = _np.concatenate((h2h_i, h2h_f, h2h_c, h2h_o))
                tf_export_params['lstm_h2h_i_bias'] = _np.transpose(h2h_i)
                tf_export_params['lstm_h2h_c_bias'] = _np.transpose(h2h_c)
                tf_export_params['lstm_h2h_f_bias'] = _np.transpose(h2h_f)
                tf_export_params['lstm_h2h_o_bias'] = _np.transpose(h2h_o)
            elif var.name.startswith('batch_normalization'):
                tf_export_params['bn_'+var.name.split('/')[-1][0:-2]] = val
            else:
                tf_export_params[var.name.split(':')[0]] = val
        tvars = _tf.all_variables()
        tvars_vals = self.sess.run(tvars)
        for var, val in zip(tvars, tvars_vals):
            if 'moving_mean' in var.name:
                tf_export_params['bn_running_mean'] = val
            if 'moving_variance' in var.name:
                tf_export_params['bn_running_var'] = val
        return tf_export_params

    def set_learning_rate(self, lr):
        self.optimizer = _tf.train.AdamOptimizer(learning_rate=lr)

    def get_weights(self):

        import mxnet as _mx 

        arg_params = {}
        tvars = _tf.trainable_variables()
        tvars_vals = self.sess.run(tvars)
        for var, val in zip(tvars, tvars_vals):
            if 'weight' in var.name:
                if var.name.startswith('conv'):
                    arg_params.update(
                        {var.name.replace(":0", ""): _mx.nd.array(_np.transpose(val, (2,1,0)))})
                elif var.name.startswith('dense'):
                    arg_params.update(
                        {var.name.replace(":0", ""): _mx.nd.array(_np.transpose(val))})
            elif var.name.startswith('rnn/lstm_cell/kernel'):
                lstm_i2h , lstm_h2h = _np.split(val, [64])
                i2h_i, i2h_c, i2h_f, i2h_o = _np.split(lstm_i2h, 4, axis=1)
                h2h_i, h2h_c, h2h_f, h2h_o = _np.split(lstm_h2h, 4, axis=1)
                lstm_i2h = _np.concatenate((i2h_i, i2h_f, i2h_c, i2h_o), axis=1)
                lstm_h2h = _np.concatenate((h2h_i, h2h_f, h2h_c, h2h_o), axis=1)
                arg_params['lstm_i2h_weight'] =  _mx.nd.array(_np.transpose(lstm_i2h))
                arg_params['lstm_h2h_weight'] =  _mx.nd.array(_np.transpose(lstm_h2h))
            elif var.name.startswith('rnn/lstm_cell/bias'):
                h2h_i, h2h_c, h2h_f, h2h_o = _np.split(val, 4)
                val = _np.concatenate((h2h_i, h2h_f, h2h_c, h2h_o))
                arg_params.update({var.name.replace('rnn/lstm_cell/bias:0','lstm_h2h_bias'): _mx.nd.array(val)})
                arg_params.update({var.name.replace('rnn/lstm_cell/bias:0','lstm_i2h_bias'): _mx.nd.array(_np.zeros(val.shape))})
            elif var.name.startswith('batch_normalization'):
                arg_params['bn_'+var.name.split('/')[-1][0:-2]] = _mx.nd.array(val)
            else:
                arg_params.update({var.name.replace(":0", ""): _mx.nd.array(val)})

        aux_params = {}
        tvars = _tf.all_variables()
        tvars_vals = self.sess.run(tvars)
        for var, val in zip(tvars, tvars_vals):
            if 'moving_mean' in var.name:
                aux_params['bn_moving_mean'] = _mx.nd.array(val)
            if 'moving_variance' in var.name:
                aux_params['bn_moving_var'] = _mx.nd.array(val)
        return arg_params, aux_params


def _fit_model_tf(model, net_params, data_iter, valid_iter, max_iterations, verbose, lr):
    
    from time import time as _time

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
            train_feed_dict = {"input": batch.data, "labels": batch.labels, "weights": batch.weights }
            result = model.train(train_feed_dict)
            log['train_loss'] += result['loss'] /train_batches
            log['train_acc'] += result['acc'] /train_batches

        # Validation iteration
        if valid_iter is not None:
            log['valid_acc'] = 0
            log['valid_loss'] = 0
            
            valid_iter.reset()
            valid_batches = float(valid_iter.num_batches) 
            for val in valid_iter:
                val_feed_dict = {"input": val.data, "labels": val.labels, "weights": val.weights }
                val_results = model.predict(val_feed_dict)
                log['valid_loss'] += val_results['loss']  / valid_batches
                log['valid_acc'] += val_results['acc']  / valid_batches

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
        
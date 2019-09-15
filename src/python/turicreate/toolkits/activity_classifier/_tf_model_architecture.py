# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import tensorflow as _tf 
from .._tf_model import TensorFlowModel
import turicreate.toolkits.libtctensorflow
import turicreate.toolkits._tf_utils as _utils

import numpy as _np

# Constant parameters for the neural network
CONV_H = 64
LSTM_H = 200
DENSE_H = 128
        
class ActivityTensorFlowModel(TensorFlowModel):
    
    def __init__(self, net_params, batch_size, num_features, num_classes, prediction_window, seq_len):

        _utils.hey()

        print("a")

        # Suppresses verbosity to only errors
        # _tf.compat.v1.logging.set_verbosity(_tf.compat.v1.logging.ERROR)

        print("b")

        for key in net_params.keys():
            net_params[key] = _utils.convert_shared_float_array_to_numpy(net_params[key])

        print("c")

        _tf.compat.v1.reset_default_graph

        print("d")

        self.num_classes = num_classes
        self.batch_size = batch_size
        self.seq_len = seq_len

        print("e")
        
        # Vars
        self.data = _tf.compat.v1.placeholder(_tf.float32, [None, prediction_window*seq_len, num_features])
        self.weight = _tf.compat.v1.placeholder(_tf.float32, [None, seq_len, 1])
        self.target = _tf.compat.v1.placeholder(_tf.int32, [None, seq_len, 1])
        self.is_training = _tf.compat.v1.placeholder(_tf.bool)

        print("f")
        
        # Reshaping weights 
        reshaped_weight = _tf.reshape(self.weight, [self.batch_size, seq_len])

        # One hot encoding target
        reshaped_target = _tf.reshape(self.target, [self.batch_size, seq_len])
        one_hot_target = _tf.one_hot(reshaped_target, depth=self.num_classes, axis=-1)
        
        # Weights 
        weights = {
        'conv_weight' : _tf.Variable(_tf.zeros([prediction_window, num_features, CONV_H]), name='conv_weight'),
        'dense0_weight': _tf.Variable(_tf.zeros([LSTM_H, DENSE_H]), name='dense0_weight'),
        'dense1_weight'  : _tf.Variable(_tf.zeros([DENSE_H, self.num_classes]), name='dense1_weight')
        }

        # Biases
        biases = {
        'conv_bias' : _tf.Variable(_tf.zeros([CONV_H]), name='conv_bias'),
        'dense0_bias': _tf.Variable(_tf.zeros([DENSE_H]), name='dense0_bias'),
        'dense1_bias'  : _tf.Variable(_tf.zeros([num_classes]), name='dense1_bias')
        }

        # Convolution 
        conv = _tf.nn.conv1d(self.data, weights['conv_weight'], stride=prediction_window, padding='SAME')
        conv = _tf.nn.bias_add(conv, biases['conv_bias'])
        conv = _tf.nn.relu(conv)
        dropout = _tf.layers.dropout(conv, rate=0.2, training=self.is_training)

        print("g")

        # Long Stem Term Memory
        i2h_i = net_params['lstm_i2h_i_weight']
        i2h_f = net_params['lstm_i2h_f_weight']
        i2h_c = net_params['lstm_i2h_c_weight']
        i2h_o = net_params['lstm_i2h_o_weight']
        h2h_i = net_params['lstm_h2h_i_weight']
        h2h_f = net_params['lstm_h2h_f_weight']
        h2h_c = net_params['lstm_h2h_c_weight']
        h2h_o = net_params['lstm_h2h_o_weight']
        lstm = _utils.convert_lstm_weight_coreml_to_tf(i2h_i, i2h_c, i2h_f, i2h_o, h2h_i, h2h_c, h2h_f, h2h_o)
        cells = _tf.nn.rnn_cell.LSTMCell(num_units=LSTM_H, reuse=_tf.AUTO_REUSE, forget_bias=0.0, 
            initializer=_tf.initializers.constant(lstm, verify_shape=True))
        init_state = cells.zero_state(batch_size, _tf.float32)
        rnn_outputs, final_state = _tf.nn.dynamic_rnn(cells, dropout, initial_state=init_state)

        print("h")
        # Dense
        dense = _tf.reshape(rnn_outputs, (-1, LSTM_H))
        dense = _tf.add(_tf.matmul(dense, weights['dense0_weight']), biases['dense0_bias'])
        dense = _tf.layers.batch_normalization(inputs=dense, 
            beta_initializer=_tf.initializers.constant(net_params['bn_beta'], verify_shape=True), 
            gamma_initializer=_tf.initializers.constant(net_params['bn_gamma'], verify_shape=True),
            moving_mean_initializer=_tf.initializers.constant(net_params['bn_running_mean'], verify_shape=True), 
            moving_variance_initializer=_tf.initializers.constant(net_params['bn_running_var'], verify_shape=True), training=self.is_training )
        dense = _tf.nn.relu(dense)
        dense = _tf.layers.dropout(dense, rate=0.5, training=self.is_training)
        print("i")
        # Output
        out = _tf.add(_tf.matmul(dense, weights['dense1_weight']), biases['dense1_bias'])
        out = _tf.reshape(out, (-1, self.seq_len, self.num_classes))
        self.probs = _tf.nn.softmax(out)
        print("j")
        # Weights
        seq_sum_weights = _tf.reduce_sum(reshaped_weight, axis=1)
        binary_seq_sum_weights = _tf.reduce_sum(_tf.cast(seq_sum_weights > 0, dtype=_tf.float32))
        print("k")
        # Loss
        self.loss_op = _tf.losses.softmax_cross_entropy(logits=out, onehot_labels=one_hot_target, weights=reshaped_weight)
        print("l")
        # Optimizer 
        update_ops = _tf.get_collection(_tf.GraphKeys.UPDATE_OPS)
        self.set_learning_rate(1e-3)
        train_op = self.optimizer.minimize(self.loss_op)
        self.train_op = _tf.group([train_op, update_ops])
        print("m")
        # Session 
        self.sess = _tf.compat.v1.Session()

        # Initialize all variables
        init = _tf.compat.v1.global_variables_initializer()
        self.sess.run(init)
        print("n")
        # Assign the initialised weights from the C++ implementation to tensorflow 
        for key in net_params.keys():
            if key in weights.keys():
                if key.startswith('conv'):
                    net_params[key] = _utils.convert_conv1d_coreml_to_tf(net_params[key])
                    self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(key+":0"), net_params[key]))
                elif key.startswith('dense'):
                    # net_params[key] =  _np.reshape(_np.transpose(net_params[key], (1, 0, 2, 3)), (net_params[key].shape[1], net_params[key].shape[0]))
                    net_params[key] = _utils.convert_dense_coreml_to_tf(net_params[key])
                    self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(key+":0"), net_params[key] ))
            if key in biases.keys():
                self.sess.run(_tf.assign(_tf.get_default_graph().get_tensor_by_name(key+":0"), net_params[key]))
        print("o")


    def train(self, feed_dict):

        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])
            feed_dict[key] =  _np.reshape(feed_dict[key], (feed_dict[key].shape[0], feed_dict[key].shape[2], feed_dict[key].shape[3]))
        _, loss, probs = self.sess.run([self.train_op, self.loss_op, self.probs], 
            feed_dict={self.data : feed_dict['input'], self.target : feed_dict['labels'], self.weight : feed_dict['weights'], self.is_training : True})
        
        prob = _np.array(probs)
        probabilities = _np.reshape(prob, (prob.shape[0], prob.shape[1]*prob.shape[2]))
        result = {'loss' : _np.array(loss), 'output': probabilities }

        return result


    def predict(self, feed_dict):

        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])
            feed_dict[key] =  _np.reshape(feed_dict[key], (feed_dict[key].shape[0], feed_dict[key].shape[2], feed_dict[key].shape[3]))

        if len(feed_dict.keys()) == 1 :
            probs = self.sess.run(self.probs, 
                feed_dict={self.data : feed_dict['input'], self.is_training: False})
            prob = _np.array(probs)
            probabilities = _np.reshape(prob, (prob.shape[0], prob.shape[1]*prob.shape[2]))
            result  = { 'output' :  probabilities}
            
        else:
            loss,  probs= self.sess.run([self.loss_op,  self.probs], 
                feed_dict={self.data : feed_dict['input'], self.target : feed_dict['labels'], self.weight : feed_dict['weights'], self.is_training: False})
            prob = _np.array(probs)
            probabilities = _np.reshape(prob, (prob.shape[0], prob.shape[1]*prob.shape[2]))
            result = {'loss' : _np.array(loss),  'output': probabilities }
        
        return result


    def export_weights(self):

        # Export weights in Core ML Spec 

        tf_export_params = {}
        tvars = _tf.trainable_variables()
        tvars_vals = self.sess.run(tvars)

        for var, val in zip(tvars, tvars_vals):
            
            if 'weight' in var.name:
                if var.name.startswith('conv'):
                    tf_export_params[var.name.split(':')[0]] = _utils.convert_conv1d_tf_to_coreml(val)
                elif var.name.startswith('dense'):
                    tf_export_params[var.name.split(':')[0]] =  _utils.convert_dense_tf_to_coreml(val)

            elif var.name.startswith('rnn/lstm_cell/kernel'):
                i2h_i, i2h_c, i2h_f, i2h_o, h2h_i, h2h_c, h2h_f, h2h_o = _utils.convert_lstm_weight_tf_to_coreml(val, CONV_H)
                tf_export_params['lstm_i2h_i_weight'] = i2h_i
                tf_export_params['lstm_i2h_c_weight'] = i2h_c
                tf_export_params['lstm_i2h_f_weight'] = i2h_f
                tf_export_params['lstm_i2h_o_weight'] = i2h_o
                tf_export_params['lstm_h2h_i_weight'] = h2h_i
                tf_export_params['lstm_h2h_c_weight'] = h2h_c
                tf_export_params['lstm_h2h_f_weight'] = h2h_f
                tf_export_params['lstm_h2h_o_weight'] = h2h_o
            elif var.name.startswith('rnn/lstm_cell/bias'):
                h2h_i, h2h_c, h2h_f, h2h_o = _utils.convert_lstm_bias_tf_to_coreml(val)
                tf_export_params['lstm_h2h_i_bias'] = h2h_i
                tf_export_params['lstm_h2h_c_bias'] = h2h_c
                tf_export_params['lstm_h2h_f_bias'] = h2h_f
                tf_export_params['lstm_h2h_o_bias'] = h2h_o
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

        # Set the learning rate
        self.optimizer = _tf.train.AdamOptimizer(learning_rate=lr)


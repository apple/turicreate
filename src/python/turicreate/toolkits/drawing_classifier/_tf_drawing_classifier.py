# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from turicreate.util import _ProgressTablePrinter
import tensorflow as tf
import tensorflow as _tf 
import numpy as np
import time as _time

class DrawingClassifierTensorFlowModel():

	def __init__(self, train_loader, validation_loader, validation_set, net_params, batch_size, num_classes, verbose):
		
		self.num_classes = num_classes
		self.batch_size = batch_size

		self.x = tf.compat.v1.placeholder("float", [None, 28, 28, 1])
		self.y = tf.compat.v1.placeholder("float", [None, self.num_classes])

		# Weights
		weights = {
		'drawing_conv0_weight' : _tf.Variable(_tf.zeros([3, 3, 1, 16]), name='drawing_conv0_weight'),
		'drawing_conv1_weight' : _tf.Variable(_tf.zeros([3, 3, 16, 32]), name='drawing_conv1_weight'),
		'drawing_conv2_weight' : _tf.Variable(_tf.zeros([3, 3, 32, 64]), name='drawing_conv2_weight'),
		'drawing_dense0_weight': _tf.Variable(_tf.zeros([576, 128]), name='drawing_dense0_weight'),
		'drawing_dense1_weight'  : _tf.Variable(_tf.zeros([128, 2]), name='drawing_dense1_weight')
		}

		# Biases
		biases = {
		'drawing_conv0_bias' : _tf.Variable(_tf.zeros([16]), name='drawing_conv0_bias'),
		'drawing_conv1_bias' : _tf.Variable(_tf.zeros([32]), name='drawing_conv1_bias'),
		'drawing_conv2_bias' : _tf.Variable(_tf.zeros([64]), name='drawing_conv2_bias'),
		'drawing_dense0_bias': _tf.Variable(_tf.zeros([128]), name='drawing_dense0_bias'),
		'drawing_dense1_bias'  : _tf.Variable(_tf.zeros([2]), name='drawing_dense1_bias')
		}

		conv_1 = _tf.nn.conv2d(self.x, weights["drawing_conv0_weight"], strides=1, padding='SAME')
		conv_1 = _tf.nn.bias_add(conv_1, biases["drawing_conv0_bias"])
		relu_1 = _tf.nn.relu(conv_1)
		pool_1 = tf.nn.max_pool2d(relu_1, 2, 2, 'SAME')

		conv_2 = _tf.nn.conv2d(pool_1, weights["drawing_conv1_weight"], strides=1, padding='VALID')
		conv_2 = _tf.nn.bias_add(conv_2, biases["drawing_conv1_bias"])
		relu_2 = _tf.nn.relu(conv_2)
		pool_2 = tf.nn.max_pool2d(relu_2, 2, 2, 'SAME')

		conv_3 = _tf.nn.conv2d(pool_2, weights["drawing_conv2_weight"], strides=1, padding='SAME')
		conv_3 = _tf.nn.bias_add(conv_3, biases["drawing_conv2_bias"])
		relu_3 = _tf.nn.relu(conv_3)
		pool_3 = tf.nn.max_pool2d(relu_3, 2, 2, 'SAME')

		# Flatten the data to a 1-D vector for the fully connected layer
		fc1 = tf.compat.v1.layers.Flatten()(pool_3)

		fc1 = tf.compat.v1.nn.xw_plus_b(fc1, weights=weights["drawing_dense0_weight"], biases=biases["drawing_dense0_bias"])
		fc1 = _tf.nn.relu(fc1)

		out = tf.compat.v1.nn.xw_plus_b(fc1, weights=weights["drawing_dense1_weight"], biases=biases["drawing_dense1_bias"])
		out = _tf.nn.softmax(out)

		self.predictions = out 

		# Loss
		self.cost = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(logits=self.predictions, labels=self.y))

		# Optimizer
		self.optimizer = tf.compat.v1.train.AdamOptimizer(learning_rate=0.001).minimize(self.cost)

		# Predictions
		correct_prediction = tf.equal(tf.argmax(self.predictions, 1), tf.argmax(self.y, 1))
		self.accuracy = tf.reduce_mean(tf.cast(correct_prediction, "float"))

		self.sess = _tf.compat.v1.Session()
		self.sess.run(tf.compat.v1.global_variables_initializer())

		# Assign the initialised weights from MXNet to tensorflow 
		layers = ['drawing_conv0_weight', 'drawing_conv0_bias', 'drawing_conv1_weight', 'drawing_conv1_bias', 'drawing_conv2_weight', 'drawing_conv2_bias', 'drawing_dense0_weight', 'drawing_dense0_bias', 'drawing_dense1_weight', 'drawing_dense1_bias']
		for key in layers:
			if 'bias' in key:
			    self.sess.run(_tf.compat.v1.assign(_tf.compat.v1.get_default_graph().get_tensor_by_name(key+":0"), net_params[key].data().asnumpy()))
			else:
			    if 'dense' in key:
			        self.sess.run(_tf.compat.v1.assign(_tf.compat.v1.get_default_graph().get_tensor_by_name(key+":0"), net_params[key].data().asnumpy().transpose(1,0)))
			    else:
			        self.sess.run(_tf.compat.v1.assign(_tf.compat.v1.get_default_graph().get_tensor_by_name(key+":0"), net_params[key].data().asnumpy().transpose(2, 3, 1, 0)))


	def tf_train_model(self, train_loader, validation_loader, validation_set, verbose):
		num_iter = 0
		if verbose:
			column_names = ['iteration', 'train_loss', 'train_accuracy', 'time']
			column_titles = ['Iteration', 'Training Loss', 'Training Accuracy', 'Elapsed Time (seconds)']
			if validation_set is not None:
			    column_names.insert(3, 'validation_accuracy')
			    column_titles.insert(3, 'Validation Accuracy')
			table_printer = _ProgressTablePrinter(column_names, column_titles)

		def process_data(batch_data):
			if batch_data.pad is not None:
				batch_x = batch_data.data[0].asnumpy().transpose(0, 2, 3, 1)[0:self.batch_size-batch_data.pad]
				batch_y = tf.keras.utils.to_categorical(batch_data.label[0].asnumpy()[0:self.batch_size-batch_data.pad], self.num_classes)
			else:
				batch_x = batch_data.data[0].asnumpy().transpose(0, 2, 3, 1)
				batch_y = tf.keras.utils.to_categorical(batch_data.label[0].asnumpy(), self.num_classes)
			return batch_x, batch_y

		start_time = _time.time()
		for train_batch in train_loader:
			batch_x, batch_y = process_data(train_batch)
			_, c, a = self.sess.run([self.optimizer, self.cost, self.accuracy], 
			                feed_dict={
			                    self.x: batch_x, 
			                    self.y: batch_y
			                })
			for val_batch in validation_loader:
				val_x, val_y = process_data(val_batch)
				val_acc = self.sess.run(self.accuracy, 
			                feed_dict={
			                    self.x: val_x, 
			                    self.y: val_y
			                })
			validation_loader.reset()
			if verbose:
				kwargs = {  "iteration": num_iter + 1,
				            "train_loss": "{:.3f}".format(c),
				            "train_accuracy": "{:.3f}".format(a),
				            "time": _time.time() - start_time}
				if validation_set is not None:
				    kwargs["validation_accuracy"] = "{:.3f}".format(val_acc)
				table_printer.print_row(**kwargs)
			num_iter+=1

		final_train_accuracy = a
		final_val_accuracy = val_acc if validation_set else None
		final_train_loss = c
		total_train_time = _time.time() - start_time

		return final_train_accuracy, final_val_accuracy, final_train_loss, total_train_time



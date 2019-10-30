# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


import mxnet as mx
import time
import turicreate as _tc


class SoundClassifierMXNETModel():


	def __init__(self, feature_extractor, num_labels, custom_layer_sizes, verbose):
		from .sound_classifier import SoundClassifier
		from .._mxnet import _mxnet_utils
		
		self.verbose = verbose
		self.custom_NN = SoundClassifier._build_custom_neural_network(feature_extractor.output_length, num_labels, custom_layer_sizes)
		self.ctx = _mxnet_utils.get_mxnet_context()
		self.custom_NN.initialize(mx.init.Xavier(), ctx=self.ctx)

		self.trainer = mx.gluon.Trainer(self.custom_NN.collect_params(), 'nag', {'learning_rate': 0.01, 'momentum': 0.9})
		self.train_accuracy = 0.0
		self.validation_accuracy = 0.0


	def train(self, train_data, validation_data, max_iterations, start_time):

		if self.verbose:
			# Setup progress table
			row_ids = ['iteration', 'train_accuracy', 'time']
			row_display_names = ['Iteration', 'Training Accuracy', 'Elapsed Time']
			if validation_data:
			    row_ids.insert(2, 'validation_accuracy')
			    row_display_names.insert(2, 'Validation Accuracy (%)')
			table_printer = _tc.util._ProgressTablePrinter(row_ids, row_display_names)

		train_metric = mx.metric.Accuracy()
		if validation_data:
		    validation_metric = mx.metric.Accuracy()
		softmax_cross_entropy_loss = mx.gluon.loss.SoftmaxCrossEntropyLoss()
		for i in range(max_iterations):
			# TODO: early stopping

			for batch in train_data:
			    data = mx.gluon.utils.split_and_load(batch.data[0], ctx_list=self.ctx, batch_axis=0, even_split=False)
			    label = mx.gluon.utils.split_and_load(batch.label[0], ctx_list=self.ctx, batch_axis=0, even_split=False)

			    # Inside training scope
			    with mx.autograd.record():
			        for x, y in zip(data, label):
			            z = self.custom_NN(x)
			            # Computes softmax cross entropy loss.
			            loss = softmax_cross_entropy_loss(z, y)
			            # Backpropagate the error for one iteration.
			            loss.backward()
			    # Make one step of parameter update. Trainer needs to know the
			    # batch size of data to normalize the gradient by 1/batch_size.
			    self.trainer.step(batch.data[0].shape[0])
			train_data.reset()

			# Calculate training metric
			for batch in train_data:
			    data = mx.gluon.utils.split_and_load(batch.data[0], ctx_list=self.ctx, batch_axis=0, even_split=False)
			    label = mx.gluon.utils.split_and_load(batch.label[0], ctx_list=self.ctx, batch_axis=0, even_split=False)
			    outputs = [self.custom_NN(x) for x in data]
			    train_metric.update(label, outputs)
			train_data.reset()

			# Calculate validation metric
			for batch in validation_data:
			    data = mx.gluon.utils.split_and_load(batch.data[0], ctx_list=self.ctx, batch_axis=0, even_split=False)
			    label = mx.gluon.utils.split_and_load(batch.label[0], ctx_list=self.ctx, batch_axis=0, even_split=False)
			    outputs = [self.custom_NN(x) for x in data]
			    validation_metric.update(label, outputs)

			# Get metrics, print progress table
			_, self.train_accuracy = train_metric.get()
			train_metric.reset()
			printed_row_values = {'iteration': i+1, 'train_accuracy': self.train_accuracy}
			if validation_data:
			    _, self.validation_accuracy = validation_metric.get()
			    printed_row_values['validation_accuracy'] = self.validation_accuracy
			    validation_metric.reset()
			    validation_data.reset()
			if self.verbose:
			    printed_row_values['time'] = time.time()-start_time
			    table_printer.print_row(**printed_row_values)

'''
	#def predict(self):


	#def evaluate(self):


#def _mx_train_model():
'''
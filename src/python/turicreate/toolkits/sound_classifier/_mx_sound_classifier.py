# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


import mxnet as mx
import time
import turicreate as _tc


class SoundClassifierMXNETModel():


	def __init__(self, feature_extractor, num_labels, custom_layer_sizes, verbose, ctx):
		from .sound_classifier import SoundClassifier
		from .._mxnet import _mxnet_utils
		
		self.verbose = verbose
		self.custom_NN = SoundClassifier._build_custom_neural_network(feature_extractor.output_length, num_labels, custom_layer_sizes)
		self.custom_NN.initialize(mx.init.Xavier(), ctx=ctx)

		self.trainer = mx.gluon.Trainer(self.custom_NN.collect_params(), 'nag', {'learning_rate': 0.01, 'momentum': 0.9})
		self.softmax_cross_entropy_loss = mx.gluon.loss.SoftmaxCrossEntropyLoss()


	def train(self, batch, data, label):
		# Inside training scope
		with mx.autograd.record():
		    for x, y in zip(data, label):
		        z = self.custom_NN(x)
		        # Computes softmax cross entropy loss.
		        loss = self.softmax_cross_entropy_loss(z, y)
		        # Backpropagate the error for one iteration.
		        loss.backward()
		# Make one step of parameter update. Trainer needs to know the
		# batch size of data to normalize the gradient by 1/batch_size.
		self.trainer.step(batch.data[0].shape[0])


	def predict(self, data):
		outputs = [self.custom_NN(x) for x in data]
		return outputs

'''
	#def evaluate(self):


#def _mx_train_model():
'''
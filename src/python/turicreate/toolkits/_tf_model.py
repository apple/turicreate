# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
@package turicreate.toolkits

Defines a basic interface for a tensorflow model object.
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import abc

class TensorFlowModel(object):
	__metaclass__ = abc.ABCMeta
	"""
	Base Class for neural networks written in tensorflow used to abstract across model
 	architectures. It defines the computational graph and initialize a session to run the graph.
 
	"""
	@abc.abstractmethod
	def __init__():
		# Make placeholders for input and targets
		# Make dictionaries for weights and biases
		# Make the graph 
        # Make loss_op with the loss and train_op with the optimizer
        # Session 
		NotImplementedError



	"""
	Train will do a forward and backward pass and update weights 
    This accepts a dictionary that has feature/target as key and 
    the numpy arrays as value corresponding to them respectively
    It returns a dictionary of loss and output (probabilities)
    This matches model backend train

	"""
	@abc.abstractmethod
	def train():
		NotImplementedError

	"""
	Predict does only a forward pass and does not update any weights
    This accepts a dictionary that has feature as key and its value
    as numpy arrays. It also returns a dictionary of loss and output 
    This matches the model backend predict
	"""
	@abc.abstractmethod
	def predict():
		NotImplementedError


	"""
	Exports the network weights.
	"""
	@abc.abstractmethod
	def export_weights():
		NotImplementedError

	"""
	Sets the learning rate to be used for future calls to train.
	"""
	@abc.abstractmethod
	def set_learning_rate():
		NotImplementedError

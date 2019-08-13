# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
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
 	architectures.
 
	"""
	@abc.abstractmethod
	def define_model_tf():
		pass

	"""
	Performs one forward-backward pass.

	"""
	@abc.abstractmethod
	def train():
		pass

	"""
	Performs one forward pass.
	"""
	@abc.abstractmethod
	def predict():
		pass


	"""
	Exports the network weights.
	"""
	@abc.abstractmethod
	def export_weights():
		pass

	"""
	Sets the learning rate to be used for future calls to train.
	"""
	@abc.abstractmethod
	def set_learning_rate():
		pass

# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import abc as _abc
import six as _six


@_six.add_metaclass(_abc.ABCMeta)
class TensorFlowModel(object):

    """
	Base Class for neural networks written in tensorflow used to abstract across model
 	architectures. It defines the computational graph and initialize a session to run the graph.

 	Make placeholders for input and targets
	self.data = tf.placeholder()
	self.target = tf.placeholder()

	Make dictionaries for weights and biases
	self.weights = {
	'conv' : tf.Variable()
	'dense0' : tf.Variable()
	}
	self.biases = {
	'conv' : tf.Variable()
	'dense0' : tf.Variable()
	}


	Make the graph
	conv = tf.nn.conv1d(self.data, self.weights['conv'], ..)
	dense = tf.add(tf.matmul() + self.bias())
	...

	Make loss_op with the loss and train_op with the optimizer
	loss_op =
	train_op =

	Define Session
	self.sess = tf.Session()
	"""

    @_abc.abstractmethod
    def __init__(self):
        raise NotImplementedError

    """
	Train will do a forward and backward pass and update weights
    This accepts a dictionary that has feature/target as key and
    the numpy arrays as value corresponding to them respectively.
    It returns a dictionary of loss and output (probabilities)
    This matches model backend train

    Argument : A dictionary of input and true labels
    Returns : A dictionary of expected output (toolkit specific)

    It will train a mini batch by running the optimizer in the session
    Running the optimizer is thepart that does back propogation
    self.sess.run([train_op, loss_op, ..], feed_dict= {self.data = ..., self.target= ..})

	"""

    def train(self, feed_dict):
        raise NotImplementedError

    """
	Predict does only a forward pass and does not update any weights
    This accepts a dictionary that has feature/target as key and
    the numpy arrays as value corresponding to them respectively.
    It also returns a dictionary of loss and output
    This matches the model backend predict

    Argument : A dictionary of input and true labels
    Returns : A dictionary of expected output (toolkit specific)

    It will calculate the specified outputs w
    self.sess.run([loss_op, ..], feed_dict= {self.data = ..., self.target= ..})

	"""

    def predict(self, feed_dict):
        raise NotImplementedError

    """
	Exports the network weights in CoreML format.
	Returns : A dictionary of weight names as keys and

    layer_names = tf.trainable_variables()
    layer_weights = self.sess.run(tvars)

    This will get you the layer names from tensorflow and their corresponding
    values. They need to be converted to CoreML format and stored back in a
    dictionary with their names and values of correct shapes.


	"""

    def export_weights(self):
        raise NotImplementedError

    """
	Sets the optimizer to learn at the specified learning rate or using a learning rate scheduler.

	"""

    def set_learning_rate(self, learning_rate):
        raise NotImplementedError

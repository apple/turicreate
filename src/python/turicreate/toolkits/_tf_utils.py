# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import numpy as np
	
def convert_shared_float_array_to_numpy(array):
	"""
	The initialization from C++ implementation is mapped to SharedFloatArray 
	in Python through Pybind. It must be converted to numpy arrays to be used
	in TensorFlow.

	All the weights from the C++ implementation are four dimensional.

	"""
	return np.array(array, copy=False)

def convert_conv1d_coreml_to_tf(conv_weights):
	"""
	The Convolutional weights in CoreML specification is sent as [C, H, 1, W] 
	and for TensorFlow it is converted to [H, W, C].

	"""
	conv_weights = np.transpose(conv_weights, (3, 1, 0, 2))
	return np.reshape(conv_weights, (conv_weights.shape[0], conv_weights.shape[1], conv_weights.shape[2]))

def convert_lstm_weight_coreml_to_tf(i2h_i, i2h_c, i2h_f, i2h_o, h2h_i, h2h_c, h2h_f, h2h_o):
	"""
	The weights of four gates of LSTM - input, state, forget, output are 
	sent separately for input to hidden layer and hidden to hidden layer. 

	For tensorflow, the layers - input to hidden and hidden to hidden are 
	combined.
	Internally we need to combine the gates it in the order of input, state,
	forget and output. 

	"""
	i2h = np.concatenate((i2h_i, i2h_c, i2h_f, i2h_o), axis=0)
	h2h = np.concatenate((h2h_i, h2h_c, h2h_f, h2h_o), axis=0)
	lstm = np.concatenate((i2h, h2h), axis=1)
	return np.transpose(lstm)

def convert_dense_coreml_to_tf(dense_weights):
	"""
	The Dense layer weights from CoreML are [C_in, C_out, 1, 1] and need to be 
	converted to [C_out, C_in] for TensorFlow.

	"""
	dense_weights = np.transpose(dense_weights, (1, 0, 2, 3))
	return np.reshape(dense_weights, (dense_weights.shape[0], dense_weights.shape[1]))

def convert_conv1d_tf_to_coreml(conv_weights):
	"""
	Convolutional weights from TensorFlow in the format [H, W, C] are converted 
	back in CoreML specifications [C, H, 1, W].

	"""
	conv_weights = np.transpose(conv_weights, (2, 1, 0))
	return np.reshape(conv_weights, (conv_weights.shape[0], conv_weights.shape[1], 1, conv_weights.shape[2]))

def convert_lstm_weight_tf_to_coreml(lstm_weight, split):
	"""
	The weights of four gates of LSTM - input, state, forget, output are 
	sent separately for input to hidden layer and hidden to hidden layer
	to abide by CoreML specification.


	"""
	lstm_i2h , lstm_h2h = np.split(lstm_weight, [split])
	i2h_i, i2h_c, i2h_f, i2h_o = np.split(lstm_i2h, 4, axis=1)
	h2h_i, h2h_c, h2h_f, h2h_o = np.split(lstm_h2h, 4, axis=1)
	i2h_i = np.transpose(i2h_i)
	i2h_c = np.transpose(i2h_c)
	i2h_f = np.transpose(i2h_f)
	i2h_o = np.transpose(i2h_o)
	h2h_i = np.transpose(h2h_i)
	h2h_c = np.transpose(h2h_c)
	h2h_f = np.transpose(h2h_f)
	h2h_o = np.transpose(h2h_o)
	return i2h_i, i2h_c, i2h_f, i2h_o, h2h_i, h2h_c, h2h_f, h2h_o

def convert_lstm_bias_tf_to_coreml(lstm_bias):
	"""
	The biases of four gates of LSTM - input, state, forget, output are 
	sent separately for hidden to hidden layer to abide by CoreML specification.

	"""
	h2h_i, h2h_c, h2h_f, h2h_o = np.split(lstm_bias, 4)
	h2h_i = np.transpose(h2h_i)
	h2h_c = np.transpose(h2h_c)
	h2h_f = np.transpose(h2h_f)
	h2h_o = np.transpose(h2h_o)
	return h2h_i, h2h_c, h2h_f, h2h_o

def convert_dense_tf_to_coreml(dense_weights):
	"""
	The Dense layer weights from TensorFlow are [C_out, C_in] which are converted 
	back in CoreML specification [C_in, C_out, 1, 1].

	"""
	dense_weights = np.transpose(dense_weights)
	return np.reshape(dense_weights, (dense_weights.shape[0], dense_weights.shape[1], 1, 1))



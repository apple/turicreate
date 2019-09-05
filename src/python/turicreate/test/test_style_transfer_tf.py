# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import pytest
from . import util as test_util
import turicreate as tc
import tempfile
import numpy as np
import platform
import sys
import os
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe, _mac_ver
import coremltools

_NUM_STYLES = 4
_MARGIN_OF_ERROR = 5e-3

def check_for_tensorflow():
    try:
        import tensorflow  
        return False
    except ImportError:
        return True

@unittest.skipIf(check_for_tensorflow(), 'Requires Tensorflow to be Installed')
class StyleTransferTFTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        pass

    def test_instance_norm_network(self):
        import tensorflow as _tf
        import mxnet as _mx
        import numpy as _np

        from ..toolkits.style_transfer._tf_model_architecture import define_instance_norm as _define_instance_norm
        from ..toolkits.style_transfer._model import InstanceNorm as _InstanceNorm

        from ..toolkits.style_transfer._tf_model_architecture import define_tensorflow_variables as _define_tensorflow_variables

        _tf.reset_default_graph()

        self.inst_norm_net = _InstanceNorm(128, 4, 1)
        self.inst_norm_net.collect_params().initialize()

        inst_norm_weight_dict = dict()
        param_keys = list(self.inst_norm_net.collect_params())
        for key in param_keys:
            weight = self.inst_norm_net.collect_params()[key].data().asnumpy()
            if len(weight.shape) == 4:
                inst_norm_weight_dict[key] = weight.transpose(2, 3, 1, 0) 
            else:
                inst_norm_weight_dict[key] = weight

        self.tf_instance_norm_input = _tf.placeholder(dtype = _tf.float32, shape = [None, 4, 4, 128])
        self.tf_instance_norm_index = _tf.placeholder(dtype = _tf.int32, shape = [1])

        tf_variables = _define_tensorflow_variables(inst_norm_weight_dict, False, "instancenorm0_")
        self.tf_instance_norm_net = _define_instance_norm(self.tf_instance_norm_input, self.tf_instance_norm_index, tf_variables, "instancenorm0_")

        tensorflow_input = np.random.random_sample((1, 4, 4, 128))
        mx_input = tensorflow_input.transpose(0, 3, 1, 2)

        self.sess = _tf.compat.v1.Session()
        init = _tf.compat.v1.global_variables_initializer()
        self.sess.run(init)

        tf_instance_norm_out = self.sess.run(self.tf_instance_norm_net, feed_dict={self.tf_instance_norm_input: tensorflow_input, self.tf_instance_norm_index: _np.array([0])})
        mx_output = self.inst_norm_net(_mx.nd.array(mx_input), _mx.nd.array([0]))

        tf_mxnet_output = mx_output.asnumpy().transpose(0, 2, 3, 1)

        flattened_mxnet = tf_mxnet_output.flatten()
        flattened_tf = tf_instance_norm_out.flatten()

        diff = flattened_mxnet - flattened_tf
        abs_diff = np.abs(diff)
        max_diff = np.max(abs_diff)

        self.assertLessEqual(max_diff, _MARGIN_OF_ERROR)

    def test_residual_network(self):
        import tensorflow as _tf
        import mxnet as _mx
        import numpy as _np

        from ..toolkits.style_transfer._tf_model_architecture import define_residual as _define_residual
        from ..toolkits.style_transfer._model import ResidualBlock as _ResidualBlock

        from ..toolkits.style_transfer._tf_model_architecture import define_tensorflow_variables as _define_tensorflow_variables

        _tf.reset_default_graph()

        tensorflow_input = np.random.random_sample((1, 4, 4, 128))
        mx_input = tensorflow_input.transpose(0, 3, 1, 2)

        self.residual_net = _ResidualBlock(4, 1)
        self.residual_net.collect_params().initialize()

        mxnet_out_tf = self.residual_net(_mx.nd.array(mx_input), _mx.nd.array([0]))

        residual_weight_dict = dict()

        param_keys = list(self.residual_net.collect_params())
        for key in param_keys:
            weight = self.residual_net.collect_params()[key].data().asnumpy()
            if len(weight.shape) == 4:
                residual_weight_dict[key] = weight.transpose(2, 3, 1, 0) 
            else:
                residual_weight_dict[key] = weight

        self.tf_input = _tf.placeholder(dtype = _tf.float32, shape = [None, 4, 4, 128])
        self.tf_index = _tf.placeholder(dtype = _tf.int32, shape = [1])

        tf_variables = _define_tensorflow_variables(residual_weight_dict, False, "residualblock0_")
        self.tf_residual_net = _define_residual(self.tf_input, self.tf_index, tf_variables, "residualblock0_")

        self.sess = _tf.compat.v1.Session()
        init = _tf.compat.v1.global_variables_initializer()
        self.sess.run(init)

        tf_residual_out = self.sess.run(self.tf_residual_net, feed_dict={self.tf_input: tensorflow_input, self.tf_index: np.array([0])})

        tf_mxnet_output = mxnet_out_tf.asnumpy().transpose(0, 2, 3, 1)

        flattened_mxnet = tf_mxnet_output.flatten()
        flattened_tf = tf_residual_out.flatten()

        diff = flattened_mxnet - flattened_tf
        abs_diff = np.abs(diff)
        max_diff = np.max(abs_diff)

        self.assertLessEqual(max_diff, _MARGIN_OF_ERROR)

    def test_resnet_network(self):
        import tensorflow as _tf
        import mxnet as _mx
        import numpy as _np

        from ..toolkits.style_transfer._tf_model_architecture import define_resnet as _define_resnet
        from ..toolkits.style_transfer._model import Transformer as _Transformer

        from ..toolkits.style_transfer._tf_model_architecture import define_tensorflow_variables as _define_tensorflow_variables

        _tf.reset_default_graph()

        tensorflow_input = _np.random.random_sample((1, 256, 256, 3))
        mx_input = tensorflow_input.transpose(0, 3, 1, 2)

        self.transformer_net = _Transformer(4, 1)
        self.transformer_net.collect_params().initialize()

        mxnet_out_tf = self.transformer_net(_mx.nd.array(mx_input), _mx.nd.array([0]))

        transformer_weight_dict = dict()

        param_keys = list(self.transformer_net.collect_params())
        for key in param_keys:
            weight = self.transformer_net.collect_params()[key].data().asnumpy()
            if len(weight.shape) == 4:
                transformer_weight_dict[key] = weight.transpose(2, 3, 1, 0) 
            else:
                transformer_weight_dict[key] = weight

        self.tf_input = _tf.placeholder(dtype = _tf.float32, shape = [None, 256, 256, 3])
        self.tf_index = _tf.placeholder(dtype = _tf.int32, shape = [1])

        tf_variables = _define_tensorflow_variables(transformer_weight_dict, False, "transformer_")
        self.tf_transformer_net = _define_resnet(self.tf_input, self.tf_index, tf_variables, "transformer_")

        self.sess = _tf.compat.v1.Session()
        init = _tf.compat.v1.global_variables_initializer()
        self.sess.run(init)

        tf_transformer_out = self.sess.run(self.tf_transformer_net, feed_dict={self.tf_input: tensorflow_input, self.tf_index: np.array([0])})

        tf_mxnet_output = mxnet_out_tf.asnumpy().transpose(0, 2, 3, 1)

        flattened_mxnet = tf_mxnet_output.flatten()
        flattened_tf = tf_transformer_out.flatten()

        diff = flattened_mxnet - flattened_tf
        abs_diff = np.abs(diff)
        max_diff = np.max(abs_diff)

        self.assertLessEqual(max_diff, _MARGIN_OF_ERROR)

    def test_vgg16_network(self):
        import tensorflow as _tf
        import mxnet as _mx
        import numpy as _np

        from ..toolkits.style_transfer._tf_model_architecture import define_vgg16 as _define_vgg16
        from ..toolkits.style_transfer._model import Vgg16 as _Vgg16

        from ..toolkits.style_transfer._tf_model_architecture import define_tensorflow_variables as _define_tensorflow_variables

        _tf.reset_default_graph()

        tensorflow_input = _np.random.random_sample((1, 256, 256, 3))
        mx_input = tensorflow_input.transpose(0, 3, 1, 2)

        self.vgg_16 = _Vgg16()
        self.vgg_16.collect_params().initialize()

        mx_output = self.vgg_16(_mx.nd.array(mx_input))

        vgg16_weight_dict = dict()

        param_keys = list(self.vgg_16.collect_params())
        for key in param_keys:
            weight = self.vgg_16.collect_params()[key].data().asnumpy()
            if len(weight.shape) == 4:
                vgg16_weight_dict[key] = weight.transpose(2, 3, 1, 0) 
            else:
                vgg16_weight_dict[key] = weight

        self.tf_input = _tf.placeholder(dtype = _tf.float32, shape = [None, 256, 256, 3])

        tf_variables = _define_tensorflow_variables(vgg16_weight_dict, False, "vgg16_")
        self.vgg16_net = _define_vgg16(self.tf_input, tf_variables, "vgg16_")

        self.sess = _tf.compat.v1.Session()
        init = _tf.compat.v1.global_variables_initializer()
        self.sess.run(init)

        tf_vgg16_out = self.sess.run(self.vgg16_net, feed_dict={self.tf_input: tensorflow_input})

        for t, m in zip(tf_vgg16_out, mx_output):
            tf_mxnet_output = m.asnumpy().transpose(0, 2, 3, 1)
            
            flattened_tf = t.flatten()
            flattened_mxnet = tf_mxnet_output.flatten()

            diff = flattened_mxnet - flattened_tf
            abs_diff = np.abs(diff)
            max_diff = np.max(abs_diff)
            
            self.assertLessEqual(max_diff, _MARGIN_OF_ERROR)

    def test_gram_matrix_network(self):
        import tensorflow as _tf
        import mxnet as _mx
        import numpy as _np

        from ..toolkits.style_transfer._tf_model_architecture import define_gram_matrix as _define_gram_matrix
        from ..toolkits.style_transfer._model import gram_matrix as _gram_matrix

        from ..toolkits.style_transfer._tf_model_architecture import define_tensorflow_variables as _define_tensorflow_variables

        tensorflow_input = np.random.random_sample((1, 256, 256, 3))
        mx_input = tensorflow_input.transpose(0, 3, 1, 2)

        mx_output = _gram_matrix(_mx.nd.array(mx_input))

        tf_input = _tf.placeholder(dtype = _tf.float32, shape = [None, 256, 256, 3])
        tensorflow_output = _define_gram_matrix(tf_input)

        sess = _tf.compat.v1.Session()
        init = _tf.compat.v1.global_variables_initializer()
        sess.run(init)

        tf_gram_matrix_out = sess.run(tensorflow_output, feed_dict={tf_input: tensorflow_input})

        tf_mxnet_output = mx_output.asnumpy()

        flattened_mxnet = tf_mxnet_output.flatten()
        flattened_tf = tf_gram_matrix_out.flatten()

        diff = flattened_mxnet - flattened_tf
        abs_diff = np.abs(diff)
        max_diff = np.max(abs_diff)

        self.assertLessEqual(max_diff, _MARGIN_OF_ERROR)

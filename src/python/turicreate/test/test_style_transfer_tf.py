# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
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

from ..toolkits.style_transfer._tf_model_architecture import define_resnet as _define_resnet
from ..toolkits.style_transfer._tf_model_architecture import define_instance_norm as _define_instance_norm

from ..toolkits.style_transfer._model import InstanceNorm as _InstanceNorm
from ..toolkits.style_transfer._model import ResidualBlock as _ResidualBlock
from ..toolkits.style_transfer._model import Transformer as _Transformer

import tensorflow as _tf
import mxnet as _mx
import numpy as _np


# TODO: Refactor back into test_style_transfer

_NUM_STYLES = 4

class StyleTransferTFTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.tf_style_image = np.random.random_sample((1, 256, 256, 3))
        self.tf_content_image = np.ones((1, 256, 256, 3))

        self.mxnet_style_image = self.tf_style_image.transpose(0, 3, 1, 2)
        self.mxnet_content_image = self.tf_content_image.transpose(0, 3, 1, 2)

        # self.tf_transformer = define_resnet()

        # define instance norm model
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
        self.tf_instance_norm_index = _tf.placeholder(dtype = _tf.int32, shape = [None, 1])
        self.tf_instance_norm_net = _define_instance_norm(self.tf_instance_norm_input, self.tf_instance_norm_index, inst_norm_weight_dict, "instancenorm0_")



        '''
        # define transformer model
        self.mx_transformer = _Transformer(_NUM_STYLES, 1)
        self.mx_transformer.collect_params().initialize()

        self.transformer_weight_dict = dict()

        param_keys = list(self.mx_transformer.collect_params())
        for key in param_keys:
            weight = self.mx_transformer.collect_params()[key].data().asnumpy()
            if len(weight.shape) == 4:
                self.transformer_weight_dict[key] = weight.transpose(2, 3, 1, 0) 
            else:
                self.transformer_weight_dict[key] = weight

        # define inputs model
        tf_input = _tf.placeholder(dtype = _tf.float32, shape = [None, 256, 256, 3])
        tf_style = _tf.placeholder(dtype = _tf.float32, shape = [None, 256, 256, 3])
        tf_index = _tf.placeholder(dtype = _tf.int32, shape = [None, 1])

        # self.tf_transformer = define_resnet(tf_input, tf_index, self.transformer_weight_dict)

        _InstanceNorm
        '''

        self.sess = _tf.Session()
        self.sess.run(_tf.global_variables_initializer())

    def test_instance_norm_network(self):
        tensorflow_input = np.random.random_sample((1, 4, 4, 128))
        mx_input = tensorflow_input.transpose(0, 3, 1, 2)


        tf_instance_norm_out = self.sess.run(self.tf_instance_norm_net, feed_dict={self.tf_instance_norm_input: tensorflow_input, self.tf_instance_norm_index: _np.array([[0]])})
        mx_output = self.inst_norm_net(_mx.nd.array(mx_input), _mx.nd.array([0]))

        tf_mxnet_output = mx_output.asnumpy().transpose(0, 2, 3, 1)

        flattened_mxnet = tf_mxnet_output.flatten()
        flattened_tf = tf_instance_norm_out.flatten()

        diff = flattened_mxnet - flattened_tf
        abs_diff = np.abs(diff)
        max_diff = np.max(abs_diff)

        self.assertLessEqual(max_diff, 5e-3)

    '''
    def test_transformer_network(self):
        mx_transformer_out = self.mx_transformer(_mx.nd.array(self.mxnet_content_image), _mx.nd.array([0]))
        tf_transformer_out = self.sess.run(self.tf_transformer, feed_dict={tf_input: self.tf_content_image, tf_index: _np.array([[0]])})

        mx_transformer_out = mx_transformer_out.asnumpy().transpose(0, 2, 3, 1)

        flattened_mxnet = mx_transformer_out.flatten()
        flattened_tf = tf_transformer_out.flatten()

        diff = flattened_mxnet - flattened_tf
        abs_diff = _np.abs(diff)
        max_diff = _np.max(abs_diff)

        
    '''

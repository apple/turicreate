# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import os

from mxnet.context import cpu
from mxnet.gluon.block import HybridBlock
from mxnet.gluon import nn
from mxnet.gluon.contrib.nn import HybridConcurrent
from mxnet import base

import mxnet as mx
from mxnet.gluon import nn

from mxnet.gluon import HybridBlock
from mxnet import gluon
import mxnet.ndarray as nd

import random
import numpy as np
import json

from ast import literal_eval as make_tuple

import os
import errno

class InstanceNorm(HybridBlock):
    def __init__(self, in_channels, num_styles, batch_size, epsilon=1e-5,
                 center=True, scale=True, beta_initializer='zeros',
                 gamma_initializer='ones', **kwargs):
        super(InstanceNorm, self).__init__(**kwargs)
        self._kwargs = {'eps': epsilon}
        self.gamma = self.params.get('gamma', grad_req='write' if scale else 'null',
                                     shape=(num_styles, in_channels, ), init=gamma_initializer,
                                     allow_deferred_init=True)
        self.beta = self.params.get('beta', grad_req='write' if center else 'null',
                                    shape=(num_styles, in_channels, ), init=beta_initializer,
                                    allow_deferred_init=True)
        self.num_styles = num_styles
        self.in_channels = in_channels
        self.batch_size = batch_size

    def hybrid_forward(self, F, X, style_idx, gamma, beta):
        if F == mx.sym and self.batch_size == 0:  # for coreml
            gamma = mx.sym.Embedding(data=style_idx, input_dim=self.num_styles, output_dim=self.in_channels)
            beta = mx.sym.Embedding(data=style_idx, input_dim=self.num_styles, output_dim=self.in_channels)
            return F.InstanceNorm(X, gamma, beta, name='_fwd', **self._kwargs)

        em_gamma = F.take(gamma, indices=style_idx, axis=0)
        em_beta = F.take(beta, indices=style_idx, axis=0)

        sp_gammas = F.split(em_gamma, axis=0, num_outputs=self.batch_size, squeeze_axis=True)
        sp_betas = F.split(em_beta, axis=0, num_outputs=self.batch_size, squeeze_axis=True)

        if self.batch_size == 1:
            return F.InstanceNorm(X, sp_gammas, sp_betas, name='_fwd', **self._kwargs)
        else:
            Xs = F.split(X, axis=0, num_outputs=self.batch_size)

            res = []
            for idx in range(self.batch_size):
                gamma0 = sp_gammas[idx]
                beta0 = sp_betas[idx]
                X_slice = Xs[idx]
                res.append(F.InstanceNorm(X_slice, gamma0, beta0, name='_fwd', **self._kwargs))

            return F.concat(*res, dim=0)

class ResidualBlock(HybridBlock):
    def __init__(self, num_styles, batch_size):
        super(ResidualBlock, self).__init__()
        with self.name_scope():
            self.refl1 = nn.ReflectionPad2D(1)
            self.conv1 = nn.Conv2D(128, 3, 1, 0, in_channels=128, use_bias=False)
            self.inst_norm1 = InstanceNorm(in_channels=128, num_styles=num_styles, batch_size=batch_size)

            self.refl2 = nn.ReflectionPad2D(1)
            self.conv2 = nn.Conv2D(128, 3, 1, 0, in_channels=128, use_bias=False)
            self.inst_norm2 = InstanceNorm(in_channels=128, num_styles=num_styles, batch_size=batch_size)
        self._batch_size = batch_size

    @property
    def batch_size(self):
        return self._batch_size

    @batch_size.setter
    def batch_size(self, batch_size):
        self.inst_norm1.batch_size = batch_size
        self.inst_norm2.batch_size = batch_size
        self._batch_size = batch_size

    def hybrid_forward(self, F, x):
        h1_res_relf1 = self.refl1(x)
        h1_res_conv1 = self.conv1(h1_res_relf1)
        h1_res_inst1 = self.inst_norm1(h1_res_conv1, mx.nd.array([0]))
        h1_res_activ1 = F.Activation(h1_res_inst1, 'relu')

        h1_res_relf2 = self.refl1(h1_res_activ1)
        h2_res_conv2 = self.conv2(h1_res_relf2)
        h2_res_inst2 = self.inst_norm2(h2_res_conv2, mx.nd.array([0]))
        
        return x + h2_res_inst2

class Transformer(HybridBlock):
    def __init__(self, num_styles, batch_size):
        super(Transformer, self).__init__(prefix='transformer_')
        self.num_styles = num_styles
        block = ResidualBlock
        self.scale255 = False
        
        self.outputs = []

        with self.name_scope():
            self.refl1 = nn.ReflectionPad2D(4)
            self.conv1 = nn.Conv2D(32, 9, 1, 0, in_channels=3, use_bias=False)
            self.inst_norm1 = nn.BatchNorm(epsilon=0.001, in_channels=32)

            self.refl2 = nn.ReflectionPad2D(1)
            self.conv2 = nn.Conv2D(64, 3, 2, 0, in_channels=32, use_bias=False)
            self.inst_norm2 = nn.BatchNorm(epsilon=0.001, in_channels=64)

            self.refl3 = nn.ReflectionPad2D(1)
            self.conv3 = nn.Conv2D(128, 3, 2, 0, in_channels=64, use_bias=False)
            self.inst_norm3 = nn.BatchNorm(epsilon=0.001, in_channels=128)

            self.residual1 = block(num_styles, batch_size=batch_size)
            self.residual2 = block(num_styles, batch_size=batch_size)
            self.residual3 = block(num_styles, batch_size=batch_size)
            self.residual4 = block(num_styles, batch_size=batch_size)
            self.residual5 = block(num_styles, batch_size=batch_size)

            self.refl4 = nn.ReflectionPad2D(1)
            self.decoder_conv1 = nn.Conv2D(64, 3, 1, 0, in_channels=128, use_bias=False)
            self.inst_norm4 = InstanceNorm(in_channels=64, num_styles=num_styles, batch_size=batch_size)

            self.refl5 = nn.ReflectionPad2D(1)
            self.decoder_conv2 = nn.Conv2D(32, 3, 1, 0, in_channels=64, use_bias=False)
            self.inst_norm5 = InstanceNorm(in_channels=32, num_styles=num_styles, batch_size=batch_size)

            self.refl6 = nn.ReflectionPad2D(4)
            self.decoder_conv3 = nn.Conv2D(3, 9, 1, 0, in_channels=32, use_bias=False)
            self.inst_norm6 = InstanceNorm(in_channels=3, num_styles=num_styles, batch_size=batch_size)

    @property
    def batch_size(self):
        return self._batch_size

    @batch_size.setter
    def batch_size(self, batch_size):
        inst_norm_layers = [
            self.inst_norm1, self.inst_norm2, self.inst_norm3,
            self.inst_norm4, self.inst_norm5, self.inst_norm6,
            self.residual1, self.residual2, self.residual3,
            self.residual4, self.residual5,
        ]

        for layer in inst_norm_layers:
            layer.batch_size = batch_size

    def hybrid_forward(self, F, X):
        h1_relf_1 = self.refl1(X)
        h1_conv_1 = self.conv1(h1_relf_1)
        h1_inst_2 = self.inst_norm1(h1_conv_1)
        h1_activ = F.Activation(h1_inst_2, 'relu')

        h1_relf_2 = self.refl2(h1_activ)
        h2_conv_2 = self.conv2(h1_relf_2)
        h2_inst_2 = self.inst_norm2(h2_conv_2)
        h2_activ = F.Activation(h2_inst_2, 'relu')
        
        h1_relf_3 = self.refl3(h2_activ)
        h3_conv_3 = self.conv3(h1_relf_3)
        h3_inst_3 = self.inst_norm3(h3_conv_3)
        h3_activ = F.Activation(h3_inst_3, 'relu')

        r1 = self.residual1(h3_activ)
        r2 = self.residual2(r1)
        r3 = self.residual3(r2)
        r4 = self.residual4(r3)
        r5 = self.residual5(r4)

        d1 = F.UpSampling(r5, scale=2, sample_type='nearest')
        d1 = self.refl4(d1)
        d1 = self.decoder_conv1(d1)
        d1 = self.inst_norm4(d1, mx.nd.array([0]))
        d1 = F.Activation(d1, 'relu')

        d2 = F.UpSampling(d1, scale=2, sample_type='nearest')
        d2 = self.refl5(d2)
        d2 = self.decoder_conv2(d2)
        d2 = self.inst_norm5(d2, mx.nd.array([0]))
        d2 = F.Activation(d2, 'relu')

        d3 = self.refl5(d2)
        d3 = self.decoder_conv3(d2)
        d3 = self.inst_norm6(d3, mx.nd.array([0]))

        z = F.Activation(d3, 'sigmoid')

        if self.scale255:
            return z * 255
        else:
            return z

def transformer(pretrained=False, num_styles=1, ctx=cpu(), **kwargs):
    net = Transformer(num_styles, 1)
    if pretrained:
        current_path = os.path.dirname(os.path.realpath(__file__))
        # TODO: upload param files
        net.load_parameters(os.path.join(current_path, "../transformer.params"), ignore_extra=True, ctx=ctx)
    return net

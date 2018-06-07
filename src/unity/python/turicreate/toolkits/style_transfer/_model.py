# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import mxnet as _mx
from mxnet.gluon import nn as _nn
from mxnet.gluon import HybridBlock
import mxnet.ndarray as nd


class InstanceNorm(HybridBlock):
    """
    Conditional Instance Norm
    """
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
        if F == _mx.sym and self.batch_size == 0:  # for coreml
            gamma = _mx.sym.Embedding(data=style_idx, input_dim=self.num_styles, output_dim=self.in_channels)
            beta = _mx.sym.Embedding(data=style_idx, input_dim=self.num_styles, output_dim=self.in_channels)
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
    """
    Residual network
    """

    def __init__(self, num_styles, batch_size):
        super(ResidualBlock, self).__init__()

        with self.name_scope():
            self.conv1 = _nn.Conv2D(128, 3, 1, 1, in_channels=128, use_bias=False)
            self.inst_norm1 = InstanceNorm(in_channels=128, num_styles=num_styles, batch_size=batch_size)
            self.conv2 = _nn.Conv2D(128, 3, 1, 1, in_channels=128, use_bias=False)
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

    def hybrid_forward(self, F, x, style_idx):
        h1 = self.conv1(x)
        h1 = self.inst_norm1(h1, style_idx)
        h1 = F.Activation(h1, 'relu')

        h2 = self.conv2(h1)
        h2 = self.inst_norm2(h2, style_idx)

        return x + h2


def gram_matrix(y):
    (b, ch, h, w) = y.shape
    features = y.reshape((b, ch, w * h))
    gram = nd.batch_dot(features, features, transpose_b=True) / (h * w)
    return gram


class Transformer(HybridBlock):
    def __init__(self, num_styles, batch_size):
        super(Transformer, self).__init__(prefix='transformer_')
        self.num_styles = num_styles
        block = ResidualBlock
        self.scale255 = False

        with self.name_scope():
            self.conv1 = _nn.Conv2D(32, 9, 1, 4, in_channels=3, use_bias=False)
            self.inst_norm1 = InstanceNorm(in_channels=32, num_styles=num_styles, batch_size=batch_size)

            self.conv2 = _nn.Conv2D(64, 3, 2, 1, in_channels=32, use_bias=False)
            self.inst_norm2 = InstanceNorm(in_channels=64, num_styles=num_styles, batch_size=batch_size)

            self.conv3 = _nn.Conv2D(128, 3, 2, 1, in_channels=64, use_bias=False)
            self.inst_norm3 = InstanceNorm(in_channels=128, num_styles=num_styles, batch_size=batch_size)

            self.residual1 = block(num_styles, batch_size=batch_size)
            self.residual2 = block(num_styles, batch_size=batch_size)
            self.residual3 = block(num_styles, batch_size=batch_size)
            self.residual4 = block(num_styles, batch_size=batch_size)
            self.residual5 = block(num_styles, batch_size=batch_size)

            self.decoder_conv1 = _nn.Conv2D(64, 3, 1, 1, in_channels=128, use_bias=False)
            self.inst_norm4 = InstanceNorm(in_channels=64, num_styles=num_styles, batch_size=batch_size)

            self.decoder_conv2 = _nn.Conv2D(32, 3, 1, 1, in_channels=64, use_bias=False)
            self.inst_norm5 = InstanceNorm(in_channels=32, num_styles=num_styles, batch_size=batch_size)

            self.decoder_conv3 = _nn.Conv2D(3, 9, 1, 4, in_channels=32, use_bias=False)
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

    def hybrid_forward(self, F, X, style_idx):
        h1 = self.conv1(X)
        h1 = self.inst_norm1(h1, style_idx)
        h1 = F.Activation(h1, 'relu')

        h2 = self.conv2(h1)
        h2 = self.inst_norm2(h2, style_idx)
        h2 = F.Activation(h2, 'relu')

        h3 = self.conv3(h2)
        h3 = self.inst_norm3(h3, style_idx)
        h3 = F.Activation(h3, 'relu')

        r1 = self.residual1(h3, style_idx)
        r2 = self.residual2(r1, style_idx)
        r3 = self.residual3(r2, style_idx)
        r4 = self.residual4(r3, style_idx)
        r5 = self.residual5(r4, style_idx)

        d1 = F.UpSampling(r5, scale=2, sample_type='nearest')
        d1 = self.decoder_conv1(d1)
        d1 = self.inst_norm4(d1, style_idx)
        d1 = F.Activation(d1, 'relu')

        d2 = F.UpSampling(d1, scale=2, sample_type='nearest')
        d2 = self.decoder_conv2(d2)
        d2 = self.inst_norm5(d2, style_idx)
        d2 = F.Activation(d2, 'relu')

        d3 = self.decoder_conv3(d2)
        d3 = self.inst_norm6(d3, style_idx)

        z = F.Activation(d3, 'sigmoid')
        if self.scale255:
            return z * 255
        else:
            return z


class Vgg16(HybridBlock):
    def __init__(self):
        super(Vgg16, self).__init__(prefix='vgg16_')

        with self.name_scope():
            self.conv1_1 = _nn.Conv2D(in_channels=3, channels=64, kernel_size=3, padding=1)
            self.conv1_2 = _nn.Conv2D(in_channels=64, channels=64, kernel_size=3, padding=1)

            self.conv2_1 = _nn.Conv2D(in_channels=64, channels=128, kernel_size=3, padding=1)
            self.conv2_2 = _nn.Conv2D(in_channels=128, channels=128, kernel_size=3, padding=1)

            self.conv3_1 = _nn.Conv2D(in_channels=128, channels=256, kernel_size=3, padding=1)
            self.conv3_2 = _nn.Conv2D(in_channels=256, channels=256, kernel_size=3, padding=1)
            self.conv3_3 = _nn.Conv2D(in_channels=256, channels=256, kernel_size=3, padding=1)

            self.conv4_1 = _nn.Conv2D(in_channels=256, channels=512, kernel_size=3, padding=1)
            self.conv4_2 = _nn.Conv2D(in_channels=512, channels=512, kernel_size=3, padding=1)
            self.conv4_3 = _nn.Conv2D(in_channels=512, channels=512, kernel_size=3, padding=1)

    def hybrid_forward(self, F, X):
        h = F.Activation(self.conv1_1(X), act_type='relu')
        h = F.Activation(self.conv1_2(h), act_type='relu')
        relu1_2 = h
        h = F.Pooling(h, pool_type='avg', kernel=(2, 2), stride=(2, 2))

        h = F.Activation(self.conv2_1(h), act_type='relu')
        h = F.Activation(self.conv2_2(h), act_type='relu')
        relu2_2 = h
        h = F.Pooling(h, pool_type='avg', kernel=(2, 2), stride=(2, 2))

        h = F.Activation(self.conv3_1(h), act_type='relu')
        h = F.Activation(self.conv3_2(h), act_type='relu')
        h = F.Activation(self.conv3_3(h), act_type='relu')
        relu3_3 = h
        h = F.Pooling(h, pool_type='avg', kernel=(2, 2), stride=(2, 2))

        h = F.Activation(self.conv4_1(h), act_type='relu')
        h = F.Activation(self.conv4_2(h), act_type='relu')
        h = F.Activation(self.conv4_3(h), act_type='relu')
        relu4_3 = h

        return [relu1_2, relu2_2, relu3_3, relu4_3]


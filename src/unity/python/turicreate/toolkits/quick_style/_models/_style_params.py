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

class StyleParams(HybridBlock):
    def __init__(self, **kwargs):
        super(StyleParams, self).__init__(**kwargs)
        with self.name_scope():
            self.expand1_conv_1_gamma = nn.Conv2D(64, 1, 1, 0, in_channels=100, use_bias=True)
            self.expand1_conv_1_beta = nn.Conv2D(64, 1, 1, 0, in_channels=100, use_bias=True)
            self.expand2_conv_1_gamma = nn.Conv2D(32, 1, 1, 0, in_channels=100, use_bias=True)
            self.expand2_conv_1_beta = nn.Conv2D(32, 1, 1, 0, in_channels=100, use_bias=True)
            self.expand3_conv_1_gamma = nn.Conv2D(3, 1, 1, 0, in_channels=100, use_bias=True)
            self.expand3_conv_1_beta = nn.Conv2D(3, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual1_conv_1_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual1_conv_1_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual1_conv_2_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual1_conv_2_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual2_conv_1_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual2_conv_1_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual2_conv_2_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual2_conv_2_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual3_conv_1_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual3_conv_1_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual3_conv_2_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual3_conv_2_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual4_conv_1_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual4_conv_1_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual4_conv_2_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual4_conv_2_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual5_conv_1_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual5_conv_1_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual5_conv_2_gamma = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)
            self.residual5_conv_2_beta = nn.Conv2D(128, 1, 1, 0, in_channels=100, use_bias=True)

    def hybrid_forward(self, F, x):
        # Upsampling 1
        e1_c_1_g = F.reshape(self.expand1_conv_1_gamma(x), shape=(1, 64))
        e1_c_1_b = F.reshape(self.expand1_conv_1_beta(x), shape=(1, 64))

        # Upsampling 2
        e2_c_1_g = F.reshape(self.expand2_conv_1_gamma(x), shape=(1, 32))
        e2_c_1_b = F.reshape(self.expand2_conv_1_beta(x), shape=(1, 32))

        # Upsampling 3
        e3_c_1_g = F.reshape(self.expand3_conv_1_gamma(x), shape=(1, 3)) 
        e3_c_1_b = F.reshape(self.expand3_conv_1_beta(x), shape=(1, 3)) 

        # Residual 1
        r1_c_1_g = F.reshape(self.residual1_conv_1_gamma(x), shape=(1, 128))
        r1_c_1_b = F.reshape(self.residual1_conv_1_beta(x), shape=(1, 128)) 
        r1_c_2_g = F.reshape(self.residual1_conv_2_gamma(x), shape=(1, 128)) 
        r1_c_2_b = F.reshape(self.residual1_conv_2_beta(x), shape=(1, 128)) 

        # Residual 2
        r2_c_1_g = F.reshape(self.residual2_conv_1_gamma(x), shape=(1, 128))
        r2_c_1_b = F.reshape(self.residual2_conv_1_beta(x), shape=(1, 128))
        r2_c_2_g = F.reshape(self.residual2_conv_2_gamma(x), shape=(1, 128))
        r2_c_2_b = F.reshape(self.residual2_conv_2_beta(x), shape=(1, 128))

        # Residual 3
        r3_c_1_g = F.reshape(self.residual3_conv_1_gamma(x), shape=(1, 128))
        r3_c_1_b = F.reshape(self.residual3_conv_1_beta(x), shape=(1, 128))
        r3_c_2_g = F.reshape(self.residual3_conv_2_gamma(x), shape=(1, 128))
        r3_c_2_b = F.reshape(self.residual3_conv_2_beta(x), shape=(1, 128))

        # Residual 4
        r4_c_1_g = F.reshape(self.residual4_conv_1_gamma(x), shape=(1, 128)) 
        r4_c_1_b = F.reshape(self.residual4_conv_1_beta(x), shape=(1, 128)) 
        r4_c_2_g = F.reshape(self.residual4_conv_2_gamma(x), shape=(1, 128)) 
        r4_c_2_b = F.reshape(self.residual4_conv_2_beta(x), shape=(1, 128)) 

        # Residual 5
        r5_c_1_g = F.reshape(self.residual5_conv_1_gamma(x), shape=(1, 128)) 
        r5_c_1_b = F.reshape(self.residual5_conv_1_beta(x), shape=(1, 128)) 
        r5_c_2_g = F.reshape(self.residual5_conv_2_gamma(x), shape=(1, 128)) 
        r5_c_2_b = F.reshape(self.residual5_conv_2_beta(x), shape=(1, 128)) 

        return {
            "residual1_conv_1_beta": r1_c_1_g,
            "residual1_conv_1_gamma": r1_c_1_b,
            "residual1_conv_2_beta": r1_c_2_g,
            "residual1_conv_2_gamma": r1_c_2_b,
            "residual2_conv_1_beta": r2_c_1_g,
            "residual2_conv_1_gamma": r2_c_1_b,
            "residual2_conv_2_beta": r2_c_2_g,
            "residual2_conv_2_gamma": r2_c_2_b,
            "residual3_conv_1_beta": r3_c_1_g,
            "residual3_conv_1_gamma": r3_c_1_b,
            "residual3_conv_2_beta": r3_c_2_g,
            "residual3_conv_2_gamma": r3_c_2_b,
            "residual4_conv_1_beta": r4_c_1_g,
            "residual4_conv_1_gamma": r4_c_1_b,
            "residual4_conv_2_beta": r4_c_2_g,
            "residual4_conv_2_gamma": r4_c_2_b,
            "residual5_conv_1_beta": r5_c_1_g,
            "residual5_conv_1_gamma": r5_c_1_b,
            "residual5_conv_2_beta": r5_c_2_g,
            "residual5_conv_2_gamma": r5_c_2_b,
            "expand1_conv_1_beta": e1_c_1_g,
            "expand1_conv_1_gamma": e1_c_1_b,
            "expand2_conv_1_beta": e2_c_1_g,
            "expand2_conv_1_gamma": e2_c_1_b,
            "expand3_conv_1_beta": e3_c_1_g,
            "expand3_conv_1_gamma": e3_c_1_b
        }



def style_params(pretrained=False, ctx=cpu(), **kwargs):
    net = StyleParams(**kwargs)
    if pretrained:
        current_path = os.path.dirname(os.path.realpath(__file__))
        net.load_parameters(os.path.join(current_path, "../style_params.params"), ignore_extra=True, ctx=ctx)
    return net
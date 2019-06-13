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

class BottleNeck(HybridBlock):
    def __init__(self, **kwargs):
        super(BottleNeck, self).__init__(**kwargs)
        with self.name_scope():
            self.conv1 = nn.Conv2D(100, 1, 1, 0, in_channels=768, use_bias=True)
    
    def hybrid_forward(self, F, x):
        x = F.mean(x, axis=(2, 3))
        x = F.reshape(x, shape=(1, 768, 1, 1))
        x = self.conv1(x)

        return x
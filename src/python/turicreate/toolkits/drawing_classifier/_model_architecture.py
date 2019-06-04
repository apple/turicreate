# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from .._mxnet import mxnet as _mx
from mxnet.gluon import nn as _nn
from mxnet.gluon import HybridBlock as _HybridBlock

class Model(_HybridBlock):
    def __init__(self, num_classes, **kwargs):
        super(Model, self).__init__(**kwargs)
        with self.name_scope():
            # layers created in name_scope will inherit name space
            # from parent layer.
            self.conv1 = _nn.Conv2D(channels=16, kernel_size=(3,3), 
                                    padding=(1,1), activation='relu')
            self.pool1 = _nn.MaxPool2D(pool_size=(2,2))
            self.conv2 = _nn.Conv2D(channels=32, kernel_size=(3,3), 
                                    padding=(1,1), activation='relu')
            self.pool2 = _nn.MaxPool2D(pool_size=(2,2))
            self.conv3 = _nn.Conv2D(channels=64, kernel_size=(3,3), 
                                    padding=(1,1), activation='relu')
            self.pool3 = _nn.MaxPool2D(pool_size=(2,2))
            self.flatten = _nn.Flatten()
            self.fc1 = _nn.Dense(units=128, activation='relu')
            self.fc2 = _nn.Dense(units=num_classes, activation=None)

    def hybrid_forward(self, F, x):
        x = self.pool1(self.conv1(x))
        x = self.pool2(self.conv2(x))
        x = self.pool3(self.conv3(x))
        x = self.flatten(x)
        x = self.fc1(x)
        x = self.fc2(x)
        return F.softmax(x)

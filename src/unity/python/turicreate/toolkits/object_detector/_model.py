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


class _SpecialCrop(_mx.gluon.HybridBlock):
    def __init__(self, **kwargs):
        super(_SpecialCrop, self).__init__(**kwargs)

    def hybrid_forward(self, F, x):
        z = F.slice(x, begin=[None, None, 1, 1], end=[None, None, None, None])
        return z


# These ops are used in the Core ML exporter to support a max pool layer that
# requires two layers in mxnet. The ops don't do anything and merely signal
# to the mxnet converter that it should deal with separately.
@_mx.operator.register("special-darknet-maxpool")
class _SpecialDarknetMaxpool(_mx.operator.CustomOpProp):
    pass


class _SpecialDarknetMaxpoolBlock(_mx.gluon.HybridBlock):
    def __init__(self, name, **kwargs):
        super(_SpecialDarknetMaxpoolBlock, self).__init__(prefix=name, **kwargs)

    def hybrid_forward(self, F, x):
        return F.Custom(data=x, name='_fwd', op_type='special-darknet-maxpool')


def tiny_darknet(output_size=125, token_op=False):
    net = _nn.HybridSequential()
    for idx, f in enumerate([16, 32, 64, 128, 256, 512, 1024, 1024], 1):
        # Using NHWC would eliminate transposes before and after the base
        # model. However, this is currently not supported by Conv2D in mxnet,
        # so we have to use NCHW for now.
        layout = 'NCHW'
        c_axis = 3 if layout == 'NHWC' else 1

        net.add(_nn.Conv2D(channels=f,
                           kernel_size=(3, 3),
                           use_bias=False,
                           layout=layout,
                           prefix='conv%d_' % (idx - 1),
                           padding=(1, 1)))
        net.add(_nn.BatchNorm(axis=c_axis,
            prefix='batchnorm%d_' % (idx - 1)))
        net.add(_nn.LeakyReLU(0.1,
            prefix='leakyrelu%d_' % (idx - 1)))

        if idx < 6:
            strides = (2, 2)
            net.add(_nn.MaxPool2D(pool_size=(2, 2),
                                  strides=strides,
                                  layout=layout,
                                  prefix='pool%d_' % (idx - 1),
                                  ))

        elif idx == 6:
            strides = (1, 1)
            net.add(_nn.MaxPool2D(pool_size=(2, 2),
                                  strides=strides,
                                  layout=layout,
                                  padding=(1, 1),
                                  ceil_mode=False,
                                  prefix='pool%d_' % (idx - 1),
                                  ))
            net.add(_SpecialCrop(prefix='specialcrop%d' % (idx - 1)))

    if output_size is not None:
        net.add(_nn.Conv2D(channels=output_size,
                                    kernel_size=(1, 1),
                                    prefix='conv8_',
                                    layout=layout))

    return net

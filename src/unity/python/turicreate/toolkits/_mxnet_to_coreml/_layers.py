# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import absolute_import as _
from __future__ import print_function as _
from __future__ import division as _
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

from . import _add_pooling
from ast import literal_eval

def _get_attr(node):
    if 'attr' in node:
        return node['attr']
    elif 'attrs' in node:
        return node['attrs']

def _get_input_output_name(net, node, index=0):
    name = node['name']
    inputs = node['inputs']

    if index == 'all':
        input_name = [_get_node_name(net, inputs[idx][0]) for idx in range(len(inputs))]
    elif type(index) == int:
        input_name = _get_node_name(net, inputs[0][0])
    else:
        input_name = [_get_node_name(net, inputs[idx][0]) for idx in index]
    return input_name, name


def _get_node_name(net, node_id):
    return net['nodes'][node_id]['name']


def _get_node_shape(net, node_id):
    return net['nodes'][node_id]['shape']


def convert_reshape(net, node, module, builder):
    """Converts a reshape layer from mxnet to coreml.

    This doesn't currently handle the deprecated parameters for the reshape layer.

    Parameters
    ----------
    network: net
        An mxnet network object.

    layer: node
        Node to convert.

    module: module
        A module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = node['attr']
    target_shape = literal_eval(param['shape'])

    if any(item <= 0 for item in target_shape):
        raise NotImplementedError('Special dimensional values less than or equal to 0 are not supported yet.'
                                  'Feel free to file an issue here: https://github.com/dmlc/mxnet/issues.')

    if 'reverse' in node and node['reverse'] == 'True':
        raise NotImplementedError('"reverse" parameter is not supported by yet.'
                                  'Feel free to file an issue here: https://github.com/dmlc/mxnet/issues.')

    mode = 0 # CHANNEL_FIRST
    builder.add_reshape(name, input_name, output_name, target_shape, mode)


def convert_transpose(net, node, module, builder):
    """Convert a transpose layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)

    axes = literal_eval(param['axes'])
    builder.add_permute(name, axes, input_name, output_name)


def convert_flatten(net, node, module, builder):
    """Convert a flatten layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    mode = 0 # CHANNEL_FIRST
    builder.add_flatten(name, mode, input_name, output_name)


def convert_softmax(net, node, module, builder):
    """Convert a softmax layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    builder.add_softmax(name=name,
                        input_name=input_name,
                        output_name=output_name)


def convert_activation(net, node, module, builder):
    """Convert an activation layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    mx_non_linearity = _get_attr(node)['act_type']
    if mx_non_linearity == 'relu':
        non_linearity = 'RELU'
    elif mx_non_linearity == 'tanh':
        non_linearity = 'TANH'
    elif mx_non_linearity == 'sigmoid':
        non_linearity = 'SIGMOID'
    else:
        raise TypeError('Unknown activation type %s' % mx_non_linearity)
    builder.add_activation(name = name,
                           non_linearity = non_linearity,
                           input_name = input_name,
                           output_name = output_name)


def convert_leaky_relu(net, node, module, builder):
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)
    assert param['act_type'] == 'leaky'
    slope = literal_eval(param['slope'])
    builder.add_activation(name=name,
                           non_linearity='LEAKYRELU',
                           input_name=input_name,
                           output_name=output_name,
                           params=[slope])


def make_convert_elementwise(mode):
    def convert_elementwise(net, node, module, builder):
        """Convert an elementwise {} layer from mxnet to coreml.

        Parameters
        ----------
        network: net
            A mxnet network object.

        layer: node
            Node to convert.

        module: module
            An module for MXNet

        builder: NeuralNetworkBuilder
            A neural network builder object.
        """.format(mode)

        input_names, output_name = _get_input_output_name(net, node, [0, 1])
        name = node['name']

        builder.add_elementwise(name, input_names, output_name, mode)
    return convert_elementwise


convert_elementwise_add = make_convert_elementwise('ADD')
convert_elementwise_mul = make_convert_elementwise('MULTIPLY')


def convert_elementwise_mul_scalar(net, node, module, builder):
    """Convert a scalar multiplication from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    import numpy
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)

    mult = literal_eval(param['scalar'])
    builder.add_scale(name=name,
                      W=numpy.array([mult]),
                      b=0,
                      has_bias=False,
                      input_name=input_name,
                      output_name=output_name)


def convert_elementwise_div_scalar(net, node, module, builder):
    """Convert a scalar division from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    import numpy
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)

    denominator = literal_eval(param['scalar'])
    builder.add_scale(name=name,
                      W=numpy.array([1.0 / denominator]),
                      b=0,
                      has_bias=False,
                      input_name=input_name,
                      output_name=output_name)


def convert_dense(net, node, module, builder):
    """Convert a dense layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    has_bias = True
    name = node['name']

    inputs = node['inputs']
    args, _ = module.get_params()
    W = args[_get_node_name(net, inputs[1][0])].asnumpy()
    if has_bias:
        Wb = args[_get_node_name(net, inputs[2][0])].asnumpy()
    else:
        Wb = None
    nC, nB = W.shape

    builder.add_inner_product(
        name=name,
        W=W,
        b=Wb,
        input_channels=nB,
        output_channels=nC,
        has_bias=has_bias,
        input_name=input_name,
        output_name=output_name
    )


def convert_convolution(net, node, module, builder):
    """Convert a convolution layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)
    inputs = node['inputs']
    args, _ = module.get_params()

    if 'no_bias' in param.keys():
        has_bias = not literal_eval(param['no_bias'])
    else:
        has_bias = True

    kernel_height, kernel_width = literal_eval(param['kernel'])
    pad = literal_eval(param['pad'])
    same_pad = (pad[0] * 2 + 1 == kernel_height and
                pad[1] * 2 + 1 == kernel_width)

    if 'pad' in param.keys() and pad != (0, 0) and not same_pad:
        builder.add_padding(
            name=name+"_pad",
            left=pad[1],
            right=pad[1],
            top=pad[0],
            bottom=pad[0],
            value=0,
            input_name=input_name,
            output_name=name+"_pad_output")
        input_name = name+"_pad_output"

    if same_pad:
        border_mode = 'same'
        padding_top = padding_bottom = pad[0]
        padding_left = padding_right = pad[1]
    else:
        border_mode = 'valid'
        padding_top = padding_bottom = padding_left = padding_right = 0

    n_filters = int(param['num_filter'])

    W = args[_get_node_name(net, inputs[1][0])].asnumpy()
    if has_bias:
        Wb = args[_get_node_name(net, inputs[2][0])].asnumpy()
    else:
        Wb = None

    channels = W.shape[1]

    stride_height = 1
    stride_width = 1
    if 'stride' in param.keys():
        stride_height, stride_width = literal_eval(param['stride'])


    W = W.transpose((2, 3, 1, 0))
    builder.add_convolution(
        name=name,
        kernel_channels=channels,
        output_channels=n_filters,
        height=kernel_height,
        width=kernel_width,
        stride_height=stride_height,
        stride_width=stride_width,
        border_mode=border_mode,
        padding_left=padding_left,
        padding_right=padding_right,
        padding_top=padding_top,
        padding_bottom=padding_bottom,
        same_padding_asymmetry_mode='TOP_LEFT_HEAVY',
        groups=1,
        W=W,
        b=Wb,
        has_bias=has_bias,
        is_deconv=False,
        output_shape=None,
        input_name=input_name,
        output_name=output_name)


def convert_pooling(net, node, module, builder):
    """Convert a pooling layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)

    layer_type_mx = param['pool_type']
    if layer_type_mx == 'max':
        layer_type = 'MAX'
    elif layer_type_mx == 'avg':
        layer_type = 'AVERAGE'
    else:
        raise TypeError("Pooling type %s not supported" % layer_type_mx)

    # Add padding if there is any
    if 'pad' in param.keys() and literal_eval(param['pad']) != (0, 0):
        pad = literal_eval(param['pad'])
        builder.add_padding(
            name=name+"_pad",
            left=pad[1],
            right=pad[1],
            top=pad[0],
            bottom=pad[0],
            value=0,
            input_name=input_name,
            output_name=name+"_pad_output")
        input_name = name+"_pad_output"

    stride_height = 1
    stride_width = 1
    if 'stride' in param.keys():
        stride_height, stride_width = literal_eval(param['stride'])

    kernel_width, kernel_height = literal_eval(param['kernel'])

    type_map = {'valid': 'VALID', 'full': 'INCLUDE_LAST_PIXEL'}
    padding_type = param['pooling_convention'] if 'pooling_convention' in param else 'valid'
    if padding_type not in type_map:
        raise KeyError("%s type is not supported in this converter. It is a Github issue.")
    padding_type = type_map[padding_type]

    if 'global_pool' in param.keys():
        is_global = literal_eval(param['global_pool'])
    else:
        is_global = False

    # For reasons why we are not using the standard builder but having our own implementation,
    # see the function documentation.
    _add_pooling.add_pooling_with_padding_types(
        builder=builder,
        name=name,
        height=kernel_height,
        width=kernel_width,
        stride_height=stride_height,
        stride_width=stride_width,
        layer_type=layer_type,
        padding_type=padding_type,
        exclude_pad_area=False,
        is_global=is_global,
        input_name=input_name,
        output_name=output_name
    )


def convert_batchnorm(net, node, module, builder):
    """Convert a transpose layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    inputs = node['inputs']


    eps = 1e-3 # Default value of eps for MXNet.
    use_global_stats = False # Default value of use_global_stats for MXNet.
    args, aux = module.get_params()
    gamma = args[_get_node_name(net, inputs[1][0])].asnumpy()
    beta = args[_get_node_name(net, inputs[2][0])].asnumpy()
    mean = aux[_get_node_name(net, inputs[3][0])].asnumpy()
    variance = aux[_get_node_name(net, inputs[4][0])].asnumpy()
    nb_channels = gamma.shape[0]
    param = _get_attr(node)
    if param is not None:
        if 'eps' in param:
            eps = literal_eval(param['eps'])
        if 'fix_gamma' in param:
            if literal_eval(param['fix_gamma']):
                gamma[:] = 1.0

    builder.add_batchnorm(
        name=name,
        channels=nb_channels,
        gamma=gamma,
        beta=beta,
        mean=mean,
        variance=variance,
        input_name=input_name,
        output_name=output_name,
        epsilon=eps)


def convert_concat(net, node, module, builder):
    """Convert concat layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_names, output_name = _get_input_output_name(net, node, 'all')
    name = node['name']
    mode = 'CONCAT'
    builder.add_elementwise(name = name, input_names = input_names,
            output_name = output_name, mode = mode)


def convert_deconvolution(net, node, module, builder):
    """Convert a deconvolution layer from mxnet to coreml.

    Parameters
    ----------
    network: net
        A mxnet network object.

    layer: node
        Node to convert.

    module: module
        An module for MXNet

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)
    inputs = node['inputs']
    args, _ = module.get_params()

    if 'no_bias' in param.keys():
        has_bias = not literal_eval(param['no_bias'])
    else:
        has_bias = False

    border_mode = "valid"

    n_filters = int(param['num_filter'])

    output_shape = None
    if 'target_shape' in param:
        target_shape = literal_eval(param['target_shape'])
        output_shape = (int(target_shape[0]), int(target_shape[1]))

    W = args[_get_node_name(net, inputs[1][0])].asnumpy()

    if has_bias:
        Wb = args[_get_node_name(net, inputs[2][0])].asnumpy()
    else:
        Wb = None

    channels = W.shape[0]
    stride_height, stride_width = literal_eval(param['stride'])
    kernel_height, kernel_width = literal_eval(param['kernel'])
    W = W.transpose((2, 3, 0, 1))

    use_crop = False
    if literal_eval(param['pad']) != (0, 0) and output_shape is None:
        use_crop = True

    builder.add_convolution(
        name=name,
        kernel_channels=channels,
        output_channels=n_filters,
        height=kernel_height,
        width=kernel_width,
        stride_height=stride_height,
        stride_width=stride_width,
        border_mode=border_mode,
        groups=1,
        W=W,
        b=Wb,
        has_bias=has_bias,
        is_deconv=True,
        output_shape=output_shape,
        input_name=input_name,
        output_name=output_name+'before_pad' if use_crop else output_name
    )

    if use_crop:
        pad = literal_eval(param['pad'])
        builder.add_crop(
            name=name+"_pad",
            left=pad[1],
            right=pad[1],
            top=pad[0],
            bottom=pad[0],
            offset=0,
            input_names=[output_name+'before_pad'],
            output_name=output_name
        )


def convert_slice_axis(net, node, module, builder):
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)
    inputs = node['inputs']
    args, _ = module.get_params()

    axis = literal_eval(param['axis'])
    begin = literal_eval(param['begin'])
    end = literal_eval(param['end'])

    AXES = ['channel', 'height', 'width']
    if 'shape' not in node or len(node['shape']) == 4:
        assert axis != 0, "Cannot slice the batch axis"
        axis_name = AXES[axis - 1]
    elif len(node['shape']) == 3:
        axis_name = AXES[axis]

    builder.add_slice(name=name,
                      axis=axis_name,
                      start_index=begin,
                      end_index=end,
                      stride=1,
                      input_name=input_name,
                      output_name=output_name)


def convert_softmax(net, node, module, builder):
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)

    if 'axis' in param:
        axis = literal_eval(param['axis'])
        assert axis == 1, "Only softmax with axis 1 is supported"

    builder.add_softmax(name=name,
                        input_name=input_name,
                        output_name=output_name)


def convert_sigmoid(net, node, module, builder):
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']

    builder.add_activation(name=name,
                           non_linearity='SIGMOID',
                           input_name=input_name,
                           output_name=output_name)


def convert_custom(net, node, module, builder):
    """Convert highly specific ops"""
    input_name, output_name = _get_input_output_name(net, node)
    name = node['name']
    param = _get_attr(node)
    if param['op_type'] == 'special-darknet-maxpool':
        _add_pooling.add_pooling_with_padding_types(
            builder=builder,
            name=name,
            height=2,
            width=2,
            stride_height=1,
            stride_width=1,
            layer_type='MAX',
            padding_type='SAME',
            is_global=False,
            same_padding_asymmetry_mode='BOTTOM_RIGHT_HEAVY',
            input_name=input_name,
            output_name=output_name
        )
    else:
        raise TypeError("MXNet layer of type Custom is not supported.")

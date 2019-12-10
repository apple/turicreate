# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Utilities to compress Neural Network Models.
Only available in coremltools 2.0b1 and onwards
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as _np
import sys, os
from .optimization_utils import _optimize_nn

from coremltools.models import (
    _SUPPORTED_QUANTIZATION_MODES,
    _QUANTIZATION_MODE_DEQUANTIZE,
    _QUANTIZATION_MODE_LOOKUP_TABLE_LINEAR,
    _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS,
    _QUANTIZATION_MODE_CUSTOM_LOOKUP_TABLE,
    _QUANTIZATION_MODE_LINEAR_QUANTIZATION,
    _QUANTIZATION_MODE_LINEAR_SYMMETRIC,
    _LUT_BASED_QUANTIZATION
)

from ..utils import _get_nn_layers, _wp_to_fp16wp, _get_model, macos_version
from ..._deps import HAS_SKLEARN as _HAS_SKLEARN
from ... import (_MINIMUM_QUANTIZED_MODEL_SPEC_VERSION,
                 _MINIMUM_FP16_SPEC_VERSION)


class QuantizedLayerSelector(object):
    """ This is the base class to implement custom selectors to skip certain
    layers during quantization. To implement a custom selector, create a class
    that inherits this class and override `do_quantize()` method.

    Examples
    --------
    .. highlight:: python
    .. code-block:: python

        class MyLayerSelector(QuantizedLayerSelector):
            def __init__(self):
                super(MyLayerSelector, self).__init__()

            def do_quantize(self, layer, **kwargs):
                ret = super(MyLayerSelector, self).do_quantize(layer)
                if not ret or layer.name == 'dense_2':
                    return False
                return True

        selector = MyLayerSelector()
        quantized_model = quantize_weights(mlmodel, 8, quantization_mode='linear', selector=selector)

    """
    def __init__(self):
        self.quantizable_layer_types = {
            'convolution', 'innerProduct', 'embedding',
            'batchnorm', 'scale', 'bias', 'loadConstant',
            'simpleRecurrent', 'gru', 'uniDirectionalLSTM',
            'biDirectionalLSTM', 'batchedMatmul', 'depthwiseConv',
            'loop', 'branch'
        }

    def do_quantize(self, layer, **kwargs):
        return layer.WhichOneof('layer') in self.quantizable_layer_types


class AdvancedQuantizedLayerSelector(QuantizedLayerSelector):
    """ Quantized layer selector allowing the user to specify some types of
    layers to skip during quantization process and the minimum size parameters
    in quantized convolution layers.

    Examples
    --------
    .. highlight:: python
    .. code-block:: python

        from coremltools.models.neural_network.quantization_utils import AdvancedQuantizedLayerSelector
        selector = AdvancedQuantizedLayerSelector(
                skip_layer_types=['batchnorm', 'bias', 'depthwiseConv'],
                minimum_conv_kernel_channels=4,
                minimum_conv_weight_count=4096)
        quantized_model = quantize_weights(model, 8, selector=selector)

    """
    def __init__(self,
                 skip_layer_types=[],
                 minimum_conv_kernel_channels=4,
                 minimum_conv_weight_count=4096):

        super(AdvancedQuantizedLayerSelector, self).__init__()
        self.skip_layer_types = skip_layer_types

        # Error checking
        invalid_skip_types = []
        for lt in skip_layer_types:
            if lt not in self.quantizable_layer_types:
                invalid_skip_types.append(lt)
        if len(invalid_skip_types) > 0:
            err_msg = 'Skip quantization layer types ({}) is not supported.\n'.format(','.join(invalid_skip_types))
            err_msg += 'Supported quantization layers: ({})'.format(','.join(self.quantizable_layer_types))
            raise ValueError(err_msg)

        self.minimum_conv_kernel_channels = minimum_conv_kernel_channels
        self.minimum_conv_weight_count = minimum_conv_weight_count

    def do_quantize(self, layer, weight_param=None):
        """ weight_param - should be name of the WeightParam field
        """
        ret = super(AdvancedQuantizedLayerSelector, self).do_quantize(layer)
        if not ret:
            return False

        layer_type = layer.WhichOneof('layer')
        if layer_type in self.skip_layer_types:
            return False

        if layer_type == 'convolution':
            oc = layer.convolution.outputChannels
            kc = layer.convolution.kernelChannels
            kh = layer.convolution.kernelSize[0]
            kw = layer.convolution.kernelSize[1]
            groups = layer.convolution.nGroups
            counts = oc * kc * kh * kw
            has_bias = layer.convolution.hasBias

            if weight_param is None or weight_param == 'weights':
                if 'depthwiseConv' in self.skip_layer_types and kc == 1 and \
                    groups > 1:
                    return False

                if kc < self.minimum_conv_kernel_channels or \
                    counts < self.minimum_conv_weight_count:
                    return False

            elif weight_param == 'bias':
                return not 'bias' in self.skip_layer_types
            else:
                raise ValueError('Unrecognized quantization weight field {}'.format(weight_param))

        elif layer_type == 'innerProduct' or 'batchedMatmul':
            if weight_param is None or weight_param == 'weights':
                return True
            if weight_param == 'bias':
                return not 'bias' in self.skip_layer_types
            else:
                raise ValueError('Unrecognized quantization weight field {}'.format(weight_param))

        return True


def _convert_1bit_array_to_byte_array(arr):
    """
    Convert bit array to byte array.

    arr: list
        Bits as a list where each element is an integer of 0 or 1

    Returns
    -------
    numpy.array
        1D numpy array of type uint8
    """
    # Padding if necessary
    while len(arr) < 8 or len(arr) % 8:
        arr.append(0)

    arr = _np.array(arr, dtype='uint8')
    bit_arr = []
    idx = 0
    # Iterate and combine 8-bits into a uint8
    for arr_idx in range(int(len(arr) / 8)):
        bit_arr.append(((arr[idx] << 7) & (1 << 7)) |
                        ((arr[idx+1] << 6) & (1 << 6)) |
                        ((arr[idx+2] << 5) & (1 << 5)) |
                        ((arr[idx+3] << 4) & (1 << 4)) |
                        ((arr[idx+4] << 3) & (1 << 3)) |
                        ((arr[idx+5] << 2) & (1 << 2)) |
                        ((arr[idx+6] << 1) & (1 << 1)) |
                        ((arr[idx+7] << 0) & (1 << 0))
                        )
        idx += 8
    return _np.array(bit_arr, dtype='uint8')


def _convert_array_to_nbit_quantized_bytes(arr, nbits):
    split_arr = []
    for idx in range(len(arr)):
        for i in reversed(range(nbits)):
            split_arr.append((arr[idx] >> i) & (1 << 0))

    return _convert_1bit_array_to_byte_array(split_arr)


def _decompose_bytes_to_bit_arr(arr):
    """
    Unpack bytes to bits

    arr: list
        Byte Stream, as a list of uint8 values

    Returns
    -------
    bit_arr: list
        Decomposed bit stream as a list of 0/1s of length (len(arr) * 8)
    """
    bit_arr = []
    for idx in range(len(arr)):
        for i in reversed(range(8)):
            bit_arr.append((arr[idx] >> i) & (1 << 0))
    return bit_arr


def _get_linear_lookup_table_and_weight(nbits, wp):
    """
    Generate a linear lookup table.

    nbits: int
        Number of bits to represent a quantized weight value

    wp: numpy.array
        Weight blob to be quantized

    Returns
    -------
    lookup_table: numpy.array
        Lookup table of shape (2^nbits, )
    qw: numpy.array
        Decomposed bit stream as a list of 0/1s of length (len(arr) * 8)
    """
    w = wp.reshape(1, -1)
    qw, scales, biases = _quantize_channelwise_linear(w, nbits, axis=0)
    indices = _np.array(range(0, 2**nbits))
    lookup_table = indices * scales[0] + biases[0]
    return lookup_table, qw


def _get_kmeans_lookup_table_and_weight(nbits, w, init='k-means++', tol=1e-2, n_init=1, rand_seed=0):
    """
    Generate K-Means lookup table given a weight parameter field

    nbits:
        Number of bits for quantization

    w:
        Weight as numpy array

    Returns
    -------
    lut: numpy.array
        Lookup table, numpy array of shape (1 << nbits, );
    wq: numpy.array
        Quantized weight of type numpy.uint8
    """
    if _HAS_SKLEARN:
        from sklearn.cluster import KMeans
    else:
        raise Exception('sklearn package required for k-means quantization')
    units = _np.prod(w.shape)
    lut_len = 1 << nbits
    n_clusters = units if (units < lut_len) else lut_len
    wf = w.reshape(-1, 1)
    kmeans = KMeans(n_clusters=n_clusters, init=init, tol=tol, n_init=n_init, random_state=rand_seed).fit(wf)
    wq = kmeans.labels_[:units]
    lut = _np.zeros(lut_len)
    lut[:n_clusters] = kmeans.cluster_centers_.flatten()
    return lut, wq

def _quantize_channelwise_linear(weight, nbits, axis=0, symmetric=False):
    """
    Linearly quantize weight blob.

    weight: numpy.array
        Weight to be quantized.

    nbits: int
        Number of bits per weight element

    axis: int
        Axis of the weight blob to compute channel-wise quantization, can be 0 or 1

    symmetric: bool
        If true, set quantization range to be symmetrical to 0.
        Otherwise, set quantization range to be the minimum and maximum of
        weight parameters.

    Returns
    -------
    quantized_weight: numpy.array
        quantized weight as float numpy array, with the same shape as weight
    scale: numpy.array
        per channel scale
    bias: numpy.array
        per channel bias
    """
    if len(weight.shape) == 1: # vector situation, treat as 1 channel
        weight = weight.reshape((1, weight.shape[0]))

    rank = len(weight.shape)
    if axis == 1:
        transposed_axis_order = (1,0) + tuple(range(2,rank))
        weight = _np.transpose(weight, transposed_axis_order)

    num_channels = weight.shape[0]
    shape = weight.shape
    weight = weight.reshape((num_channels, -1)) # [C, L]

    a = _np.amin(weight, axis=-1) # [C,]
    b = _np.amax(weight, axis=-1) # [C,]

    if symmetric:
        r = _np.maximum(_np.abs(a), _np.abs(b))
        scale = r / ((1 << nbits) / 2.0 - 1)
        bias = -(1 << nbits) / 2.0 * scale
        num = (weight - bias[:,None])
        denom = scale[:,None]
        qw = _np.divide(num, denom, out=_np.zeros_like(num),
                where=(_np.abs(denom) > 1e-6))
        qw = _np.round(qw)
    else:
        qb = (1 << nbits) - 1
        scale = (b - a) / qb
        inv_scale = _np.divide(1.0, scale, out=_np.zeros_like(scale),
                where=(_np.abs(scale) > 1e-6))
        bias = a
        qw = (weight - a[:,None]) * inv_scale[:,None]
        qw = _np.round(qw)

    # Reshape
    quantized_weight = qw.reshape(shape)
    if axis == 1:
        quantized_weight = _np.transpose(quantized_weight, transposed_axis_order)

    return (quantized_weight, scale, bias)


def _quantize_wp(wp, nbits, qm, axis=0, **kwargs):
    """
    Quantize the weight blob

    wp: numpy.array
        Weight parameters
    nbits: int
        Number of bits
    qm:
        Quantization mode
    lut_function: (``callable function``)
        Python callable representing a look-up table

    Returns
    -------
    scale: numpy.array
        Per-channel scale
    bias: numpy.array
        Per-channel bias
    lut: numpy.array
        Lookup table
    quantized_wp: numpy.array
        Quantized weight of same shape as wp, with dtype numpy.uint8
    """

    scale = bias = lut = None

    # Linear Quantization
    if qm in [_QUANTIZATION_MODE_LINEAR_QUANTIZATION,
        _QUANTIZATION_MODE_LINEAR_SYMMETRIC]:
        symmetric = (qm == _QUANTIZATION_MODE_LINEAR_SYMMETRIC)
        qw, scale, bias = _quantize_channelwise_linear(wp, nbits, axis,
            symmetric)
    # Lookup tables
    elif qm == _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS:
        lut, qw = _get_kmeans_lookup_table_and_weight(nbits, wp)
    elif qm == _QUANTIZATION_MODE_CUSTOM_LOOKUP_TABLE:
        if 'lut_function' not in kwargs.keys():
            raise Exception('Custom lookup table quantization mode '
                            'selected but no lookup table function passed')
        lut_function = kwargs['lut_function']
        if not callable(lut_function):
            raise Exception('Argument for Lookup Table passed in but is '
                            'not callable')
        try:
            lut, qw = lut_function(nbits, wp)
        except Exception as e:
            raise Exception('{}\nCall to Lookup Table function failed'
                            .format(e.message))
    elif qm == _QUANTIZATION_MODE_LOOKUP_TABLE_LINEAR:
        lut, qw = _get_linear_lookup_table_and_weight(nbits, wp)
    else:
        raise NotImplementedError(
            'Quantization method "{}" not supported'.format(qm))

    quantized_wp = _np.uint8(qw)
    return scale, bias, lut, quantized_wp


def _quantize_wp_field(wp, nbits, qm, shape, axis=0, **kwargs):

    """
    Quantize WeightParam field in Neural Network Protobuf

    wp: MLModel.NeuralNetwork.WeightParam
        WeightParam field
    nbits: int
        Number of bits to be quantized
    qm: str
        Quantization mode
    shape: tuple
        Tensor shape held by wp
    axis: int
        Axis over which quantization is performed on, can be either 0 or 1
    lut_function: (``callable function``)
        Python callable representing a LUT table function
    """

    # De-quantization
    if qm == _QUANTIZATION_MODE_DEQUANTIZE:
        return _dequantize_wp(wp, shape, axis)

    # If the float32 field is empty do nothing and return
    if len(wp.floatValue) == 0:
        return

    # Half precision (16-bit) quantization
    if nbits == 16:
        return _wp_to_fp16wp(wp)

    if nbits > 8:
        raise Exception('Only 8-bit and lower quantization is supported')

    if qm not in _SUPPORTED_QUANTIZATION_MODES:
        raise Exception('Quantization mode {} not supported'.format(qm))

    # axis parameter check
    if axis == 1 and len(shape) != 4:
        raise Exception('Quantization on second axis is only supported '
                        'for rank-4 weight blob.')
    if axis != 0 and axis != 1:
        raise Exception('Invalid quantization axis {} passed in. Allowed'
                        'values are 0 (first axis) and 1 (second axis)'.format(axis))

    # WeightParam size check - non-linear quantizations are applied on layer level
    num_channels = shape[axis] if qm in [_QUANTIZATION_MODE_LINEAR_QUANTIZATION,
        _QUANTIZATION_MODE_LINEAR_SYMMETRIC] else 1
    if len(wp.floatValue) % num_channels:
        raise Exception('Number of quantization channels does not divide evenly into weights')

    qparams = wp.quantization
    qparams.numberOfBits = nbits

    weights = _np.array(wp.floatValue).reshape(shape)
    scale, bias, lut, uint8_weights = _quantize_wp(weights, nbits, qm, axis, **kwargs)
    uint8_weights = uint8_weights.flatten()
    if qm in [_QUANTIZATION_MODE_LINEAR_QUANTIZATION, 
              _QUANTIZATION_MODE_LINEAR_SYMMETRIC]:
        qparams.linearQuantization.scale.extend(scale)
        qparams.linearQuantization.bias.extend(bias)
    else:
        qparams.lookupTableQuantization.floatValue.extend(lut)

    wp.rawValue = bytes()
    if nbits == 8:
        wp.rawValue += uint8_weights.tobytes()
    else:
        wp.rawValue += _convert_array_to_nbit_quantized_bytes(uint8_weights,
            nbits).tobytes()
    del wp.floatValue[:]


def unpack_to_bytes(byte_arr, num_weights, nbits):
    assert num_weights % 1 == 0
    num_weights = int(num_weights)
    bit_arr = _decompose_bytes_to_bit_arr(byte_arr.flatten().tolist())
    bit_arr = _np.array(bit_arr[:num_weights * nbits]).reshape((num_weights, nbits))
    expo = 2**_np.array(list(reversed(range(0,nbits))))
    byte_arr = _np.sum(bit_arr * expo, axis=1)
    return byte_arr


def _dequantize_linear(weight_8bit, scale, bias, axis=0):

    if len(weight_8bit.shape) == 1: # vector situation, treat as 1 channel
        weight_8bit = weight_8bit.reshape((1, weight_8bit.shape[0]))

    rank = len(weight_8bit.shape)
    if axis == 1:
        transposed_axis_order = (1,0) + tuple(range(2,rank))
        weight_8bit = _np.transpose(weight_8bit, transposed_axis_order)

    num_channels = weight_8bit.shape[0]
    broadcast_shape = (num_channels, ) + (1,) * (rank - 1)
    scale = scale.reshape(broadcast_shape)
    bias = bias.reshape(broadcast_shape)
    weight = weight_8bit.astype('float') * scale + bias
    if axis == 1:
        weight = _np.transpose(weight, transposed_axis_order)

    return weight


def _dequantize_lut(weight_8bit, lut):
    return lut[weight_8bit.astype('uint8')]

def _dequantize_wp(wp, shape, axis=0):
    if len(wp.floatValue) != 0:
        return

    is_linear = wp.quantization.WhichOneof('QuantizationType') == 'linearQuantization'
    if is_linear:
        if len(wp.quantization.linearQuantization.scale) != \
                len(wp.quantization.linearQuantization.bias):
            raise Exception('Linear quantization scale and bias vectors are '
                            'different lengths')

    # axis parameter check
    if axis == 1 and len(shape) != 4:
        raise Exception('Dequantization on second axis is only supported '
                        'for rank-4 weight blob.')
    if axis != 0 and axis != 1:
        raise Exception('Invalid quantization axis {} passed in. Allowed'
                        'values are 0 (first axis) and 1 (second axis)'.format(axis))

    nbits = wp.quantization.numberOfBits
    num_weights = _np.prod(shape)
    byte_arr = _np.frombuffer(wp.rawValue, dtype=_np.uint8)

    weight_8bit = byte_arr if nbits == 8 else unpack_to_bytes(byte_arr, num_weights, nbits)
    weight_8bit = weight_8bit.reshape(shape)

    if is_linear:
        scale = _np.array(wp.quantization.linearQuantization.scale)
        bias = _np.array(wp.quantization.linearQuantization.bias)
        dequantized_weight = _dequantize_linear(weight_8bit, scale, bias, axis)
    else:
        lut = _np.array(wp.quantization.lookupTableQuantization.floatValue)
        dequantized_weight = _dequantize_lut(weight_8bit, lut)

    wp.rawValue = bytes()
    wp.quantization.Clear()
    wp.floatValue.extend(dequantized_weight.flatten())


def _dequantize_nn_spec(spec):
    """ Dequantize weights in NeuralNetwork type mlmodel specifications.
    """
    _quantize_nn_spec(spec, None, _QUANTIZATION_MODE_DEQUANTIZE)


def _quantize_nn_spec(nn_spec, nbits, qm, **kwargs):
    """ Quantize weights in NeuralNetwork type mlmodel specifications.
    """
    selector = kwargs.get('selector', QuantizedLayerSelector())

    if qm not in _SUPPORTED_QUANTIZATION_MODES:
        raise Exception('Quantization mode {} not supported'.format(qm))

    if qm != _QUANTIZATION_MODE_DEQUANTIZE:
        if nbits is None:
            raise Exception('Missing argument "nbits"')
        if not (nbits > 0 and nbits <= 8 or nbits == 16):
            raise Exception('Only half precision (16-bit), 1 to 8-bit '
                    'quantization is supported')

    if qm == _QUANTIZATION_MODE_LINEAR_SYMMETRIC and nbits != 8:
        raise Exception('Symmetric quantization is only applicable for 8 bit'
                        'linear')

    layers = nn_spec.layers

    # Perform optimization step
    if nbits is not None and nbits < 16 and qm != _QUANTIZATION_MODE_DEQUANTIZE:
        print('Optimizing Neural Network before Quantization:')
        _optimize_nn(layers)
        print('Finished optimizing network. Quantizing neural network..')

    # Quantize each layer
    for layer in layers:
        layer_type = layer.WhichOneof('layer')
        if not selector.do_quantize(layer):
            continue
        print('Quantizing layer {}'.format(layer.name))

        # Convolution
        if layer_type == 'convolution':
            output_channels = layer.convolution.outputChannels
            kernel_channels = layer.convolution.kernelChannels
            kernel_height = layer.convolution.kernelSize[0]
            kernel_width = layer.convolution.kernelSize[1]
            groups = layer.convolution.nGroups
            counts = output_channels * kernel_channels * kernel_height * kernel_width
            has_bias = layer.convolution.hasBias
            if layer.convolution.isDeconvolution:
                shape = (kernel_channels, int(output_channels/groups), kernel_height, kernel_width)
                _quantize_wp_field(layer.convolution.weights, nbits, qm, shape, axis=1, **kwargs)
            else:
                shape = (output_channels, kernel_channels, kernel_height, kernel_width)
                _quantize_wp_field(layer.convolution.weights, nbits, qm, shape, **kwargs)

            if has_bias and selector.do_quantize(layer, weight_param='bias'):
                _quantize_wp_field(layer.convolution.bias, nbits, qm,
                        shape=(output_channels,), **kwargs)

        # Batchnorm
        elif layer_type == 'batchnorm':
            nw = layer.batchnorm.channels
            _quantize_wp_field(layer.batchnorm.gamma, nbits, qm, shape=(nw,), **kwargs)
            _quantize_wp_field(layer.batchnorm.beta, nbits, qm, shape=(nw,), **kwargs)
            _quantize_wp_field(layer.batchnorm.mean, nbits, qm, shape=(nw,), **kwargs)
            _quantize_wp_field(layer.batchnorm.variance, nbits, qm, shape=(nw,), **kwargs)

        # InnerProduct
        elif layer_type == 'innerProduct':
            output_channels = layer.innerProduct.outputChannels
            input_channels = layer.innerProduct.inputChannels
            _quantize_wp_field(layer.innerProduct.weights, nbits, qm,
                shape=(output_channels, input_channels), **kwargs)
            has_bias = layer.innerProduct.hasBias
            if has_bias and selector.do_quantize(layer, weight_param='bias'):
                _quantize_wp_field(layer.innerProduct.bias, nbits, qm,
                    shape=(output_channels,), **kwargs)

        # BatchedMatmul
        elif layer_type == 'batchedMatmul':
            x1 = layer.batchedMatmul.weightMatrixFirstDimension
            x2 = layer.batchedMatmul.weightMatrixSecondDimension
            _quantize_wp_field(layer.batchedMatmul.weights, nbits, qm,
                shape=(x2, x1), **kwargs)
            has_bias = layer.batchedMatmul.hasBias
            if has_bias and selector.do_quantize(layer, weight_param='bias'):
                _quantize_wp_field(layer.batchedMatmul.bias, nbits, qm,
                    shape=(x2,), **kwargs)

        # Embedding layer
        elif layer_type == 'embedding':
            output_channels = layer.embedding.outputChannels
            input_channels = layer.embedding.inputDim
            _quantize_wp_field(layer.embedding.weights, nbits, qm, shape=(output_channels, input_channels), **kwargs)
            if layer.embedding.hasBias:
                _quantize_wp_field(layer.embedding.bias, nbits, qm, shape=(output_channels,), **kwargs)

        # Scale layer
        elif layer_type == 'scale':
            nw = _np.prod(layer.scale.shapeScale)
            _quantize_wp_field(layer.scale.scale, nbits, qm, shape=(nw,), **kwargs)
            if layer.scale.hasBias:
                nw = _np.prod(layer.scale.shapeBias)
                _quantize_wp_field(layer.scale.bias, nbits, qm, shape=(nw,), **kwargs)

        # Bias layer
        elif layer_type == 'bias':
            nw = _np.prod(layer.bias.shape)
            _quantize_wp_field(layer.bias.bias, nbits, qm, shape=(nw,), **kwargs)

        # LoadConstant layer
        elif layer_type == 'loadConstant':
            nw = _np.prod(layer.loadConstant.shape)
            _quantize_wp_field(layer.loadConstant.data, nbits, qm, shape=(nw,), **kwargs)

        # Simple Recurrent
        elif layer_type == 'simpleRecurrent':
            i_size = layer.simpleRecurrent.inputVectorSize
            o_size = layer.simpleRecurrent.outputVectorSize
            _quantize_wp_field(layer.simpleRecurrent.weightMatrix, nbits, qm, shape=(o_size, i_size), **kwargs)
            _quantize_wp_field(layer.simpleRecurrent.recursionMatrix, nbits, qm, shape=(o_size, o_size), **kwargs)
            if layer.simpleRecurrent.hasBiasVector:
                _quantize_wp_field(layer.simpleRecurrent.biasVector,nbits, qm, shape=(o_size,), **kwargs)

        # GRU
        elif layer_type == 'gru':
            i_size = layer.gru.inputVectorSize
            o_size = layer.gru.outputVectorSize
            # Weight Matrix
            _quantize_wp_field(layer.gru.updateGateWeightMatrix, nbits, qm, shape=(o_size,i_size), **kwargs)
            _quantize_wp_field(layer.gru.resetGateWeightMatrix, nbits, qm, shape=(o_size,i_size), **kwargs)
            _quantize_wp_field(layer.gru.outputGateWeightMatrix, nbits, qm, shape=(o_size,i_size), **kwargs)
            # Recursion Weights
            _quantize_wp_field(layer.gru.updateGateRecursionMatrix, nbits, qm, shape=(o_size, o_size), **kwargs)
            _quantize_wp_field(layer.gru.resetGateRecursionMatrix, nbits, qm, shape=(o_size, o_size), **kwargs)
            _quantize_wp_field(layer.gru.outputGateRecursionMatrix, nbits, qm, shape=(o_size, o_size), **kwargs)
            # Bias
            if layer.gru.hasBiasVectors:
                _quantize_wp_field(layer.gru.updateGateBiasVector, nbits, qm, shape=(o_size,), **kwargs)
                _quantize_wp_field(layer.gru.resetGateBiasVector, nbits, qm, shape=(o_size,), **kwargs)
                _quantize_wp_field(layer.gru.outputGateBiasVector, nbits, qm, shape=(o_size,), **kwargs)

        # LSTM Layers
        elif layer_type in ['uniDirectionalLSTM', 'biDirectionalLSTM']:

            def _lstmwp_to_fp16_lstmwp(lstm_wp, nbits, qm, i_size, o_size, has_peephole=True):
                assert lstm_wp
                _quantize_wp_field(lstm_wp.inputGateWeightMatrix, nbits, qm, shape=(o_size, i_size), **kwargs)
                _quantize_wp_field(lstm_wp.forgetGateWeightMatrix, nbits, qm,  shape=(o_size, i_size), **kwargs)
                _quantize_wp_field(lstm_wp.blockInputWeightMatrix, nbits, qm,  shape=(o_size, i_size), **kwargs)
                _quantize_wp_field(lstm_wp.outputGateWeightMatrix, nbits, qm,  shape=(o_size, i_size), **kwargs)

                _quantize_wp_field(lstm_wp.inputGateRecursionMatrix, nbits, qm, shape=(o_size, o_size), **kwargs)
                _quantize_wp_field(lstm_wp.forgetGateRecursionMatrix, nbits, qm, shape=(o_size, o_size), **kwargs)
                _quantize_wp_field(lstm_wp.blockInputRecursionMatrix, nbits, qm, shape=(o_size, o_size), **kwargs)
                _quantize_wp_field(lstm_wp.outputGateRecursionMatrix, nbits, qm, shape=(o_size, o_size), **kwargs)

                _quantize_wp_field(lstm_wp.inputGateBiasVector, nbits, qm, shape=(o_size,), **kwargs)
                _quantize_wp_field(lstm_wp.forgetGateBiasVector, nbits, qm, shape=(o_size,), **kwargs)
                _quantize_wp_field(lstm_wp.blockInputBiasVector, nbits, qm, shape=(o_size,), **kwargs)
                _quantize_wp_field(lstm_wp.outputGateBiasVector, nbits, qm, shape=(o_size,), **kwargs)

                if has_peephole:
                    _quantize_wp_field(lstm_wp.inputGatePeepholeVector, nbits, qm, shape=(o_size,), **kwargs)
                    _quantize_wp_field(lstm_wp.forgetGatePeepholeVector, nbits, qm, shape=(o_size,), **kwargs)
                    _quantize_wp_field(lstm_wp.outputGatePeepholeVector, nbits, qm, shape=(o_size,), **kwargs)

            if layer_type == 'uniDirectionalLSTM':
                _lstmwp_to_fp16_lstmwp(
                    lstm_wp=layer.uniDirectionalLSTM.weightParams,
                    nbits=nbits,
                    qm=qm,
                    i_size=layer.uniDirectionalLSTM.inputVectorSize,
                    o_size=layer.uniDirectionalLSTM.outputVectorSize,
                    has_peephole=layer.uniDirectionalLSTM.params.hasPeepholeVectors)

            elif layer_type == 'biDirectionalLSTM':
                for lstm_wp in layer.biDirectionalLSTM.weightParams:
                    _lstmwp_to_fp16_lstmwp(
                        lstm_wp=lstm_wp,
                        nbits=nbits,
                        qm=qm,
                        i_size=layer.biDirectionalLSTM.inputVectorSize,
                        o_size=layer.biDirectionalLSTM.outputVectorSize,
                        has_peephole=layer.biDirectionalLSTM.params.hasPeepholeVectors)

        elif layer_type == 'custom':
            print('Skipping custom layer {}. Weights for this layer need to'
                   'be converted manually'.format(layer.name))
        elif layer_type == 'branch':
            _quantize_nn_spec(layer.branch.ifBranch, nbits, qm, **kwargs)
            _quantize_nn_spec(layer.branch.elseBranch, nbits, qm, **kwargs)
        elif layer_type == 'loop':
            _quantize_nn_spec(layer.loop.conditionNetwork, nbits, qm, **kwargs)
            _quantize_nn_spec(layer.loop.bodyNetwork, nbits, qm, **kwargs)
        else:
            raise Exception('Unknown layer ' + layer_type + ' to be quantized')


def quantize_spec_weights(spec, nbits, quantization_mode, **kwargs):

    nn_model_types = ['neuralNetwork', 'neuralNetworkClassifier',
                      'neuralNetworkRegressor']

    model_type = spec.WhichOneof('Type')

    # Neural network models
    if model_type in nn_model_types:
        # Bump up to appropriate spec version if required
        if nbits == 16:
            spec.specificationVersion = max(_MINIMUM_FP16_SPEC_VERSION,
                                            spec.specificationVersion)
        else:
            spec.specificationVersion = max(_MINIMUM_QUANTIZED_MODEL_SPEC_VERSION,
                                            spec.specificationVersion)

        if spec.WhichOneof('Type') == 'neuralNetwork':
            _quantize_nn_spec(spec.neuralNetwork, nbits, quantization_mode,
                    **kwargs)

        elif spec.WhichOneof('Type') in 'neuralNetworkClassifier':
            _quantize_nn_spec(spec.neuralNetworkClassifier, nbits,
                    quantization_mode, **kwargs)

        elif spec.WhichOneof('Type') in 'neuralNetworkRegressor':
            _quantize_nn_spec(spec.neuralNetworkRegressor, nbits,
                    quantization_mode, **kwargs)

    # Recursively convert all pipeline models
    elif spec.WhichOneof('Type') == 'pipeline':
        for model_spec in spec.pipeline.models:
            quantize_spec_weights(model_spec, nbits, quantization_mode, **kwargs)

    elif spec.WhichOneof('Type') in ['pipelineClassifier', 'pipelineRegressor']:
        quantize_spec_weights(spec.pipeline, nbits, quantization_mode, **kwargs)

    return spec


def _load_and_resize_image(image_path, size):
    from PIL import Image
    img = Image.open(image_path)
    return img.resize(size, Image.ANTIALIAS)


class TopKMetrics():
    def __init__(self, topk):
        self._topk = topk
        self._correct_count = 0
        self._total_count = 0

    def add_metric(self, output1, output2):
        self._total_count += 1
        if self._topk == 1:
            if output1 == output2:
                self._correct_count += 1
        else:
            self._topk = min(len(output1.keys()), self._topk)
            out1_topk =  sorted(output1, key=output1.get,reverse=True)[:self._topk]
            out2_topk =  sorted(output2, key=output2.get,reverse=True)[:self._topk]
            if out1_topk[0] in out2_topk:
                self._correct_count += 1

    def display_metrics(self):
        pcorrect = (float(self._correct_count) / float(self._total_count))* 100
        pcorrect = _np.round(pcorrect, decimals=2)
        if self._topk == 1:
            print('Top 1 Agreement: {}%\n'.format(pcorrect))
        else:
            print('Top {} Agreement: {}%\n'.format(self._topk, pcorrect))


class NoiseMetrics():
    def __init__(self):
        self._snr = []
        self._psnr = []

    @staticmethod
    def _compute_snr(arr1, arr2):
        noise = arr1 - arr2
        noise_var = _np.sum(noise ** 2) / len(noise) + 1e-7
        signal_energy = _np.sum(arr2 ** 2) / len(arr2)
        max_signal_energy = _np.amax(arr2 ** 2)
        snr = 10 * _np.log10(signal_energy / noise_var)
        psnr = 10 * _np.log10(max_signal_energy / noise_var)
        return snr, psnr

    def add_metric(self, output1, output2):
        import PIL

        # Output is Image
        if isinstance(output1, PIL.Image.Image):
            if output1.mode == 'RGBA':
                output1 = output1.convert('RGB')
                output2 = output2.convert('RGB')
            arr1 = _np.array(output1).flatten()
            arr2 = _np.array(output2).flatten()
            snr, psnr = self._compute_snr(arr1, arr2)
            self._snr.append(snr)
            self._psnr.append(psnr)

        # Output is multiArray
        else:
            arr1 = output1.flatten()
            arr2 = output2.flatten()
            snr, psnr = self._compute_snr(arr1, arr2)
            self._snr.append(snr)
            self._psnr.append(psnr)

    def display_metrics(self):
        print('SNR:  {} +/- {}'.format(_np.mean(self._snr), _np.var(self._snr)))
        print('PSNR: {} +/- {}\n'.format(_np.mean(self._psnr), _np.var(self._psnr)))


class OutputMetric():
    """
    Utility class to calculate and hold metrics between
    two model outputs
    """
    def __init__(self, name, type):
        self.name = name
        self._metrics = []

        if type == 'stringType':
            self._metrics.append(TopKMetrics(topk=1))

        elif type == 'dictionaryType':
            self._metrics.append(TopKMetrics(topk=5))

        elif type == 'imageType' or type == 'multiArrayType':
            self._metrics.append(NoiseMetrics())

        else:
            raise Exception("""Unable to determine which metric to
            compute for output: {}""".format(name))

    def add_metric(self, output1, output2):
        for metric in self._metrics:
            metric.add_metric(output1, output2)

    def display_metrics(self):
        for metric in self._metrics:
            metric.display_metrics()


class ModelMetrics():
    """
    A utility class to hold evaluation metrics
    """
    def __init__(self, spec):
        self.model_metrics = {}
        for output in spec.description.output:
            output_type = output.type.WhichOneof('Type')
            self.model_metrics[output.name] = OutputMetric(output.name, output_type)

    def add_metrics(self, model1_output, model2_output):
        outputs = model1_output.keys()
        for output in outputs:
            self.model_metrics[output].add_metric(model1_output[output], model2_output[output])

    def display_metrics(self):
        for metric in self.model_metrics:
            print('Output {}:'.format(metric))
            dash = '----------'
            for x in range(0, len(metric)):
                dash += '-'
            print(dash)
            self.model_metrics[metric].display_metrics()


def _characterize_qmodel_perf_with_data_dir(fpmodel, qspec, data_dir):
    supported_image_exts = ['jpg', 'bmp', 'png', 'jpeg']
    test_image_paths = ['{}/{}'.format(data_dir, fn) for fn in
                        os.listdir(data_dir) if
                        any(fn.endswith(ext) for ext in supported_image_exts)]

    if not test_image_paths:
        raise Exception("""Path contains no supported image files.
        Supported file types include jpg, bmp, png and jpeg.
        """.format(data_dir))

    qmodel = _get_model(qspec)
    model_metrics = ModelMetrics(qspec)

    input_name = qspec.description.input[0].name
    input_size = (qspec.description.input[0].type.imageType.width,
                  qspec.description.input[0].type.imageType.height)

    print('\n\n')
    print('Analyzing {} images'.format(len(test_image_paths)))
    print('Running Analysis this may take a while ...')
    print('\n')

    analyzed = 0
    tried = 0
    for image in test_image_paths:
        try:
            input = {input_name: _load_and_resize_image(image, input_size)}
            fp_pred = fpmodel.predict(input, useCPUOnly=True)
            q_pred = qmodel.predict(input, useCPUOnly=True)
            analyzed += 1
            model_metrics.add_metrics(fp_pred, q_pred)

        except Exception as e:
            print(e)
            continue

        # Update Progress
        tried += 1
        if tried % 10 == 0:
            sys.stdout.write('\r')
            sys.stdout.write(
                'Analyzed {}/{}'.format(tried, len(test_image_paths)))
            sys.stdout.flush()

    print('\n')
    model_metrics.display_metrics()


def _characterize_quantized_model_perf(fpmodel, qspec, sample_data):
    qmodel = _get_model(qspec)
    model_metrics = ModelMetrics(qspec)

    print('\n\n')
    print('Analyzing {} samples'.format(len(sample_data)))
    print('Running Analysis this may take a while ...')
    print('\n')

    analyzed = 0
    tried = 0
    for data in sample_data:
        try:
            fp_pred = fpmodel.predict(data, useCPUOnly=True)
            q_pred = qmodel.predict(data, useCPUOnly=True)
            analyzed += 1
            model_metrics.add_metrics(fp_pred, q_pred)

        except Exception as e:
            print(e)
            continue

        # Update Progress
        tried += 1
        if tried % 10 == 0:
            sys.stdout.write('\r')
            sys.stdout.write(
                'Analyzed {}/{}'.format(tried, len(sample_data)))
            sys.stdout.flush()

    print('\n')
    model_metrics.display_metrics()


def compare_models(full_precision_model, quantized_model,
                              sample_data):
    """
    Utility function to compare the performance of a full precision vs quantized model

    full_precision_model: MLModel
        The full precision model with float32 weights

    quantized_model: MLModel
        Quantized version of the model with quantized weights

    sample_data: str | [dict]
        Data used to characterize performance of the quantized model in
        comparison to the full precision model. Either a list of sample input
        dictionaries or an absolute path to a directory containing images.
        Path to a directory containing images is only valid for models with
        one image input. For all other models a list of sample inputs must be
        provided.

    :return:
        None. Performance metrics are printed out
    """
    emessage = ("""
    Invalid sample data provided. Only a list of dictionaries
    containing sample data or path to a folder containing images is
    supported""")

    spec = full_precision_model.get_spec()
    num_inputs = len(spec.description.input)
    if isinstance(sample_data, str):
        input_type = spec.description.input[0].type.WhichOneof('Type')
        if num_inputs != 1 or input_type != 'imageType':
            raise Exception("""Unable to analyze quantized models. Sample data
            was a path to a directory which is only supported with models with
            one image type input. Please try passing in a list of sample inputs
            as sample data.
            """)
        _characterize_qmodel_perf_with_data_dir(full_precision_model, quantized_model.get_spec(), sample_data)

    elif isinstance(sample_data, list):
        if not all(type(d) is dict for d in sample_data):
            raise Exception(emessage)
        _characterize_quantized_model_perf(full_precision_model, quantized_model.get_spec(), sample_data)

    else:
        raise Exception(emessage)


def quantize_weights(full_precision_model,
                     nbits,
                     quantization_mode="linear",
                     sample_data=None,
                     **kwargs):
    """
    Utility function to convert a full precision (float) MLModel to a
    nbit quantized MLModel (float16).

    full_precision_model: MLModel
        Model which will be converted to half precision. Currently conversion
        for only neural network models is supported. If a pipeline model is
        passed in then all embedded neural network models embedded within
        will be converted.
        
    nbits: int
        Number of bits per quantized weight. Only 16-bit float point and 
            1-8 bit is supported

    quantization_mode: str
        One of the following:

        "linear":
            Linear quantization with scale and bias assuming the range of weight
            values is [A, B], where A = min(weight), B = max(weight)
        "linear_lut":
            Simple linear quantization represented as a lookup table
        "kmeans_lut":
            LUT based quantization, where LUT is generated by K-Means clustering
        "custom_lut":
            LUT quantization where LUT and quantized weight params are
            calculated using a custom function. If this mode is selected then
            a custom function must be passed in kwargs with key lut_function.
            The function must have input params (nbits, wp) where nbits is the
            number of quantization bits and wp is the list of weights for a
            given layer. The function should return two parameters (lut, qw)
            where lut is an array of length (2^n bits)containing LUT values and
            qw is the list of quantized weight parameters. See
            ``_get_linear_lookup_table_and_weight`` for a sample implementation.
        "linear_symmetric":
            Linear quantization with scale and bias assuming the range of weight
            values is [-A, A], where A = max(abs(weight)).

    sample_data: str | [dict]
        Data used to characterize performance of the quantized model in
        comparison to the full precision model. Either a list of sample input
        dictionaries or an absolute path to a directory containing images.
        Path to a directory containing images is only valid for models with
        one image input. For all other models a list of sample inputs must be
        provided.

    kwargs: keyword arguments
            *lut_function* : (``callable function``)
                A callable function provided when quantization mode is set to
                ``_QUANTIZATION_MODE_CUSTOM_LOOKUP_TABLE``. See ``quantization_mode``
                for more details.
            *selector*: QuantizedLayerSelector
                A QuanatizedLayerSelector object that can be derived to provide
                custom quantization selection.

    Returns
    -------
    model: MLModel
        The quantized MLModel instance if running on macOS 10.14 or later,
        otherwise the quantized model specification is returned

    Examples
    --------
    .. sourcecode:: python

        >>> import coremltools
        >>> from coremltools.models.neural_network import quantization_utils
        >>> model = coremltools.models.MLModel('my_model.mlmodel')
        >>> quantized_model = quantization_utils.quantize_weights(model, 8, "linear")
    """

    qmode_mapping = {
        "linear": _QUANTIZATION_MODE_LINEAR_QUANTIZATION,
        "kmeans": _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS,
        "kmeans_lut": _QUANTIZATION_MODE_LOOKUP_TABLE_KMEANS,
        "linear_lut": _QUANTIZATION_MODE_LOOKUP_TABLE_LINEAR,
        "custom_lut": _QUANTIZATION_MODE_CUSTOM_LOOKUP_TABLE,
        "dequantization": _QUANTIZATION_MODE_DEQUANTIZE,
        "linear_symmetric": _QUANTIZATION_MODE_LINEAR_SYMMETRIC
    }
    try:
        qmode = qmode_mapping[quantization_mode]
    except KeyError:
        # kmeans is deprecated. Instead kmeans_lut is used. No need to show it.
        del qmode_mapping['kmeans']
        raise Exception("Invalid quantization mode. Quantization mode must be "
                        "one of {}".format(qmode_mapping))

    print("Quantizing using {} quantization".format(quantization_mode))
    spec = full_precision_model.get_spec()
    qspec = quantize_spec_weights(spec, nbits, qmode, **kwargs)

    if macos_version() < (10, 14):
        print("WARNING! Unable to return a quantized MLModel instance since"
              "OS != macOS 10.14 or later")
        print("Returning quantized model specification instead")
        return qspec

    quantized_model = _get_model(qspec)
    if not sample_data:
        return quantized_model

    compare_models(full_precision_model, quantized_model, sample_data)
    return quantized_model

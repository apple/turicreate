# Copyright (c) 2017-2019, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from . import _utils
import logging
import keras as _keras
import numpy as _np
from ...proto import NeuralNetwork_pb2 as _NeuralNetwork_pb2

from distutils.version import StrictVersion as _StrictVersion

if _keras.__version__ >= _StrictVersion("2.2.1"):
    from keras.layers import DepthwiseConv2D
elif _keras.__version__ >= _StrictVersion("2.2.0"):
    from keras.layers import DepthwiseConv2D
    from keras_applications.mobilenet import relu6
else:
    from keras.applications.mobilenet import DepthwiseConv2D, relu6


def _get_recurrent_activation_name_from_keras(activation):
    if activation == _keras.activations.sigmoid:
        activation_str = "SIGMOID"
    elif activation == _keras.activations.hard_sigmoid:
        activation_str = "SIGMOID_HARD"
    elif activation == _keras.activations.tanh:
        activation_str = "TANH"
    elif activation == _keras.activations.relu:
        activation_str = "RELU"
    elif activation == _keras.activations.linear:
        activation_str = "LINEAR"
    else:
        raise NotImplementedError(
            "activation %s not supported for Recurrent layer." % activation
        )

    return activation_str


def _get_activation_name_from_keras_layer(keras_layer):
    if isinstance(keras_layer, _keras.layers.advanced_activations.LeakyReLU):
        non_linearity = "LEAKYRELU"
    elif isinstance(keras_layer, _keras.layers.advanced_activations.PReLU):
        non_linearity = "PRELU"
    elif isinstance(keras_layer, _keras.layers.advanced_activations.ELU):
        non_linearity = "ELU"
    elif isinstance(keras_layer, _keras.layers.advanced_activations.ThresholdedReLU):
        non_linearity = "THRESHOLDEDRELU"
    elif isinstance(keras_layer, _keras.layers.advanced_activations.Softmax):
        non_linearity = "SOFTMAX"
    else:
        import six

        if six.PY2:
            act_name = keras_layer.activation.func_name
        else:
            act_name = keras_layer.activation.__name__

        if act_name == "softmax":
            non_linearity = "SOFTMAX"
        elif act_name == "sigmoid":
            non_linearity = "SIGMOID"
        elif act_name == "tanh":
            non_linearity = "TANH"
        elif act_name == "relu":
            non_linearity = "RELU"
        elif act_name == "relu6":
            non_linearity = "RELU6"
        elif act_name == "softplus":
            non_linearity = "SOFTPLUS"
        elif act_name == "softsign":
            non_linearity = "SOFTSIGN"
        elif act_name == "hard_sigmoid":
            non_linearity = "SIGMOID_HARD"
        elif act_name == "elu":
            non_linearity = "UNIT_ELU"
        elif act_name == "linear":
            non_linearity = "LINEAR"
        elif act_name == "selu":
            non_linearity = "SELU"
        else:
            non_linearity = "CUSTOM"

    return non_linearity


def _get_elementwise_name_from_keras_layer(keras_layer):
    """
    Get the keras layer name from the activation name.
    """
    if isinstance(keras_layer, _keras.layers.Add):
        return "ADD"
    elif isinstance(keras_layer, _keras.layers.Multiply):
        return "MULTIPLY"
    elif isinstance(keras_layer, _keras.layers.Concatenate):
        if len(keras_layer.input_shape[0]) == 3 and (
            keras_layer.axis == 1 or keras_layer.axis == -2
        ):
            return "SEQUENCE_CONCAT"
        if len(keras_layer.input_shape[0]) == 3 and (
            keras_layer.axis == 2 or keras_layer.axis == -1
        ):
            return "CONCAT"
        elif len(keras_layer.input_shape[0]) == 4 and (
            keras_layer.axis == 3 or keras_layer.axis == -1
        ):
            return "CONCAT"
        elif len(keras_layer.input_shape[0]) == 2 and (
            keras_layer.axis == 1 or keras_layer.axis == -1
        ):
            return "CONCAT"
        else:
            raise ValueError("Only channel and sequence concatenation are supported.")
    elif isinstance(keras_layer, _keras.layers.Dot):
        if len(keras_layer.input_shape[0]) == 2:
            if type(keras_layer.axes) is list or type(keras_layer.axes) is tuple:
                if len(keras_layer.axes) > 1:
                    raise ValueError("Only vector dot-product is supported.")
                else:
                    axis = keras_layer.axes[0]
            else:
                axis = keras_layer.axes
            if axis != -1 and axis != 1:
                raise ValueError("Only vector dot-product is supported.")

            if keras_layer.normalize:
                return "COS"
            else:
                return "DOT"
        else:
            raise ValueError("Only vector dot-product is supported.")
    elif isinstance(keras_layer, _keras.layers.Maximum):
        return "MAX"
    elif isinstance(keras_layer, _keras.layers.Average):
        return "AVE"
    else:
        _utils.raise_error_unsupported_option(
            str(type(keras_layer)), "merge", keras_layer.name
        )


def _same_elements_per_channel(x):
    """ Test if a 3D (H,W,C) matrix x has the same element in each (H,W)
    matrix for each channel
    """
    eps = 1e-5
    dims = x.shape
    for c in range(dims[-1]):
        xc = x[:, :, c].flatten()
        if not _np.all(_np.absolute(xc - xc[0]) < eps):
            return False
    return True


def _check_data_format(keras_layer):
    if hasattr(keras_layer, ("data_format")):
        if keras_layer.data_format != "channels_last":
            raise ValueError(
                "Converter currently supports data_format = " "'channels_last' only."
            )


def convert_dense(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert a dense layer from keras to coreml.

    Parameters
    keras_layer: layer
    ----------
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether or not to carry over Keras' "trainable" parameter.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    has_bias = keras_layer.use_bias
    # Get the weights from keras
    W = keras_layer.get_weights()[0].T
    Wb = keras_layer.get_weights()[1].T if has_bias else None
    output_channels, input_channels = W.shape

    builder.add_inner_product(
        name=layer,
        W=W,
        b=Wb,
        input_channels=input_channels,
        output_channels=output_channels,
        has_bias=has_bias,
        input_name=input_name,
        output_name=output_name,
    )
    if respect_train and keras_layer.trainable:
        builder.make_updatable([layer])


def convert_embedding(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """Convert a dense layer from keras to coreml.

    Parameters
    keras_layer: layer
    ----------
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether to support Keras' "trainable" flag (unsupported).
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    # Get the weights from keras
    W = keras_layer.get_weights()[0].T

    # assuming keras embedding layers don't have biases
    builder.add_embedding(
        name=layer,
        W=W,
        b=None,
        input_dim=keras_layer.input_dim,
        output_channels=keras_layer.output_dim,
        has_bias=False,
        input_name=input_name,
        output_name=output_name,
    )

    if respect_train and keras_layer.trainable:
        logging.warning(
            "Embedding layer '%s' is marked updatable, but Core "
            "ML does not yet support updating layers of this "
            "type. The layer will be frozen in Core ML.",
            layer,
        )


def convert_activation(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert an activation layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean,
        Ignored.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])
    non_linearity = _get_activation_name_from_keras_layer(keras_layer)

    # Add a non-linearity layer
    if non_linearity == "SOFTMAX":
        builder.add_softmax(name=layer, input_name=input_name, output_name=output_name)
        return
    if non_linearity == "RELU6":
        # No direct support of RELU with max-activation value - use negate and
        # clip layers
        relu_output_name = output_name + "_relu"
        builder.add_activation(layer, "RELU", input_name, relu_output_name)
        # negate it
        neg_output_name = relu_output_name + "_neg"
        builder.add_activation(
            layer + "__neg__", "LINEAR", relu_output_name, neg_output_name, [-1.0, 0]
        )
        # apply threshold
        clip_output_name = relu_output_name + "_clip"
        builder.add_unary(
            layer + "__clip__",
            neg_output_name,
            clip_output_name,
            "threshold",
            alpha=-6.0,
        )
        # negate it back
        builder.add_activation(
            layer + "_neg2", "LINEAR", clip_output_name, output_name, [-1.0, 0]
        )
        return

    if non_linearity == "SELU":
        elu_output_name = output_name + "_elu"
        builder.add_activation(
            layer + "__elu__", "ELU", input_name, elu_output_name, params=1.6732
        )
        builder.add_elementwise(
            layer,
            input_names=elu_output_name,
            output_name=output_name,
            mode="MULTIPLY",
            alpha=1.0507,
        )
        return

    params = None
    if non_linearity == "UNIT_ELU":
        params = 1.0
        non_linearity = "ELU"
    elif non_linearity == "LEAKYRELU":
        params = [keras_layer.alpha]
    elif non_linearity == "PRELU":
        shared_axes = list(keras_layer.shared_axes)
        if not (shared_axes == [1, 2, 3] or shared_axes == [1, 2]):
            _utils.raise_error_unsupported_scenario(
                "Shared axis not being [1,2,3] or [1,2]", "parametric_relu", layer
            )
        params = _keras.backend.eval(keras_layer.weights[0])
    elif non_linearity == "ELU":
        params = keras_layer.alpha
    elif non_linearity == "THRESHOLDEDRELU":
        params = keras_layer.theta
    else:
        pass  # do nothing to parameters

    builder.add_activation(
        name=layer,
        non_linearity=non_linearity,
        input_name=input_name,
        output_name=output_name,
        params=params,
    )


def convert_advanced_relu(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert an ReLU layer with maximum value from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Ignored.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    if keras_layer.max_value is None:
        builder.add_activation(layer, "RELU", input_name, output_name)
        return

    # No direct support of RELU with max-activation value - use negate and
    # clip layers
    relu_output_name = output_name + "_relu"
    builder.add_activation(layer, "RELU", input_name, relu_output_name)
    # negate it
    neg_output_name = relu_output_name + "_neg"
    builder.add_activation(
        layer + "__neg__", "LINEAR", relu_output_name, neg_output_name, [-1.0, 0]
    )
    # apply threshold
    clip_output_name = relu_output_name + "_clip"
    builder.add_unary(
        layer + "__clip__",
        neg_output_name,
        clip_output_name,
        "threshold",
        alpha=-keras_layer.max_value,
    )
    # negate it back
    builder.add_activation(
        layer + "_neg2", "LINEAR", clip_output_name, output_name, [-1.0, 0]
    )


def convert_convolution(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert convolution layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether or not to carry over Keras' "trainable" parameter.
    """

    _check_data_format(keras_layer)

    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    has_bias = keras_layer.use_bias
    is_deconv = isinstance(keras_layer, _keras.layers.convolutional.Conv2DTranspose)

    # Get the weights from _keras.
    weightList = keras_layer.get_weights()

    # Dimensions and weights
    if is_deconv:
        height, width, n_filters, channels = weightList[0].shape
        W = weightList[0].transpose([0, 1, 3, 2])
        try:
            output_blob_shape = list(filter(None, keras_layer.output_shape))
            output_shape = output_blob_shape[:-1]
        except:
            output_shape = None
    else:
        height, width, channels, n_filters = weightList[0].shape
        W = weightList[0]
        output_shape = None
    b = weightList[1] if has_bias else None

    output_channels = n_filters

    stride_height, stride_width = keras_layer.strides

    # Dilations
    dilations = [1, 1]
    if (type(keras_layer.dilation_rate) is list) or (
        type(keras_layer.dilation_rate) is tuple
    ):
        dilations = [keras_layer.dilation_rate[0], keras_layer.dilation_rate[1]]
    else:
        dilations = [keras_layer.dilation_rate, keras_layer.dilation_rate]
    if is_deconv and not dilations == [1, 1]:
        raise ValueError("Unsupported non-unity dilation for Deconvolution layer")

    groups = 1
    kernel_channels = channels
    # depth-wise convolution
    if isinstance(keras_layer, DepthwiseConv2D):
        groups = channels
        kernel_channels = 1
        depth_multiplier = keras_layer.depth_multiplier
        W = _np.reshape(W, (height, width, 1, channels * depth_multiplier))
        output_channels = channels * depth_multiplier

    builder.add_convolution(
        name=layer,
        kernel_channels=kernel_channels,
        output_channels=output_channels,
        height=height,
        width=width,
        stride_height=stride_height,
        stride_width=stride_width,
        border_mode=keras_layer.padding,
        groups=groups,
        W=W,
        b=b,
        has_bias=has_bias,
        is_deconv=is_deconv,
        output_shape=output_shape,
        input_name=input_name,
        output_name=output_name,
        dilation_factors=dilations,
    )

    if respect_train and keras_layer.trainable:
        builder.make_updatable([layer])


def convert_convolution1d(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert convolution layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether to honor Keras' "trainable" flag.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    has_bias = keras_layer.use_bias

    # Get the weights from _keras.
    # Keras stores convolution weights as list of numpy arrays
    weightList = keras_layer.get_weights()
    output_shape = list(filter(None, keras_layer.output_shape))[:-1]

    # Parameter
    filter_length, input_dim, n_filters = weightList[0].shape
    stride_width = (
        keras_layer.strides
        if type(keras_layer.strides) is int
        else keras_layer.strides[0]
    )

    # Weights and bias terms
    W = _np.expand_dims(weightList[0], axis=0)
    b = weightList[1] if has_bias else None

    dilations = [1, 1]
    if (type(keras_layer.dilation_rate) is list) or (
        type(keras_layer.dilation_rate) is tuple
    ):
        dilations = [1, keras_layer.dilation_rate[0]]
    else:
        dilations = [1, keras_layer.dilation_rate]

    keras_padding = keras_layer.padding
    if keras_padding == "causal":
        builder.add_padding(
            name=layer + "__causal_pad__",
            left=filter_length - 1,
            right=0,
            top=0,
            bottom=0,
            value=0,
            input_name=input_name,
            output_name=input_name + "__causal_pad__",
        )
        input_name = input_name + "__causal_pad__"
        keras_padding = "valid"

    builder.add_convolution(
        name=layer,
        kernel_channels=input_dim,
        output_channels=n_filters,
        height=1,
        width=filter_length,
        stride_height=1,
        stride_width=stride_width,
        border_mode=keras_padding,
        groups=1,
        W=W,
        b=b,
        has_bias=has_bias,
        is_deconv=False,
        output_shape=output_shape,
        input_name=input_name,
        output_name=output_name,
        dilation_factors=dilations,
    )

    if respect_train and keras_layer.trainable:
        builder.make_updatable([layer])


def convert_separable_convolution(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert separable convolution layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether to honor Keras' "trainable" flag.
    """
    _check_data_format(keras_layer)

    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    has_bias = keras_layer.use_bias

    # Get the weights from _keras.
    weight_list = keras_layer.get_weights()
    output_blob_shape = list(filter(None, keras_layer.output_shape))
    output_channels = output_blob_shape[-1]

    # D: depth mutliplier
    # w[0] is (H,W,Cin,D)
    # w[1] is (1,1,Cin * D, Cout)
    W0 = weight_list[0]
    W1 = weight_list[1]
    height, width, input_channels, depth_mult = W0.shape
    b = weight_list[2] if has_bias else None

    W0 = _np.reshape(W0, (height, width, 1, input_channels * depth_mult))

    stride_height, stride_width = keras_layer.strides

    # Dilations
    if (type(keras_layer.dilation_rate) is list) or (
        type(keras_layer.dilation_rate) is tuple
    ):
        dilations = [keras_layer.dilation_rate[0], keras_layer.dilation_rate[1]]
    else:
        dilations = [keras_layer.dilation_rate, keras_layer.dilation_rate]

    intermediate_name = output_name + "_intermin_"

    builder.add_convolution(
        name=layer + "_step_1",
        kernel_channels=1,
        output_channels=input_channels * depth_mult,
        height=height,
        width=width,
        stride_height=stride_height,
        stride_width=stride_width,
        border_mode=keras_layer.padding,
        groups=input_channels,
        W=W0,
        b=None,
        has_bias=False,
        is_deconv=False,
        output_shape=None,
        input_name=input_name,
        output_name=intermediate_name,
        dilation_factors=dilations,
    )

    builder.add_convolution(
        name=layer + "_step_2",
        kernel_channels=input_channels * depth_mult,
        output_channels=output_channels,
        height=1,
        width=1,
        stride_height=1,
        stride_width=1,
        border_mode=keras_layer.padding,
        groups=1,
        W=W1,
        b=b,
        has_bias=has_bias,
        is_deconv=False,
        output_shape=None,
        input_name=intermediate_name,
        output_name=output_name,
        dilation_factors=[1, 1],
    )

    if respect_train and keras_layer.trainable:
        builder.make_updatable([layer + "_step_1", layer + "_step_2"])


def convert_batchnorm(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert a Batch Normalization layer.

    Parameters
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether to honor Keras' "trainable" flag (unsupported).
    """

    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    axis = keras_layer.axis
    nb_channels = keras_layer.input_shape[axis]

    # Set parameters
    # Parameter arrangement in Keras: gamma, beta, mean, variance
    idx = 0
    gamma, beta = None, None
    if keras_layer.scale:
        gamma = keras_layer.get_weights()[idx]
        idx += 1
    if keras_layer.center:
        beta = keras_layer.get_weights()[idx]
        idx += 1
    mean = keras_layer.get_weights()[idx]
    std = keras_layer.get_weights()[idx + 1]

    gamma = _np.ones(mean.shape) if gamma is None else gamma
    beta = _np.zeros(mean.shape) if beta is None else beta

    # compute adjusted parameters
    variance = std * std
    f = 1.0 / _np.sqrt(std + keras_layer.epsilon)
    gamma1 = gamma * f
    beta1 = beta - gamma * mean * f
    mean[:] = 0.0  # mean
    variance[:] = 1.0 - 0.00001  # stddev

    builder.add_batchnorm(
        name=layer,
        channels=nb_channels,
        gamma=gamma1,
        beta=beta1,
        mean=mean,
        variance=variance,
        input_name=input_name,
        output_name=output_name,
    )

    if respect_train and keras_layer.trainable:
        logging.warning(
            "BatchNorm layer '%s' is marked updatable, but Core "
            "ML does not yet support updating layers of this "
            "type. The layer will be frozen in Core ML.",
            layer,
        )


def convert_flatten(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert a flatten layer from keras to coreml.
    ----------
    Parameters
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Ignored.
    """
    input_name, output_name = (input_names[0], output_names[0])

    # blob_order == 0 if the input blob needs not be rearranged
    # blob_order == 1 if the input blob needs to be rearranged
    blob_order = 0

    # using keras_layer.input.shape have a "?" (Dimension[None] at the front),
    # making a 3D tensor with unknown batch size 4D

    try:
        in_shape = keras_layer.input_shape
        if len(in_shape) == 4:
            blob_order = 1
        if len(in_shape) == 3 and in_shape[0] is None:
            # handling Keras rank-3 tensor (Batch, Sequence, Channels)
            permute_output_name = output_name + "__permute__"
            builder.add_permute(
                name=layer + "__permute__",
                dim=(2, 1, 0, 3),
                input_name=input_name,
                output_name=permute_output_name,
            )
            builder.add_flatten(
                name=layer,
                mode=1,
                input_name=permute_output_name,
                output_name=output_name,
            )
        else:
            builder.add_flatten(
                name=layer,
                mode=blob_order,
                input_name=input_name,
                output_name=output_name,
            )
    except:
        builder.add_flatten(
            name=layer, mode=1, input_name=input_name, output_name=output_name
        )


def convert_merge(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert concat layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Ignored.
    """
    # Get input and output names
    output_name = output_names[0]

    mode = _get_elementwise_name_from_keras_layer(keras_layer)
    builder.add_elementwise(
        name=layer, input_names=input_names, output_name=output_name, mode=mode
    )


def convert_pooling(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert pooling layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Ignored.
    """
    _check_data_format(keras_layer)
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    # Pooling layer type
    if (
        isinstance(keras_layer, _keras.layers.convolutional.MaxPooling2D)
        or isinstance(keras_layer, _keras.layers.convolutional.MaxPooling1D)
        or isinstance(keras_layer, _keras.layers.pooling.GlobalMaxPooling2D)
        or isinstance(keras_layer, _keras.layers.pooling.GlobalMaxPooling1D)
    ):
        layer_type_str = "MAX"
    elif (
        isinstance(keras_layer, _keras.layers.convolutional.AveragePooling2D)
        or isinstance(keras_layer, _keras.layers.convolutional.AveragePooling1D)
        or isinstance(keras_layer, _keras.layers.pooling.GlobalAveragePooling2D)
        or isinstance(keras_layer, _keras.layers.pooling.GlobalAveragePooling1D)
    ):
        layer_type_str = "AVERAGE"
    else:
        raise TypeError("Pooling type %s not supported" % keras_layer)

    # if it's global, set the global flag
    if isinstance(keras_layer, _keras.layers.pooling.GlobalMaxPooling2D) or isinstance(
        keras_layer, _keras.layers.pooling.GlobalAveragePooling2D
    ):
        # 2D global pooling
        global_pooling = True
        height, width = (0, 0)
        stride_height, stride_width = (0, 0)
        padding_type = "VALID"
    elif isinstance(
        keras_layer, _keras.layers.pooling.GlobalMaxPooling1D
    ) or isinstance(keras_layer, _keras.layers.pooling.GlobalAveragePooling1D):
        # 1D global pooling: 1D global pooling seems problematic in the backend,
        # use this work-around
        global_pooling = False
        _, width, channels = keras_layer.input_shape
        height = 1
        stride_height, stride_width = height, width
        padding_type = "VALID"
    else:
        global_pooling = False
        # Set pool sizes and strides
        # 1D cases:
        if (
            isinstance(keras_layer, _keras.layers.convolutional.MaxPooling1D)
            or isinstance(keras_layer, _keras.layers.pooling.GlobalMaxPooling1D)
            or isinstance(keras_layer, _keras.layers.convolutional.AveragePooling1D)
            or isinstance(keras_layer, _keras.layers.pooling.GlobalAveragePooling1D)
        ):
            pool_size = (
                keras_layer.pool_size
                if type(keras_layer.pool_size) is int
                else keras_layer.pool_size[0]
            )
            height, width = 1, pool_size
            if keras_layer.strides is not None:
                strides = (
                    keras_layer.strides
                    if type(keras_layer.strides) is int
                    else keras_layer.strides[0]
                )
                stride_height, stride_width = 1, strides
            else:
                stride_height, stride_width = 1, pool_size
        # 2D cases:
        else:
            height, width = keras_layer.pool_size
            if keras_layer.strides is None:
                stride_height, stride_width = height, width
            else:
                stride_height, stride_width = keras_layer.strides

        # Padding
        padding = keras_layer.padding
        if keras_layer.padding == "valid":
            padding_type = "VALID"
        elif keras_layer.padding == "same":
            padding_type = "SAME"
        else:
            raise TypeError("Border mode %s not supported" % padding)

    builder.add_pooling(
        name=layer,
        height=height,
        width=width,
        stride_height=stride_height,
        stride_width=stride_width,
        layer_type=layer_type_str,
        padding_type=padding_type,
        input_name=input_name,
        output_name=output_name,
        exclude_pad_area=True,
        is_global=global_pooling,
    )


def convert_padding(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert padding layer from keras to coreml.
    Keras only supports zero padding at this time.
    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Ignored.
    """
    _check_data_format(keras_layer)
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    is_1d = isinstance(keras_layer, _keras.layers.ZeroPadding1D)

    padding = keras_layer.padding
    top = left = bottom = right = 0
    if is_1d:
        if type(padding) is int:
            left = right = padding
        elif type(padding) is tuple:
            if type(padding[0]) is int:
                left, right = padding
            elif type(padding[0]) is tuple and len(padding[0]) == 2:
                left, right = padding[0]
            else:
                raise ValueError("Unrecognized padding option: %s" % (str(padding)))
        else:
            raise ValueError("Unrecognized padding option: %s" % (str(padding)))
    else:
        if type(padding) is int:
            top = left = bottom = right = padding
        elif type(padding) is tuple:
            if type(padding[0]) is int:
                top, left = padding
                bottom, right = padding
            elif type(padding[0]) is tuple:
                top, bottom = padding[0]
                left, right = padding[1]
            else:
                raise ValueError("Unrecognized padding option: %s" % (str(padding)))
        else:
            raise ValueError("Unrecognized padding option: %s" % (str(padding)))

    # Now add the layer
    builder.add_padding(
        name=layer,
        left=left,
        right=right,
        top=top,
        bottom=bottom,
        value=0,
        input_name=input_name,
        output_name=output_name,
    )


def convert_cropping(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert padding layer from keras to coreml.
    Keras only supports zero padding at this time.
    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Ignored.
    """

    _check_data_format(keras_layer)
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])
    is_1d = isinstance(keras_layer, _keras.layers.Cropping1D)

    cropping = keras_layer.cropping
    top = left = bottom = right = 0
    if is_1d:
        if type(cropping) is int:
            left = right = cropping
        elif type(cropping) is tuple:
            if type(cropping[0]) is int:
                left, right = cropping
            elif type(cropping[0]) is tuple and len(cropping[0]) == 2:
                left, right = cropping[0]
            else:
                raise ValueError("Unrecognized cropping option: %s" % (str(cropping)))
        else:
            raise ValueError("Unrecognized cropping option: %s" % (str(cropping)))
    else:
        if type(cropping) is int:
            top = left = bottom = right = cropping
        elif type(cropping) is tuple:
            if type(cropping[0]) is int:
                top, left = cropping
                bottom, right = cropping
            elif type(cropping[0]) is tuple:
                top, bottom = cropping[0]
                left, right = cropping[1]
            else:
                raise ValueError("Unrecognized cropping option: %s" % (str(cropping)))
        else:
            raise ValueError("Unrecognized cropping option: %s" % (str(cropping)))

    # Now add the layer
    builder.add_crop(
        name=layer,
        left=left,
        right=right,
        top=top,
        bottom=bottom,
        offset=[0, 0],
        input_names=[input_name],
        output_name=output_name,
    )


def convert_upsample(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert upsample layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Ignored.
    """
    _check_data_format(keras_layer)
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    is_1d = isinstance(keras_layer, _keras.layers.UpSampling1D)

    # Currently, we only support upsample of same dims
    fh = fw = 1
    if is_1d:
        if type(keras_layer.size) is tuple and len(keras_layer.size) == 1:
            fh, fw = 1, keras_layer.size[0]
        elif type(keras_layer.size) is int:
            fh, fw = 1, keras_layer.size
        else:
            raise ValueError(
                "Unrecognized upsample factor format %s" % (str(keras_layer.size))
            )
    else:
        if type(keras_layer.size) is int:
            fh = fw = keras_layer.size
        elif len(keras_layer.size) == 2:
            if keras_layer.size[0] != keras_layer.size[1]:
                raise ValueError(
                    "Upsample with different rows and columns not " "supported."
                )
            else:
                fh = keras_layer.size[0]
                fw = keras_layer.size[1]
        else:
            raise ValueError(
                "Unrecognized upsample factor format %s" % (str(keras_layer.size))
            )

    kerasmode2coreml = {"nearest": "NN", "bilinear": "BILINEAR"}
    interpolation = getattr(
        keras_layer, "interpolation", "nearest"
    )  # Defaults to 'nearest' for Keras < 2.2.3

    if interpolation not in kerasmode2coreml:
        raise ValueError(
            'Only supported "nearest" or "bilinear" interpolation for upsampling layers.'
        )

    mode = kerasmode2coreml[interpolation]

    builder.add_upsample(
        name=layer,
        scaling_factor_h=fh,
        scaling_factor_w=fw,
        input_name=input_name,
        output_name=output_name,
        mode=mode,
    )


def convert_permute(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert a softmax layer from keras to coreml.

    Parameters
    keras_layer: layer
    ----------
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Ignored.
    """
    input_name, output_name = (input_names[0], output_names[0])

    keras_dims = keras_layer.dims
    # Keras permute layer index begins at 1
    if len(keras_dims) == 3:
        # Keras input tensor interpret as (H,W,C)
        x = list(_np.array(keras_dims))
        arr = [2, 3, 1]  # HWC in Keras
        arr_permuted = [arr[x[0] - 1], arr[x[1] - 1], arr[x[2] - 1]]
        arr_permuted = [
            arr_permuted[2],
            arr_permuted[0],
            arr_permuted[1],
        ]  # coreml format: channel first
        # add a sequence axis
        dim = [0] + arr_permuted
        dim = tuple(dim)
    elif len(keras_dims) == 4:
        # Here we use Keras converter as a place holder for inserting
        # permutations - the values here are not valid Keras dim parameters
        # but parameters we need to use to convert to CoreML model
        dim = keras_dims
    else:
        raise NotImplementedError("Supports only 3d permutation.")

    builder.add_permute(
        name=layer, dim=dim, input_name=input_name, output_name=output_name
    )


def convert_reshape(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    respect_train: boolean
        Ignored.
    """

    input_name, output_name = (input_names[0], output_names[0])

    input_shape = keras_layer.input_shape
    target_shape = keras_layer.target_shape

    def get_coreml_target_shape(target_shape):
        if len(target_shape) == 1:  # (D,)
            coreml_shape = (1, target_shape[0], 1, 1)
        elif len(target_shape) == 2:  # (S,D)
            coreml_shape = target_shape + (1, 1)
        elif len(target_shape) == 3:  # (H,W,C)
            coreml_shape = (1, target_shape[2], target_shape[0], target_shape[1])
        else:
            coreml_shape = None
        return coreml_shape

    def get_mode(input_shape, target_shape):
        in_shape = input_shape[1:]
        if len(in_shape) == 3 or len(target_shape) == 3:
            return 1
        else:
            return 0

    new_shape = get_coreml_target_shape(target_shape)
    if new_shape is not None:
        mode = get_mode(input_shape, target_shape)
        builder.add_reshape(
            name=layer,
            input_name=input_name,
            output_name=output_name,
            target_shape=new_shape,
            mode=mode,
        )
    else:
        _utils.raise_error_unsupported_categorical_option(
            "input_shape", str(input_shape), "reshape", layer
        )


def convert_simple_rnn(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert an SimpleRNN layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether to honor Keras' "trainable" flag (unsupported).
    """
    # Get input and output names
    hidden_size = keras_layer.units
    input_size = keras_layer.input_shape[-1]

    output_all = keras_layer.return_sequences
    reverse_input = keras_layer.go_backwards

    W_h = _np.zeros((hidden_size, hidden_size))
    W_x = _np.zeros((hidden_size, input_size))
    b = None

    implementation = (
        keras_layer.implementation if hasattr(keras_layer, "implementation") else 0
    )
    if implementation == 0:
        W_h = keras_layer.get_weights()[1].T
        W_x = keras_layer.get_weights()[0].T
        if keras_layer.use_bias:
            b = keras_layer.get_weights()[2]

    # Set actication type
    activation_str = _get_recurrent_activation_name_from_keras(keras_layer.activation)

    # Add to the network
    builder.add_simple_rnn(
        name=layer,
        W_h=W_h,
        W_x=W_x,
        b=b,
        hidden_size=hidden_size,
        input_size=input_size,
        activation=activation_str,
        input_names=input_names,
        output_names=output_names,
        output_all=output_all,
        reverse_input=reverse_input,
    )

    if respect_train and keras_layer.trainable:
        logging.warning(
            "RNN layer '%s' is marked updatable, but Core "
            "ML does not yet support updating layers of this "
            "type. The layer will be frozen in Core ML.",
            layer,
        )


def convert_lstm(builder, layer, input_names, output_names, keras_layer, respect_train):
    """
    Convert an LSTM layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether to honor Keras' "trainable" flag (unsupported).
    """

    hidden_size = keras_layer.units
    input_size = keras_layer.input_shape[-1]
    output_all = keras_layer.return_sequences
    reverse_input = keras_layer.go_backwards

    # Keras: [W_x, W_h, b] each in I F C O
    # CoreML: I F O G; W_h and W_x are separated
    W_h, W_x, b = ([], [], [])
    keras_W_h = keras_layer.get_weights()[1].T
    W_h.append(keras_W_h[0 * hidden_size :][:hidden_size])
    W_h.append(keras_W_h[1 * hidden_size :][:hidden_size])
    W_h.append(keras_W_h[3 * hidden_size :][:hidden_size])
    W_h.append(keras_W_h[2 * hidden_size :][:hidden_size])

    keras_W_x = keras_layer.get_weights()[0].T
    W_x.append(keras_W_x[0 * hidden_size :][:hidden_size])
    W_x.append(keras_W_x[1 * hidden_size :][:hidden_size])
    W_x.append(keras_W_x[3 * hidden_size :][:hidden_size])
    W_x.append(keras_W_x[2 * hidden_size :][:hidden_size])
    if keras_layer.use_bias:
        keras_b = keras_layer.get_weights()[2]
        b.append(keras_b[0 * hidden_size :][:hidden_size])
        b.append(keras_b[1 * hidden_size :][:hidden_size])
        b.append(keras_b[3 * hidden_size :][:hidden_size])
        b.append(keras_b[2 * hidden_size :][:hidden_size])
    if len(b) == 0:
        b = None

    # Set activation type
    inner_activation_str = _get_recurrent_activation_name_from_keras(
        keras_layer.recurrent_activation
    )
    activation_str = _get_recurrent_activation_name_from_keras(keras_layer.activation)

    # Add to the network
    builder.add_unilstm(
        name=layer,
        W_h=W_h,
        W_x=W_x,
        b=b,
        hidden_size=hidden_size,
        input_size=input_size,
        input_names=input_names,
        output_names=output_names,
        inner_activation=inner_activation_str,
        cell_state_update_activation=activation_str,
        output_activation=activation_str,
        output_all=output_all,
        forget_bias=keras_layer.unit_forget_bias,
        reverse_input=reverse_input,
    )

    if respect_train and keras_layer.trainable:
        logging.warning(
            "LSTM layer '%s' is marked updatable, but Core "
            "ML does not yet support updating layers of this "
            "type. The layer will be frozen in Core ML.",
            layer,
        )


def convert_gru(builder, layer, input_names, output_names, keras_layer, respect_train):
    """
    Convert a GRU layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether to honor Keras' "trainable" flag (unsupported).
    """

    hidden_size = keras_layer.units
    input_size = keras_layer.input_shape[-1]

    output_all = keras_layer.return_sequences
    reverse_input = keras_layer.go_backwards

    # Keras: Z R O
    # CoreML: Z R O
    W_h, W_x, b = ([], [], [])
    keras_W_h = keras_layer.get_weights()[1].T
    W_h.append(keras_W_h[0 * hidden_size :][:hidden_size])
    W_h.append(keras_W_h[1 * hidden_size :][:hidden_size])
    W_h.append(keras_W_h[2 * hidden_size :][:hidden_size])

    keras_W_x = keras_layer.get_weights()[0].T
    W_x.append(keras_W_x[0 * hidden_size :][:hidden_size])
    W_x.append(keras_W_x[1 * hidden_size :][:hidden_size])
    W_x.append(keras_W_x[2 * hidden_size :][:hidden_size])

    if keras_layer.use_bias:
        keras_b = keras_layer.get_weights()[2]
        b.append(keras_b[0 * hidden_size :][:hidden_size])
        b.append(keras_b[1 * hidden_size :][:hidden_size])
        b.append(keras_b[2 * hidden_size :][:hidden_size])
    if len(b) == 0:
        b = None

    # Set actication type
    inner_activation_str = _get_recurrent_activation_name_from_keras(
        keras_layer.recurrent_activation
    )
    activation_str = _get_recurrent_activation_name_from_keras(keras_layer.activation)

    # Add to the network
    builder.add_gru(
        name=layer,
        W_h=W_h,
        W_x=W_x,
        b=b,
        input_size=input_size,
        hidden_size=hidden_size,
        input_names=input_names,
        output_names=output_names,
        activation=activation_str,
        inner_activation=inner_activation_str,
        output_all=output_all,
        reverse_input=reverse_input,
    )

    if respect_train and keras_layer.trainable:
        logging.warning(
            "GRU layer '%s' is marked updatable, but Core "
            "ML does not yet support updating layers of this "
            "type. The layer will be frozen in Core ML.",
            layer,
        )


def convert_bidirectional(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    Convert a bidirectional layer from keras to coreml.
    Currently assumes the units are LSTMs.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    respect_train: boolean
        Whether to honor Keras' "trainable" flag (unsupported).
    """

    input_size = keras_layer.input_shape[-1]

    lstm_layer = keras_layer.forward_layer
    if type(lstm_layer) != _keras.layers.recurrent.LSTM:
        raise TypeError("Bidirectional layers only supported with LSTM")

    if lstm_layer.go_backwards:
        raise TypeError(" 'go_backwards' mode not supported with Bidirectional layers")

    output_all = keras_layer.return_sequences
    hidden_size = lstm_layer.units

    # Keras: I C F O; W_x, W_h, b
    # CoreML: I F O G; W_h and W_x are separated
    # Keras has all forward weights, followed by backward in the same order
    W_h, W_x, b = ([], [], [])
    keras_W_h = keras_layer.forward_layer.get_weights()[1].T
    W_h.append(keras_W_h[0 * hidden_size :][:hidden_size])
    W_h.append(keras_W_h[1 * hidden_size :][:hidden_size])
    W_h.append(keras_W_h[3 * hidden_size :][:hidden_size])
    W_h.append(keras_W_h[2 * hidden_size :][:hidden_size])

    keras_W_x = keras_layer.forward_layer.get_weights()[0].T
    W_x.append(keras_W_x[0 * hidden_size :][:hidden_size])
    W_x.append(keras_W_x[1 * hidden_size :][:hidden_size])
    W_x.append(keras_W_x[3 * hidden_size :][:hidden_size])
    W_x.append(keras_W_x[2 * hidden_size :][:hidden_size])

    if keras_layer.forward_layer.use_bias:
        keras_b = keras_layer.forward_layer.get_weights()[2]
        b.append(keras_b[0 * hidden_size :][:hidden_size])
        b.append(keras_b[1 * hidden_size :][:hidden_size])
        b.append(keras_b[3 * hidden_size :][:hidden_size])
        b.append(keras_b[2 * hidden_size :][:hidden_size])

    if len(b) == 0:
        b = None

    W_h_back, W_x_back, b_back = ([], [], [])
    keras_W_h = keras_layer.backward_layer.get_weights()[1].T
    W_h_back.append(keras_W_h[0 * hidden_size :][:hidden_size])
    W_h_back.append(keras_W_h[1 * hidden_size :][:hidden_size])
    W_h_back.append(keras_W_h[3 * hidden_size :][:hidden_size])
    W_h_back.append(keras_W_h[2 * hidden_size :][:hidden_size])

    keras_W_x = keras_layer.backward_layer.get_weights()[0].T
    W_x_back.append(keras_W_x[0 * hidden_size :][:hidden_size])
    W_x_back.append(keras_W_x[1 * hidden_size :][:hidden_size])
    W_x_back.append(keras_W_x[3 * hidden_size :][:hidden_size])
    W_x_back.append(keras_W_x[2 * hidden_size :][:hidden_size])

    if keras_layer.backward_layer.use_bias:
        keras_b = keras_layer.backward_layer.get_weights()[2]
        b_back.append(keras_b[0 * hidden_size :][:hidden_size])
        b_back.append(keras_b[1 * hidden_size :][:hidden_size])
        b_back.append(keras_b[3 * hidden_size :][:hidden_size])
        b_back.append(keras_b[2 * hidden_size :][:hidden_size])
    if len(b_back) == 0:
        b_back = None

    if (b == None and b_back != None) or (b != None and b_back == None):
        raise ValueError(
            "Unsupported Bi-directional LSTM configuration. Bias "
            "must be enabled/disabled for both directions."
        )

    # Set activation type
    inner_activation_str = _get_recurrent_activation_name_from_keras(
        lstm_layer.recurrent_activation
    )
    activation_str = _get_recurrent_activation_name_from_keras(lstm_layer.activation)

    output_name_1 = output_names[0]
    if hasattr(keras_layer, "merge_mode"):
        merge_mode = keras_layer.merge_mode
        if merge_mode not in ["concat", "sum", "mul", "ave"]:
            raise NotImplementedError(
                "merge_mode '%s' in Bidirectional LSTM "
                "not supported currently" % merge_mode
            )
        if merge_mode != "concat":
            output_name_1 += "_concatenated_bilstm_output"

    # Add to the network
    builder.add_bidirlstm(
        name=layer,
        W_h=W_h,
        W_x=W_x,
        b=b,
        W_h_back=W_h_back,
        W_x_back=W_x_back,
        b_back=b_back,
        hidden_size=hidden_size,
        input_size=input_size,
        input_names=input_names,
        output_names=[output_name_1] + output_names[1:],
        inner_activation=inner_activation_str,
        cell_state_update_activation=activation_str,
        output_activation=activation_str,
        forget_bias=lstm_layer.unit_forget_bias,
        output_all=output_all,
    )

    if output_name_1 != output_names[0]:
        mode = "CONCAT"
        if merge_mode == "sum":
            mode = "ADD"
        elif merge_mode == "ave":
            mode = "AVE"
        elif merge_mode == "mul":
            mode = "MULTIPLY"
        builder.add_split(
            name=layer + "_split",
            input_name=output_name_1,
            output_names=[output_names[0] + "_forward", output_names[0] + "_backward"],
        )
        builder.add_elementwise(
            name=layer + "_elementwise",
            input_names=[output_names[0] + "_forward", output_names[0] + "_backward"],
            output_name=output_names[0],
            mode=mode,
        )

    if respect_train and keras_layer.trainable:
        logging.warning(
            "Bidirectional layer '%s' is marked updatable, but "
            "Core ML does not yet support updating layers of this "
            "type. The layer will be frozen in Core ML.",
            layer,
        )


def convert_repeat_vector(
    builder, layer, input_names, output_names, keras_layer, respect_train
):
    """
    respect_train: boolean
        Ignored.
    """
    # Keras RepeatVector only deals with 1D input
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    builder.add_sequence_repeat(
        name=layer, nrep=keras_layer.n, input_name=input_name, output_name=output_name
    )


def default_skip(builder, layer, input_names, output_names, keras_layer, respect_train):
    """
    Layers that can be skipped.
    """
    return

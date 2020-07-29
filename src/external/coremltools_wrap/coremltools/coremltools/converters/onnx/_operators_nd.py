from __future__ import absolute_import as _
from __future__ import division as _
from __future__ import print_function as _
from __future__ import unicode_literals as _

import numpy as np
import copy

from typing import Sequence, Callable, List, Tuple, Optional, Text, Any
from coremltools.models.neural_network import NeuralNetworkBuilder  # type: ignore
from onnx import TensorProto
from ._graph import Node, Graph
from coremltools.proto import NeuralNetwork_pb2  # type: ignore
from ._error_utils import ErrorHandling

from ._operators import (
    _convert_abs,
    _convert_relu,
    _convert_sqrt,
    _convert_exp,
    _convert_elu,
    _convert_selu,
    _convert_sigmoid,
    _convert_sign,
    _convert_prelu,
    _convert_upsample,
    _convert_softsign,
    _convert_softplus,
    _convert_log,
    _convert_neg,
    _convert_reciprocal,
    _convert_hardsigmoid,
    _convert_reorganize_data,
    _add_pool,
    _get_pool_params,
    _add_conv,
    _get_conv_params,
    _convert_thresholdedrelu,
    _convert_leaky_relu,
    _convert_lrn,
)

from ._operators import _convert_pad as _convert_pad_5d

INT_MAX = 2 ** 63 - 1


## Helper functions
def load_input_constants(builder, node, graph, err):
    for i in range(len(node.inputs)):
        if (
            node.inputs[i] in node.input_tensors
            and node.inputs[i] not in graph.constants_loaded
        ):
            value = node.input_tensors[node.inputs[i]]
            builder.add_load_constant_nd(
                name=node.name + "_load_constant_" + str(i),
                output_name=node.inputs[i],
                constant_value=value,
                shape=[1] if value.shape == () else value.shape,
            )
            graph.constants_loaded.add(node.inputs[i])


def _add_conv_like_op(
    add_func, get_params_func, params_dict, builder, node, graph, err
):
    rank = builder._get_rank(node.inputs[0])
    if rank == 4:
        get_params_func(builder, node, graph, err, params_dict)
        add_func(
            node.inputs,
            node.outputs,
            params_dict=params_dict,
            builder=builder,
            node=node,
            graph=graph,
            err=err,
        )
    elif rank == 3:
        axes = [0, 3]
        # Make 5d tensor
        expanded_node_output = node.name + "_" + node.inputs[0] + "_expanded"
        builder.add_expand_dims(
            name=node.name + "_ip_expand",
            input_name=node.inputs[0],
            output_name=expanded_node_output,
            axes=axes,
        )
        node.inputs[0] = expanded_node_output
        output_name = node.outputs[0]
        node.outputs[0] = node.name + "_" + output_name + "_expanded"
        # Add conversion op
        get_params_func(builder, node, graph, err, params_dict, axis="width")
        add_func(
            node.inputs,
            node.outputs,
            params_dict=params_dict,
            builder=builder,
            node=node,
            graph=graph,
            err=err,
        )
        # Make 3d tensor back
        builder.add_squeeze(
            name=node.name + "_ip_squeeze_out",
            input_name=node.outputs[0],
            output_name=output_name,
            axes=axes,
        )
    else:
        return err.unsupported_op_configuration(
            builder, node, graph, "provided number axes {} not supported".format(rank)
        )


def add_broadcastable_op_chain(builder, node, err, add_op_function):
    """
    Splits list of input into chain of operator with two inputs
    where output of first node is fed into next one until the final input
    is processed
    Pass node:            Node to be converted
         add_op_function: Conversion function to be used
    """
    total_nodes = len(node.inputs)

    if total_nodes < 2:
        # TODO: Skip or CopyProp + DeadCode elimination
        builder.add_activation(
            name=node.name,
            non_linearity="LINEAR",
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            params=[1.0, 0.0],
        )
    elif total_nodes == 2:
        add_op_function(
            name=node.name, input_names=node.inputs, output_name=node.outputs[0]
        )
    else:
        decorator = 0
        out_name = node.outputs[0]
        # Add broadcastable layer for first two inputs
        add_op_function(
            name=node.name,
            input_names=[node.inputs[0], node.inputs[1]],
            output_name=out_name + "_" + str(decorator),
        )
        # Continue chain of broadcastable layers
        for i in range(2, total_nodes - 1):
            add_op_function(
                name=node.name,
                input_names=[out_name + "_" + str(decorator), node.inputs[i]],
                output_name=out_name + "_" + str(decorator + 1),
            )
            decorator += 1
        # End chain of broadcastable layers with final output
        add_op_function(
            name=node.name + "_" + str(decorator),
            input_names=[out_name + "_" + str(decorator), node.inputs[total_nodes - 1]],
            output_name=out_name,
        )


def add_bn_with_expansion(
    builder,
    node,
    err,
    node_name,
    input_name,
    output_name,
    channels,
    scale,
    bias,
    mean=None,
    var=None,
    epsilon=None,
    compute_mean_var=False,
    instance_normalization=False,
    axes_for_expansion=[],
):
    real_input_name = input_name
    real_output_name = output_name

    # Expand input if needed
    if len(axes_for_expansion) != 0:
        input_name = node_name + "_" + input_name + "_expanded"
        output_name = output_name + "_expanded"
        builder.add_expand_dims(
            name=node_name + "_expand",
            input_name=real_input_name,
            output_name=input_name,
            axes=axes_for_expansion,
        )

    builder.add_batchnorm(
        name=node.name,
        channels=channels,
        gamma=scale,
        beta=bias,
        mean=mean,
        variance=var,
        input_name=input_name,
        output_name=output_name,
        compute_mean_var=compute_mean_var,
        instance_normalization=instance_normalization,
        epsilon=epsilon,
    )

    # Squeeze output if needed
    if len(axes_for_expansion) != 0:
        builder.add_squeeze(
            name=node_name + "_squeeze",
            input_name=output_name,
            output_name=real_output_name,
            axes=axes_for_expansion,
        )


# Helper function to convert RandomNormal, RandomUniform and it's variants
def add_random(builder, node, graph, err, add_op_function):
    # Ignoring attribute `dtype` as CoreML internally represents tensors into 'Float'
    mean = node.attrs.get("mean", 0.0)
    scale = node.attrs.get("scale", 1.0)
    seed = node.attrs.get("seed", -1)
    shape = node.attrs.get("shape", None)
    if shape is None:
        return err.unsupported_op_configuration(
            builder, node, graph, "Shape not provided"
        )
    add_op_function(
        name=node.name,
        output_name=node.outputs[0],
        output_shape=shape,
        mean=mean,
        stddev=scale,
        seed=seed,
    )


## Converter functions


def _convert_acos(builder, node, graph, err):
    """
    convert to CoreML Acos Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3793
    """
    load_input_constants(builder, node, graph, err)
    builder.add_acos(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_acosh(builder, node, graph, err):
    """
    convert to CoreML Acosh Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3925
    """
    load_input_constants(builder, node, graph, err)
    builder.add_acosh(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_add(builder, node, graph, err):
    """
    convert to CoreML Add Broadcastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4117
    """
    load_input_constants(builder, node, graph, err)
    add_broadcastable_op_chain(builder, node, err, builder.add_add_broadcastable)


def _convert_argmax(builder, node, graph, err):
    """
    convert to CoreML ArgMax Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4961
    """
    axis = node.attrs.get("axis", 0)
    keepdims = node.attrs.get("keepdims", True)
    builder.add_argmax(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        axis=axis,
        keepdims=keepdims,
    )


def _convert_argmin(builder, node, graph, err):
    """
    convert to CoreML ArgMin Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4988
    """
    axis = node.attrs.get("axis", 0)
    keepdims = node.attrs.get("keepdims", True)
    builder.add_argmin(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        axis=axis,
        keepdims=keepdims,
    )


def _convert_asin(builder, node, graph, err):
    """
    convert to CoreML Asin Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3771
    """
    load_input_constants(builder, node, graph, err)
    builder.add_asin(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_asinh(builder, node, graph, err):
    """
    convert to CoreML Asinh Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3903
    """
    load_input_constants(builder, node, graph, err)
    builder.add_asinh(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_atan(builder, node, graph, err):
    """
    convert to CoreML Atan Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3815
    """
    load_input_constants(builder, node, graph, err)
    builder.add_atan(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_atanh(builder, node, graph, err):
    """
    convert to CoreML Atanh Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3947
    """
    load_input_constants(builder, node, graph, err)
    builder.add_atanh(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_bn(builder, node, graph, err):
    """
    convert to CoreML BatchNorm Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L1633
    """
    if len(node.outputs) > 1:
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "This converter only supports BatchNormalization with one output",
        )

    epsilon = node.attrs.get("epsilon", 1e-5)
    scale_name = node.inputs[1]

    if scale_name in node.input_tensors:
        channels = node.input_tensors[scale_name].shape
    elif scale_name in graph.shape_dict:
        channels = graph.shape_dict[scale_name]
    else:
        err.unsupported_op_configuration(
            builder, node, graph, "Input shape not available"
        )

    # TODO: Move error check under VERBOSE / DEBUG Mode
    for i in range(2, len(node.inputs)):
        ip_name = node.inputs[i]
        if ip_name in node.input_tensors:
            tensor_shape = node.input_tensors[ip_name].shape
        else:
            if ip_name not in graph.shape_dict:
                return err.unsupported_op_configuration(
                    builder, node, graph, "Input shape not available"
                )
            tensor_shape = graph.shape_dict[ip_name]
        if tensor_shape != channels:
            err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "Shape mismatch between Scale, Bias, Mean and Variance",
            )

    scale = (
        node.input_tensors[node.inputs[1]]
        if node.inputs[1] in node.input_tensors
        else np.ones(shape=channels, dtype=np.float32)
    )
    bias = (
        node.input_tensors[node.inputs[2]]
        if node.inputs[2] in node.input_tensors
        else np.zeros(shape=channels, dtype=np.float32)
    )
    mean = (
        node.input_tensors[node.inputs[3]]
        if node.inputs[3] in node.input_tensors
        else np.zeros(shape=channels, dtype=np.float32)
    )
    var = (
        node.input_tensors[node.inputs[4]]
        if node.inputs[4] in node.input_tensors
        else np.ones(shape=channels, dtype=np.float32)
    )

    rank = builder._get_rank(node.inputs[0])
    # ONNX converts B x C tensor into B x C x 1 hence
    # Rank 2 BN is mapped to Rank 3 BN
    if rank == 3:
        # 1D Batch Norm
        add_bn_with_expansion(
            builder,
            node,
            err,
            node.name,
            node.inputs[0],
            node.outputs[0],
            channels[0],
            scale,
            bias,
            mean,
            var,
            epsilon,
            axes_for_expansion=[0, 3],
        )
    elif rank == 4:
        # 2D Batch Norm
        add_bn_with_expansion(
            builder,
            node,
            err,
            node.name,
            node.inputs[0],
            node.outputs[0],
            channels[0],
            scale,
            bias,
            mean,
            var,
            epsilon,
            axes_for_expansion=[],
        )
    else:
        # Unsupported 1D, 3D and above
        err.unsupported_op_configuration(
            builder, node, graph, "provided number axes {} not supported".format(rank)
        )


def _convert_cast(builder, node, graph, err):
    """
    Perform cast operation in CoreML
        e.g. Casting from Float (assumed) to Int maps to Floor Layer
             For Others, add copy layer
    """
    convert_to = node.attrs.get("to")
    convert_to_int = set(
        {
            TensorProto.UINT8,
            TensorProto.INT8,
            TensorProto.UINT16,
            TensorProto.INT32,
            TensorProto.INT64,
            TensorProto.UINT32,
            TensorProto.UINT64,
        }
    )

    ## TODO: Add support for conversion from STRING TO FLOAT
    ## Currently, such input will error out in parsing
    if convert_to in convert_to_int:
        builder.add_floor(
            name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
        )
    else:
        load_input_constants(builder, node, graph, err)
        builder.add_activation(
            name=node.name,
            non_linearity="LINEAR",
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            params=[1.0, 0.0],
        )


def _convert_ceil(builder, node, graph, err):
    """
    convert to CoreML Ceil Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5018
    """
    builder.add_ceil(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0],
    )


def _convert_clip(builder, node, graph, err):
    """
    convert to CoreML Clip Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5066
    """
    max_value = node.attrs.get("max", 3.4028234663852886e38)
    min_value = node.attrs.get("min", -3.4028234663852886e38)
    builder.add_clip(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        min_value=min_value,
        max_value=max_value,
    )


def _convert_concat(builder, node, graph, err):
    """
    convert to CoreML ConcatND Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3521
    """
    axis = node.attrs.get("axis")
    load_input_constants(builder, node, graph, err)

    # TODO: Adding Linear layer will change to
    #       either: Skip the op right away
    #       or:     Insert Linear and perform copy-propogation followed by dead code elimination
    if len(node.inputs) == 1:
        builder.add_activation(
            name=node.name,
            non_linearity="LINEAR",
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            params=[1.0, 0.0],
        )
    else:
        builder.add_concat_nd(
            name=node.name,
            input_names=node.inputs,
            output_name=node.outputs[0],
            axis=axis,
        )


def _convert_constant(builder, node, graph, err):
    """
    convert to CoreML Load Constant ND Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3596
    """
    value = node.attrs["value"]
    # HACK: If Value is 0-Rank then make it 1-Rank
    builder.add_load_constant_nd(
        name=node.name,
        output_name=node.outputs[0],
        constant_value=value,
        shape=[1] if value.shape == () else value.shape,
    )
    graph.constants_loaded(node.outputs[0])


def _convert_constant_of_shape(builder, node, graph, err):
    """
    convert to CoreML Fill Static Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3641
    """
    value = node.attrs.get("value", [0.0])
    # if shape is known, create tensor of given shape
    # otherwise create tensor at runtime
    if node.inputs[0] in node.input_tensors:
        output_shape = node.input_tensors[node.inputs[0]]
        # add_fill_static requires shape to be more than rank-1
        if len(output_shape.shape) == 1:
            output_shape = output_shape.reshape(output_shape.shape[0], 1)
        builder.add_fill_static(
            name=node.name,
            output_name=node.outputs[0],
            output_shape=output_shape,
            value=value[0],
        )
    else:
        builder.add_fill_dynamic(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            value=value[0],
        )


def _convert_conv(builder, node, graph, err):
    """
    convert to CoreML Convolution Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L1418
    """
    params_dict = dict()
    params_dict["is_deconv"] = False
    if node.op_type.endswith("Transpose"):
        params_dict["is_deconv"] = True
    # get weights for convolution
    weight_name = node.inputs[1]
    W = None
    if weight_name in node.input_tensors:
        W = node.input_tensors[weight_name]
        params_dict["w_shape"] = W.shape
    else:
        # W is provided as a input
        # Make W compatible for CoreML Conv Layer
        # W ONNX format: OC x KC x H x W
        # Expected CoreML Format: H x W x KC x OC
        W_name = node.inputs[1]
        W_shape = graph.shape_dict[W_name]
        W_rank = len(W_shape)

        params_dict["w_shape"] = W_shape
        if W_rank == 3:
            expanded_node_name = node.name + "_" + W_name + "_expanded"
            builder.add_expand_dims(
                name=node.name + "_w_expand",
                input_name=W_name,
                output_name=expanded_node_name,
                axes=[-2],
            )
            W_name = expanded_node_name

        # Now Permute the W tensor
        W_transpose_axes = [2, 3, 1, 0]
        # If ConvTranpose then, Kernel and Output channels are shuffled
        if params_dict["is_deconv"]:
            W_transpose_axes = [2, 3, 0, 1]

        builder.add_transpose(
            name=node.name + "_w_transpose",
            axes=W_transpose_axes,
            input_name=W_name,
            output_name=W_name + "_transposed",
        )
        W_name = W_name + "_transposed"
        node.inputs[1] = W_name

    params_dict["W"] = W
    bias = None
    if len(node.inputs) > 2:
        bias = node.input_tensors[node.inputs[2]]
    params_dict["bias"] = bias
    params_dict["groups"] = node.attrs.get("group", 1)

    _add_conv_like_op(
        _add_conv, _get_conv_params, params_dict, builder, node, graph, err
    )


def _convert_cos(builder, node, graph, err):
    """
    convert to CoreML Cos Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3727
    """
    load_input_constants(builder, node, graph, err)
    builder.add_cos(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_cosh(builder, node, graph, err):
    """
    convert to CoreML Cosh Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3859
    """
    load_input_constants(builder, node, graph, err)
    builder.add_cosh(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_div(builder, node, graph, err):
    """
    convert to CoreML Divide Broadcastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4180
    """
    load_input_constants(builder, node, graph, err)
    add_broadcastable_op_chain(builder, node, err, builder.add_divide_broadcastable)


def _convert_equal(builder, node, graph, err):
    """
    convert to CoreML Equal Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L961
    """
    load_input_constants(builder, node, graph, err)
    builder.add_equal(
        name=node.name, input_names=node.inputs, output_name=node.outputs[0]
    )


def _convert_erf(builder, node, graph, err):
    """
    convert to CoreML Erf Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5140
    """
    load_input_constants(builder, node, graph, err)
    builder.add_erf(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_expand(builder, node, graph, err):
    """
    convert to CoreML Broadcast To Static/Dynamic Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4086
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4108
    """
    load_input_constants(builder, node, graph, err)
    if node.inputs[1] in node.input_tensors:
        output_shape = node.input_tensors[node.inputs[1]].astype(np.int64)
        builder.add_broadcast_to_static(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            output_shape=output_shape,
        )
    else:
        builder.add_broadcast_to_dynamic(
            name=node.name, input_names=node.inputs, output_name=node.outputs[0],
        )


def _convert_flatten(builder, node, graph, err):
    """
    convert to CoreML Flatten Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4826
    """
    axis = node.attrs.get("axis", 1)
    builder.add_flatten_to_2d(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        axis=axis,
    )


def _convert_floor(builder, node, graph, err):
    """
    convert to CoreML Floor Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5040
    """
    builder.add_floor(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_gather(builder, node, graph, err):
    """
    convert to CoreML Gather Along Axis Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4296
    """
    axis = node.attrs.get("axis", 0)

    if len(node.inputs) != 2:
        err.unsupported_op_configuration(
            builder, node, graph, "Error in ONNX model: Gather expects two inputs"
        )

    if (
        node.inputs[0] in node.input_tensors
        and node.inputs[0] not in graph.constants_loaded
    ):
        value = node.input_tensors[node.inputs[0]]
        builder.add_load_constant_nd(
            name=node.name + "_load_data",
            output_name=node.inputs[0],
            constant_value=value,
            shape=[1] if value.shape == () else value.shape,
        )
        graph.constants_loaded.add(node.inputs[0])

    if (
        node.inputs[1] in node.input_tensors
        and node.inputs[1] not in graph.constants_loaded
    ):
        value = node.input_tensors[node.inputs[1]]
        builder.add_load_constant_nd(
            name=node.name + "_load_indices",
            output_name=node.inputs[1],
            constant_value=value,
            shape=[1] if value.shape == () else value.shape,
        )
        graph.constants_loaded.add(node.inputs[1])

    builder.add_gather(
        name=node.name,
        input_names=[node.inputs[0], node.inputs[1]],
        output_name=node.outputs[0],
        axis=axis,
    )


def _convert_gemm(builder, node, graph, err):
    """
    convert to CoreML Tranpose (Optional) and Inner Product Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4180
    """
    # Read attributes
    alpha = node.attrs.get("alpha", 1.0)
    beta = node.attrs.get("beta", 1.0)
    transA = node.attrs.get("transA", False)
    transB = node.attrs.get("transB", False)

    A = node.inputs[0]
    if A in node.input_tensors:
        A_tensor = node.input_tensors[A]
        builder.add_load_constant_nd(
            name=node.name + A + "_const",
            output_name="const_" + A,
            constant_value=A_tensor,
            shape=A_tensor.shape,
        )
        A = "const_" + A

    if alpha != 1.0:
        builder.add_load_constant_nd(
            name=node.name + "_load_alpha",
            output_name="alpha_for_" + A,
            constant_value=np.array([alpha]),
            shape=[1],
        )
        builder.add_multiply_broadcastable(
            name=node.name + "_alphaA",
            input_names=[A, "alpha_for_" + A],
            output_name=A + "_alphaA",
        )
        A = A + "_alphaA"

    B = node.inputs[1]
    C = node.inputs[2]
    if B in node.input_tensors and C in node.input_tensors:
        B = node.input_tensors[B]
        C = node.input_tensors[C]

        if transB:
            B = B.transpose()

        C = C.flatten()
        builder.add_batched_mat_mul(
            name=node.name,
            input_names=[A],
            output_name=node.outputs[0],
            transpose_a=transA,
            weight_matrix_rows=B.shape[0],
            weight_matrix_columns=B.shape[1],
            W=B,
            bias=C,
        )
    else:
        ## TODO: Test coverage when B and C are non-constant
        ## Should C be of Rank-1? or it's okay to keep it that way?
        if beta != 1.0:
            builder.add_load_constant_nd(
                name=node.name + "_load_beta",
                output_name="beta_for_" + B,
                constant_value=np.array([beta]),
                shape=[1],
            )
            builder.add_multiply_broadcastable(
                name=node.name + "_betaC",
                input_names=[C, "beta_for_" + B],
                output_name=C + "_betaC",
            )
            C = C + "_betaC"

        builder.add_batched_mat_mul(
            name=node.name,
            input_names=[A, B],
            output_name=node.outputs[0] + "_b_mat_mul",
            transpose_a=transA,
            transpose_b=transB,
        )

        builder.add_add_broadcastable(
            name=node.name + "_add_bias",
            input_names=[node.outputs[0] + "_b_mat_mul", C],
            output_name=node.outputs[0],
        )


def _convert_greater(builder, node, graph, err):
    """
    convert to CoreML Greater than Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L853
    """
    load_input_constants(builder, node, graph, err)
    builder.add_greater_than(
        name=node.name, input_names=node.inputs, output_name=node.outputs[0],
    )


def _convert_gru(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    """
    convert to CoreML GRU Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3104
    """

    def get_weights(W, W_name, R, R_name, B):
        """
        Helper routine to return weights in CoreML LSTM required format
        """
        W = np.expand_dims(np.expand_dims(W, 3), 3)
        R = np.expand_dims(np.expand_dims(R, 3), 3)

        if W is None:
            err.missing_initializer(
                node,
                "Weight tensor: {} not found in the graph initializer".format(W_name),
            )
        if R is None:
            err.missing_initializer(
                node,
                "Weight tensor: {} not found in the graph initializer".format(R_name),
            )

        W_z, W_r, W_h = np.split(np.squeeze(W), 3)  # type: ignore
        R_z, R_r, R_h = np.split(np.squeeze(R), 3)  # type: ignore

        W_x = [W_z, W_r, W_h]
        W_h = [R_z, R_r, R_h]
        b = None
        if B is not None:
            b_Wz, b_Wr, b_Wh, b_Rz, b_Rr, b_Rh = np.split(np.squeeze(B), 6)  # type: ignore
            b = [b_Wz + b_Rz, b_Wr + b_Rr, b_Wh + b_Rh]

        return W_x, W_h, b

    def expand_dim(node_name, input_name, output_name, axes):
        builder.add_expand_dims(
            name=node_name, input_name=input_name, output_name=output_name, axes=axes
        )

    # Read attributes
    # activation alpha and beta
    if "activation_alpha" in node.attrs or "activation_beta" in node.attrs:
        err.unsupported_feature_warning(
            node, "Activation parameter alpha and beta are currently not used"
        )

    inner_activation = "SIGMOID"
    output_activation = "TANH"

    if "activations" in node.attrs:
        activations_list = node.attrs["activations"]

        if len(activations_list) < 2:
            err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "Error in ONNX model: Less number of activations provided",
            )

        inner_activation = activations_list[0].upper()
        output_activation = activations_list[1].upper()

    # Extract direction from ONNX attribute
    direction = node.attrs.get("direction", "forward")
    if direction == "bidirectional":
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "Bidirectional GRU not supported!! Please consider adding custom conversion function/layer",
        )

    hidden_size = node.attrs.get("hidden_size")

    # Read inputs
    W_name = node.inputs[1]
    R_name = node.inputs[2]
    B = None
    if len(node.inputs) > 3:
        B_name = node.inputs[3]
        B = node.input_tensors.get(B_name, None)

    if W_name not in node.input_tensors or R_name not in node.input_tensors:
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "Input and Recursion weights must be known!! Please consider adding custom conversion function/layer",
        )

    W = node.input_tensors.get(W_name, None)
    R = node.input_tensors.get(R_name, None)

    # Get weights for forward direction
    W_x, W_h, b = get_weights(W, W_name, R, R_name, B)

    # shape of input
    input_size = W_x[0].shape[1]

    # Get input and output for hidden and cell
    input_h = node.inputs[5] if len(node.inputs) > 5 else node.inputs[0] + "_h_input"
    output_h = (
        node.outputs[1] if len(node.outputs) > 1 else node.outputs[0] + "_h_output"
    )
    output_h_5d = output_h + "_5d"

    if len(node.inputs) < 6:
        # if input is not present in the network, load they as constant
        if node.inputs[0] not in graph.shape_dict:
            err.unsupported_op_configuration(
                builder, node, graph, "Input shape not represented within Graph"
            )

        # Input is represented as [Seq Len, Batch Size, Input Size]
        batch_size = graph.shape_dict[node.inputs[0]][1]
        builder.add_load_constant_nd(
            name=node.name + "_load_initial_h",
            output_name=input_h,
            constant_value=0.0,
            shape=[1, batch_size, hidden_size],
        )

    # CoreML GRU expects 5-d tensor
    # Expand dimensions of input to 5-d for compatibility
    input_rank = builder._get_rank(node.inputs[0])
    if input_rank == -1:
        return err.unsupported_op_configuration(
            builder, node, graph, "Rank unknown for input"
        )

    if input_rank < 5:
        add_nodes = 5 - input_rank

        # TODO: Add one expand instead of adding one after another for input, h
        expand_dim(
            node.name + "_expand_in_0",
            node.inputs[0],
            node.inputs[0] + "_expand_out_0",
            [input_rank],
        )
        expand_dim(
            node.name + "_expand_in_h_0",
            input_h,
            input_h + "_expand_out_h_0",
            [input_rank],
        )

        for i in range(1, add_nodes):
            i_str = str(i)
            i_p_str = str(i - 1)
            expand_dim(
                node.name + "_expand_in_" + i_str,
                node.inputs[0] + "_expand_out_" + i_p_str,
                node.inputs[0] + "_expand_out_" + i_str,
                [input_rank + i],
            )
            expand_dim(
                node.name + "_expand_in_h_" + i_str,
                input_h + "_expand_out_h_" + i_p_str,
                input_h + "_expand_out_h_" + i_str,
                [input_rank + i],
            )

    builder.add_gru(
        name=node.name,
        W_h=W_h,
        W_x=W_x,
        b=b,
        hidden_size=hidden_size,
        input_size=input_size,
        input_names=[
            node.inputs[0] + "_expand_out_" + str(add_nodes - 1),
            input_h + "_expand_out_h_" + str(add_nodes - 1),
        ],
        output_names=[node.outputs[0] + "_5d_out", output_h_5d],
        inner_activation=inner_activation,
        activation=output_activation,
        output_all=True,
        reverse_input=(direction == "reverse"),
    )

    # CoreML output is [Seq Len, Batch Size, Num Dir * Hidden Size, 1, 1]
    # Return output as [Seq Len, Num Dir, Batch Size, Hidden Size]
    # Following steps:
    #       a. Reshape and split hidden size for direction [Seq Len, Batch Size, Num Dir, Hidden Size, 1]
    #       b. Squeeze last dimension [Seq Len, Batch Size, Num Dir, Hidden Size]
    #       c. Permute to fix the order [Seq Len, Num Dir, Batch Size, Hidden Size, 1]
    builder.add_rank_preserving_reshape(
        name=node.name + "_reshape_",
        input_name=node.outputs[0] + "_5d_out",
        output_name=node.outputs[0] + "_5d_reshaped",
        output_shape=[0, 0, 1, -1, 0],
    )

    builder.add_squeeze(
        name=node.name + "_squeeze_out",
        input_name=node.outputs[0] + "_5d_reshaped",
        output_name=node.outputs[0] + "_4d",
        axes=[-1],
    )

    builder.add_transpose(
        name=node.name + "_transpose",
        axes=[0, 2, 1, 3],
        input_name=node.outputs[0] + "_4d",
        output_name=node.outputs[0],
    )

    # Squeeze dimensions of output_h
    builder.add_squeeze(
        name=node.name + "_squeeze_out_h",
        input_name=output_h_5d,
        output_name=output_h,
        axes=[-1, -2],
    )


def _convert_identity(builder, node, graph, err):
    """
    convert to CoreML Linear Activation Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L417
    """
    # TODO: Skip or CopyProp + DeadCode elimination
    builder.add_activation(
        name=node.name,
        non_linearity="LINEAR",
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        params=[1.0, 0.0],
    )


def _convert_instancenorm(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    """
    convert to CoreML BatchNorm Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L1633
    """
    epsilon = node.attrs.get("epsilon", 1e-5)
    if (
        node.inputs[1] not in node.input_tensors
        or node.inputs[2] not in node.input_tensors
    ):
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "CoreML InstanceNorm requires Scale and Bias to be known",
        )

    scale = node.input_tensors[node.inputs[1]]
    bias = node.input_tensors[node.inputs[2]]

    rank = builder._get_rank(node.inputs[0])
    # ONNX converts B x C tensor into B x C x 1 hence
    # Rank 2 BN is mapped to Rank 3 BN
    if rank == 3:
        # 1D Batch Norm
        add_bn_with_expansion(
            builder,
            node,
            err,
            node.name,
            node.inputs[0],
            node.outputs[0],
            scale.shape[0],
            scale,
            bias,
            epsilon=epsilon,
            compute_mean_var=True,
            instance_normalization=True,
            axes_for_expansion=[0, 3],
        )
    elif rank == 4:
        # 2D Batch Norm
        add_bn_with_expansion(
            builder,
            node,
            err,
            node.name,
            node.inputs[0],
            node.outputs[0],
            scale.shape[0],
            scale,
            bias,
            epsilon=epsilon,
            compute_mean_var=True,
            instance_normalization=True,
            axes_for_expansion=[],
        )
    else:
        # Unsupported 1D, 3D and above
        err.unsupported_op_configuration(
            builder, node, graph, "provided number axes {} not supported".format(rank)
        )


def _convert_less(builder, node, graph, err):
    """
    convert to CoreML Less Than Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L907
    """
    load_input_constants(builder, node, graph, err)
    builder.add_less_than(
        name=node.name, input_names=node.inputs, output_name=node.outputs[0],
    )


def _convert_lstm(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    """
    convert to CoreML Uni/Bi-Directional LSTM Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3282
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3348
    """

    def get_weights(W, W_name, R, R_name, B):
        """
        Helper routine to return weights in CoreML LSTM required format
        """
        W = np.expand_dims(np.expand_dims(W, 3), 3)
        R = np.expand_dims(np.expand_dims(R, 3), 3)

        if W is None:
            err.missing_initializer(
                node,
                "Weight tensor: {} not found in the graph initializer".format(W_name),
            )
        if R is None:
            err.missing_initializer(
                node,
                "Weight tensor: {} not found in the graph initializer".format(R_name),
            )

        W_i, W_o, W_f, W_c = np.split(np.squeeze(W), 4)  # type: ignore
        R_i, R_o, R_f, R_c = np.split(np.squeeze(R), 4)  # type: ignore

        W_x = [W_i, W_f, W_o, W_c]
        W_h = [R_i, R_f, R_o, R_c]
        b = None
        if B is not None:
            b_Wi, b_Wo, b_Wf, b_Wc, b_Ri, b_Ro, b_Rf, b_Rc = np.split(np.squeeze(B), 8)  # type: ignore
            b = [b_Wi + b_Ri, b_Wf + b_Rf, b_Wo + b_Ro, b_Wc + b_Rc]

        return W_x, W_h, b

    def expand_dim(node_name, input_name, output_name, axes):
        builder.add_expand_dims(
            name=node_name, input_name=input_name, output_name=output_name, axes=axes
        )

    # Read attributes
    # activation alpha and beta
    if "activation_alpha" in node.attrs or "activation_beta" in node.attrs:
        err.unsupported_feature_warning(
            node, "Activation parameter alpha and beta are currently not used"
        )

    inner_activation = "SIGMOID"
    cell_state_update_activation = "TANH"
    output_activation = "TANH"

    if "activations" in node.attrs:
        activations_list = node.attrs["activations"]

        if len(activations_list) < 3:
            err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "Error in ONNX model: Less number of activations provided",
            )

        if len(activations_list) == 6:
            err.unsupported_feature_warning(
                node, "Forward and backward pass will use same activations."
            )

        inner_activation = activations_list[0].upper()
        cell_state_update_activation = activations_list[1].upper()
        output_activation = activations_list[2].upper()

    # Provide max Clip Value if not provided
    clip_threshold = node.attrs.get("clip", 500000.0)

    # Extract direction from ONNX attribute
    direction = 1
    if (
        "direction" in node.attrs
        and node.attrs["direction"].decode("utf-8") == "bidirectional"
    ):
        direction = 2

    hidden_size = node.attrs.get("hidden_size")

    input_forget = node.attrs.get("input_forget", 0) == 1

    # Read inputs
    W_name = node.inputs[1]
    R_name = node.inputs[2]
    B = None
    if len(node.inputs) > 3:
        B_name = node.inputs[3]
        B = node.input_tensors.get(B_name, None)

    W = node.input_tensors.get(W_name, None)
    R = node.input_tensors.get(R_name, None)

    W = np.split(W, direction)
    R = np.split(R, direction)
    if B is not None:
        B = np.split(B, direction)
    else:
        B = [None, None]

    # Get weights for forward direction
    W_x, W_h, b = get_weights(W[0], W_name, R[0], R_name, B[0])

    # shape of input
    input_size = W_x[0].shape[1]

    # Get input and output for hidden and cell
    input_h = node.inputs[5] if len(node.inputs) > 5 else node.inputs[0] + "_h_input"
    input_c = node.inputs[6] if len(node.inputs) > 6 else node.inputs[0] + "_c_input"
    output_h = (
        node.outputs[1] if len(node.outputs) > 1 else node.outputs[0] + "_h_output"
    )
    output_c = (
        node.outputs[2] if len(node.outputs) > 2 else node.outputs[0] + "_c_output"
    )
    output_h_5d = output_h + "_5d"
    output_c_5d = output_c + "_5d"

    # if input is not present in the network, load they as constant
    load_input_constants(builder, node, graph, err)

    # Input is represented as [Seq Len, Batch Size, Input Size]
    if len(node.inputs) < 6:
        batch_size = graph.shape_dict[node.inputs[0]][1]
        builder.add_load_constant_nd(
            name=node.name + "_load_initial_h_and_c",
            output_name=input_h,
            constant_value=0.0,
            shape=[direction, batch_size, hidden_size],
        )
        # OPTIMIZATION: let's reuse the intial weights
        input_c = input_h

    # Get tensors for peepholes
    peepholes = node.inputs[7] if len(node.inputs) > 7 else None

    # CoreML LSTM expects 5-d tensor
    # Expand dimensions of input to 5-d for compatibility
    rank = builder._get_rank(node.inputs[0])
    if rank == -1:
        return err.unsupported_op_configuration(
            builder, node, graph, "Rank unknown for input"
        )
    if rank < 5:
        add_nodes = 5 - rank
        # TODO: Add one expand instead of adding one after another for input, h and c
        expand_dim(
            node.name + "_expand_in_0",
            node.inputs[0],
            node.inputs[0] + "_expand_out_0",
            [rank],
        )
        expand_dim(
            node.name + "_expand_in_h_0", input_h, input_h + "_expand_out_h_0", [rank]
        )
        expand_dim(
            node.name + "_expand_in_c_0", input_c, input_c + "_expand_out_c_0", [rank]
        )

        for i in range(1, add_nodes):
            i_str = str(i)
            i_p_str = str(i - 1)
            expand_dim(
                node.name + "_expand_in_" + i_str,
                node.inputs[0] + "_expand_out_" + i_p_str,
                node.inputs[0] + "_expand_out_" + i_str,
                [rank + i],
            )
            expand_dim(
                node.name + "_expand_in_h_" + i_str,
                input_h + "_expand_out_h_" + i_p_str,
                input_h + "_expand_out_h_" + i_str,
                [rank + i],
            )
            expand_dim(
                node.name + "_expand_in_c_" + i_str,
                input_c + "_expand_out_c_" + i_p_str,
                input_c + "_expand_out_c_" + i_str,
                [rank + i],
            )

    if direction == 1:
        # Peephole from ONNX are of shape [Num Dir, 3 * hidden_size]
        # Reshape into CoreML format of [input hs, forget hs, cell hs]
        if peepholes is not None:
            builder.add_reshape_static(
                name=node.name + "_peephole_reshape",
                input_name=peepholes,
                output_name=peepholes + "_reshaped",
                output_shape=[hidden_size, hidden_size, hidden_size],
            )
            peepholes = peepholes + "_reshaped"

        builder.add_unilstm(
            name=node.name,
            W_h=W_h,
            W_x=W_x,
            b=b,
            hidden_size=hidden_size,
            input_size=input_size,
            input_names=[
                node.inputs[0] + "_expand_out_" + str(add_nodes - 1),
                input_h + "_expand_out_h_" + str(add_nodes - 1),
                input_c + "_expand_out_c_" + str(add_nodes - 1),
            ],
            output_names=[node.outputs[0] + "_5d_out", output_h_5d, output_c_5d],
            inner_activation=inner_activation,
            cell_state_update_activation=cell_state_update_activation,
            output_activation=output_activation,
            peep=peepholes,
            output_all=True,
            forget_bias=True,
            coupled_input_forget_gate=input_forget,
            cell_clip_threshold=clip_threshold,
            reverse_input=False,
        )
    elif direction == 2:
        if len(W) != 2 and len(R) != 2 and len(B) != 2:
            err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "Bi-Directional LSTM does not have weights for both the directions",
            )

        W_x_back, W_h_back, b_back = get_weights(W[1], W_name, R[1], R_name, B[1])

        peephole_f = None
        peephole_b = None
        if peepholes is not None:
            builder.add_reshape_static(
                name=node.name + "_peephole_reshape",
                input_name=peepholes,
                output_name=peepholes + "_reshaped",
                output_shape=[direction, hidden_size, hidden_size, hidden_size],
            )

            peepholes_f = peepholes + "_f"
            peepholes_b = peepholes + "_b"

            builder.add_split_nd(
                name=node.name + "_peephole_split",
                input_name=peepholes + "_reshaped",
                output_names=[peepholes_f, peepholes_b],
                axis=0,
            )

        # split input_h and input_c into two parts
        builder.add_split_nd(
            name=node.name + "_split_h",
            input_name=input_h + "_expand_out_h_" + str(add_nodes - 1),
            output_names=[input_h + "_f", input_h + "_b"],
            axis=0,
        )

        # OPTIMIZATION: If input_h and input_c are same
        # Avoid creating new split and instead reuse
        if input_h != input_c:
            builder.add_split_nd(
                name=node.name + "_split_c",
                input_name=input_c + "_expand_out_c_" + str(add_nodes - 1),
                output_names=[input_c + "_f", input_c + "_b"],
                axis=0,
            )

        builder.add_bidirlstm(
            name=node.name,
            W_h=W_h,
            W_x=W_x,
            b=b,
            W_h_back=W_h_back,
            W_x_back=W_x_back,
            b_back=b_back,
            hidden_size=hidden_size,
            input_size=input_size,
            input_names=[
                node.inputs[0] + "_expand_out_" + str(add_nodes - 1),
                input_h + "_f",
                input_c + "_f",
                input_h + "_b",
                input_c + "_b",
            ],
            output_names=[
                node.outputs[0] + "_5d_out",
                output_h + "_f",
                output_c + "_f",
                output_h + "_b",
                output_c + "_b",
            ],
            inner_activation=inner_activation,
            cell_state_update_activation=cell_state_update_activation,
            output_activation=output_activation,
            output_all=True,
            peep=peephole_f,
            peep_back=peephole_b,
            forget_bias=True,
            coupled_input_forget_gate=input_forget,
            cell_clip_threshold=clip_threshold,
        )

        # Combine output_h and output_c
        builder.add_concat_nd(
            name=node.name + "concat_output_h",
            input_names=[output_h + "_f", output_h + "_b"],
            output_name=output_h_5d,
            axis=0,
        )

        builder.add_concat_nd(
            name=node.name + "concat_output_c",
            input_names=[output_c + "_f", output_c + "_b"],
            output_name=output_c_5d,
            axis=0,
        )
    else:
        err.unsupported_op_configuration(
            builder, node, graph, "Unsupported direction {} for LSTM".format(direction)
        )

    # CoreML output is [Seq Len, Batch Size, Num Dir * Hidden Size, 1, 1]
    # Return output as [Seq Len, Num Dir, Batch Size, Hidden Size]
    # Following steps:
    #       a. Reshape and split hidden size for direction [Seq Len, Batch Size, Num Dir, Hidden Size, 1]
    #       b. Squeeze last dimension [Seq Len, Batch Size, Num Dir, Hidden Size]
    #       c. Permute to fix the order [Seq Len, Num Dir, Batch Size, Hidden Size, 1]
    builder.add_rank_preserving_reshape(
        name=node.name + "_reshape_",
        input_name=node.outputs[0] + "_5d_out",
        output_name=node.outputs[0] + "_5d_reshaped",
        output_shape=[0, 0, direction, -1, 0],
    )

    builder.add_squeeze(
        name=node.name + "_squeeze_out",
        input_name=node.outputs[0] + "_5d_reshaped",
        output_name=node.outputs[0] + "_4d",
        axes=[-1],
    )

    builder.add_transpose(
        name=node.name + "_transpose",
        axes=[0, 2, 1, 3],
        input_name=node.outputs[0] + "_4d",
        output_name=node.outputs[0],
    )

    # Squeeze dimensions of output_h and output_c
    builder.add_squeeze(
        name=node.name + "_squeeze_out_h",
        input_name=output_h_5d,
        output_name=output_h,
        axes=[-1, -2],
    )
    builder.add_squeeze(
        name=node.name + "_squeeze_out_c",
        input_name=output_c_5d,
        output_name=output_c,
        axes=[-1, -2],
    )


def _convert_logical(builder, node, graph, err):
    """
    convert to CoreML Logical And/Or/Xor/Not Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L1013
    """
    mode = node.op_type.upper()
    builder.add_logical(
        name=node.name, input_names=node.inputs, output_name=node.outputs[0], mode=mode
    )


def _convert_pad(builder, node, graph, err):
    """
    convert to CoreML Padding / ConstantPadding Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4397
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L1822
    """
    mode = node.attrs.get("mode", "constant")

    try:
        mode = mode.decode()
    except (UnicodeDecodeError, AttributeError):
        pass

    if mode == "constant":
        pads = node.attrs.get("pads", [])
        value = node.attrs.get("value", 0.0)

        builder.add_constant_pad(
            name=node.name,
            input_names=node.inputs,
            output_name=node.outputs[0],
            value=value,
            pad_to_given_output_size_mode=False,
            pad_amounts=pads,
        )
    else:
        _convert_pad_5d(builder, node, graph, err)


def _convert_matmul(builder, node, graph, err):
    """
    convert to CoreML BatchedMatMul Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3473
    """
    weight_name = node.inputs[1]
    W = None
    weight_as_layer_parameter = False
    if weight_name in node.input_tensors:
        W = node.input_tensors[weight_name]

    if W is not None:
        if len(W.shape) != 2:
            # since weight as parameter in batchedMatMul layer must be rank 2
            builder.add_load_constant_nd(
                node.name + "_const_weight_input",
                weight_name,
                constant_value=W,
                shape=W.shape,
            )
        else:
            weight_as_layer_parameter = True

    if weight_as_layer_parameter:
        builder.add_batched_mat_mul(
            name=node.name,
            input_names=[node.inputs[0]],
            output_name=node.outputs[0],
            weight_matrix_rows=W.shape[0],
            weight_matrix_columns=W.shape[1],
            W=W,
        )
    else:
        builder.add_batched_mat_mul(
            name=node.name,
            input_names=[node.inputs[0], weight_name],
            output_name=node.outputs[0],
        )


def _convert_max(builder, node, graph, err):
    """
    convert to CoreML Max Broadcastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4126
    """
    load_input_constants(builder, node, graph, err)
    add_broadcastable_op_chain(builder, node, err, builder.add_max_broadcastable)


def _convert_mean(builder, node, graph, err):
    """
    convert to CoreML Add Broadcastable Layer and Divide BroadCastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4117
    """
    number_of_inputs = len(node.inputs)
    output_name = node.outputs[0]
    node.outputs[0] = node.outputs[0] + "_sum"

    builder.add_load_constant_nd(
        name=node.name + "_divider",
        output_name=output_name + "_divider",
        constant_value=np.array(number_of_inputs),
        shape=[1],
    )
    add_broadcastable_op_chain(builder, node, err, builder.add_add_broadcastable)
    builder.add_divide_broadcastable(
        name=node.name + "_mean",
        input_names=[node.outputs[0], output_name + "_divider"],
        output_name=output_name,
    )


def _convert_pow(builder, node, graph, err):
    """
    convert to CoreML Pow Broadcastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3969
    """
    load_input_constants(builder, node, graph, err)
    add_broadcastable_op_chain(builder, node, err, builder.add_pow_broadcastable)


def _convert_randomnormal(builder, node, graph, err):
    """
    convert to CoreML Random Normal Static Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4457
    """
    add_random(builder, node, graph, err, builder.add_random_normal_static)


def _convert_randomnormallike(builder, node, graph, err):
    """
    convert to CoreML Random Normal Like Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4434
    """
    # Ignoring attribute `dtype` as CoreML internally represents tensors into 'Float'
    mean = node.attributes.get("mean", 0.0)
    scale = node.attributes.get("scale", 1.0)
    seed = node.attributes.get("seed", -1)

    builder.add_random_normal_like(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mean=mean,
        stddev=scale,
        seed=seed,
    )


def _convert_randomuniform(builder, node, graph, err):
    """
    convert to CoreML Random Uniform Static Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4526
    """
    add_random(builder, node, graph, err, builder.random_uniform_static)


def _convert_randomuniformlike(builder, node, graph, err):
    """
    convert to CoreML Random Normal Like Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4503
    """
    # Ignoring attribute `dtype` as CoreML internally represents tensors into 'Float'
    mean = node.attributes.get("mean", 0.0)
    scale = node.attributes.get("scale", 1.0)
    seed = node.attributes.get("seed", -1)

    builder.add_random_uniform_like(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mean=mean,
        stddev=scale,
        seed=seed,
    )


def _convert_min(builder, node, graph, err):
    """
    convert to CoreML Min Broadcastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4135
    """
    load_input_constants(builder, node, graph, err)
    add_broadcastable_op_chain(builder, node, err, builder.add_min_broadcastable)


def _convert_mod(builder, node, graph, err):
    """
    convert to CoreML Mod Broadcastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4144
    """
    load_input_constants(builder, node, graph, err)
    add_broadcastable_op_chain(builder, node, err, builder.add_mod_broadcastable)


def _convert_mul(builder, node, graph, err):
    """
    convert to CoreML Multiply Broadcastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4171
    """
    load_input_constants(builder, node, graph, err)
    add_broadcastable_op_chain(builder, node, err, builder.add_multiply_broadcastable)


def _convert_nonzero(builder, node, graph, err):
    """
    convert to CoreML Where Non Zero Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4002
    """
    load_input_constants(builder, node, graph, err)
    builder.add_where_nonzero(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_pool(builder, node, graph, err):
    """
    convert to CoreML Pooling Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L477
    """
    params_dict = dict()
    params_dict["is_global"] = False
    if node.op_type.startswith("Global"):
        params_dict["is_global"] = True
    if node.op_type.endswith("MaxPool"):
        params_dict["layer_type"] = "MAX"
    elif node.op_type.endswith("AveragePool"):
        params_dict["layer_type"] = "AVERAGE"
    else:
        return err.unsupported_op_configuration(
            builder, node, graph, "Unsupported pool type"
        )

    if len(node.outputs) == 2:
        return err.unsupported_op_configuration(
            builder, node, graph, "argmax with pool unsupported"
        )

    if "ceil_mode" in node.attrs and node.attrs["ceil_mode"] == 1:
        return err.unsupported_op_configuration(
            builder, node, graph, "ceil_mode=1 not supported"
        )

    if "dilations" in node.attrs:
        return err.unsupported_op_configuration(
            builder, node, graph, "dilations not supported"
        )

    _add_conv_like_op(
        _add_pool, _get_pool_params, params_dict, builder, node, graph, err
    )


def _convert_reduce(builder, node, graph, err):
    """
    convert to CoreML ReduceSum Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4707
    """
    load_input_constants(builder, node, graph, err)

    # read attributes
    axes = node.attrs.get("axes", None)
    reduce_all = False
    if axes is None:
        reduce_all = True
    keepdims = node.attrs.get("keepdims", True)

    # add respective operator
    op_type = node.op_type
    if op_type == "ReduceSum":
        builder.add_reduce_sum(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    elif op_type == "ReduceProd":
        builder.add_reduce_prod(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    elif op_type == "ReduceMean":
        builder.add_reduce_mean(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    elif op_type == "ReduceMax":
        builder.add_reduce_max(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    elif op_type == "ReduceMin":
        builder.add_reduce_min(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    elif op_type == "ReduceL2":
        builder.add_reduce_l2(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    elif op_type == "ReduceL1":
        builder.add_reduce_l1(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    elif op_type == "ReduceSumSquare":
        builder.add_reduce_sumsquare(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    elif op_type == "ReduceLogSum":
        builder.add_reduce_logsum(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    elif op_type == "ReduceLogSumExp":
        builder.add_reduce_logsumexp(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            axes=axes,
            keepdims=keepdims,
            reduce_all=reduce_all,
        )
    else:
        err.unsupported_op_configuration(
            builder, node, graph, "Unsupported reduce operation: {}".format(op_type)
        )


def _convert_reshape(builder, node, graph, err):
    """
    convert to CoreML Reshape Static Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4844
    """
    shape_node = node.inputs[1]
    if shape_node in node.input_tensors:
        output_shape = node.input_tensors[shape_node].astype(np.int64)

        # if rank is same, then call rank preserving reshape
        if node.inputs[0] not in graph.shape_dict:
            # If Input shape is not present and output shape is known
            # add reshape static as
            # TODO: ONNX should be able to infer the shape
            builder.add_reshape_static(
                name=node.name,
                input_name=node.inputs[0],
                output_name=node.outputs[0],
                output_shape=output_shape,
            )
            return

        len_of_input_shape = builder._get_rank(node.inputs[0])
        if len(output_shape) == len_of_input_shape:
            builder.add_rank_preserving_reshape(
                name=node.name,
                input_name=node.inputs[0],
                output_name=node.outputs[0],
                output_shape=output_shape,
            )
        else:
            add_static_reshape = True
            if len_of_input_shape > len(output_shape):
                # Output rank is less than input rank
                # Case when most of the dims size is unchanged
                num_zeros = 0
                num_neg_ones = 0
                for i in output_shape:
                    if i == 0:
                        num_zeros += 1
                    elif i == -1:
                        num_neg_ones += 1

                if num_neg_ones > 1:
                    err.unsupported_op_configuration(
                        builder,
                        node,
                        graph,
                        "Error in ONNX model: At most one dimension of new shape can be -1, found {}".format(
                            num_neg_ones
                        ),
                    )

                if num_neg_ones + num_zeros == len(output_shape):
                    # Rank of output is less than input
                    # Make Rank equivalent for reshape and then squeeze
                    add_static_reshape = False
                    new_shape = []
                    i = 0
                    for i in range(len(output_shape)):
                        new_shape.append(output_shape[i])
                        if output_shape[i] == -1:
                            break
                    while i < len_of_input_shape - 1:
                        new_shape.append(1)
                        i += 1

                    builder.add_rank_preserving_reshape(
                        name=node.name + "_reshape_preserving",
                        input_name=node.inputs[0],
                        output_name=node.outputs[0] + "_reshape_dim_preserved",
                        output_shape=new_shape,
                    )

                    squeeze_axes = list(
                        range(len(output_shape) - len_of_input_shape, 0)
                    )
                    squeeze_axes.reverse()

                    builder.add_squeeze(
                        name=node.name,
                        input_name=node.outputs[0] + "_reshape_dim_preserved",
                        output_name=node.outputs[0],
                        axes=squeeze_axes,
                    )

            if add_static_reshape:
                builder.add_reshape_static(
                    name=node.name,
                    input_name=node.inputs[0],
                    output_name=node.outputs[0],
                    output_shape=output_shape,
                )
    else:
        builder.add_reshape_dynamic(
            name=node.name, input_names=node.inputs, output_name=node.outputs[0],
        )


def _convert_resize(builder, node, graph, err):
    """
    convert to CoreML Upsample or Resize Bilinear Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L2139
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L2178
    """
    mode = node.attrs.get("mode", "nearest")
    if node.inputs[1] not in node.input_tensors:
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "Scaling factor unknown!! CoreML does not support dynamic scaling for Resize",
        )

    mode = "NN" if mode == "nearest" else "BILINEAR"
    scale = node.input_tensors[node.inputs[1]]

    builder.add_upsample(
        name=node.name,
        scaling_factor_h=scale[-2],
        scaling_factor_w=scale[-1],
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mode=mode,
    )


def _convert_reverse_sequence(builder, node, graph, err):
    """
    convert to CoreML Reverse Sequence Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3577
    """
    batch_axis = node.attrs.get("batch_axis", 1)
    time_axis = node.attrs.get("time_axis", 0)

    output_name = node.outputs[0]
    add_transpose = False
    if batch_axis > time_axis:
        output_name += "_before_reverse"
        batch_axis, time_axis = time_axis, batch_axis
        add_transpose = True

    builder.add_reverse_sequence(
        name=node.name,
        input_names=node.inputs,
        output_name=output_name,
        batch_axis=batch_axis,
        seq_axis=time_axis,
    )

    if add_transpose:
        output_name_post = "_before_reverse"
        rank = builder._get_rank(node.inputs[0])
        if rank == -1:
            return err.unsupported_op_configuration(
                builder, node, graph, "Rank unknown for input"
            )
        axes = list(range(rank))
        axes[batch_axis], axes[time_axis] = axes[time_axis], axes[batch_axis]
        builder.add_transpose(
            name=node.name + "_transpose",
            axes=axes,
            input_name=output_name,
            output_name=node.outputs[0],
        )


def _convert_roialign(builder, node, graph, err):
    """
    convert to CoreML CropResize and Pooling Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L2239
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L1702
    """

    target_height = node.attrs.get("output_height", 1)
    target_width = node.attrs.get("output_width", 1)
    mode = node.attrs.get("mode", "AVERAGE").upper()
    sampling_ratio = node.attrs.get("sampling_ratio", 0)
    spatial_scale = node.attrs.get("sampling_scale", 1.0)

    if node.inputs[2] in graph.inputs:
        graph.inputs.remove(node.inputs[2])

    builder.add_expand_dims(
        name=node.name + "_expand_0",
        input_name=node.inputs[0],
        output_name=node.inputs[0] + "_expanded",
        axes=[0],
    )
    node.inputs[0] += "_expanded"

    builder.add_expand_dims(
        name=node.name + "_expand_2",
        input_name=node.inputs[2],
        output_name=node.inputs[2] + "_expanded",
        axes=[1],
    )
    node.inputs[2] += "_expanded"

    builder.add_concat_nd(
        name=node.name + "_concat_indices",
        input_names=[node.inputs[2], node.inputs[1]],
        output_name=node.inputs[1] + "_rois",
        axis=1,
    )
    node.inputs[1] += "_rois"

    builder.add_expand_dims(
        name=node.name + "_expand_1",
        input_name=node.inputs[1],
        output_name=node.inputs[1] + "_expanded",
        axes=[1, 3, 4],
    )
    node.inputs[1] += "_expanded"

    builder.add_crop_resize(
        name=node.name + "_crop_resize",
        input_names=[node.inputs[0], node.inputs[1]],
        output_name=node.outputs[0] + "_crop_resized",
        target_height=target_height * sampling_ratio,
        target_width=target_width * sampling_ratio,
        mode="ROI_ALIGN_MODE",
        box_indices_mode="CORNERS_WIDTH_FIRST",
        spatial_scale=spatial_scale,
    )

    builder.add_squeeze(
        name=node.name + "_squeeze",
        input_name=node.outputs[0] + "_crop_resized",
        output_name=node.outputs[0] + "_crop_resized_squeezed",
        axes=[1],
    )

    builder.add_pooling(
        name=node.name + "_pool",
        height=sampling_ratio,
        width=sampling_ratio,
        layer_type=mode,
        input_name=node.outputs[0] + "_crop_resized_squeezed",
        output_name=node.outputs[0],
        stride_height=sampling_ratio,
        stride_width=sampling_ratio,
        padding_type="VALID",
    )


def _convert_round(builder, node, graph, err):
    """
    convert to CoreML Round Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5029
    """
    builder.add_round(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_scatter(builder, node, graph, err):
    """
    convert to CoreML Scatter Along Axis Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4308
    """
    axis = node.attrs.get("axis", 0)
    builder.add_scatter_along_axis(
        name=node.name, input_names=node.inputs, output_name=node.outputs[0], axis=axis
    )


def _convert_size(builder, node, graph, err):
    """
    convert to CoreML GetShape and ReduceProd Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5131
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4722
    """
    builder.add_get_shape(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.inputs[0] + "_getshape",
    )
    builder.add_reduce_prod(
        name=node.name + "_reduce_prod",
        input_name=node.inputs[0] + "_getshape",
        output_name=node.outputs[0],
    )


def _convert_slice_ir4v9(builder, node, graph, err):
    """
    convert to CoreML Slice Static Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5082
    """
    if node.inputs[0] in graph.shape_dict:
        data_shape = graph.shape_dict[node.inputs[0]]
    else:
        rank = builder._get_rank(node.inputs[0])
        if rank == -1:
            return err.unsupported_op_configuration(
                builder, node, graph, "Input shape not available"
            )
        data_shape = [INT_MAX] * rank

    len_of_data = len(data_shape)
    begin_masks = [True] * len_of_data
    end_masks = [True] * len_of_data

    default_axes = list(range(len_of_data))
    default_steps = [1] * len_of_data

    ip_starts = node.attrs.get("starts")
    ip_ends = node.attrs.get("ends")
    axes = node.attrs.get("axes", default_axes)
    steps = node.attrs.get("steps", default_steps)

    starts = [0] * len_of_data
    ends = [0] * len_of_data

    for i in range(len(axes)):
        current_axes = axes[i]
        starts[current_axes] = ip_starts[i]
        ends[current_axes] = ip_ends[i]
        # n <= end <= INT_MAX implies end is -1, hence end_mask should be True
        # otherwise end_mask should be False
        if ends[current_axes] < data_shape[current_axes]:
            # this means end is not -1
            end_masks[current_axes] = False

        if starts[current_axes] != 0:
            begin_masks[current_axes] = False

    builder.add_slice_static(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        begin_ids=starts,
        end_ids=ends,
        strides=steps,
        begin_masks=begin_masks,
        end_masks=end_masks,
    )


def _convert_slice(builder, node, graph, err):
    """
    convert to CoreML Slice Static Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5082
    """
    if len(node.inputs) == 1:
        return _convert_slice_ir4v9(builder, node, graph, err)

    if node.inputs[0] not in graph.shape_dict:
        err.unsupported_op_configuration(
            builder, node, graph, "Input shape not available"
        )

    data_shape = graph.shape_dict[node.inputs[0]]
    len_of_data = len(data_shape)
    begin_masks = [True] * len_of_data
    end_masks = [True] * len_of_data

    default_axes = list(range(len_of_data))

    add_static_slice_layer = False
    if node.inputs[1] in node.input_tensors and node.inputs[2] in node.input_tensors:
        if len(node.inputs) > 3:
            if node.inputs[3] in node.input_tensors:
                if len(node.inputs) > 4:
                    if node.inputs[4] in node.input_tensors:
                        add_static_slice_layer = True
                else:
                    add_static_slice_layer = True
        else:
            add_static_slice_layer = True

    if add_static_slice_layer:
        ip_starts = node.input_tensors[node.inputs[1]]
        ip_ends = node.input_tensors[node.inputs[2]]
        axes = (
            node.input_tensors[node.inputs[3]] if len(node.inputs) > 3 else default_axes
        )
        ip_steps = node.input_tensors[node.inputs[4]] if len(node.inputs) > 4 else None

        starts = [0] * len_of_data
        ends = [0] * len_of_data
        steps = [1] * len_of_data

        for i in range(len(axes)):
            current_axes = axes[i]
            starts[current_axes] = ip_starts[i]
            ends[current_axes] = ip_ends[i]
            # n <= end <= INT_MAX implies end is -1, hence end_mask should be True
            # otherwise end_mask should be False
            if ends[current_axes] < data_shape[current_axes]:
                # this means end is not -1
                end_masks[current_axes] = False

            if starts[current_axes] != 0:
                begin_masks[current_axes] = False

            if isinstance(ip_steps, list):
                steps[current_axes] = ip_steps[i]

        builder.add_slice_static(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            begin_ids=starts,
            end_ids=ends,
            strides=steps,
            begin_masks=begin_masks,
            end_masks=end_masks,
        )
    else:
        err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "CoreML does not support Dynamic Slice with unknown axes. Please provide Custom Function/Layer",
        )


def _convert_softmax_nd(builder, node, graph, err):
    """
    convert to CoreML SoftMax ND Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#3547
    """
    axis = node.attrs.get("axis", 1)
    builder.add_softmax_nd(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0]
        + ("_softmax" if node.op_type == "LogSoftmax" else ""),
        axis=axis,
    )
    if node.op_type == "LogSoftmax":
        builder.add_unary(
            name=node.name + "_log",
            input_name=node.outputs[0] + "_softmax",
            output_name=node.outputs[0],
            mode="log",
        )


def _convert_softmax(builder, node, graph, err):
    """
    convert to CoreML SoftMax ND Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#3547
    """

    def add_softmax(output_name, rank=-1, axis=-3):
        softmax_axis = 3
        axes = list(range(5 - rank))
        if axis < 0:
            axis = rank + axis
        axis += len(axes)
        softmax_output_name = output_name + "_expanded"

        expanded_node = node.name + "_" + node.inputs[0] + "_expanded"
        builder.add_expand_dims(
            name=node.name + "_expand_dims",
            input_name=node.inputs[0],
            output_name=expanded_node,
            axes=axes,
        )
        input_name = expanded_node
        rank = 5

        if axis != -3 and axis != rank - softmax_axis:
            transpose_axes = list(range(rank))
            transpose_axes[-3], transpose_axes[axis] = (
                transpose_axes[axis],
                transpose_axes[-3],
            )

            builder.add_transpose(
                name=node.name + "_transpose",
                axes=transpose_axes,
                input_name=input_name,
                output_name=input_name + "_transposed",
            )
            input_name += "_transposed"
            softmax_output_name += "_transposed"

        builder.add_softmax(
            name=node.name, input_name=input_name, output_name=softmax_output_name
        )

        if axis != -3 and axis != rank - softmax_axis:
            transpose_axes = list(range(rank))
            transpose_axes[-3], transpose_axes[axis] = (
                transpose_axes[axis],
                transpose_axes[-3],
            )

            builder.add_transpose(
                name=node.name + "_transpose_back",
                axes=transpose_axes,
                input_name=softmax_output_name,
                output_name=softmax_output_name + "_transposed_back",
            )
            softmax_output_name += "_transposed_back"

        builder.add_squeeze(
            name=node.name + "_squeeze_dims",
            input_name=softmax_output_name,
            output_name=output_name,
            axes=axes,
        )

    axis = node.attrs.get("axis", 1)
    rank = builder._get_rank(node.inputs[0])
    if rank == -1:
        return _convert_softmax_nd(builder, node, graph, err)

    if node.op_type == "LogSoftmax":
        add_softmax(node.outputs[0] + "_softmax", rank=rank, axis=axis)
        builder.add_unary(
            name=node.name + "_log",
            input_name=node.outputs[0] + "_softmax",
            output_name=node.outputs[0],
            mode="log",
        )
    else:
        add_softmax(node.outputs[0], rank=rank, axis=axis)


def _convert_split(builder, node, graph, err):
    """
    convert to CoreML Squeeze Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#5003
    """
    axis = node.attrs.get("axis", 0)
    split = node.attrs.get("split", None)
    num_splits = len(node.outputs) if split is None else 2

    builder.add_split_nd(
        name=node.name,
        input_name=node.inputs[0],
        output_names=node.outputs,
        axis=axis,
        num_splits=num_splits,
        split_sizes=split,
    )


def _convert_shape(builder, node, graph, err):
    """
    convert to CoreML GetShape Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5131
    """
    builder.add_get_shape(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_squeeze(builder, node, graph, err):
    """
    convert to CoreML Squeeze Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4903
    """
    axes = node.attrs.get("axes", None)
    builder.add_squeeze(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        axes=axes,
    )


def _convert_sub(builder, node, graph, err):
    """
    convert to CoreML Subtract Broadcastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4117
    """
    load_input_constants(builder, node, graph, err)
    add_broadcastable_op_chain(builder, node, err, builder.add_subtract_broadcastable)


def _convert_tanh(builder, node, graph, err):
    """
    convert to CoreML Tanh Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3881
    """
    load_input_constants(builder, node, graph, err)
    builder.add_tanh(
        name=node.name, input_name=node.inputs[0], output_name=node.outputs[0]
    )


def _convert_tile(builder, node, graph, err):
    """
    convert to CoreML Tile Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5117
    """
    load_input_constants(builder, node, graph, err)
    if node.inputs[1] not in node.input_tensors:
        err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "CoreML Tile layer does not support dynamic 'reps'. 'reps' should be known statically",
        )
    builder.add_tile(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        reps=node.input_tensors[node.inputs[1]].astype(np.int32).tolist(),
    )


def _convert_topk(builder, node, graph, err):
    """
    convert to CoreML TopK Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L5190
    """
    load_input_constants(builder, node, graph, err)
    axis = node.attrs.get("axis", -1)
    bottom_k = node.attrs.get("largest", True) == False
    # NOTE: Sorted order attribute is currently ignored in CoreML
    sorted_order = node.attrs.get("sorted", True)
    if "sorted" in node.attrs:
        err.unsupported_feature_warning(
            node, "Sorted Order attribute('sorted') is currently ignored in CoreML 3.0"
        )

    builder.add_topk(
        name=node.name,
        input_names=node.inputs,
        output_names=node.outputs,
        axis=axis,
        use_bottom_k=bottom_k,
    )


def _convert_transpose(builder, node, graph, err):
    """
    convert to CoreML Transpose Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3426
    """

    axes = node.attrs.get("perm", [])
    # If 'perm' not provided, the reverse the dimensions
    if axes == []:
        rank = builder._get_rank(node.inputs[0])
        if rank == -1:
            return err.unsupported_op_configuration(
                builder, node, graph, "Rank unknown for input"
            )
        axes = list(range(-1, -(rank + 1), -1))

    builder.add_transpose(
        name=node.name,
        axes=axes,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
    )


def _convert_unsqueeze(builder, node, graph, err):
    """
    convert to CoreML ExpandDim Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L4810
    """
    axes = node.attrs.get("axes")
    builder.add_expand_dims(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        axes=axes,
    )


def _convert_where(builder, node, graph, err):
    """
    convert to CoreML WhereBroadcastable Layer:
    https://github.com/apple/coremltools/blob/655b3be5cc0d42c3c4fa49f0f0e4a93a26b3e492/mlmodel/format/NeuralNetwork.proto#L3742
    """
    load_input_constants(builder, node, graph, err)
    builder.add_where_broadcastable(
        name=node.name, input_names=node.inputs, output_name=node.outputs[0],
    )


_ONNX_NODE_REGISTRY_ND = {
    "Abs": _convert_abs,
    "Acos": _convert_acos,
    "Acosh": _convert_acosh,
    "Add": _convert_add,
    "And": _convert_logical,
    "ArgMax": _convert_argmax,
    "ArgMin": _convert_argmin,
    "Asin": _convert_asin,
    "Asinh": _convert_asinh,
    "Atan": _convert_atan,
    "Atanh": _convert_atanh,
    "AveragePool": _convert_pool,
    "BatchNormalization": _convert_bn,
    "Cast": _convert_cast,
    "Ceil": _convert_ceil,
    "Clip": _convert_clip,
    "Concat": _convert_concat,
    "Constant": _convert_constant,
    "ConstantOfShape": _convert_constant_of_shape,
    "Conv": _convert_conv,
    "ConvTranspose": _convert_conv,
    "Cos": _convert_cos,
    "Cosh": _convert_cosh,
    "DepthToSpace": _convert_reorganize_data,
    "Div": _convert_div,
    "Elu": _convert_elu,
    "Equal": _convert_equal,
    "Erf": _convert_erf,
    "Exp": _convert_exp,
    "Expand": _convert_expand,
    "Flatten": _convert_flatten,
    "Floor": _convert_floor,
    "Gather": _convert_gather,
    "Gemm": _convert_gemm,
    "Greater": _convert_greater,
    "GRU": _convert_gru,
    "GlobalAveragePool": _convert_pool,
    "GlobalMaxPool": _convert_pool,
    "HardSigmoid": _convert_hardsigmoid,
    "Identity": _convert_identity,
    "InstanceNormalization": _convert_instancenorm,
    "LeakyRelu": _convert_leaky_relu,
    "Log": _convert_log,
    "LogSoftmax": _convert_softmax,
    "LRN": _convert_lrn,
    "Less": _convert_less,
    "LSTM": _convert_lstm,
    "MatMul": _convert_matmul,
    "Max": _convert_max,
    "MaxPool": _convert_pool,
    "Mean": _convert_mean,
    "Min": _convert_min,
    "Mod": _convert_mod,
    "Mul": _convert_mul,
    "Neg": _convert_neg,
    "NonZero": _convert_nonzero,
    "Not": _convert_logical,
    "Or": _convert_logical,
    "Pad": _convert_pad,
    "Pow": _convert_pow,
    "PRelu": _convert_prelu,
    "RandomNormal": _convert_randomnormal,
    "RandomNormalLike": _convert_randomnormallike,
    "RandomUniform": _convert_randomuniform,
    "RandomUniformLike": _convert_randomuniformlike,
    "Reciprocal": _convert_reciprocal,
    "ReduceL1": _convert_reduce,
    "ReduceL2": _convert_reduce,
    "ReduceLogSum": _convert_reduce,
    "ReduceLogSumExp": _convert_reduce,
    "ReduceMax": _convert_reduce,
    "ReduceMean": _convert_reduce,
    "ReduceMin": _convert_reduce,
    "ReduceProd": _convert_reduce,
    "ReduceSum": _convert_reduce,
    "ReduceSumSquare": _convert_reduce,
    "Relu": _convert_relu,
    "Reshape": _convert_reshape,
    "Resize": _convert_resize,
    "ReverseSequence": _convert_reverse_sequence,
    "RoiAlign": _convert_roialign,
    "Round": _convert_round,
    "Scatter": _convert_scatter,
    "Selu": _convert_selu,
    "Sigmoid": _convert_sigmoid,
    "Sign": _convert_sign,
    "Size": _convert_size,
    "Slice": _convert_slice,
    "Softmax": _convert_softmax,
    "Softplus": _convert_softplus,
    "Softsign": _convert_softsign,
    "SpaceToDepth": _convert_reorganize_data,
    "Split": _convert_split,
    "Shape": _convert_shape,
    "Sqrt": _convert_sqrt,
    "Squeeze": _convert_squeeze,
    "Sub": _convert_sub,
    "Sum": _convert_add,
    "Tanh": _convert_tanh,
    "ThresholdedRelu": _convert_thresholdedrelu,
    "Tile": _convert_tile,
    "TopK": _convert_topk,
    "Transpose": _convert_transpose,
    "Unsqueeze": _convert_unsqueeze,
    "Upsample": _convert_upsample,
    "Xor": _convert_logical,
    "Where": _convert_where,
}


def _get_node_converter_fn(
    builder, node, err
):  # type: (NeuralNetworkBuilder, Node, ErrorHandling) -> Callable[[NeuralNetworkBuilder, Node, Graph, ErrorHandling], None]
    """
    Get the right converter function for ONNX node op_type
    """
    op_type = node.op_type
    # Return custom conversion function if provided
    # If both node type and node name custom function
    # is provided, then use node name specific custom function, as
    # type specific custom function is more generic than name specific
    if node.name in err.custom_conversion_functions:
        return err.custom_conversion_functions[node.name]
    elif op_type in err.custom_conversion_functions:
        return err.custom_conversion_functions[op_type]
    elif op_type in _ONNX_NODE_REGISTRY_ND:
        return _ONNX_NODE_REGISTRY_ND[op_type]
    else:
        return err.unsupported_op(node)


def _convert_node_nd(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    converter_fn = _get_node_converter_fn(builder, node, err)
    return converter_fn(builder, node, graph, err)

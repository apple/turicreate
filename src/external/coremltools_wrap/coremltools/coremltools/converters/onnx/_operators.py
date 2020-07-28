from __future__ import absolute_import as _
from __future__ import division as _
from __future__ import print_function as _
from __future__ import unicode_literals as _

import numpy as np
import copy

from typing import Sequence, Callable, List, Tuple, Optional, Text, Any
from coremltools.models.neural_network import NeuralNetworkBuilder  # type: ignore
from ._graph import Node, Graph
from coremltools.proto import NeuralNetwork_pb2  # type: ignore
from ._error_utils import ErrorHandling

INT_MAX = 2 ** 30

"""
General common functions
"""


def _compare(a, b, encoding="utf8"):  # type: (Text, Text, Text) -> bool
    if isinstance(a, bytes):
        a = a.decode(encoding)
    if isinstance(b, bytes):
        b = b.decode(encoding)
    return a == b


def _is_input_shape_mapping_defined(node, graph):  # type: (Node, Graph) -> bool
    if node.inputs[0] in graph.onnx_coreml_shape_mapping:
        return True
    else:
        return False


def _update_shape_mapping_unchanged(
    node, graph, err
):  # type: (Node, Graph, ErrorHandling) -> None
    if _is_input_shape_mapping_defined(node, graph):
        graph.onnx_coreml_shape_mapping[
            node.outputs[0]
        ] = graph.onnx_coreml_shape_mapping[node.inputs[0]]


def _convert_broadcast_op(
    builder, node, graph, err, mode
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling, Text) -> None
    if node.op_type == "Max" or node.op_type == "Min" or node.op_type == "Mean":
        if len(node.inputs) == 1:
            inputs = [node.inputs[0], node.inputs[0]]
        else:
            inputs = node.inputs
    else:
        inputs = node.inputs

    if node.op_type == "Sub":
        builder.add_elementwise(
            name=node.name + "_neg",
            input_names=[inputs[1]],
            output_name=inputs[1] + "_neg",
            mode="MULTIPLY",
            alpha=-1.0,
        )
        builder.add_elementwise(
            name=node.name,
            input_names=[inputs[0], inputs[1] + "_neg"],
            output_name=node.outputs[0],
            mode=mode,
        )
    else:
        builder.add_elementwise(
            name=node.name, input_names=inputs, output_name=node.outputs[0], mode=mode
        )

    if _is_input_shape_mapping_defined(node, graph):
        ranks = [len(graph.onnx_coreml_shape_mapping[input_]) for input_ in node.inputs]
        max_id = np.argmax(np.array(ranks))
        graph.onnx_coreml_shape_mapping[
            node.outputs[0]
        ] = graph.onnx_coreml_shape_mapping[node.inputs[max_id]]


def _get_coreml_target_shape(target_shape, builder, node, graph, err):
    # type: (Tuple[int, ...], NeuralNetworkBuilder, node, Graph, ErrorHandling) -> Optional[Tuple[int, ...]]

    if len(target_shape) == 1:  # (D,)
        coreml_shape = (1, target_shape[0], 1, 1)  # type: Optional[Tuple[int, ...]]
        if _is_input_shape_mapping_defined(node, graph):
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = [2]
    elif len(target_shape) == 2:  # (S,D)
        coreml_shape = target_shape + (1, 1)
        if _is_input_shape_mapping_defined(node, graph):
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = [0, 2]
    elif len(target_shape) == 3:  # (C,H,W)
        coreml_shape = (1, target_shape[0], target_shape[1], target_shape[2])
        if _is_input_shape_mapping_defined(node, graph):
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = [2, 3, 4]
    elif len(target_shape) == 4:
        coreml_shape = target_shape
        if _is_input_shape_mapping_defined(node, graph):
            mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
            if mapp[0] == 1 and coreml_shape[0] == 1:
                graph.onnx_coreml_shape_mapping[node.outputs[0]] = [1, 2, 3, 4]
            else:
                graph.onnx_coreml_shape_mapping[node.outputs[0]] = [0, 2, 3, 4]
    elif len(target_shape) > 4:
        # return err.unsupported_op_configuration(builder, node, graph, "Supports tensors not more than 4d")  # type: ignore
        diff = len(target_shape) - 4
        if all([d == 1 for d in target_shape[:diff]]):
            coreml_shape = target_shape[diff:]
        else:
            err.unsupported_op_configuration(builder, node, graph, "Tensors more than rank 4 are not supported")  # type: ignore
        if _is_input_shape_mapping_defined(node, graph):
            if target_shape[0] == 1 and len(target_shape) == 5:
                graph.onnx_coreml_shape_mapping[node.outputs[0]] = [1, 0, 2, 3, 4]
        else:
            return err.unsupported_op_configuration(builder, node, graph, "Supports tensors not more than 4d")  # type: ignore
    else:
        coreml_shape = None
    return coreml_shape


def _get_coreml_axis(
    axes, builder, node, graph, err
):  # type: (List[int], NeuralNetworkBuilder, node, Graph, ErrorHandling) -> Text
    coreml_axis = ""
    if node.inputs[0] not in graph.shape_dict:
        return err.unsupported_op_configuration(
            builder, node, graph, "Failed to translate axis"
        )
    input_shape = graph.shape_dict[node.inputs[0]]
    if len(input_shape) == 1:
        coreml_axis = "C"
    elif len(input_shape) == 2:
        if len(axes) == 1 and axes[0] == 1:
            coreml_axis = "C"
    elif len(input_shape) == 3:
        for ind in [["C", "H", "W"][i] for i in axes]:
            coreml_axis += ind
    elif len(input_shape) == 4:
        for ind in [["B", "C", "H", "W"][i] for i in axes]:
            coreml_axis += ind
    return coreml_axis


def _add_transpose_before_after(
    layer_func,  # function for layer conversion
    input_names,  # List[str]
    output_names,  # List[str]
    transpose_dims,  # List[int]
    **kwargs
):  # type: ignore

    for i, input_ in enumerate(input_names):
        kwargs["builder"].add_permute(
            name=kwargs["node"].name + "_input_transpose" + str(i),
            dim=transpose_dims,
            input_name=input_,
            output_name=kwargs["node"].name + "_" + input_ + "_transpose",
        )

    new_input_names = [
        kwargs["node"].name + "_" + input_ + "_transpose" for input_ in input_names
    ]
    new_output_names = [output_ + "_transpose" for output_ in output_names]
    layer_func(new_input_names, new_output_names, **kwargs)

    for i, output_ in enumerate(output_names):
        kwargs["builder"].add_permute(
            name=kwargs["node"].name + "_output_transpose" + str(i),
            dim=transpose_dims,
            input_name=output_ + "_transpose",
            output_name=output_,
        )


def _add_inner_product(input_names, output_names, **kwargs):
    node = kwargs["node"]
    builder = kwargs["builder"]
    builder.add_inner_product(
        name=node.name,
        W=kwargs["W"],
        b=kwargs["b"],
        input_channels=kwargs["W"].shape[1],
        output_channels=kwargs["W"].shape[0],
        has_bias=kwargs["b"] is not None,
        input_name=input_names[0],
        output_name=output_names[0],
    )


def _add_conv_like_op(
    add_func, get_params_func, params_dict, builder, node, graph, err
):
    if node.inputs[0] in graph.onnx_coreml_shape_mapping:
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]

        r = len(mapp)
        if not (r == 3 or r == 4):
            return err.unsupported_op_configuration(
                builder, node, graph, "more than 4 axes not supported"
            )
        if r == 4:
            if not (mapp == [1, 2, 3, 4] or mapp == [0, 2, 3, 4]):
                return err.unsupported_op_configuration(
                    builder,
                    node,
                    graph,
                    "error in axes alignment between onnx and coreml",
                )
            get_params_func(builder, node, graph, err, params_dict)
            add_func(
                node.inputs,
                node.outputs,
                params_dict=params_dict,
                node=node,
                builder=builder,
                graph=graph,
                err=err,
            )
        if r == 3:
            if mapp == [1, 2, 3]:  # [B,C,H]
                # spatial dimension: height
                get_params_func(builder, node, graph, err, params_dict, axis="height")
                add_func(
                    node.inputs,
                    node.outputs,
                    params_dict=params_dict,
                    node=node,
                    builder=builder,
                    graph=graph,
                    err=err,
                )
            elif mapp == [1, 2, 4]:  # [B,C,W]
                # spatial dimension: width
                get_params_func(builder, node, graph, err, params_dict, axis="width")
                add_func(
                    node.inputs,
                    node.outputs,
                    params_dict=params_dict,
                    node=node,
                    builder=builder,
                    graph=graph,
                    err=err,
                )
            elif mapp == [
                2,
                3,
                4,
            ]:  # [C,H,W] in CoreML, but it represents [B,C,D] in ONNX.
                # spatial dimension: sequence
                get_params_func(builder, node, graph, err, params_dict, axis="width")
                node.inputs = [node.inputs[0]]
                _add_transpose_before_after(
                    add_func,
                    node.inputs,
                    node.outputs,
                    [0, 2, 1, 3],  # swap C & H
                    builder=builder,
                    node=node,
                    params_dict=params_dict,
                    graph=graph,
                    err=err,
                )

            elif mapp == [1, 2, 0]:  # [B,C,S]
                # spatial dimension: sequence
                get_params_func(builder, node, graph, err, params_dict, axis="width")
                node.inputs = [node.inputs[0]]
                _add_transpose_before_after(
                    add_func,
                    node.inputs,
                    node.outputs,
                    [3, 1, 2, 0],
                    builder=builder,
                    node=node,
                    params_dict=params_dict,
                    graph=graph,
                    err=err,
                )
            else:
                return err.unsupported_op_configuration(
                    builder,
                    node,
                    graph,
                    "error in axes alignment between onnx and coreml",
                )

    else:
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


def _is_no_op(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> bool

    if node.inputs[0] in graph.shape_dict and node.outputs[0] in graph.shape_dict:
        if graph.shape_dict[node.inputs[0]] == graph.shape_dict[node.outputs[0]]:
            builder.add_activation(
                name=node.name,
                non_linearity="LINEAR",
                input_name=node.inputs[0],
                output_name=node.outputs[0],
                params=[1.0, 0.0],
            )
            _update_shape_mapping_unchanged(node, graph, err)
            return True

    return False


"""
Layer conversion functions
"""


def _convert_abs(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_unary(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mode="abs",
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_add(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None

    # check if its equivalent to a bias layer
    if len(node.inputs) > 1:
        if node.inputs[1] in node.input_tensors:
            second_input = np.squeeze(node.input_tensors[node.inputs[1]])
            if len(second_input.shape) == 1:
                builder.add_bias(
                    name=node.name,
                    b=second_input,
                    input_name=node.inputs[0],
                    output_name=node.outputs[0],
                    shape_bias=[second_input.shape[0]],
                )
                return
    """
    Supported shapes by CoreML 2.0 for broadcasting (-1 means it can be 1 or greater than 1):
    (i.e. all of the outputs must have one of these shapes for broadcasting support)
    - (S=-1,B=-1,1,1,1)
    - (S=-1,B=-1,C,1,1)
    - (S=-1,B=-1,1,H,W)
    - (S=-1,B=-1,C,H,W)
    Unsupported:
    - (S=-1,B=-1,1,1,W)
    - (S=-1,B=-1,1,H,1)
    - (S=-1,B=-1,C,1,W)
    - (S=-1,B=-1,C,H,1)
    """
    _convert_broadcast_op(builder, node, graph, err, "ADD")


def _convert_sub(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    _convert_broadcast_op(builder, node, graph, err, "ADD")


def _get_conv_params(builder, node, graph, err, params_dict, axis=None):
    if "dilations" not in node.attrs:
        params_dict["dilations"] = [1, 1]
    else:
        if axis == "height":
            params_dict["dilations"] = node.attrs["dilations"]
            params_dict["dilations"].append(1)
        elif axis == "width":
            params_dict["dilations"] = node.attrs["dilations"]
            params_dict["dilations"].insert(0, 1)
        else:
            params_dict["dilations"] = node.attrs["dilations"]

    if "pads" not in node.attrs:
        params_dict["pads"] = [0, 0, 0, 0]
    else:
        pads = node.attrs["pads"]
        if axis == "height":
            pads = [pads[0], 0, pads[1], 0]
        elif axis == "width":
            pads = [0, pads[0], 0, pads[1]]
        params_dict["pads"] = pads

    if "kernel_shape" in node.attrs:
        params_dict["kernel_shape"] = node.attrs["kernel_shape"]
    else:
        # w_shape is ONNX format shape
        w_shape = params_dict["w_shape"]
        if len(w_shape) == 4:
            params_dict["kernel_shape"] = [w_shape[-2], w_shape[-1]]
        else:
            params_dict["kernel_shape"] = [w_shape[-1]]
    params_dict["strides"] = node.attrs.get("strides", [1, 1] if axis is None else [1])

    if axis == "height":
        if params_dict["W"] is not None:
            params_dict["W"] = np.expand_dims(params_dict["W"], axis=-1)
        params_dict["kernel_shape"].append(1)
        params_dict["strides"].append(1)
    elif axis == "width":
        if params_dict["W"] is not None:
            params_dict["W"] = np.expand_dims(params_dict["W"], axis=-2)
        params_dict["strides"].insert(0, 1)
        params_dict["kernel_shape"].insert(0, 1)

    params_dict["out_shape"] = None
    params_dict["padding_type"] = "valid"
    params_dict["same_padding_asymmetry_mode"] = "BOTTOM_RIGHT_HEAVY"

    if params_dict["W"] is not None:
        if not params_dict["is_deconv"]:
            params_dict["W"] = params_dict["W"].transpose((2, 3, 1, 0))  # type: ignore
        else:
            params_dict["W"] = params_dict["W"].transpose((2, 3, 0, 1))  # type: ignore

    if "auto_pad" in node.attrs and not _compare(node.attrs["auto_pad"], "VALID"):
        params_dict["padding_type"] = "same"
        if _compare(node.attrs["auto_pad"], "SAME_LOWER"):
            params_dict["same_padding_asymmetry_mode"] = "TOP_LEFT_HEAVY"

    if params_dict["is_deconv"]:
        if "output_shape" in node.attrs:
            if axis == "height":
                params_dict["out_shape"] = (
                    node.attrs["output_shape"][-1],
                    1,
                )  # (Hout, wout)
            elif axis == "width":
                params_dict["out_shape"] = (
                    1,
                    node.attrs["output_shape"][-1],
                )  # (Hout, wout)
            else:
                params_dict["out_shape"] = (
                    node.attrs["output_shape"][-2],
                    node.attrs["output_shape"][-1],
                )  # (Hout, wout)
        elif "output_padding" in node.attrs:
            params_dict["crops"] = copy.copy(params_dict["pads"])
            params_dict["pads"] = [0, 0, 0, 0]
            post_pads = node.attrs["output_padding"]
            if sum(post_pads) != 0:
                t = l = b = r = 0
                if len(post_pads) == 1:
                    if axis == "height":
                        b = post_pads[0]
                    elif axis == "width":
                        r = post_pads[0]
                    else:
                        err.unsupported_op_configuration(
                            builder,
                            node,
                            graph,
                            "length 1 output padding attribute only supported for 1D conv",
                        )
                elif len(post_pads) == 2:
                    if axis == "height":
                        b, r = post_pads
                    elif axis == "width":
                        r, b = post_pads
                    else:
                        b, r = post_pads
                elif len(post_pads) == 4:
                    b, r, t, l = post_pads
                else:
                    return err.unsupported_op_configuration(
                        builder,
                        node,
                        graph,
                        "Supports only length 1 or 2 or 4 output padding attribute",
                    )

                def _update_crop_pad(idx, v):
                    if params_dict["crops"][idx] >= v:
                        params_dict["crops"][idx] -= v
                    else:
                        params_dict["pads"][idx] = v - params_dict["crops"][idx]

                _update_crop_pad(0, t)
                _update_crop_pad(1, l)
                _update_crop_pad(2, b)
                _update_crop_pad(3, r)
                params_dict["is_post_crop"] = (
                    True if sum(params_dict["crops"]) > 0 else False
                )
                params_dict["is_pre_pad"] = (
                    True if sum(params_dict["pads"]) > 0 else False
                )


def _add_conv(input_names, output_names, **kwargs):
    params_dict = kwargs["params_dict"]
    node = kwargs["node"]
    builder = kwargs["builder"]
    graph = kwargs["graph"]
    err = kwargs["err"]

    W_shape = params_dict["w_shape"]

    output_name = output_names[0]
    pre_padding_input_name = input_names[0]

    if params_dict.get("is_post_crop", False):
        output_name += "_conv_tranpose_post_crop"
    if params_dict.get("is_pre_pad", False):
        input_names[0] += "_conv_tranpose_pre_pad"

    if params_dict["W"] is None and len(node.inputs) == 1:
        return err.unsupported_op_configuration(
            builder, node, graph, "Kernel weight missing"
        )

    if params_dict["is_deconv"]:
        oc = W_shape[1] * params_dict["groups"]
        kc = W_shape[0]
    else:
        oc = W_shape[0]
        kc = W_shape[1]

    if params_dict.get("is_pre_pad", False):
        builder.add_padding(
            name=node.name + "_pre_pad",  # type: ignore
            left=params_dict["pads"][1],
            right=params_dict["pads"][3],
            top=params_dict["pads"][0],
            bottom=params_dict["pads"][2],
            input_name=pre_padding_input_name,
            output_name=input_names[0],
            value=0,
        )
    builder.add_convolution(
        name=node.name,
        kernel_channels=kc,
        output_channels=oc,
        height=params_dict["kernel_shape"][0],
        width=params_dict["kernel_shape"][1],
        stride_height=params_dict["strides"][0],
        stride_width=params_dict["strides"][1],
        border_mode=params_dict["padding_type"],
        same_padding_asymmetry_mode=params_dict["same_padding_asymmetry_mode"],
        groups=params_dict["groups"],
        W=params_dict["W"],
        b=params_dict["bias"],
        has_bias=params_dict["bias"] is not None,
        is_deconv=params_dict["is_deconv"],
        output_shape=params_dict["out_shape"],
        input_name=input_names[0]
        if params_dict["W"] is not None
        else [input_names[0], input_names[1]],
        output_name=output_name,
        dilation_factors=params_dict["dilations"],
        padding_top=params_dict["pads"][0],
        padding_bottom=params_dict["pads"][2],
        padding_left=params_dict["pads"][1],
        padding_right=params_dict["pads"][3],
    )
    if params_dict.get("is_post_crop", False):
        builder.add_crop(
            name=node.name + "_post_crop",  # type: ignore
            left=params_dict["crops"][1],
            right=params_dict["crops"][3],
            top=params_dict["crops"][0],
            bottom=params_dict["crops"][2],
            input_names=[output_name],
            output_name=output_names[0],
            offset=[0, 0],
        )


def _convert_conv(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    params_dict = dict()
    # get weights for convolution
    weight_name = node.inputs[1]
    W = None
    if weight_name in node.input_tensors:
        W = node.input_tensors[weight_name]
        params_dict["w_shape"] = W.shape
    else:
        err.missing_initializer(
            node,
            "Weight tensor: {} not found in the graph initializer".format(weight_name,),
        )
    params_dict["W"] = W

    params_dict["is_deconv"] = False
    if node.op_type.endswith("Transpose"):
        params_dict["is_deconv"] = True
    bias = None
    if len(node.inputs) > 2:
        bias = node.input_tensors[node.inputs[2]]
    params_dict["bias"] = bias
    params_dict["groups"] = node.attrs.get("group", 1)

    _add_conv_like_op(
        _add_conv, _get_conv_params, params_dict, builder, node, graph, err
    )

    # update map
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_relu(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_activation(
        name=node.name,
        non_linearity="RELU",
        input_name=node.inputs[0],
        output_name=node.outputs[0],
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_thresholdedrelu(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    alpha = node.attrs.get("alpha", 1.0)
    builder.add_activation(
        name=node.name,
        non_linearity="THRESHOLDEDRELU",
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        params=alpha,
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_reshape(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None

    shape = tuple(node.attrs.get("shape", ()))  # type: (Tuple[int, ...])
    if len(shape) == 0:
        shape_name = node.inputs[1]
        if shape_name in node.input_tensors:
            shape = tuple(node.input_tensors[shape_name].astype(int))  # type: ignore
        else:
            err.missing_initializer(
                node,
                "CoreML only supports Reshape layer when the target shape is static and known apriori",
            )

    # check if all entries in shape are 1/-1
    is_flatten = True
    for s in shape:
        if abs(s) != 1:
            is_flatten = False
            break
    if is_flatten:
        builder.add_flatten(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            mode=0,
        )
        if _is_input_shape_mapping_defined(node, graph):
            mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
            if len(shape) == 4:
                mapp_out = [mapp[0], 2, 3, 4]
            elif len(shape) == 3:
                mapp_out = [2, 3, 4]
            elif len(shape) == 2:
                mapp_out = [mapp[0], 2]
            elif len(shape) == 1:
                mapp_out = [2]
            else:
                return err.unsupported_op_configuration(
                    builder, node, graph, "Supports only less than equal to 4d tensors"
                )
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = mapp_out
        return

    new_shape = _get_coreml_target_shape(shape, builder, node, graph, err)

    if new_shape is None:
        return err.unsupported_op_configuration(
            builder, node, graph, "Unsupported shape for reshape"
        )

    builder.add_reshape(
        name=node.name,
        target_shape=new_shape,
        mode=0,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
    )


def _convert_transpose(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None

    if _is_input_shape_mapping_defined(node, graph):
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        r = len(mapp)
        default_perm = list(range(r))
        default_perm.reverse()
        perm = node.attrs.get("perm", default_perm)
        coreml_perm = []
        for p in perm:
            coreml_perm.append(mapp[p])
        if 1 in mapp:
            batch_index = mapp.index(1)
            batch_index_new = coreml_perm.index(1)
            if batch_index != batch_index_new:
                return err.unsupported_op_configuration(
                    builder, node, graph, "cannot transpose batch dimension"
                )
        perm_translated = []
        for c in coreml_perm:
            if c == 0:
                perm_translated.append(c)
            elif c == 1:
                continue
            else:
                perm_translated.append(c - 1)
        perm_final = [
            -1,
            -1,
            -1,
            -1,
        ]  # has to be of length 4 corresponding to [S,C,H,W]
        for i in range(4):
            if i not in perm_translated:
                perm_final[i] = i
        if perm_final.count(-1) != len(perm_translated):
            return err.unsupported_op_configuration(
                builder, node, graph, "unable to translate transpose op to CoreML"
            )
        ctr = 0
        for i, v in enumerate(perm_final):
            if v == -1:
                perm_final[i] = perm_translated[ctr]
                ctr += 1
        perm = tuple(perm_final)
    else:
        perm = node.attrs.get("perm", [0, 3, 2, 1])
        if len(perm) > 4:
            diff = len(perm) - 4
            if all([perm[i] == i for i in range(diff)]):
                perm = [p - diff for p in perm[diff:]]
            else:
                return err.unsupported_op_configuration(
                    builder, node, graph, "Supports only 4d tensors"
                )
        elif len(perm) < 4:
            diff = 4 - len(perm)
            perm = [d for d in range(diff)] + [d + diff for d in perm]
        perm = tuple(perm)

    builder.add_permute(
        name=node.name, dim=perm, input_name=node.inputs[0], output_name=node.outputs[0]
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _get_pool_params(builder, node, graph, err, params_dict, axis=None):
    (
        params_dict["pad_b"],
        params_dict["pad_l"],
        params_dict["pad_r"],
        params_dict["pad_t"],
    ) = (0, 0, 0, 0)
    params_dict["stride_height"], params_dict["stride_width"] = 1, 1
    params_dict["padding_type"] = "VALID"
    params_dict["same_padding_asymmetry_mode"] = "BOTTOM_RIGHT_HEAVY"

    if params_dict["is_global"]:
        params_dict["height"], params_dict["width"] = 0, 0
        params_dict["stride_height"], params_dict["stride_width"] = 1, 1
    else:
        kernel_shape = node.attrs["kernel_shape"]
        if axis == "height":
            params_dict["height"] = kernel_shape[0]
        elif axis == "width":
            params_dict["width"] = kernel_shape[0]
        else:
            params_dict["height"] = kernel_shape[0]
            params_dict["width"] = kernel_shape[1]

        pads = node.attrs.get("pads", None)
        if pads:
            if axis == "height":
                params_dict["pad_t"] = pads[0]
                params_dict["pad_b"] = pads[1]
            elif axis == "width":
                params_dict["pad_l"] = pads[0]
                params_dict["pad_r"] = pads[1]
            else:
                params_dict["pad_t"] = pads[0]
                params_dict["pad_l"] = pads[1]
                params_dict["pad_b"] = pads[2]
                params_dict["pad_r"] = pads[3]

        strides = node.attrs.get("strides", [1, 1])
        if axis == "height":
            params_dict["stride_height"] = strides[0]
        elif axis == "width":
            params_dict["stride_width"] = strides[0]
        else:
            params_dict["stride_height"] = strides[0]
            params_dict["stride_width"] = strides[1]

        if "auto_pad" in node.attrs and not _compare(node.attrs["auto_pad"], "VALID"):
            params_dict["padding_type"] = "SAME"
            if _compare(node.attrs["auto_pad"], "SAME_LOWER"):
                params_dict["same_padding_asymmetry_mode"] = "TOP_LEFT_HEAVY"

    params_dict["exclude_pad_area"] = node.attrs.get("count_include_pad", 0) == 0


def _add_pool(input_names, output_names, **kwargs):
    params_dict = kwargs["params_dict"]
    node = kwargs["node"]
    kwargs["builder"].add_pooling(
        name=node.name,
        height=params_dict.get("height", 1),
        width=params_dict.get("width", 1),
        stride_height=params_dict.get("stride_height", 1),
        stride_width=params_dict.get("stride_width", 1),
        layer_type=params_dict["layer_type"],
        padding_type=params_dict["padding_type"],
        exclude_pad_area=params_dict["exclude_pad_area"],
        is_global=params_dict["is_global"],
        input_name=input_names[0],
        output_name=output_names[0],
        padding_top=params_dict.get("pad_t", 0),
        padding_bottom=params_dict.get("pad_b", 0),
        padding_left=params_dict.get("pad_l", 0),
        padding_right=params_dict.get("pad_r", 0),
        same_padding_asymmetry_mode=params_dict["same_padding_asymmetry_mode"],
    )


def _convert_pool(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    input_name = node.inputs[0]
    output_name = node.outputs[0]
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
            builder, node, graph, "ceil_mod=1 not supported"
        )

    if "dilations" in node.attrs:
        return err.unsupported_op_configuration(
            builder, node, graph, "dilations not supported"
        )

    _add_conv_like_op(
        _add_pool, _get_pool_params, params_dict, builder, node, graph, err
    )

    # update map
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_bn(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    def add_bn(input_names, output_names, **kwargs):
        kwargs["builder"].add_batchnorm(
            name=node.name,
            input_name=input_names[0],
            output_name=output_names[0],
            channels=kwargs["channels"][0],
            gamma=kwargs["scale"],
            beta=kwargs["bias"],
            mean=kwargs["mean"],
            variance=kwargs["var"],
            epsilon=kwargs["epsilon"],
        )

    if len(node.outputs) > 1:
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "This converter only supports BatchNormalization with one output",
        )

    epsilon = node.attrs.get("epsilon", 1e-5)
    channels = set()
    for v in node.input_tensors.values():
        channels.add(v.shape)
    assert len(channels) == 1
    channels = channels.pop()
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

    mapp = graph.onnx_coreml_shape_mapping.get(node.inputs[0], None)
    if mapp == [2, 3, 4]:
        _add_transpose_before_after(
            add_bn,
            [node.inputs[0]],
            node.outputs,
            [0, 2, 1, 3],
            builder=builder,
            node=node,
            scale=scale,
            bias=bias,
            mean=mean,
            var=var,
            epsilon=epsilon,
            channels=channels,
        )
    else:
        builder.add_batchnorm(
            name=node.name,
            channels=channels[0],
            gamma=scale,
            beta=bias,
            mean=mean,
            variance=var,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            epsilon=epsilon,
        )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_instancenorm(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    epsilon = node.attrs.get("epsilon", 1e-5)
    scale = node.input_tensors[node.inputs[1]]
    bias = node.input_tensors[node.inputs[2]]

    builder.add_batchnorm(
        name=node.name,
        channels=scale.shape[0],
        gamma=scale,
        beta=bias,
        compute_mean_var=True,
        instance_normalization=True,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        epsilon=epsilon,
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_mul(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    _convert_broadcast_op(builder, node, graph, err, "MULTIPLY")


def _convert_mean(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    _convert_broadcast_op(builder, node, graph, err, "AVE")


def _convert_div(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_unary(
        name=node.name + "_inverse",  # type: ignore
        input_name=node.inputs[1],
        output_name=node.inputs[1] + "_inverse",
        mode="inverse",
    )
    builder.add_elementwise(
        name=node.name,
        input_names=[node.inputs[0], node.inputs[1] + "_inverse"],
        output_name=node.outputs[0],
        mode="MULTIPLY",
    )
    if _is_input_shape_mapping_defined(node, graph):
        ranks = [len(graph.onnx_coreml_shape_mapping[input_]) for input_ in node.inputs]
        max_id = np.argmax(np.array(ranks))
        graph.onnx_coreml_shape_mapping[
            node.outputs[0]
        ] = graph.onnx_coreml_shape_mapping[node.inputs[max_id]]


def _convert_leaky_relu(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    alpha = node.attrs.get("alpha", 0.01)
    builder.add_activation(
        name=node.name,
        non_linearity="LEAKYRELU",
        params=[alpha],
        input_name=node.inputs[0],
        output_name=node.outputs[0],
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_concat(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    def _add_concat(input_names, output_names, **kwargs):
        kwargs["builder"].add_elementwise(
            name=kwargs["node"].name,
            input_names=input_names,
            output_name=output_names[0],
            mode=kwargs["mode"],
        )

    axis = node.attrs.get("axis", 1)
    parent_op_type = graph.blob_from_op_type.get(node.inputs[0], None)

    if _is_input_shape_mapping_defined(node, graph):
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        caxis = mapp[axis]
        if caxis == 0:
            _add_concat(
                node.inputs,
                node.outputs,
                node=node,
                builder=builder,
                mode="SEQUENCE_CONCAT",
            )
        elif caxis == 2:
            _add_concat(
                node.inputs, node.outputs, node=node, builder=builder, mode="CONCAT"
            )
        elif caxis == 3:
            _add_transpose_before_after(
                _add_concat,
                node.inputs,
                node.outputs,
                [0, 2, 1, 3],
                mode="CONCAT",
                node=node,
                builder=builder,
            )
        elif caxis == 4:
            _add_transpose_before_after(
                _add_concat,
                node.inputs,
                node.outputs,
                [0, 3, 2, 1],
                mode="CONCAT",
                node=node,
                builder=builder,
            )
        else:
            return err.unsupported_op_configuration(
                builder, node, graph, "Concat not supported along batch axis"
            )
    else:
        mode = None
        first_input_shape = None
        if node.inputs[0] in graph.shape_dict:
            first_input_shape = graph.shape_dict[node.inputs[0]]
            if (
                parent_op_type in _SEQUENCE_LAYERS_REGISTRY
                and len(first_input_shape) == 3
            ):
                if axis == 0:
                    mode = "SEQUENCE_CONCAT"
                if axis == 2:
                    mode = "CONCAT"
            elif (
                (len(first_input_shape) == 1 and axis == 0)
                or (len(first_input_shape) == 3 and axis == 0)
                or (len(first_input_shape) == 4 and axis == 1)
                or (len(first_input_shape) == 2 and axis == 1)
            ):
                mode = "CONCAT"
        else:  # shape info is not available. Fall back to guessing (ideally this should not happen)
            if axis == 0:
                mode = "SEQUENCE_CONCAT"
            elif axis == 1:
                mode = "CONCAT"
        if mode is None:
            return err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "Unsupported axis {} in input of shape".format(
                    axis, str(first_input_shape)
                ),
            )
        _add_concat(node.inputs, node.outputs, node=node, builder=builder, mode=mode)

    _update_shape_mapping_unchanged(node, graph, err)


def _convert_split(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    def _add_split(input_names, output_names, **kwargs):
        kwargs["builder"].add_split(
            name=kwargs["node"].name,
            input_name=input_names[0],
            output_names=output_names,
        )

    axis = node.attrs.get("axis", 0)
    splits = node.attrs.get("split", None)
    # check that splits are equal
    if splits:
        if splits.count(splits[0]) != len(splits):
            return err.unsupported_op_configuration(
                builder, node, graph, "Only Equal splits are supported"
            )

    if _is_input_shape_mapping_defined(node, graph):
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        if mapp[axis] == 2:
            _add_split(node.inputs, node.outputs, node=node, builder=builder)
        elif mapp[axis] == 0:
            _add_transpose_before_after(
                _add_split,
                node.inputs,
                node.outputs,
                [1, 0, 2, 3],
                builder=builder,
                node=node,
            )
        elif mapp[axis] == 3:
            _add_transpose_before_after(
                _add_split,
                node.inputs,
                node.outputs,
                [0, 2, 1, 3],
                builder=builder,
                node=node,
            )
        elif mapp[axis] == 4:
            _add_transpose_before_after(
                _add_split,
                node.inputs,
                node.outputs,
                [0, 3, 2, 1],
                builder=builder,
                node=node,
            )
        else:
            err.unsupported_op_configuration(
                builder, node, graph, "Split along Batch axis not supported"
            )
    else:
        if not (axis == 0 or axis == 1):
            return err.unsupported_op_configuration(
                builder, node, graph, "Unsupported axis {}".format(axis,)
            )
        _add_split(node.inputs, node.outputs, node=node, builder=builder)

    if _is_input_shape_mapping_defined(node, graph):
        for out_ in node.outputs:
            graph.onnx_coreml_shape_mapping[out_] = graph.onnx_coreml_shape_mapping[
                node.inputs[0]
            ]


def _convert_argmax(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    def _add_argmax_or_argmin(input_names, output_names, **kwargs):
        input_name = input_names[0]
        output_name = output_names[0]
        if kwargs["node"].op_type == "ArgMin":
            kwargs["builder"].add_elementwise(
                name=kwargs["node"].name + "_multiply_minus_1",  # type: ignore
                input_names=[input_name],
                output_name=input_name + "_multiply_minus_1",
                mode="MULTIPLY",
                alpha=-1,
            )
            input_name += "_multiply_minus_1"
        kwargs["builder"].add_reduce(
            name=kwargs["node"].name,
            input_name=input_name,
            output_name=output_name,
            axis=kwargs["coreml_axis"],
            mode="argmax",
        )

    """
    Conversion
    """
    axis = node.attrs.get("axis", 0)
    keepdims = node.attrs.get("keepdims", 1)

    input_name = node.inputs[0]
    output_name = node.outputs[0]

    if _is_input_shape_mapping_defined(node, graph):
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        coreml_axis = mapp[axis]
        coreml_axis_string = "C"
        if coreml_axis == 1:  # coreml_axis corresponds to the batch dimension
            return err.unsupported_op_configuration(
                builder, node, graph, "Cannot apply operation along Batch axis"
            )
        if coreml_axis != 0:
            coreml_axis_string = ["C", "H", "W"][coreml_axis - 2]
            _add_argmax_or_argmin(
                [input_name],
                [output_name],
                builder=builder,
                node=node,
                coreml_axis=coreml_axis_string,
            )
        else:  # coreml_axis corresponds to the sequence dimension
            _add_transpose_before_after(
                _add_argmax_or_argmin,
                [input_name],
                [output_name],
                [1, 0, 2, 3],
                builder=builder,
                node=node,
                coreml_axis=coreml_axis_string,
            )

    else:
        coreml_axis_string = _get_coreml_axis([axis], builder, node, graph, err)
        if coreml_axis_string not in ["C", "H", "W", "HW", "CHW"]:
            return err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "Unable to translate axes attribute to CoreML axis parameter for %s"
                % axis,
            )
        _add_argmax_or_argmin(
            [input_name],
            [output_name],
            builder=builder,
            node=node,
            coreml_axis=coreml_axis_string,
        )

    """
    update output shape map
    """
    if _is_input_shape_mapping_defined(node, graph):
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        if keepdims == 1:
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = mapp
        else:
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = (
                mapp[:axis] + mapp[axis + 1 :]
            )


def _convert_reduce(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None

    # CoreML reduction supported along: C, H, W, CHW, HW

    def _add_reduce(input_names, output_names, **kwargs):
        input_name = input_names[0]
        output_name = output_names[0]

        if "add_log" in kwargs and kwargs["add_log"]:
            if kwargs["node"].op_type == "ReduceLogSum":
                output_name = output_names[0] + "_before_log"

        kwargs["builder"].add_reduce(
            name=kwargs["node"].name + "_" + output_name,
            input_name=input_name,
            output_name=output_name,
            axis=kwargs["coreml_axis"],
            mode=kwargs["mode"],
        )

        if "add_log" in kwargs and kwargs["add_log"]:
            if node.op_type == "ReduceLogSum":
                kwargs["builder"].add_unary(
                    name=kwargs["node"].name + "_log",
                    input_name=output_name,
                    output_name=output_names[0],
                    mode="log",
                )

    """
    Conversion
    """
    input_name = node.inputs[0]
    output_name = node.outputs[0]

    axes = node.attrs.get("axes", None)
    keepdims = node.attrs.get("keepdims", 1)

    if axes is None:
        if node.inputs[0] in graph.onnx_coreml_shape_mapping:
            axes = range(0, len(graph.onnx_coreml_shape_mapping[node.inputs[0]]))
        elif node.inputs[0] in graph.shape_dict:
            axes = range(0, len(graph.shape_dict[node.inputs[0]]))
        else:
            return err.unsupported_op_configuration(
                builder, node, graph, "Shape inference failed for reduce op"
            )

    if node.op_type == "ReduceMean":
        mode = "avg"
    elif node.op_type == "ReduceL1":
        mode = "L1"
    elif node.op_type == "ReduceL2":
        mode = "L2"
    elif node.op_type == "ReduceLogSum":
        mode = "sum"
    elif node.op_type == "ReduceMax":
        mode = "max"
    elif node.op_type == "ReduceMin":
        mode = "min"
    elif node.op_type == "ReduceProd":
        mode = "prod"
    elif node.op_type == "ReduceSum":
        mode = "sum"
    elif node.op_type == "ReduceSumSquare":
        mode = "sumsquare"
    else:
        return err.unsupported_op_configuration(builder, node, graph, "Unsupported op")

    if _is_input_shape_mapping_defined(node, graph):
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        coreml_axis = ""
        for ind in [["S", "B", "C", "H", "W"][mapp[i]] for i in axes]:
            coreml_axis += ind
        coreml_axis = "".join(sorted(coreml_axis))
    else:
        coreml_axis = _get_coreml_axis(axes, builder, node, graph, err)

    if coreml_axis in ["C", "H", "W", "HW", "CHW"]:
        _add_reduce(
            [input_name],
            [output_name],
            builder=builder,
            node=node,
            coreml_axis=coreml_axis,
            mode=mode,
            add_log=True,
        )
    else:
        if node.op_type in ["ReduceMean"]:
            return err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "Unable to translate axes attribute to CoreML axis parameter for %s"
                % axes,
            )
        n = len(coreml_axis)
        for i, ax in enumerate(coreml_axis):
            if ax not in ["C", "H", "W"]:
                return err.unsupported_op_configuration(
                    builder,
                    node,
                    graph,
                    "Unable to translate axes attribute to CoreML axis parameter for %s"
                    % axes,
                )
            else:
                if i == 0:
                    iname = input_name
                else:
                    iname = input_name + str(i)
                if i == n - 1:
                    oname = output_name
                else:
                    oname = input_name + str(i + 1)
                if i < n - 1:
                    _add_reduce(
                        [iname],
                        [oname],
                        builder=builder,
                        node=node,
                        coreml_axis=ax,
                        mode=mode,
                        add_log=False,
                    )
                else:
                    _add_reduce(
                        [iname],
                        [oname],
                        builder=builder,
                        node=node,
                        coreml_axis=ax,
                        mode=mode,
                        add_log=True,
                    )

    """
    update output shape map
    """
    if _is_input_shape_mapping_defined(node, graph):
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        if keepdims == 1:
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = mapp
        else:
            out_mapp = []
            for i, m in enumerate(mapp):
                if i not in axes:
                    out_mapp.append(m)
            if len(out_mapp) == 0:
                out_mapp = [2]
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = out_mapp


def _convert_softmax(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    def _add_softmax(input_names, output_names, **kwargs):
        node = kwargs["node"]
        builder = kwargs["builder"]

        if node.op_type == "LogSoftmax":
            builder.add_softmax(
                name=node.name + "_softmax",  # type: ignore
                input_name=node.inputs[0],
                output_name=node.outputs[0] + "_softmax",
            )
            builder.add_unary(
                name=node.name,
                input_name=node.outputs[0] + "_softmax",
                output_name=node.outputs[0],
                mode="log",
            )
        else:
            builder.add_softmax(
                name=node.name, input_name=input_names[0], output_name=output_names[0]
            )

    axis = node.attrs.get("axis", 1)
    if axis != 1:
        return err.unsupported_op_configuration(
            builder, node, graph, "Unsupported axis {} for softmax".format(axis,)
        )

    _add_softmax(node.inputs, node.outputs, node=node, builder=builder)

    if node.inputs[0] in graph.onnx_coreml_shape_mapping:
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        graph.onnx_coreml_shape_mapping[node.outputs[0]] = mapp


def _convert_gemm(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None

    """
    operation:  alpha * (A * B) + beta * C
    so far the case only handled is :
    - B is a constant matrix
    - C is a constant vector
    - alpha == beta == 1.0
    - transA is off
    """

    if node.attrs.get("transA", 0) != 0:
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "This Gemm layer cannot be converted to CoreML inner_product layer",
        )

    if (
        abs(node.attrs.get("alpha", 1.0) - 1.0) > 1e-3
        or abs(node.attrs.get("beta", 1.0) - 1.0) > 1e-3
    ):
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "This Gemm layer cannot be converted to CoreML inner_product layer",
        )

    weight_name = node.inputs[1]
    if weight_name in node.input_tensors:
        W = node.input_tensors[weight_name]
        if not node.attrs.get("transB", 0):
            W = np.transpose(W)
    else:
        err.missing_initializer(node, "Second input to Gemm layer must be a constant")

    b = None
    if len(node.inputs) > 2:
        b = (node.input_tensors[node.inputs[2]]).flatten()
    if len(W.shape) != 2 or (b is not None and len(b.shape) != 1):
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "This Gemm layer cannot be converted to CoreML inner_product layer",
        )

    if b is not None:
        if W.shape[0] != b.shape[0]:
            return err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "This Gemm layer cannot be converted to CoreML inner_product layer",
            )

    if node.inputs[0] in graph.onnx_coreml_shape_mapping:
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        if mapp == [1, 2] or mapp == [0, 2]:  # [B,C] or [S,C]
            _add_inner_product(
                [node.inputs[0]], node.outputs, W=W, b=b, node=node, builder=builder
            )
        elif mapp == [3, 4]:  # [H,W]
            _add_transpose_before_after(
                _add_inner_product,
                [node.inputs[0]],
                node.outputs,
                [2, 3, 0, 1],
                W=W,
                b=b,
                node=node,
                builder=builder,
            )
        elif mapp == [2, 3]:  # (C,H)
            _add_transpose_before_after(
                _add_inner_product,
                [node.inputs[0]],
                node.outputs,
                [1, 2, 0, 3],
                W=W,
                b=b,
                node=node,
                builder=builder,
            )
        elif mapp == [2, 4]:  # (C,W)
            _add_transpose_before_after(
                _add_inner_product,
                [node.inputs[0]],
                node.outputs,
                [1, 3, 2, 0],
                W=W,
                b=b,
                node=node,
                builder=builder,
            )
        else:
            return err.unsupported_op_configuration(
                builder, node, graph, "CoreML incompatible axis placement"
            )
    else:
        _add_inner_product(
            [node.inputs[0]], node.outputs, W=W, b=b, node=node, builder=builder
        )

    if node.inputs[0] in graph.onnx_coreml_shape_mapping:
        graph.onnx_coreml_shape_mapping[
            node.outputs[0]
        ] = graph.onnx_coreml_shape_mapping[node.inputs[0]]


def _convert_matmul(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None

    weight_name = node.inputs[1]
    if weight_name in node.input_tensors:
        W = node.input_tensors[weight_name]
    else:
        err.missing_initializer(node, "Second input to Matmul layer must be a constant")

    if len(W.shape) != 2:
        return err.unsupported_op_configuration(
            builder,
            node,
            graph,
            "This Matmul layer cannot be converted to CoreML inner_product layer",
        )

    W = np.transpose(W)

    if node.inputs[0] in graph.onnx_coreml_shape_mapping:
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        if mapp == [1, 2] or mapp == [0, 2]:  # [B,C] or [S,C]
            _add_inner_product(
                [node.inputs[0]], node.outputs, W=W, b=None, node=node, builder=builder
            )
        elif mapp == [3, 4]:  # [H,W]
            _add_transpose_before_after(
                _add_inner_product,
                [node.inputs[0]],
                node.outputs,
                [2, 3, 0, 1],
                W=W,
                b=None,
                node=node,
                builder=builder,
            )
        elif mapp == [2, 3]:  # (C,H)
            _add_transpose_before_after(
                _add_inner_product,
                [node.inputs[0]],
                node.outputs,
                [1, 2, 0, 3],
                W=W,
                b=None,
                node=node,
                builder=builder,
            )
        elif mapp == [2, 4]:  # (C,W)
            _add_transpose_before_after(
                _add_inner_product,
                [node.inputs[0]],
                node.outputs,
                [1, 3, 2, 0],
                W=W,
                b=None,
                node=node,
                builder=builder,
            )
        else:
            return err.unsupported_op_configuration(
                builder, node, graph, "CoreML incompatible axis placement"
            )
    else:
        _add_inner_product(
            [node.inputs[0]], node.outputs, W=W, b=None, node=node, builder=builder
        )

    if node.inputs[0] in graph.onnx_coreml_shape_mapping:
        graph.onnx_coreml_shape_mapping[
            node.outputs[0]
        ] = graph.onnx_coreml_shape_mapping[node.inputs[0]]


def _convert_lrn(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    alpha = node.attrs.get("alpha", 1.0e-4)
    beta = node.attrs.get("beta", 0.75)
    bias = node.attrs.get("bias", 1.0)
    size = node.attrs["size"]
    builder.add_lrn(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        alpha=alpha,
        beta=beta,
        k=bias,
        local_size=size,
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_sigmoid(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_activation(
        name=node.name,
        non_linearity="SIGMOID",
        input_name=node.inputs[0],
        output_name=node.outputs[0],
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_sign(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_activation(
        name=node.name,
        non_linearity="SIGMOID_HARD",
        input_name=node.inputs[0],
        output_name=node.outputs[0] + "_step",
        params=[10000, 0],
    )
    builder.add_elementwise(
        name=node.name + "_subtract_half",
        input_names=node.outputs[0] + "_step",
        output_name=node.outputs[0] + "_step_half",
        mode="ADD",
        alpha=-0.5,
    )
    builder.add_elementwise(
        name=node.name + "_multiply_2",
        input_names=node.outputs[0] + "_step_half",
        output_name=node.outputs[0],
        mode="MULTIPLY",
        alpha=2,
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_elu(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    alpha = node.attrs.get("alpha", 1.0)
    builder.add_activation(
        name=node.name,
        non_linearity="ELU",
        params=alpha,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_selu(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    alpha = node.attrs.get("alpha", 1.6732)
    gamma = node.attrs.get("gamma", 1.0507)
    builder.add_activation(
        name=node.name + "_elu",  # type: ignore
        non_linearity="ELU",
        params=alpha,
        input_name=node.inputs[0],
        output_name=node.inputs[0] + "_elu",
    )
    builder.add_elementwise(
        name=node.name,
        input_names=node.inputs[0] + "_elu",
        output_name=node.outputs[0],
        mode="MULTIPLY",
        alpha=gamma,
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_prelu(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    if node.inputs[1] not in node.input_tensors:
        return err.unsupported_op_configuration(
            builder, node, graph, "Slope must be known!"
        )

    slope = node.input_tensors[node.inputs[1]]
    builder.add_activation(
        name=node.name,
        non_linearity="PRELU",
        params=slope,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_tanh(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_activation(
        name=node.name,
        non_linearity="TANH",
        input_name=node.inputs[0],
        output_name=node.outputs[0],
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_pad(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    def _get_pad_params(builder, node, graph, err, params_dict, axis=None):

        pads = node.attrs["pads"]
        if not (len(pads) % 2 == 0 and len(pads) >= 2):
            return err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "pads attribute: {}."
                "Length of pads must be a multiple of 2".format(str(pads)),
            )
        if len(pads) == 8:
            az = pads[:2] + pads[4:6]
            if az.count(0) != len(az):
                return err.unsupported_op_configuration(
                    builder,
                    node,
                    graph,
                    "Paddings value {} not supported".format(pads,),
                )
            pads = pads[2:4] + pads[6:8]

        if len(pads) == 6:
            az = pads[:2] + pads[3:5]
            if az.count(0) != len(az):
                return err.unsupported_op_configuration(
                    builder,
                    node,
                    graph,
                    "Paddings value {} not supported".format(pads,),
                )
            pads = [pads[2], pads[5]]

        pad_t, pad_b, pad_l, pad_r = 0, 0, 0, 0
        if axis == "height":
            pad_t, pad_b = pads
        elif axis == "width":
            pad_l, pad_r = pads
        else:
            pad_t, pad_l, pad_b, pad_r = pads
        params_dict["pad_t"] = pad_t
        params_dict["pad_b"] = pad_b
        params_dict["pad_l"] = pad_l
        params_dict["pad_r"] = pad_r

    def _add_pad(input_names, output_names, **kwargs):
        params_dict = kwargs["params_dict"]
        node = kwargs["node"]
        builder = kwargs["builder"]
        builder.add_padding(
            name=node.name,
            left=params_dict["pad_l"],
            right=params_dict["pad_r"],
            top=params_dict["pad_t"],
            bottom=params_dict["pad_b"],
            value=params_dict["value"],
            input_name=input_names[0],
            output_name=output_names[0],
            padding_type=params_dict["mode"],
        )

    params_dict = dict()
    mode = node.attrs["mode"]
    if mode == "reflect" or mode == b"reflect":
        mode = "reflection"
    elif mode == "edge" or mode == b"edge":
        mode = "replication"
    else:
        mode = "constant"
    params_dict["mode"] = mode
    params_dict["value"] = node.attrs.get("value", 0.0)

    _add_conv_like_op(_add_pad, _get_pad_params, params_dict, builder, node, graph, err)

    # update map
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_slice(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None

    if _is_no_op(builder, node, graph, err):
        return

    def _add_slice(input_names, output_names, **kwargs):
        node = kwargs["node"]
        builder = kwargs["builder"]
        params_dict = kwargs["params_dict"]
        builder.add_slice(
            name=node.name + "_" + output_names[0],
            input_name=input_names[0],
            output_name=output_names[0],
            axis=params_dict["axis"],
            start_index=params_dict["start_index"],
            end_index=params_dict["end_index"],
            stride=1,
        )

    params_dict = dict()
    starts = node.attrs["starts"]
    ends = node.attrs["ends"]
    axes = node.attrs.get("axes", range(len(starts)))

    if node.inputs[0] in graph.shape_dict:
        for ii, _ in enumerate(axes):
            if ends[ii] > INT_MAX:
                ends[ii] = graph.shape_dict[node.inputs[0]][ii]

    if _is_input_shape_mapping_defined(node, graph):
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        r = len(starts)
        for i, ax in enumerate(axes):
            params_dict["start_index"] = starts[i]
            params_dict["end_index"] = ends[i]
            if i == 0:
                iname = node.inputs[0]
            else:
                iname = node.inputs[0] + str(i)
            oname = node.inputs[0] + str(i + 1)
            if i == r - 1:
                oname = node.outputs[0]

            if mapp[ax] == 2:
                params_dict["axis"] = "channel"
                _add_slice(
                    [iname],
                    [oname],
                    node=node,
                    builder=builder,
                    params_dict=params_dict,
                )
            elif mapp[ax] == 3:
                params_dict["axis"] = "height"
                _add_slice(
                    [iname],
                    [oname],
                    node=node,
                    builder=builder,
                    params_dict=params_dict,
                )
            elif mapp[ax] == 4:
                params_dict["axis"] = "width"
                _add_slice(
                    [iname],
                    [oname],
                    node=node,
                    builder=builder,
                    params_dict=params_dict,
                )
            elif mapp[ax] == 0:
                params_dict["axis"] = "channel"
                _add_transpose_before_after(
                    _add_slice,
                    [iname],
                    [oname],
                    [1, 0, 2, 3],
                    node=node,
                    builder=builder,
                    params_dict=params_dict,
                )
            else:
                err.unsupported_op_configuration(
                    builder, node, graph, "cannot slice along batch axis"
                )
    else:
        params_dict["start_index"] = starts[0]
        params_dict["end_index"] = ends[0]
        input_shape = graph.shape_dict.get(node.inputs[0], None)
        if len(axes) != 1:
            return err.unsupported_op_configuration(
                builder, node, graph, "Only single axis Slice is supported now"
            )
        if input_shape and len(input_shape) == 4 and len(axes) == 1:
            axis = ["B", "channel", "height", "width"][axes[0]]
        elif len(axes) == 1:
            if axes[0] == 0:
                axis = "channel"
            elif axes[0] == 1:
                axis = "height"
            elif axes[0] == 2:
                axis = "width"
            else:
                return err.unsupported_op_configuration(
                    builder,
                    node,
                    graph,
                    "Slice is supported only along H, W or C dimensions",
                )
        else:
            return err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "Slice is supported only along one axis for 3D or 4D Tensors",
            )
        params_dict["axis"] = axis
        _add_slice(
            node.inputs,
            node.outputs,
            builder=builder,
            node=node,
            params_dict=params_dict,
        )

    _update_shape_mapping_unchanged(node, graph, err)


def _convert_exp(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_unary(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mode="exp",
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_pow(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None

    input2 = node.inputs[1]
    is_supported = False
    if input2 in node.input_tensors:
        alpha = node.input_tensors[input2]
        if len(alpha.shape) == 0:
            is_supported = True

    if not is_supported:
        err.missing_initializer(
            node, "Only mode supported is when the second input is a scalar constant"
        )

    builder.add_unary(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mode="power",
        alpha=float(alpha),
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_flatten(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    def _add_flatten(input_names, output_names, **kwargs):
        kwargs["builder"].add_flatten(
            name=kwargs["node"].name,
            input_name=input_names[0],
            output_name=output_names[0],
            mode=0,
        )

    axis = node.attrs.get("axis", 1)
    if node.inputs[0] in graph.onnx_coreml_shape_mapping:
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        if (mapp[0] == 0 or mapp[0] == 1) and (axis == 0 or axis == 1):
            _add_flatten(node.inputs, node.outputs, builder=builder, node=node)
        elif mapp[0:2] == [0, 1] and axis == 2:
            _add_flatten(node.inputs, node.outputs, builder=builder, node=node)
        elif len(mapp) == 1 and axis == 1 and mapp[0] < 4:
            _add_flatten(node.inputs, node.outputs, builder=builder, node=node)
        else:
            return err.unsupported_op_configuration(
                builder, node, graph, "Flatten axis mode not supported"
            )
    else:
        _add_flatten(node.inputs, node.outputs, builder=builder, node=node)

    if node.inputs[0] in graph.onnx_coreml_shape_mapping:
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        if len(mapp) == 1 and axis == 1 and mapp[0] < 4:
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = [mapp[0], mapp[0] + 1]
        else:
            graph.onnx_coreml_shape_mapping[node.outputs[0]] = [mapp[0], 2]


def _convert_max(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    _convert_broadcast_op(builder, node, graph, err, "MAX")


def _convert_min(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    _convert_broadcast_op(builder, node, graph, err, "MIN")


def _convert_softsign(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_activation(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        non_linearity="SOFTSIGN",
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_softplus(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_activation(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        non_linearity="SOFTPLUS",
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_hardsigmoid(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    alpha = node.attrs.get("alpha", 0.2)
    beta = node.attrs.get("beta", 0.5)
    builder.add_activation(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        non_linearity="SIGMOID_HARD",
        params=[alpha, beta],
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_neg(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_elementwise(
        name=node.name,
        input_names=node.inputs,
        output_name=node.outputs[0],
        mode="MULTIPLY",
        alpha=-1.0,
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_log(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_unary(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mode="log",
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_sqrt(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_unary(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mode="sqrt",
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_reciprocal(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_unary(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mode="inverse",
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_reorganize_data(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    mode = "SPACE_TO_DEPTH"
    if node.op_type == "DepthToSpace":
        mode = "DEPTH_TO_SPACE"
    block_size = node.attrs.get("blocksize", 2)
    builder.add_reorganize_data(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mode=mode,
        block_size=block_size,
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_upsample(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    if "scales" in node.attrs:
        scales = node.attrs["scales"]
        if len(scales) != 4 or scales[0] != 1.0 or scales[1] != 1.0:
            err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "Unsupported scales {} for upsample".format(scales),
            )
        height_scale = int(scales[2])
        width_scale = int(scales[3])
    elif len(node.input_tensors):
        key = next(iter(node.input_tensors.keys()))
        scales = node.input_tensors[key]
        height_scale = int(scales[2])
        width_scale = int(scales[3])
    else:
        if len(node.inputs) > 1:
            return err.unsupported_op_configuration(
                builder,
                node,
                graph,
                "This ONNX upsample layer has 'scales' provided as an input. CoreML upsample requires 'scales' as an attribute of the layer.",
            )
        height_scale = int(node.attrs.get("height_scale", 1))
        width_scale = int(node.attrs.get("width_scale", 1))
    mode_convert = {
        "nearest": "NN",
        "linear": "BILINEAR",
    }
    mode = mode_convert[node.attrs["mode"].decode("UTF-8")]
    builder.add_upsample(
        name=node.name,
        scaling_factor_h=height_scale,
        scaling_factor_w=width_scale,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        mode=mode,
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_clip(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    # clip(x, a, b) = max(min(x, a), b) = -min(-min(x, a), -b)

    if node.attrs.get("max") is None:
        min_limit = node.attrs.get("min", float(-(2 ** 16) - 1))
        builder.add_unary(
            name=node.name,
            input_name=node.inputs[0],
            output_name=node.outputs[0],
            mode="threshold",
            alpha=min_limit,
            shift=0,
            scale=1.0,
        )
    elif node.attrs.get("min") is None:
        max_limit = node.attrs.get("max", float(2 ** 16 - 1))
        builder.add_unary(
            name=node.name + "_min_minus_x_minus_b",
            input_name=node.inputs[0],
            output_name=node.inputs[0] + "_min_minus_x_minus_b",
            mode="threshold",
            alpha=-max_limit,
            shift=0,
            scale=-1.0,
        )

        builder.add_activation(
            name=node.name,
            non_linearity="LINEAR",
            input_name=node.inputs[0] + "_min_minus_x_minus_b",
            output_name=node.outputs[0],
            params=[-1.0, 0],
        )

    else:
        min_limit = node.attrs.get("min")
        max_limit = node.attrs.get("max")
        builder.add_unary(
            name=node.name + "_min_x_a",
            input_name=node.inputs[0],
            output_name=node.inputs[0] + "_min_x_a",
            mode="threshold",
            alpha=min_limit,
            shift=0,
            scale=1.0,
        )

        builder.add_unary(
            name=node.name + "_min_minus_x_minus_b",
            input_name=node.inputs[0] + "_min_x_a",
            output_name=node.inputs[0] + "_min_minus_x_minus_b",
            mode="threshold",
            alpha=-max_limit,
            shift=0,
            scale=-1.0,
        )

        builder.add_activation(
            name=node.name,
            non_linearity="LINEAR",
            input_name=node.inputs[0] + "_min_minus_x_minus_b",
            output_name=node.outputs[0],
            params=[-1.0, 0],
        )

    _update_shape_mapping_unchanged(node, graph, err)


def _convert_mvn(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_mvn(
        name=node.name,
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        across_channels=node.attrs.get("across_channels", 0),
        normalize_variance=node.attrs.get("normalize_variance", 1),
        epsilon=1e-5,
    )
    _update_shape_mapping_unchanged(node, graph, err)


def _convert_lstm(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    W_name = node.inputs[1]
    R_name = node.inputs[2]
    B = None
    if len(node.inputs) > 3:
        B_name = node.inputs[3]
        B = node.input_tensors.get(B_name, None)
    W = node.input_tensors.get(W_name, None)
    R = node.input_tensors.get(R_name, None)
    if W is None:
        err.missing_initializer(
            node, "Weight tensor: {} not found in the graph initializer".format(W_name,)
        )
    if R is None:
        err.missing_initializer(
            node, "Weight tensor: {} not found in the graph initializer".format(R_name,)
        )

    h = node.attrs["hidden_size"]
    W_i, W_o, W_f, W_c = np.split(np.squeeze(W), 4)  # type: ignore
    R_i, R_o, R_f, R_c = np.split(np.squeeze(R), 4)  # type: ignore
    x = W_i.shape[1]
    h = W_i.shape[0]
    W_x = [W_i, W_f, W_o, W_c]
    W_h = [R_i, R_f, R_o, R_c]
    b = None
    if B is not None:
        b_Wi, b_Wo, b_Wf, b_Wc, b_Ri, b_Ro, b_Rf, b_Rc = np.split(np.squeeze(B), 8)  # type: ignore
        b = [b_Wi + b_Ri, b_Wf + b_Rf, b_Wo + b_Ro, b_Wc + b_Rc]

    input_h = node.inputs[5] if len(node.inputs) > 5 else node.inputs[0] + "_h_input"
    input_c = node.inputs[6] if len(node.inputs) > 6 else node.inputs[0] + "_c_input"
    output_h = (
        node.outputs[1] if len(node.outputs) > 1 else node.outputs[0] + "_h_output"
    )
    output_c = (
        node.outputs[2] if len(node.outputs) > 2 else node.outputs[0] + "_c_output"
    )

    graph.optional_inputs.append((input_h, (h)))
    graph.optional_inputs.append((input_c, (h)))
    graph.optional_outputs.append((output_h, (h)))
    graph.optional_outputs.append((output_c, (h)))

    builder.add_unilstm(
        name=node.name,
        W_h=W_h,
        W_x=W_x,
        b=b,
        hidden_size=h,
        input_size=x,
        input_names=[node.inputs[0], input_h, input_c],
        output_names=[node.outputs[0], output_h, output_c],
        inner_activation="SIGMOID",
        cell_state_update_activation="TANH",
        output_activation="TANH",
        peep=None,
        output_all=True,
        forget_bias=False,
        coupled_input_forget_gate=False,
        cell_clip_threshold=50000.0,
        reverse_input=False,
    )

    if _is_input_shape_mapping_defined(node, graph):
        graph.onnx_coreml_shape_mapping[
            node.outputs[0]
        ] = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        graph.onnx_coreml_shape_mapping[
            node.outputs[1]
        ] = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        graph.onnx_coreml_shape_mapping[
            node.outputs[2]
        ] = graph.onnx_coreml_shape_mapping[node.inputs[0]]


def _convert_custom(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    params = NeuralNetwork_pb2.CustomLayerParams()
    params.className = node.op_type
    params.description = "Custom layer that corresponds to the ONNX op {}".format(
        node.op_type,
    )

    inputs_ = []
    # skip the inputs that are initializers
    for inp in node.inputs:
        if inp not in node.input_tensors:
            inputs_.append(inp)

    builder.add_custom(
        name=node.name,
        input_names=inputs_,
        output_names=node.outputs,
        custom_proto_spec=params,
    )
    err.custom_layer_nodes.append(node)


def _convert_identity(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    builder.add_activation(
        name=node.name,
        non_linearity="LINEAR",
        input_name=node.inputs[0],
        output_name=node.outputs[0],
        params=[1.0, 0.0],
    )
    if _is_input_shape_mapping_defined(node, graph):
        mapp = graph.onnx_coreml_shape_mapping[node.inputs[0]]
        mapp_out = []
        if node.op_type == "Squeeze":
            axes = node.attrs.get("axes", None)
            if axes is None:
                if node.inputs[0] not in graph.shape_dict:
                    return err.unsupported_op_configuration(
                        builder, node, graph, "shape not known"
                    )
                else:
                    ishape = graph.shape_dict[node.inputs[0]]
                    if ishape.count(1) == len(ishape):
                        mapp_out = [2]
                    else:
                        for i, d in enumerate(ishape):
                            if d != 1:
                                mapp_out.append(mapp[i])
            else:
                for i, a in enumerate(mapp):
                    if i in axes:
                        continue
                    else:
                        mapp_out.append(a)
                if len(mapp_out) == 0:
                    mapp_out = [2]
        elif node.op_type == "Unsqueeze":
            axes = node.attrs["axes"]
            available_set = [0, 1, 2, 3, 4]
            for d in mapp:
                if d in available_set:
                    available_set.remove(d)
            if len(axes) > len(available_set):
                return err.unsupported_op_configuration(
                    builder,
                    node,
                    graph,
                    "cannot unsqueeze to a dimension greater than 5",
                )
            mapp_out = [1] * (len(axes) + len(mapp))
            mapp_ptr = 0
            available_set_ptr = 0
            for i in range(len(mapp_out)):
                if i in axes:
                    mapp_out[i] = available_set[available_set_ptr]
                    available_set_ptr += 1
                else:
                    mapp_out[i] = mapp[mapp_ptr]
                    mapp_ptr += 1
        else:
            raise ValueError("convert_identity incorrectly called")
        graph.onnx_coreml_shape_mapping[node.outputs[0]] = mapp_out


def _convert_const(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None

    mapp = None
    for input_ in node.inputs:
        if input_ in graph.onnx_coreml_shape_mapping:
            mapp = graph.onnx_coreml_shape_mapping[input_]

    for name, value in node.input_tensors.items():
        output_name = name
        if name not in graph.constant_layers_added:
            add_transpose_later = False
            shape = value.shape
            coreml_shape = [1, 1, 1]
            if len(shape) == 0:
                graph.onnx_coreml_shape_mapping[name] = [2]  # [C]
            elif len(shape) == 3:
                coreml_shape = list(shape)
                graph.onnx_coreml_shape_mapping[name] = [2, 3, 4]  # [C,H,W]
            elif len(shape) == 1:
                coreml_shape = [shape[0], 1, 1]
                graph.onnx_coreml_shape_mapping[name] = [2]  # [C]
            elif len(shape) == 2:
                coreml_shape = [1, shape[0], shape[1]]
                if mapp is not None and (mapp == [1, 2] or mapp == [0, 2]):
                    add_transpose_later = True
                    transpose_dims = [2, 3, 0, 1]
                    graph.onnx_coreml_shape_mapping[name] = [0, 2]  # [S,C]
                else:
                    graph.onnx_coreml_shape_mapping[name] = [3, 4]  # [H,W]
            else:
                return err.unsupported_op_configuration(
                    builder,
                    node,
                    graph,
                    "unable to translate constant array shape to CoreML shape",
                )

            if add_transpose_later:
                output_name += "_pre_transpose"
            builder.add_load_constant(
                name=output_name,
                output_name=output_name,
                constant_value=value.flatten(),
                shape=coreml_shape,
            )
            if add_transpose_later:
                builder.add_permute(
                    name=name,
                    dim=transpose_dims,
                    input_name=output_name,
                    output_name=name,
                )

            graph.constant_layers_added[output_name] = True


_ONNX_NODE_REGISTRY = {
    "Abs": _convert_abs,
    "Add": _convert_add,
    "ArgMax": _convert_argmax,
    "ArgMin": _convert_argmax,
    "AveragePool": _convert_pool,
    "BatchNormalization": _convert_bn,
    "Clip": _convert_clip,
    "Concat": _convert_concat,
    "Conv": _convert_conv,
    "ConvTranspose": _convert_conv,
    "DepthToSpace": _convert_reorganize_data,
    "Div": _convert_div,
    "Elu": _convert_elu,
    "Exp": _convert_exp,
    "Flatten": _convert_flatten,  # Todo: handle more cases
    "Gemm": _convert_gemm,
    "GlobalAveragePool": _convert_pool,
    "GlobalMaxPool": _convert_pool,
    "HardSigmoid": _convert_hardsigmoid,
    "InstanceNormalization": _convert_instancenorm,
    "LeakyRelu": _convert_leaky_relu,
    "Log": _convert_log,
    "LogSoftmax": _convert_softmax,
    "LRN": _convert_lrn,
    "LSTM": _convert_lstm,
    "MatMul": _convert_matmul,
    "Max": _convert_max,
    "MaxPool": _convert_pool,
    "Mean": _convert_mean,
    "MeanVarianceNormalization": _convert_mvn,
    "Min": _convert_min,
    "Mul": _convert_mul,
    "Neg": _convert_neg,
    "Pad": _convert_pad,
    "Pow": _convert_pow,
    "PRelu": _convert_prelu,
    "Reciprocal": _convert_reciprocal,
    "ReduceL1": _convert_reduce,
    "ReduceL2": _convert_reduce,
    "ReduceLogSum": _convert_reduce,
    "ReduceMax": _convert_reduce,
    "ReduceMean": _convert_reduce,
    "ReduceMin": _convert_reduce,
    "ReduceProd": _convert_reduce,
    "ReduceSum": _convert_reduce,
    "ReduceSumSquare": _convert_reduce,
    "Relu": _convert_relu,
    "Reshape": _convert_reshape,
    "Selu": _convert_selu,
    "Sigmoid": _convert_sigmoid,
    "Sign": _convert_sign,
    "Slice": _convert_slice,
    "Softmax": _convert_softmax,  # Todo: handle more cases
    "Softplus": _convert_softplus,
    "Softsign": _convert_softsign,
    "SpaceToDepth": _convert_reorganize_data,
    "SpatialBN": _convert_bn,
    "Split": _convert_split,
    "Sqrt": _convert_sqrt,
    "Squeeze": _convert_identity,
    "Sub": _convert_sub,
    "Sum": _convert_add,
    "Tanh": _convert_tanh,
    "ThresholdedRelu": _convert_thresholdedrelu,
    "Transpose": _convert_transpose,
    "Unsqueeze": _convert_identity,
    "Upsample": _convert_upsample,
}

_SEQUENCE_LAYERS_REGISTRY = set(["LSTM"])

_CONST_INPUT_ALLOWED_LAYERS = set(
    ["Add", "Sub", "Sum", "Mul", "Concat", "Max", "Min", "Div", "Reciprocal"]
)


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
    elif op_type in _ONNX_NODE_REGISTRY:
        return _ONNX_NODE_REGISTRY[op_type]
    else:
        return err.unsupported_op(node)


def _add_const_inputs_if_required(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    if node.op_type in _CONST_INPUT_ALLOWED_LAYERS:
        if len(node.input_tensors) > 0:
            _convert_const(builder, node, graph, err)


def _convert_node(
    builder, node, graph, err
):  # type: (NeuralNetworkBuilder, Node, Graph, ErrorHandling) -> None
    converter_fn = _get_node_converter_fn(builder, node, err)
    return converter_fn(builder, node, graph, err)

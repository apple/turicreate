# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as np

from coremltools.converters.mil.mil import types
from ..parsed_tf_node import ParsedTFNode
from ..basic_graph_ops import replace_source, delete_node


def _try_same(input_h, input_w, W_h, W_w, dilation_factor, padding, crop):
    base_paddings = [0] * 4

    dilated_W_h = dilation_factor[0] * (W_h - 1) + 1
    dilated_W_w = dilation_factor[1] * (W_w - 1) + 1

    base_paddings[0] = (dilated_W_h - 1) // 2
    base_paddings[1] = dilated_W_h - 1 - (dilated_W_h - 1) // 2
    base_paddings[2] = (dilated_W_w - 1) // 2
    base_paddings[3] = dilated_W_w - 1 - (dilated_W_w - 1) // 2

    pad_start_h = base_paddings[0]
    pad_start_w = base_paddings[2]
    orig_pad_end_h = base_paddings[1]
    orig_pad_end_w = base_paddings[3]
    full_input_h = input_h + pad_start_h + orig_pad_end_h
    full_input_w = input_w + pad_start_w + orig_pad_end_w
    pad_end_extra_h = (
        dilation_factor[0] - full_input_h % dilation_factor[0]
    ) % dilation_factor[0]
    pad_end_extra_w = (
        dilation_factor[1] - full_input_w % dilation_factor[1]
    ) % dilation_factor[1]
    pad_end_h = orig_pad_end_h + pad_end_extra_h
    pad_end_w = orig_pad_end_w + pad_end_extra_w

    return (
        padding[0] == pad_start_h
        and padding[1] == pad_end_h
        and padding[2] == pad_start_w
        and padding[3] == pad_end_w
        and crop[0] == 0
        and crop[1] == pad_end_extra_h
        and crop[2] == 0
        and crop[3] == pad_end_extra_w
    )


def _pattern_match_and_rewrite(gddict, conv_op):
    node = gddict[conv_op]
    channel_first = node.attr["data_format"].startswith("NC")

    if len(node.inputs) == 0 or len(node.outputs) == 0:
        return

    prev_node = gddict[node.inputs[0]]
    next_node = gddict[node.outputs[0]]

    expand_node = None
    squeeze_node = None
    # Check for Conv1D cases
    if prev_node.op == "ExpandDims":
        # All Conv1D has ExpandDims and Squeeze as pairs.
        if next_node.op != "Squeeze":
            return

        expand_node = prev_node
        squeeze_node = next_node

        if len(prev_node.inputs) == 0 or len(next_node.outputs) == 0:
            return
        prev_node = gddict[prev_node.inputs[0]]
        next_node = gddict[next_node.outputs[0]]

    # Check if Conv1D/Conv2D is surrounded by SpaceToBatchND and BatchToSpaceND
    if prev_node.op != "SpaceToBatchND" or next_node.op != "BatchToSpaceND":
        return
    else:
        stb_node = prev_node
        bts_node = next_node

    dilation_node = gddict[stb_node.inputs[1]]
    if dilation_node.value is None:
        return
    dilation_factor = dilation_node.value.val
    if gddict[bts_node.inputs[1]].value is None or np.any(
        dilation_factor != gddict[bts_node.inputs[1]].value.val
    ):
        # If SpaceToBatchND and BatchToSpaceND doesn't match, we do not fuse.
        return

    padding_node = gddict[stb_node.inputs[2]]
    if padding_node.value is None:
        return
    padding_val = padding_node.value.val.flatten()

    crop_node = gddict[bts_node.inputs[2]]
    if crop_node.value is None:
        return
    crop_val = crop_node.value.val.flatten()

    if expand_node:
        dilation_factor = [1] + list(dilation_factor)
        padding_val = [0, 0] + list(padding_val)
        crop_val = [0, 0] + list(crop_val)
    # Trying to inverse the logic of TF generating padding/cropping values for
    # SpaceToBatchND and BatchToSpaceND with different padding values in Conv2D.
    # Logic extracted from TF's builder at:
    # tensorflow/python/ops/nn_ops.py and tensorflow/python/ops/array_ops.py
    is_same = False
    if np.any(padding_val != 0):
        input_shape = gddict[stb_node.inputs[0]].attr.get("_output_shapes", None)
        if input_shape is None:
            input_shape = gddict[stb_node.inputs[0]].attr.get("shape", None)
        else:
            input_shape = input_shape[0]
        W_node = gddict[node.inputs[1]]
        W_shape = None if W_node.op != "Const" else W_node.datatype.get_shape()
        if input_shape is None or W_shape is None:
            return
        W_h, W_w = W_shape[0], W_shape[1]
        HW = input_shape[2:] if channel_first else input_shape[1:-1]
        if expand_node:
            HW = [1] + list(HW)
        is_same = _try_same(
            HW[0], HW[1], W_h, W_w, dilation_factor, padding_val, crop_val
        )

    # Re-wiring the nodes to skip SpaceToBatchND.
    # We change BatchToSpaceND to Identity since it might be a terminate op.
    deleted_nodes = set()
    if expand_node:
        replace_source(gddict, stb_node, expand_node, stb_node.inputs[0])
    else:
        replace_source(gddict, stb_node, node, stb_node.inputs[0])

    bts_node.op = "Identity"
    bts_node.attr = {}

    deleted_nodes.update(stb_node.inputs[1:])
    deleted_nodes.update([stb_node.name])
    deleted_nodes.update(bts_node.inputs[1:])

    # Rewrite dilation attribute for (Depthwise)Conv2D
    dilation_val = (
        [1, 1] + list(dilation_factor)
        if node.attr["data_format"] == "NCHW"
        else [1] + list(dilation_factor) + [1]
    )
    node.attr["dilations"] = dilation_val
    # Rewrite padding attribute for (Depthwise)Conv2D
    # This is due to, TF always plug in VALID padding for Conv2D after
    # SpaceToBatchND. If, the original Conv2D is SAME padding, TF would
    # automatically insert padding, therefore, we set it as SAME over here.
    if is_same:
        node.attr["padding"] = "SAME"

    # Removing stale attributes for nodes.
    if expand_node and "_output_shapes" in expand_node.attr:
        del expand_node.attr["_output_shapes"]
    if squeeze_node and "_output_shapes" in squeeze_node.attr:
        del squeeze_node.attr["_output_shapes"]
    if "_output_shapes" in node.attr:
        del node.attr["_output_shapes"]
    if expand_node and "shape" in expand_node.attr:
        del expand_node.attr["shape"]
    if squeeze_node and "shape" in squeeze_node.attr:
        del squeeze_node.attr["shape"]
    if "shape" in node.attr:
        del node.attr["shape"]

    for d in deleted_nodes:
        delete_node(gddict, d)


def _fuse_dilation_conv(gddict):
    """
    A dilated convolution in older tensorflow versions might not be fused in the
    Conv2D or DepthwiseConv2D op, but represented with the following format:

        SpaceToBatchND -> (Depthwise)Conv2D -> BatchToSpaceND

    We try to fuse it back into (Depthwise)Conv2D with the dilation parameter
    set in attribute.
    There are several patterns that exist in tensorflow for breaking up dilation
    convolutions. We detect the following patterns:

      SpaceToBatchND -> ExpandDims -> Conv2D -> Squeeze -> BatchToSpaceND

      SpaceToBatchND -> Conv2D -> BatchToSpaceND

    The first case appears when Conv1D is used, TF expands/squeeze the inputs to
    conform Conv2D pattern.
    The second case is a basic Conv2D pattern.

    """
    for name in list(gddict.keys()):
        if name not in gddict:
            # Node might have been removed from graph during fusion.
            continue
        node = gddict[name]
        if node.op in {"Conv2D", "DepthwiseConv2dNative"}:
            _pattern_match_and_rewrite(gddict, name)


def fuse_dilation_conv(tfssa):
    """
    Tensorflow decomposes Depthwise Convolution with dialtion into:

    SpaceToBatchND ---> Conv2D/DepthwiseConv2D ---> BatchToSpaceND

    We identify such pattern and use Conv2D/DepthwiseConv2D to represent it.
    """
    for f in tfssa.functions.keys():
        _fuse_dilation_conv(tfssa.functions[f].graph)

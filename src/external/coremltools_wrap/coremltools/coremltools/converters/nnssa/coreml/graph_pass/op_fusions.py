# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as np
from ...commons import builtins
from ...commons.basic_graph_ops import disconnect_edge, connect_edge, \
    delete_node, replace_node, connect_dests, topsort
from ...nnssa import ParsedNode


ELEMENTWISE_OPS = {
    'Maximum',
    'Minimum',
    'Add',
    'Sub',
    'BiasAdd',
    'Mul',
    'RealDiv',
    'Sigmoid',
    'Relu',
    'Relu6',
    'LeakyRelu',
    'Tanh',
    'Identity',
    'Sqrt',
    'Rsqrt',
    'Pow',
    'LRN',
    'Square',
    'SquaredDifference'
}

# Native SSA nodes with data_format attributes of NHWC / NCHW
NATIVE_NHWC_OPS = {
    'Conv2D', 'Conv2DBackpropInput', 'DepthwiseConv2dNative',
    'Pooling', 'MaxPool', 'AvgPool', 'DepthToSpace', 'SpaceToDepth',
}

REDUCTION_OPS = {
    'Mean',
    'Max'
}


def _check_number_inputs(node, n):
    return len(node.inputs) == n


def _check_number_outputs(node, n):
    return len(node.outputs) == n


def _check_single_out_vector_constant_node(node):
    return node.op == 'Const' and len(node.outputs) == 1 and \
           node.value is not None and len(np.squeeze(node.value.val).shape) == 1


def _is_NHWC(graph, node):
    if node.op == 'ResizeBilinear' or node.op == 'ResizeNearestNeighbor' \
            or node.op == 'MirrorPad':
        return True
    if node.op in NATIVE_NHWC_OPS and node.attr.get('data_format') == 'NHWC':
        return True

    if node.op == 'Concat':  # Concat's first input is axis
        return all(graph[inp].attr.get('data_format') == 'NHWC_format_inserted'
                   for inp in node.inputs[1:])
    if node.op == 'ConcatV2':  # ConcatV2's last input is axis
        return all(graph[inp].attr.get('data_format') == 'NHWC_format_inserted'
                   for inp in node.inputs[:-1])

    if node.op == 'Pad':
        # adjust constant padding values
        parent_node = graph[node.inputs[1]]
        val = np.array(parent_node.value.val)
        if len(val) == 4 and builtins.is_tensor(parent_node.datatype) and len(parent_node.outputs) == 1:
            parent_node.value.val = parent_node.value.val[[0, 3, 1, 2]]
        return True

    if node.op in REDUCTION_OPS:
        if not any([graph[inp].attr.get('data_format' '') ==
                    'NHWC_format_inserted' for inp in node.inputs]):
            return False
        # adjust axis / dims / reduction_indices values
        for inp in node.inputs:
            parent_node = graph[inp]
            if parent_node.value is not None:
                val = np.array(parent_node.value.val)
                m_nhwc_to_nchw = {0: 0, 1: 2, 2: 3, 3: 1}
                reduction_indices = np.array([m_nhwc_to_nchw[x] for x in val], dtype=np.int32)
                parent_node.value.val = np.reshape(reduction_indices, parent_node.value.val.shape)
                node.attr['reduction_indices'] = reduction_indices
        return True

    if node.op in ELEMENTWISE_OPS:
        # if its an element-wise op and if all of its parent(s) are
        # "NHWC_format_inserted" or given that at least one of the parents
        # is "NHWC_format_inserted" and rest are vector constants, then the
        # node is also declared to be "NHWC_format_inserted"

        NHWC_parent = any([graph[inp].attr.get('data_format',
            None) == 'NHWC_format_inserted' for inp in node.inputs])

        if NHWC_parent:
            for inp in node.inputs:
                parent_node = graph[inp]
                if parent_node.attr.get('data_format', None) == 'NHWC_format_inserted':
                    continue
                elif parent_node.value is not None:
                    val = np.array(parent_node.value.val)
                    # constant scalar
                    if val.shape == () and not builtins.is_tensor(parent_node.datatype) and len(parent_node.outputs) == 1:
                        continue
                    # constant vector
                    if len(val.shape) == 1 and builtins.is_tensor(parent_node.datatype) and len(parent_node.outputs) == 1:
                        continue
                    else:
                        return False
                else:
                    return False
            return True

    return False


def _insert_transpose_to_or_from_nchw(graph, src, dst, transpose_node_name, transpose_params=None):
    """
    Insert a node called 'transpose_node_name' between src and dst
    This node should be a transpose node with params 'transpose_params'
    """

    if not transpose_params:
        transpose_params = [0, 3, 1, 2]

    # First check whether the node already exists in the graph or not.

    if transpose_node_name in graph:
        tp_node = graph[transpose_node_name]
        if dst.name not in tp_node.outputs:
            tp_node.outputs.append(dst.name)
    else:
        # the node does not exist, so create a fresh one
        tp_node = ParsedNode()
        tp_node.op = 'Transpose'
        tp_node.name = transpose_node_name

        # Adjust type inference
        if builtins.is_tensor(src.datatype):
            s = src.datatype.get_shape()
            if len(s) == 4:
                tp_shape = tuple([s[transpose_params[0]], s[transpose_params[1]], s[transpose_params[2]], s[transpose_params[3]]])
                tp_node.datatype = builtins.tensor(src.datatype.get_primitive(), tp_shape)

        tp_node.inputs = [src.name]
        tp_node.outputs = [dst.name]
        tp_node.attr['dim'] = transpose_params
        if '_output_shapes' in src.attr:
            input_shape = src.attr['_output_shapes'][0]
            tp_node.attr['_output_shapes'] = [
                [input_shape[transpose_params[0]],
                 input_shape[transpose_params[1]],
                 input_shape[transpose_params[2]],
                 input_shape[transpose_params[3]]]
            ]
        graph[transpose_node_name] = tp_node

    # Rename dst's input 'src' to 'transpose_node_name'
    for idx, inp in enumerate(dst.inputs):
        if inp == src.name:
            dst.inputs[idx] = transpose_node_name
            break

    # Rename src's output from 'dst' to 'transpose_node_name'
    if transpose_node_name in src.outputs:
        # 'transpose_node_name' already exists as an output of the src,
        # we just need to delete dst node from the output list of src, instead of replacing it
        if dst.name in src.outputs:
            src.outputs.remove(dst.name)
    else:
        for idx, outp in enumerate(src.outputs):
            if outp == dst.name:
                src.outputs[idx] = transpose_node_name
                break


def _insert_transpose_to_nchw(graph, src, dst):
    tp_node_name = src.name + "_to_nchw"
    _insert_transpose_to_or_from_nchw(graph, src, dst, tp_node_name, [0, 3, 1, 2])


def _insert_transpose_from_nchw(graph, src, dst):
    tp_node_name = src.name + "_to_nhwc"
    _insert_transpose_to_or_from_nchw(graph, src, dst, tp_node_name, [0, 2, 3, 1])


def transform_nhwc_to_nchw(nnssa):
    """
    Mark each one of the node with "NHWC", so that the conversion process
    could avoid inserting unnecessary transpositions.
    A node's format is "NHWC" if and only if:
    (1) it is a conv or pooling or image_resize layer with "NHWC" data format
    (2) it is a rank-preserving operation whose inputs are all "NHWC"
    """
    for fn_key in list(nnssa.functions.keys()):
        graph = nnssa.functions[fn_key].graph
        # this pass needs the ssa to be in the topologically sorted order
        node_names = topsort(graph)

        # Mark all NHWC nodes
        nhwc_nodes = []
        for name in node_names:
            node = graph[name]
            if len(node.outputs) > 0 and len(node.inputs) > 0 and _is_NHWC(graph, node):
                node.attr['data_format'] = 'NHWC_format_inserted'
                nhwc_nodes.append(name)

        for name in nhwc_nodes:

            node = graph[name]

            # Adjust type inference
            if builtins.is_tensor(node.datatype):
                s = node.datatype.get_shape()
                if len(s) == 4:
                    new_shape = tuple([s[0], s[3], s[1], s[2]])
                    node.datatype = builtins.tensor(node.datatype.get_primitive(), new_shape)
                    node.attr['symbolic_datatype'] = node.datatype

            if '_output_shapes' in node.attr:
                orig_out_shapes = node.attr['_output_shapes']
                if len(orig_out_shapes) == 1 and len(orig_out_shapes[0]) == 4:
                    s = orig_out_shapes[0]
                    node.attr['_output_shapes'] = [[s[0], s[3], s[1], s[2]]]

            if node.op in ELEMENTWISE_OPS:
                for inp in node.inputs:
                    parent_node = graph[inp]
                    if parent_node.value is not None:
                        # if there is a constant vector input
                        val = np.array(parent_node.value.val)
                        if len(val.shape) == 1 and builtins.is_tensor(parent_node.datatype):
                            new_shape = (1, val.shape[0], 1, 1)
                            parent_node.datatype = builtins.tensor(parent_node.datatype.get_primitive(), new_shape)
                            parent_node.value.val = np.reshape(parent_node.value.val, new_shape)

            # Insert NHWC -> NCHW transpose
            for i, inp_node_name in enumerate(node.inputs):
                inp_node_format = graph[inp_node_name].attr.get('data_format')
                if graph[inp_node_name].op == 'Const':
                    # Const weights and parameters
                    continue
                if inp_node_format != 'NHWC_format_inserted':
                    _insert_transpose_to_nchw(graph, graph[inp_node_name], node)

            # Insert NCHW -> NHWC transpose
            for i, out_node_name in enumerate(node.outputs):
                out_node_format = graph[out_node_name].attr.get('data_format')
                if out_node_format != 'NHWC_format_inserted':
                    _insert_transpose_from_nchw(graph, node, graph[out_node_name])

            # Adjust output shape and concat layer's axis parameter
            if node.op == 'Concat' and len(node.inputs) > 1 and graph[node.inputs[0]].value is not None:
                axis = graph[node.inputs[0]].value.val
                axis = 4 + axis if axis < 0 else axis
                if axis == 3:
                    node.attr['axis'] = 1
                elif axis == 2 or axis == 1:
                    node.attr['axis'] = axis + 1
                else:
                    node.attr['axis'] = axis

            if node.op == 'ConcatV2' and len(node.inputs) > 1 and graph[node.inputs[-1]].value is not None:
                axis = graph[node.inputs[-1]].value.val
                axis = 4 + axis if axis < 0 else axis
                if axis == 3:
                    node.attr['axis'] = 1
                elif axis == 2 or axis == 1:
                    node.attr['axis'] = axis + 1
                else:
                    node.attr['axis'] = axis


def fuse_bias_add(nnssa):
    # look for 'BiasAdd' nodes following 'MatMul' or 'Conv2D'. If the other input in
    # 'BiasAdd' is coming from a const node, then copy the value of that const
    # in the parent and remove the 'BiasAdd', i.e. connect its children
    # to its parent.
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        keys = list(f.graph.keys())
        nodes_fused = []
        for k in keys:
            if k not in f.graph:
                continue
            current_node = f.graph[k]
            if current_node.op == 'BiasAdd' and len(current_node.inputs) == 2:
                parent_node = f.graph[current_node.inputs[0]]
                second_p_node = f.graph[current_node.inputs[1]]
                if (parent_node.op == 'MatMul' or parent_node.op == 'Conv2D' and len(parent_node.outputs) == 1) and \
                    (second_p_node.value is not None and len(second_p_node.outputs) == 1 and second_p_node.outputs[0] == k):

                    parent_node.attr['bias'] = second_p_node.value.val
                    disconnect_edge(f.graph, second_p_node.name, k)  # disconnect the const
                    disconnect_edge(f.graph, parent_node.name, k)  # disconnect the first parent
                    for out_node in current_node.outputs:
                        f.graph[parent_node.name].outputs.append(out_node)
                        if current_node.name in f.graph[out_node].inputs:
                            idx = f.graph[out_node].inputs.index(current_node.name)
                            f.graph[out_node].inputs[idx] = parent_node.name
                        else:
                            raise ValueError('[Op Fusion] fuse_bias_add() cannot identify biasAdd output.')
                    nodes_fused.append(k)
                    nodes_fused.append(second_p_node.name)

        for nf in nodes_fused:
            delete_node(f.graph, nf)
        if len(nodes_fused) > 0:
            print("[Op Fusion] fuse_bias_add() deleted {} nodes.".format(len(nodes_fused)))


def onehot_matmul_to_embedding(nnssa):
    # Look for 'MatMul' whose first input is 'OneHot'
    # and replace it with embedding op
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        keys = list(f.graph.keys())

        for k in keys:
            if k not in f.graph:
                continue
            current_node = f.graph[k]
            if len(current_node.inputs) < 1:
                continue
            inp_node = f.graph[current_node.inputs[0]]
            if (current_node.op == 'BatchMatMul' or current_node.op == 'MatMul') and inp_node.op == 'OneHot':
                assert len(inp_node.inputs) == 4, 'OneHot node should have 4 inputs'
                onehot_params = [f.graph[name].attr.get('value') for name in inp_node.inputs[1:]]
                depth_val, on_val, off_val = [x.val[0] for x in onehot_params]
                # Change the current node operation to Embedding
                current_node.op = 'Embedding'
                current_node.attr['depth'] = depth_val
                current_node.attr['on_value'] = on_val
                current_node.attr['off_value'] = off_val
                # Replace OneHot with its first input
                onehot_inp_node_names = inp_node.inputs[:]
                replace_node(f.graph, inp_node.name, onehot_inp_node_names[0])

                # Now delete the OneHot node and other input nodes
                delete_node(f.graph, onehot_inp_node_names[1])
                print('[Op Fusion] Node %s is removed.' % (onehot_inp_node_names[1]))
                delete_node(f.graph, onehot_inp_node_names[2])
                print('[Op Fusion] Node %s is removed.' % (onehot_inp_node_names[2]))
                delete_node(f.graph, onehot_inp_node_names[3])
                print('[Op Fusion] Node %s is removed.' % (onehot_inp_node_names[3]))
                delete_node(f.graph, inp_node.name)
                print('[Op Fusion] Node %s is removed.' % inp_node.name)


def _search_nodes_by_type(gf, node_names, op_type):
    for name in node_names:
        if gf[name].op == op_type:
            return gf[name]


def _match_layernorm_pattern(gf, entry_node):
    """ Return the nodes that form the subgraph of a LayerNormalization layer
    """
    def _axes_in_range(axes, rank):
        return all([x in range(-rank, rank) for x in axes])

    try:
        params = {}
        mean_1 = _search_nodes_by_type(gf, entry_node.outputs, 'Mean')
        sqdiff_2 = _search_nodes_by_type(gf, entry_node.outputs, 'SquaredDifference')
        mul_3 = _search_nodes_by_type(gf, entry_node.outputs, 'Mul')

        if not (mean_1.op == 'Mean' and sqdiff_2.op == 'SquaredDifference' and
            mul_3.op == 'Mul'):
            return None
        const_4 = gf[mean_1.inputs[1]]
        mean_1_rank = len(mean_1.datatype.get_shape())
        if not (const_4.op == 'Const' and len(const_4.value.val) == 1 and
            _axes_in_range(const_4.value.val, mean_1_rank)):
            return None
        axes = const_4.value.val
        mean_5 = gf[sqdiff_2.outputs[0]]
        if not (mean_5.op == 'Mean'):
            return None
        const_6 = gf[mean_5.inputs[1]]
        mean_5_rank = len(mean_5.datatype.get_shape())
        if not (const_6.op == 'Const' and len(const_6.value.val) == 1 and
            axes == const_6.value.val):
            return None

        axes = sorted([x if x > 0 else mean_1_rank - x for x in
            const_4.value.val])
        ref_axes = list(range(mean_1_rank-len(axes), mean_1_rank))
        if not all([x == y for (x,y) in zip(axes, ref_axes)]):
            return None
        params['axes'] = axes

        add_7 = gf[mean_5.outputs[0]]
        const_8 = gf[add_7.inputs[1]] # epsilon
        params['epsilon'] = const_8.value.val
        rsqrt_9 = gf[add_7.outputs[0]]
        mul_10 = gf[rsqrt_9.outputs[0]]
        if not (add_7.op == 'Add' and const_8.op == 'Const' and
            rsqrt_9.op == 'Rsqrt' and mul_10.op == 'Mul'):
            return None
        const_11 = gf[mul_10.inputs[1]]
        params['gamma'] = const_11.value.val
        if not (mul_3.name in mul_10.outputs and len(mul_10.outputs) == 2):
            return None
        mul_12 = gf[mul_10.outputs[1]] if gf[mul_10.outputs[0]] == mul_3 else \
            gf[mul_10.outputs[0]]

        sub_13 = gf[mul_12.outputs[0]]
        if not (mul_12.op == 'Mul' and sub_13.op == 'Sub'):
            return None
        const_14 = gf[sub_13.inputs[0]]
        if not const_14.op == 'Const':
            return None
        params['beta'] = const_14.value.val
        add_15 = gf[sub_13.outputs[0]]
        if not (gf[add_15.inputs[0]] == mul_3 and add_15.op == 'Add'):
            return None

        layernorm_nodes = [mean_1, sqdiff_2, mul_3, const_4, mean_5, const_6,
            add_7, const_8, rsqrt_9, mul_10, const_11, mul_12, sub_13, const_14,
            add_15]

        return (layernorm_nodes, params)
    except Exception as e:
        return None


def _fuse_layer_norm(graph):
    keys = list(graph.keys())
    count = 0
    for k in keys:
        if k not in graph:
            continue
        current_node = graph[k]
        layernorm_nodes_params = _match_layernorm_pattern(graph, current_node)
        if layernorm_nodes_params is not None:
            ln_nodes, ln_params = layernorm_nodes_params
            out_node = ln_nodes[-1]
            ln_outputs = out_node.outputs[:]

            # Instantiate a new fused node in the graph
            fused_ln_node = ParsedNode()
            fused_ln_node.op = 'LayerNormalization'
            fused_ln_node.name = out_node.name + '_layernorm'
            fused_ln_node.attr = ln_params
            fused_ln_node.datatype = current_node.datatype

            graph[fused_ln_node.name] = fused_ln_node

            # Connect fused node to entry and output nodes
            connect_edge(graph, current_node.name, fused_ln_node.name)
            replace_node(graph, out_node.name, fused_ln_node.name)
            # connect_dests(graph, fused_ln_node.name, ln_outputs)

            # Delete nodes
            ln_node_names = [x.name for x in ln_nodes]
            for name in ln_node_names:
                delete_node(graph, name)

            count += 1

    if count > 0:
        print('[Op Fusion] Fused {} layer normalizations.'.format(count))


def fuse_layer_norm(nnssa):
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        _fuse_layer_norm(f.graph)


def _match_gelu_pattern(gf, entry_node):
    """ Return the nodes that form the subgraph of a GELU layer
    """
    try:
        if not len(entry_node.outputs) == 3:
            return None
        pow_1 = _search_nodes_by_type(gf, entry_node.outputs, 'Pow')
        add_2 = _search_nodes_by_type(gf, entry_node.outputs, 'Add')
        mul_3 = _search_nodes_by_type(gf, entry_node.outputs, 'Mul')

        if not (pow_1.op == 'Pow' and add_2.op == 'Add' and mul_3.op == 'Mul'):
            return None
        const_4 = gf[pow_1.inputs[1]]
        if not (const_4.op == 'Const' and int(round(const_4.value.val)) == 3):
            return None
        mul_5 = gf[pow_1.outputs[0]]
        const_6 = gf[mul_5.inputs[0]]
        if not (const_6.op == 'Const' and \
            abs(const_6.value.val - 0.0447) < 1e-3):
            return None
        if not (gf[add_2.inputs[0]] == entry_node and \
            gf[add_2.inputs[1]] == mul_5):
            return None
        mul_7 = gf[add_2.outputs[0]]
        const_8 = gf[mul_7.inputs[0]]
        if not abs(const_8.value.val - np.sqrt(2 / np.pi)) < 1e-3:
            return None
        tanh_9 = gf[mul_7.outputs[0]]
        add_10 = gf[tanh_9.outputs[0]]
        const_11 = gf[add_10.inputs[0]]
        if not (tanh_9.op == 'Tanh' and add_10.op == 'Add' and \
            const_11.op == 'Const' and int(round(const_11.value.val)) == 1):
            return None
        mul_12 = gf[add_10.outputs[0]]
        const_13 = gf[mul_12.inputs[0]]
        if not (mul_12.op == 'Mul' and const_13.op == 'Const' and \
            abs(const_13.value.val - 0.5) < 1e-3):
            return None
        if not (gf[mul_3.inputs[0]] == entry_node and \
            gf[mul_3.inputs[1]] == mul_12):
            return None

        gelu_nodes = [pow_1, add_2, mul_3, const_4, mul_5, const_6, mul_7,
            const_8, tanh_9, add_10, const_11, mul_12, const_13]

        return gelu_nodes

    except:
        return None


def _fuse_gelu(graph):
    keys = list(graph.keys())
    count = 0
    for k in keys:
        if k not in graph:
            continue
        current_node = graph[k]
        gelu_nodes = _match_gelu_pattern(graph, current_node)
        if gelu_nodes is not None:
            out_node = gelu_nodes[2]
            gelu_outputs = out_node.outputs[:]

            # Instantiate a new fused node in the graph
            fused_gelu_node = ParsedNode()
            fused_gelu_node.op = 'GeLU'
            fused_gelu_node.name = out_node.name + '_gelu'
            fused_gelu_node.attr = {}
            fused_gelu_node.datatype = current_node.datatype

            graph[fused_gelu_node.name] = fused_gelu_node

            # Delete nodes
            gelu_node_names = [x.name for x in gelu_nodes]
            for name in gelu_node_names:
                delete_node(graph, name)

            # Connect fused node to entry and output nodes
            connect_edge(graph, current_node.name, fused_gelu_node.name)
            connect_dests(graph, fused_gelu_node.name, gelu_outputs)

            count += 1

    if count > 0:
        print('[Op Fusion] Fused {} GeLUs.'.format(count))


def fuse_gelu(nnssa):
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        _fuse_gelu(f.graph)


def fuse_conv_mul_add_into_batchnorm(nnssa):
    """
    A graph pass that match and fuses following op patterns into one BatchNorm op.

    Pattern 1:
                [Const]   [Const]
                   |         |
                   V         V
    [Conv2D] --> [Mul] --> [Add] --> [...] to [Conv2D] --> [BatchNorm] --> [...]

    Pattern 2:
                [Const]   [Const]   [Const]
                   |         |         |
                   V         V         V
    [Conv2D] --> [Sub] --> [Mul] --> [Add] --> [...] to [Conv2D] --> [BatchNorm] --> [...]

    Pattern 3:
                [Const]   [Const]       [Const]     [Const]
                   |         |            |            |
                   V         V            V            V
    [Conv2D] --> [Sub] --> [RealDiv] --> [Mul] --> [BiasAdd] --> [...] to [Conv2D] --> [BatchNorm] --> [...]
    """

    def _match_batch_norm_pattern(graph, entry_node, pattern_ops):
        if not _check_number_outputs(entry_node, 1):
            return None
        nodes_to_merge = list()
        node = graph[entry_node.outputs[0]]
        for i, op in enumerate(pattern_ops):
            if node.op != op:
                return None
            if node.op != pattern_ops[-1] and not _check_number_outputs(node, 1):
                return None
            if not _check_number_inputs(node, 2):
                return None
            const_node = graph[node.inputs[1]]
            if not _check_single_out_vector_constant_node(const_node):
                return None
            nodes_to_merge.extend([const_node, node])
            node = graph[node.outputs[0]]
        if len(nodes_to_merge) != len(pattern_ops) * 2:
            return None
        return nodes_to_merge

    def _merge_into_batchnorm(graph, nodes, pattern_id=1):
        expected_num_nodes = 4
        if pattern_id == 2:
            expected_num_nodes = 6
        elif pattern_id == 3:
            expected_num_nodes = 8
        assert len(nodes) == expected_num_nodes

        current_node = graph[nodes[1].inputs[0]]
        out_node = nodes[-1]
        bn_outputs = out_node.outputs[:]

        fused_bn_node = ParsedNode()
        fused_bn_node.op = 'BatchNorm'
        fused_bn_node.name = out_node.name + '_batch_norm'

        fused_bn_node.attr = {
            'gamma': np.squeeze(nodes[0].value.val),
            'beta': np.squeeze(nodes[2].value.val),
        }
        if pattern_id == 2:
            fused_bn_node.attr = {
                'mean': np.squeeze(nodes[0].value.val),
                'gamma': np.squeeze(nodes[2].value.val),
                'beta': np.squeeze(nodes[4].value.val),
            }
        elif pattern_id == 3:
            fused_bn_node.attr = {
                'mean': np.squeeze(nodes[0].value.val),
                'gamma': np.squeeze(nodes[4].value.val) / np.squeeze(nodes[2].value.val),
                'beta': np.squeeze(nodes[6].value.val),
            }

        fused_bn_node.datatype = current_node.datatype
        graph[fused_bn_node.name] = fused_bn_node

        # Delete nodes
        bn_node_names = [x.name for x in nodes]
        for name in bn_node_names:
            delete_node(graph, name)

        # Connect fused node to entry and output nodes
        connect_edge(graph, current_node.name, fused_bn_node.name)
        connect_dests(graph, fused_bn_node.name, bn_outputs)

    def _fuse_conv_mul_add_into_batchnorm(graph):
        keys = list(graph.keys())
        count = 0
        for k in keys:
            if k not in graph:
                continue
            current_node = graph[k]
            if current_node.op not in ['Conv2D', 'DepthwiseConv2dNative']:
                continue

            # return nodes order: : [Const, Mul, Const, Add]
            nodes1 = _match_batch_norm_pattern(graph, current_node, ['Mul', 'Add'])
            # return nodes order: : [Const, Sub, Const, Mul, Const, Add]
            nodes2 = _match_batch_norm_pattern(graph, current_node, ['Sub', 'Mul', 'Add'])
            # return nodes order: [Const, Sub, Const, RealDiv, Const, Mul, Const, BiasAdd]
            nodes3 = _match_batch_norm_pattern(graph, current_node, ['Sub', 'RealDiv', 'Mul', 'BiasAdd'])

            if nodes1:
                _merge_into_batchnorm(graph, nodes=nodes1, pattern_id=1)
                count += len(nodes1)

            if nodes2:
                _merge_into_batchnorm(graph, nodes=nodes2, pattern_id=2)
                count += len(nodes2)

            if nodes3:
                _merge_into_batchnorm(graph, nodes=nodes3, pattern_id=3)
                count += len(nodes3)

        if count > 0:
            print('[Op Fusion] Fused {} nodes into BatchNorms.'.format(count))

    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        _fuse_conv_mul_add_into_batchnorm(f.graph)


def fuse_pad_into_conv(nnssa):
    """
    A graph pass that match and fuses following op patterns into one Conv2D op.

    Pattern 1:
    [Const]
      |
      V
    [Pad] --> [Conv2D] --> [...] to [Conv2D] --> [...]
    """

    def _match_pad_conv2d_pattern(graph, entry_node):
        if not _check_number_outputs(entry_node, 1):
            return None
        conv2d_node = graph[entry_node.outputs[0]]
        if not (conv2d_node.op == 'Conv2D' and _check_number_outputs(conv2d_node, 1) and _check_number_inputs(conv2d_node, 1)):
            return None
        if conv2d_node.attr.get('padding', '').lower() != 'valid':
            return None
        return [entry_node, conv2d_node]

    def _fuse_pad_into_conv(graph):
        keys = list(graph.keys())
        count = 0
        for k in keys:
            if k not in graph:
                continue
            current_node = graph[k]
            if current_node.op != 'Pad':
                continue

            nodes = _match_pad_conv2d_pattern(graph, current_node)  # [Pad, Conv2D]

            if nodes:
                pad_node, conv2d_node = nodes
                previous_node = pad_node.inputs[0]
                paddings = graph[pad_node.inputs[1]].value.val
                pad_h, pad_w = paddings[-2], paddings[-1]

                # fused node in the graph
                conv2d_node.attr.update({
                    'pad_h': pad_h, 'pad_w': pad_w
                })
                graph[conv2d_node.name] = conv2d_node

                # delete pad const node and pad node
                delete_node(graph, pad_node.inputs[1])
                delete_node(graph, pad_node.name)
                connect_edge(graph, previous_node, conv2d_node.name)

                count += 1

        if count > 0:
            print('[Op Fusion] Fused {} Pad nodes into Conv2D.'.format(count))

    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        _fuse_pad_into_conv(f.graph)


def spatial_reduce_to_global_pool(nnssa):
    """
    A graph pass to translate a spatial reduce op to global pool op for better GPU performance.
    """
    reduce_ops = {'mean', 'max'}

    def _spatial_reduce_to_global_pool(graph):
        keys = list(graph.keys())
        count = 0
        for k in keys:
            if k not in graph:
                continue
            current_node = graph[k]
            if current_node.op.lower() not in reduce_ops:
                continue
            reduction_indices = current_node.attr.get('reduction_indices')
            # reduction on height and weight dimensions
            hw_dims = {(2, 3), (3, 2), (-2, -1), (-1, -2), (2, -1), (-1, 2), (-2, 3), (3, -2)}
            if tuple(reduction_indices) in hw_dims:
                # replace reduce op to global pooling op
                previous_node = current_node.inputs[0]
                output_nodes = current_node.outputs[:]

                pooling_node = ParsedNode()
                pooling_node.op = 'AvgPool' if current_node.op.lower() == 'mean' else 'MaxPool'
                pooling_node.name = current_node.name + '_pooling'
                pooling_node.attr = {
                    'padding': 'valid'.upper(),
                    'global_pooling': True,
                }
                pooling_node.datatype = current_node.datatype
                graph[pooling_node.name] = pooling_node

                delete_node(graph, current_node.name)
                connect_edge(graph, previous_node, pooling_node.name)
                connect_dests(graph, pooling_node.name, output_nodes)

                count += 1

        if count > 0:
            print('[Op Fusion] Tuned {} Reductions.'.format(count))

    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        _spatial_reduce_to_global_pool(f.graph)

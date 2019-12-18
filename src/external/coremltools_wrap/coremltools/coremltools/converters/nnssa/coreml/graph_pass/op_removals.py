# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _


import copy
from ...commons.basic_graph_ops import delete_node, disconnect_edge, replace_node, replace_control_dest


def remove_no_ops_and_shift_control_dependencies(nnssa):
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        for name, node in f.graph.copy().items():
            if node.op == "NoOp":
                for each_control_output in node.control_outputs:
                    f.graph[each_control_output].control_inputs.remove(node.name)

                for each_control_input in node.control_inputs:
                    f.graph[each_control_input].control_outputs.remove(node.name)

                for each_control_output in node.control_outputs:
                    for each_control_input in node.control_inputs:
                        f.graph[each_control_output].control_inputs.append(each_control_input)
                        f.graph[each_control_input].control_outputs.append(each_control_output)

                del f.graph[name]


def constant_weight_link_removal(nnssa):
    # look for constant nodes and if they are feeding into
    # 'MatMul' or 'Conv2D', then copy the value to their attributes and delete the link.
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        keys = list(f.graph.keys())
        for k in keys:
            if k not in f.graph:
                continue
            is_const = (f.graph[k].value is not None) and (k not in f.outputs)
            if is_const:
                for o in f.graph[k].outputs:
                    nextnode = f.graph[o]
                    op_type = nextnode.op
                    if op_type == 'MatMul' or op_type == 'Conv2D':
                        if nextnode.inputs[1] == k:
                            nextnode.attr['W'] = f.graph[k].value.val
                            disconnect_edge(f.graph, k, o)


def remove_single_isolated_node(nnssa):
    # remove nodes that do not have any output and input
    delete_count = 0
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        keys = list(f.graph.keys())
        for k in keys:
            if k not in f.graph:
                continue
            if len(f.graph[k].outputs) == 0 and len(f.graph[k].inputs) == 0:
                delete_count += 1
                delete_node(f.graph, k)

    print('%d disconnected nodes deleted' % delete_count)


def _remove_internal_identity_nodes(nnssa):
    '''
    remove identity nodes that are not connected to the model outputs
    '''
    delete_count = 0
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        keys = list(f.graph.keys())
        for k in keys:
            if k not in f.graph:
                continue
            node = f.graph[k]
            if len(node.inputs) != 1 or len(node.outputs) != 1:
                continue
            inp_node = f.graph[node.inputs[0]]
            if node.op == 'Identity' and inp_node.op != 'get_tuple':
                delete_count += 1
                parent_name = f.graph[k].inputs[0]
                disconnect_edge(f.graph, parent_name, k)
                for control_input in f.graph[k].control_inputs:
                    replace_control_dest(f.graph, control_input, k, parent_name)
                replace_node(f.graph, k, parent_name)  # join parent to children
                delete_node(f.graph, k)

    return delete_count


def _remove_output_identity_nodes(nnssa):
    '''
    remove identity nodes that ARE connected to the model outputs
    '''
    delete_count = 0
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        keys = list(f.graph.keys())
        for k in keys:
            if k not in f.graph:
                continue
            if f.graph[k].op == 'Identity' and len(f.graph[k].inputs) == 1:
                if len(f.graph[k].outputs) == 0 and (k in f.outputs) and k == f.graph[k].name:
                    # this means node k is the "output-identity" node that nnssa inserts
                    # we remove it here
                    delete_count += 1
                    parent_name = f.graph[k].inputs[0]
                    f.graph[parent_name].control_outputs = f.graph[k].control_outputs
                    del f.graph[k]
                    f.graph[k] = copy.deepcopy(f.graph[parent_name])
                    del f.graph[parent_name]
                    f.graph[k].name = k
                    f.graph[k].outputs = []
                    for p in f.graph[k].inputs:
                        for idx, out in enumerate(f.graph[p].outputs):
                            if out == parent_name:
                                f.graph[p].outputs[idx] = k
                    for p in f.graph[k].control_inputs:
                        for idx, out in enumerate(f.graph[p].control_outputs):
                            if out == parent_name:
                                f.graph[p].control_outputs[idx] = k
    return delete_count


def remove_identity(nnssa):
    '''
    remove node of type 'identity', connect its parent to its child.
    Disable this pass, if ssa contains more than 1 functions. In that case
    a few 'identity' nodes are crucial to get data in/out of body of loops
    '''
    if len(nnssa.functions.keys()) > 1:
        return
    delete_count = _remove_internal_identity_nodes(nnssa)
    delete_count += _remove_output_identity_nodes(nnssa)
    print('%d identity nodes deleted' % delete_count)


def remove_oneway_split(nnssa):
    """ Remove split op with 1 output that splits the input into itself.
    """
    for fn_key in list(nnssa.functions.keys()):
        f = nnssa.functions[fn_key]
        keys = list(f.graph.keys())
        for k in keys:
            if k not in f.graph:
                continue
            node = f.graph[k]
            if not (node.op == 'Split' and node.attr['num_split'] == 1 and
                len(node.datatype.T) == 1 and len(node.inputs) == 2):
                continue

            if f.graph[node.inputs[0]].op == 'Const':
                axis_name, parent_name = node.inputs
            elif f.graph[node.inputs[1]].op == 'Const':
                parent_name, axis_name = node.inputs
            else:
                continue

            if len(node.outputs) == 1 and f.graph[node.outputs[0]].op == 'get_tuple':
                get_tuple_name = node.outputs[0]
            else:
                continue

            parent_node = f.graph[parent_name]
            get_tuple_node = f.graph[get_tuple_name]
            for out_name in get_tuple_node.outputs:
                out_node = f.graph[out_name]
                out_node.inputs = [parent_name if x == get_tuple_name else x \
                    for x in out_node.inputs]
                out_node.control_inputs = [parent_name if x == get_tuple_name \
                    else x for x in out_node.control_inputs]
            parent_node.outputs = get_tuple_node.outputs[:]
            parent_node.control_outputs = get_tuple_node.control_outputs[:]

            del f.graph[axis_name], f.graph[k], f.graph[get_tuple_name]

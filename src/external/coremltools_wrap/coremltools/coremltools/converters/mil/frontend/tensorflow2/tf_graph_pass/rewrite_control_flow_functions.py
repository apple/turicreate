# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import logging
from coremltools.converters.mil.frontend.tensorflow.parsed_tf_node import ParsedTFNode
from coremltools.converters.mil.frontend.tensorflow.basic_graph_ops import (
    disconnect_edge,
    connect_edge,
    delete_node,
    replace_node,
    replace_dest,
    connect_edge_at_index,
)


def _rename_node_in_fn(node, new_name, fn):
    """
    Rename a node and all it's connections.

    Parameters
    ----------
    node: ParsedTFNode
        Node to rename.
    new_name: str
        New name of the node.
    fn: SSAFunction
        Function that contains graph to operate on.
    """
    old_name = node.name
    node.name = new_name
    for i in node.inputs:
        idx = fn.graph[i].outputs.index(old_name)
        fn.graph[i].outputs[idx] = new_name
        if old_name in fn.graph[i].control_outputs:
            idx = fn.graph[i].control_outputs.index(old_name)
            fn.graph[i].control_outputs[idx] = new_name

    for o in node.outputs:
        idx = fn.graph[o].inputs.index(old_name)
        fn.graph[o].inputs[idx] = new_name
        if old_name in fn.graph[o].control_inputs:
            idx = fn.graph[o].control_inputs.index(old_name)
            fn.graph[o].control_inputs[idx] = new_name

    for i in node.control_inputs:
        if old_name in fn.graph[i].control_outputs:
            idx = fn.graph[i].control_outputs.index(old_name)
            fn.graph[i].control_outputs[idx] = new_name

    for o in node.control_outputs:
        if old_name in fn.graph[o].control_inputs:
            idx = fn.graph[o].control_inputs.index(old_name)
            fn.graph[o].control_inputs[idx] = new_name

    fn.graph[new_name] = fn.graph.pop(old_name)


def _flatten_sub_graph_namespaces(tf_ssa, fn_name):
    """
    A pass to flatten namespaces for sub-graphs of the control flow while_loop
    op. For example, the while_loop's has two sub-graphs, "cond" and "body",
    all the nodes in the graph will be prefixing the sub-graph's name. This
    pass is required for converting control flow v2 ops (enabled by default in
    TensorFlow 2.0+) as the original sub-graphs will contain duplicated names.

    Parameters
    ----------
    tf_ssa: NetworkEnsemble
        An object that contains multiple functions / sub-graphs.
    fn_name: str
        Name of the function / sub-graph to operate on.
    """
    count = 0
    fn = tf_ssa.functions.get(fn_name)
    for name, node in fn.graph.copy().items():
        if node.op not in {"StatelessWhile", "While", "StatelessIf", "If"}:
            continue

        if node.op in {"StatelessWhile", "While"}:
            sub_fn_names = [node.attr.get("cond"), node.attr.get("body")]
        else:
            sub_fn_names = [node.attr.get("then_branch"), node.attr.get("else_branch")]

        for sf_name in sub_fn_names:
            sf = tf_ssa.functions.get(sf_name)
            prefix = "{}/{}".format(node.name, sf_name)

            for old_name, n in sf.graph.copy().items():
                _rename_node_in_fn(n, "{}/{}".format(prefix, old_name), sf)
                count += 1

            ios = set(sf.inputs + sf.outputs)
            io_name_mappings = {n: "{}/{}".format(prefix, n) for n in ios}
            sf.inputs = [io_name_mappings[n] for n in sf.inputs]
            sf.outputs = [io_name_mappings[n] for n in sf.outputs]
            _flatten_sub_graph_namespaces(tf_ssa, sf_name)

            msg = "flatten_sub_graph_namespaces: {} nodes renamed in '{}'"
            logging.info(msg.format(count, sf_name))


def _insert_op(fn, op, name, attr=None):
    """
    Create a node with given attributes, then insert to the target graph in
    given function.

    Parameters
    ----------
    fn: SSAFunction
        Function that contains graph to operate on.
    op: str
        Type of the operation for the new node.
    name: str
        Name of the new node.
    attr: dict or None (optional)
        Attributes of the new node.

    Returns
    -------
    node: ParsedTFNode
        New node object.
    """
    node = ParsedTFNode()
    node.op = op
    node.name = name
    if attr is not None:
        node.attr = attr
    fn.graph[node.name] = node
    return node


def _insert_function_entry(fn):
    return _insert_op(fn=fn, op="function_entry", name="entry")


def _insert_return(fn):
    return _insert_op(fn=fn, op="return", name="return")


def _insert_make_tuple(fn, name=None):
    name = "make_tuple" if name is None else name
    return _insert_op(fn=fn, op="make_tuple", name=name)


def _insert_get_tuple(fn, name, idx):
    return _insert_op(fn=fn, op="get_tuple", name=name, attr={"index": idx})


def _rewrite_cond_functions(tf_ssa, fn):
    r"""
    Rewrite tf.cond's sub-graphs with get_tuple, make_tuple, function_entry and
    return ops. This rewrite is required in order to convert functional form
    control flow v2 nodes 'StatelessIf' and 'If'.

    Parameters
    ----------
    tf_ssa: NetworkEnsemble
        An object that contains multiple functions / sub-graphs.
    fn: SSAFunction
        Function that contains graph to operate on.

    Examples
    --------

    Input:

        Before pass "main" graph:

            [const/greater/y] ---------\
            [placeholder/args_0] -> [greater] -> [if] -> [identity]
                              \------------------/  \--> [identity]
            [placeholder/args_1] ----------------/

        Before pass "then" graph:

            [const/sub/y] ---------------\
            [placeholder/sub_args_0] -> [sub]
            [placeholder/sub_args_1] -> [identity]

        Before pass "else" graph:

            [const/add/y] ---------------\
            [placeholder/add_args_0] -> [add]

            [const/mul/y] ---------------\
            [placeholder/add_args_1] -> [mul]

    Output:

        After pass "main" graph:

            [const/greater/y] ---------\
            [placeholder/args_0] -> [greater] -> [make_tuple] -> [if] -> [get_tuple] -> [identity]
                              \---------------------/               \--> [get_tuple] -> [identity]
            [placeholder/args_1] -------------------/

        After pass "then" graph:

                                      [const/sub/y] ---------------\
            [entry] -> [get_tuple] -> [placeholder/sub_args_0] -> [sub] -> [make_tuple] -> [return]
                    -> [get_tuple] -> [placeholder/sub_args_1] -----------------/

        After pass "else" graph:

                                      [const/add/y] ---------------\
            [entry] -> [get_tuple] -> [placeholder/add_args_0] -> [add] -> [make_tuple] -> [return]
                    -> [get_tuple] -> [placeholder/add_args_1] -> [mul] --------/
                                      [const/mul/y] ---------------/

    """
    for cond_name, cond_node in fn.graph.copy().items():
        if cond_node.op not in {"StatelessIf", "If"}:
            continue

        then_fn_name = cond_node.attr.get("then_branch")
        else_fn_name = cond_node.attr.get("else_branch")

        msg = "Rewriting '{}' ({}) sub-graphs: then '{}', else '{}'"
        logging.info(
            msg.format(cond_node.name, cond_node.op, then_fn_name, else_fn_name)
        )

        then_fn = tf_ssa.functions.get(then_fn_name)
        else_fn = tf_ssa.functions.get(else_fn_name)

        # insert function entry nodes
        then_entry = _insert_function_entry(then_fn)
        else_entry = _insert_function_entry(else_fn)

        # pack node inputs to a single tuple
        cond_input = _insert_make_tuple(fn, "make_tuple/{}".format(cond_name))
        for ci in cond_node.inputs:
            disconnect_edge(fn.graph, ci, cond_node.name)
            connect_edge(fn.graph, ci, cond_input)
        connect_edge(fn.graph, cond_input, cond_node.name)

        # unpack node outputs to multiple get_tuples
        for i, co in enumerate(cond_node.outputs):
            # utilize FunctionDef's ret to make sure function outputs and
            # node outputs order matches when multiple outputs are there.
            # Fallback to use original cond_node.outputs order if fails.
            o_original = fn.graph[co].original_node
            if o_original:
                c_input = [n for n in o_original.input if str(n).startswith(cond_name)][
                    0
                ]
                c_index = c_input.split(":")[-1] if ":" in c_input else 0
                mapped_name = then_fn.ret["identity_{}".format(c_index)].split(":")[0]
                if mapped_name in then_fn.outputs:
                    idx = then_fn.outputs.index(mapped_name)
                else:  # in else_fn.outputs
                    idx = else_fn.outputs.index(mapped_name)
            else:
                idx = i

            cond_output = _insert_get_tuple(
                fn, "get_tuple/{}/{}".format(idx, cond_name), idx
            )
            edge_idx = fn.graph[co].inputs.index(cond_node.name)
            replace_dest(fn.graph, cond_node, co, cond_output)
            connect_edge_at_index(fn.graph, cond_output, co, edge_idx)

        # fetch inputs using get_tuple for then branch
        for i, ti in enumerate(then_fn.inputs):
            then_input = _insert_get_tuple(
                then_fn, "get_tuple/{}/{}".format(i, ti), i + 1
            )
            connect_edge(then_fn.graph, then_entry, then_input)
            replace_node(then_fn.graph, ti, then_input)
            delete_node(then_fn.graph, ti)

        # fetch inputs using get_tuple for else branch
        for i, ei in enumerate(else_fn.inputs):
            else_input = _insert_get_tuple(
                else_fn, "get_tuple/{}/{}".format(i, ei), i + 1
            )
            connect_edge(else_fn.graph, else_entry, else_input)
            replace_node(else_fn.graph, ei, else_input)
            delete_node(else_fn.graph, ei)

        # returns a tuple of value(s) as output for then branch
        then_output = _insert_make_tuple(then_fn)
        for to in then_fn.outputs:
            if to not in then_fn.graph.keys():
                # from identity, map back to get_tuple node
                to = "get_tuple/{}/{}".format(then_fn.inputs.index(to), to)
            connect_edge(then_fn.graph, to, then_output.name)

        then_return = _insert_return(then_fn)
        connect_edge(then_fn.graph, then_output.name, then_return.name)

        # returns a tuple of value(s) as output for else branch
        else_output = _insert_make_tuple(else_fn)
        for eo in else_fn.outputs:
            if eo not in else_fn.graph.keys():
                # from identity, map back to get_tuple node
                eo = "get_tuple/{}/{}".format(else_fn.inputs.index(eo), eo)
            connect_edge(else_fn.graph, eo, else_output.name)

        else_return = _insert_return(else_fn)
        connect_edge(else_fn.graph, else_output.name, else_return.name)


def _eliminate_loop_cond_nodes(tf_ssa, fn):
    """
    Eliminate loop condition nodes, such as loop_counters, max_iterations from
    the cond sub-graph and body sub-graph of tf.while_loop.

    Parameters
    ----------
    tf_ssa: NetworkEnsemble
        An object that contains multiple functions / sub-graphs.
    fn: SSAFunction
        Function that contains graph to operate on.

    Examples
    --------

    Input:

        Before pass "main" graph:

            [while/maximum_iterations] -----\
            [while/loop_counter] -------> [while] --> [identity]
            [placeholder/args_0] ----------/

        Before pass "cond" graph:

            [const/mean] -------\
            [placeholder] --> [mean] --> [greater]
            [const/greater/y] --------------/

            [while_maximum_iterations], [while_loop_counter] (not connected)

        Before pass "body" graph:

            [const/sub/y] ------\
            [placeholder] ---> [sub]

            [const/add/y] ------------\
            [while_loop_counter] --> [add]

            [while_maximum_iterations] (not connected)

    Output:

        After pass "main" graph:

            [placeholder/args_0] --> [while] --> [identity]

        After pass "cond" graph:

            [const/mean] -------\
            [placeholder] --> [mean] --> [greater]
            [const/greater/y] --------------/

        After pass "body" graph:

            [const/sub/y] ------\
            [placeholder] ---> [sub]
    """
    for name, node in fn.graph.copy().items():
        if node.op not in {"StatelessWhile", "While"}:
            continue

        cond_fn = tf_ssa.functions.get(node.attr.get("cond"))
        body_fn = tf_ssa.functions.get(node.attr.get("body"))

        cond_lc_nodes = {cond_fn.inputs.pop(0), cond_fn.inputs.pop(0)}
        logging.info("Removing {} from cond graph".format(cond_lc_nodes))
        for n in cond_lc_nodes:
            delete_node(cond_fn.graph, n)

        body_lc_nodes = {body_fn.inputs.pop(0), body_fn.inputs.pop(0)}
        q = list(body_lc_nodes)

        # delete entire sub-fn
        while len(q) > 0:
            n = body_fn.graph[q.pop(0)]
            for o in n.outputs:
                if o not in body_lc_nodes:
                    q.append(o)
                body_lc_nodes.add(o)
                for i in body_fn.graph[o].inputs:
                    if i not in body_lc_nodes:
                        q.append(i)
                    body_lc_nodes.add(i)

        # remove if in outputs
        for n in body_lc_nodes:
            if n in body_fn.outputs:
                msg = "Removing '{}' ({}) from body fn outputs"
                logging.info(msg.format(n, body_fn.graph[n].op))
                body_fn.outputs.remove(n)

        logging.info("Removing {} from body graph".format(body_lc_nodes))
        for n in body_lc_nodes:
            delete_node(body_fn.graph, n)


def _rewrite_while_loop_functions(tf_ssa, fn):
    """
    Rewrite tf.while_loop's sub-graphs with get_tuple, make_tuple,
    function_entry and return ops. This rewrite is required in order to convert
    functional form control flow v2 nodes 'StatelessWhile' and 'While'.

    Parameters
    ----------
    tf_ssa: NetworkEnsemble
        An object that contains multiple functions / sub-graphs.
    fn: SSAFunction
        Function that contains graph to operate on.

    Example
    -------

    Input:

        Before pass "main" graph:

            [placeholder/args_0] --> [while] --> [identity]

        Before pass "cond" graph:

            [const/mean] -------\
            [placeholder] --> [mean] --> [greater]
            [const/greater/y] --------------/

        Before pass "body" graph:

            [const/sub/y] ------\
            [placeholder] ---> [sub]

    Output:

        After pass "main" graph:

            [placeholder/args_0] --> [make_tuple] --> [while] --> [get_tuple] --> [identity]

        After pass "cond" graph:

                                      [const/mean] ------\
            [entry] -> [get_tuple] -> [placeholder] -> [mean] -> [greater] -> [make_tuple] -> [return]
                                      [const/greater/y] ------------/

        After pass "body" graph:

                                      [const/sub/y] ----\
            [entry] -> [get_tuple] -> [placeholder] -> [sub] -> [make_tuple] -> [return]
    """
    for while_name, while_node in fn.graph.copy().items():
        if while_node.op not in {"StatelessWhile", "While"}:
            continue

        cond_fn_name = while_node.attr.get("cond")
        body_fn_name = while_node.attr.get("body")

        msg = "Rewriting '{}' ({}) sub-graphs: cond '{}', body '{}'"
        logging.info(
            msg.format(while_node.name, while_node.op, cond_fn_name, body_fn_name)
        )

        cond_fn = tf_ssa.functions.get(cond_fn_name)
        body_fn = tf_ssa.functions.get(body_fn_name)

        # insert function entry nodes
        cond_entry = _insert_function_entry(cond_fn)
        body_entry = _insert_function_entry(body_fn)

        # pack node inputs to a single tuple
        while_input_tuple = _insert_make_tuple(fn, "make_tuple/{}".format(while_name))
        for wi in while_node.inputs:
            disconnect_edge(fn.graph, wi, while_node.name)
            connect_edge(fn.graph, wi, while_input_tuple)
        connect_edge(fn.graph, while_input_tuple, while_node.name)

        # unpack node outputs to multiple get_tuples
        for i, wo in enumerate(while_node.outputs):
            # utilize FunctionDef's ret to make sure function outputs and
            # node outputs order matches when multiple outputs are there.
            o_original = fn.graph[wo].original_node
            while_input = [
                n for n in o_original.input if str(n).startswith(while_name)
            ][0]
            while_index = while_input.split(":")[-1]
            mapped_name = body_fn.ret["identity_{}".format(while_index)].split(":")[0]
            idx = body_fn.outputs.index(mapped_name)

            loop_output = _insert_get_tuple(
                fn, "get_tuple/{}/{}".format(idx, while_input), idx
            )

            edge_idx = fn.graph[wo].inputs.index(while_node.name)
            replace_dest(fn.graph, while_node, wo, loop_output)
            connect_edge_at_index(fn.graph, loop_output, wo, edge_idx)

        # fetch inputs using get_tuple for cond fn
        for i, ci in enumerate(cond_fn.inputs):
            cond_input = _insert_get_tuple(cond_fn, "get_tuple/{}/{}".format(i, ci), i)
            connect_edge(cond_fn.graph, cond_entry, cond_input)
            replace_node(cond_fn.graph, ci, cond_input)
            delete_node(cond_fn.graph, ci)

        # fetch inputs using get_tuple for body fn
        for i, bi in enumerate(body_fn.inputs):
            new_name = "get_tuple/{}/{}".format(i, bi)

            if bi in body_fn.outputs:  # input is also an output
                body_fn.outputs[body_fn.outputs.index(bi)] = new_name

            body_input = _insert_get_tuple(body_fn, new_name, i)

            connect_edge(body_fn.graph, body_entry, body_input)
            replace_node(body_fn.graph, bi, body_input)
            delete_node(body_fn.graph, bi)

        # returns a tuple of value(s) as output for cond fn
        cond_output = _insert_make_tuple(cond_fn)
        for co in cond_fn.outputs:
            connect_edge(cond_fn.graph, co, cond_output.name)

        cond_return = _insert_return(cond_fn)
        connect_edge(cond_fn.graph, cond_output.name, cond_return.name)

        # returns a tuple of value(s) as output for body branch
        body_output = _insert_make_tuple(body_fn)

        for bo in body_fn.outputs:
            connect_edge(body_fn.graph, bo, body_output.name)

        body_return = _insert_return(body_fn)
        connect_edge(body_fn.graph, body_output.name, body_return.name)


def rewrite_control_flow_functions(tf_ssa):
    for fn_name, fn in tf_ssa.functions.items():
        _rewrite_cond_functions(tf_ssa, fn)
    for fn_name, fn in tf_ssa.functions.items():
        _eliminate_loop_cond_nodes(tf_ssa, fn)
        _rewrite_while_loop_functions(tf_ssa, fn)


def flatten_sub_graph_namespaces(tf_ssa):
    _flatten_sub_graph_namespaces(tf_ssa, fn_name="main")

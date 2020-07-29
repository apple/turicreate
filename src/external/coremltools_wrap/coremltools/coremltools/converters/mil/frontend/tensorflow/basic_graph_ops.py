# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import six


def connect_edge(g, source, dest):
    if isinstance(source, six.string_types):
        source = g[source]
    if isinstance(dest, six.string_types):
        dest = g[dest]
    source.outputs.append(dest.name)
    dest.inputs.append(source.name)


def connect_edge_at_index(g, source, dest, idx):
    if isinstance(source, six.string_types):
        source = g[source]
    if isinstance(dest, six.string_types):
        dest = g[dest]
    source.outputs.insert(idx, dest.name)
    dest.inputs.insert(idx, source.name)


def replace_source(g, source, dest, new_source):
    if isinstance(source, six.string_types):
        source = g[source]
    if isinstance(dest, six.string_types):
        dest = g[dest]
    if isinstance(new_source, six.string_types):
        new_source = g[new_source]
    dest_inputs = []
    for inp in dest.inputs:
        if inp == source.name:
            dest_inputs.append(new_source.name)
            g[new_source.name].outputs.append(dest.name)
        else:
            dest_inputs.append(inp)
    dest.inputs = dest_inputs
    source.outputs = [i for i in g[source.name].outputs if i != dest.name]


def replace_control_source(g, source, dest, new_source):
    if isinstance(source, six.string_types):
        source = g[source]
    if isinstance(dest, six.string_types):
        dest = g[dest]
    if isinstance(new_source, six.string_types):
        new_source = g[new_source]
    dest_inputs = []
    for inp in dest.control_inputs:
        if inp == source.name:
            dest_inputs.append(new_source.name)
            g[new_source.name].control_outputs.append(dest.name)
        else:
            dest_inputs.append(inp)
    dest.control_inputs = dest_inputs
    source.control_outputs = [i for i in g[source.name].outputs if i != dest.name]


def replace_dest(g, source, dest, new_dest):
    if isinstance(source, six.string_types):
        source = g[source]
    if isinstance(dest, six.string_types):
        dest = g[dest]
    if isinstance(new_dest, six.string_types):
        new_dest = g[new_dest]
    for idx, d in enumerate(source.outputs):
        if d == dest.name:
            source.outputs[idx] = new_dest.name
            new_dest.inputs = new_dest.inputs[:] + [source.name]

    dest.inputs = [i for i in dest.inputs if i != source.name]


def replace_control_dest(g, source, dest, new_dest):
    if isinstance(source, six.string_types):
        source = g[source]
    if isinstance(dest, six.string_types):
        dest = g[dest]
    if isinstance(new_dest, six.string_types):
        new_dest = g[new_dest]
    for idx, d in enumerate(source.control_outputs):
        if d == dest.name:
            source.control_outputs[idx] = new_dest.name
            new_dest.control_inputs = new_dest.control_inputs[:] + [source.name]

    dest.control_inputs = [i for i in dest.control_inputs if i != source.name]


def connect_dests(g, source, dests):
    for i in dests:
        connect_edge(g, source, i)


def connect_sources(g, sources, dest):
    for i in sources:
        connect_edge(g, i, dest)


def disconnect_edge(g, source, dest):
    if isinstance(source, six.string_types):
        source = g[source]
    if isinstance(dest, six.string_types):
        dest = g[dest]
    source.outputs = [i for i in source.outputs if i != dest.name]

    dest.inputs = [i for i in dest.inputs if i != source.name]


def disconnect_control_edge(g, source, dest):
    if isinstance(source, six.string_types):
        source = g[source]
    if isinstance(dest, six.string_types):
        dest = g[dest]
    source.control_outputs = [i for i in source.control_outputs if i != dest.name]

    dest.control_inputs = [i for i in dest.control_inputs if i != source.name]


def disconnect_vertex_outs(g, source):
    if isinstance(source, six.string_types):
        source = g[source]
    for out in source.outputs:
        g[out].inputs = [i for i in g[out].inputs if i != source.name]
    source.outputs = []


def disconnect_vertex_ins(g, dest):
    if isinstance(dest, six.string_types):
        dest = g[dest]
    for inp in dest.inputs:
        if isinstance(inp, six.string_types):
            innode = g[inp]
        else:
            innode = inp
        innode.outputs = [i for i in innode.outputs if i != dest.name]
    dest.inputs = []


def disconnect_vertex_control_ins(g, dest):
    if isinstance(dest, six.string_types):
        dest = g[dest]
    for inp in dest.control_inputs:
        if isinstance(inp, six.string_types):
            innode = g[inp]
        else:
            innode = inp
        innode.control_outputs = [i for i in innode.control_outputs if i != dest.name]
    dest.control_inputs = []


def disconnect_vertex_control_outs(g, source):
    if isinstance(source, six.string_types):
        source = g[source]
    for out in source.control_outputs:
        g[out].control_inputs = [i for i in g[out].control_inputs if i != source.name]
    source.control_outputs = []


def delete_node(g, node):
    if not isinstance(node, six.string_types):
        node = node.name
    disconnect_vertex_ins(g, node)
    disconnect_vertex_outs(g, node)
    disconnect_vertex_control_ins(g, node)
    disconnect_vertex_control_outs(g, node)
    del g[node]


def replace_node(g, original_node, new_node):
    if isinstance(new_node, six.string_types):
        new_node = g[new_node]
    if not isinstance(original_node, six.string_types):
        original_node = original_node.name

    for o in list(g[original_node].control_outputs):
        replace_control_source(g, original_node, o, new_node)
    for o in list(g[original_node].outputs):
        replace_source(g, original_node, o, new_node)
    for i in list(g[original_node].control_inputs):
        replace_control_dest(g, i, original_node, new_node)
    for i in list(g[original_node].inputs):
        replace_dest(g, i, original_node, new_node)


def fill_outputs(gd):
    """
    Fills the output lists of of a graph of ParsedNode

    Takes a graph in "dict{str, ParsedNode}" form, and returns a new graph.
    """
    # fill outputs
    for k, v in gd.items():
        for i in v.inputs:
            gd[i].outputs.append(v.name)
        for i in v.control_inputs:
            gd[i].control_outputs.append(v.name)
    get_tuple_ops = ["Split", "SplitV", "LSTMBlock"]
    for k, v in gd.items():
        if v.op in get_tuple_ops:
            outputs = [[out, int(gd[out].attr["index"])] for out in v.outputs]
            outputs.sort(key=lambda x: x[1])
            gd[k].outputs = [out for [out, _] in outputs]

    return gd


def check_connections(gd):
    """
    Given a graph, checks that all
     - inputs/outputs are symmetric
     - control_inputs/control_outputs are symmetric
     - The graph does not reference vertices outside of the graph

    Takes a graph in "dict{str, ParsedNode}" form. Does not return,
    asserts false on failure.
    """
    # check that inputs and outputs line up
    for k, v in gd.items():
        for i in v.inputs:
            if isinstance(i, six.string_types):
                assert k in gd[i].outputs
            else:
                assert k in gd[i.name].outputs
        for i in v.outputs:
            inputs = [
                inp if isinstance(inp, six.string_types) else inp.name
                for inp in gd[i].inputs
            ]
            assert k in inputs
        for i in v.control_inputs:
            if isinstance(i, six.string_types):
                assert k in gd[i].control_outputs
            else:
                assert k in gd[i.name].control_outputs
        for i in v.control_outputs:
            control_inputs = [
                inp if isinstance(inp, six.string_types) else inp.name
                for inp in gd[i].control_inputs
            ]
            assert k in control_inputs


def const_determined_nodes(gd, assume_variable_nodes=None):
    """
    Given a graph, extract all nodes that only depends on const nodes.

    # TODO: extract nodes that depends on the "const part" of placeholders.
    """
    if assume_variable_nodes is None:
        assume_variable_nodes = []
    vis = {}

    def visit(node):
        # make sure node is a ParsedNode
        if isinstance(node, six.string_types):
            node = gd[node]
        if node.name in vis:
            return

        if "Const" in node.op:
            vis[node.name] = True
        elif "Variable" in node.op:
            vis[node.name] = False
        elif "Placeholder" in node.op:
            vis[node.name] = False
        # TF1 uses TensorArray* while TF2 uses TensorList* ops
        elif "TensorArray" in node.op or "TensorList" in node.op:
            vis[node.name] = False
        elif "function" in node.op:
            vis[node.name] = False
        elif "global" in node.op:
            vis[node.name] = False
        elif node.name in assume_variable_nodes:
            vis[node.name] = False
        else:
            ret = True
            vis[node.name] = False
            for innode in node.inputs:
                if isinstance(innode, six.string_types):
                    inname = innode
                else:
                    inname = innode.name
                if inname not in vis:
                    visit(innode)
                if not vis[inname]:
                    ret = False
                    break
            vis[node.name] = ret

    for k, v in gd.items():
        if k in vis:
            continue
        visit(k)

    ret = []
    for k, v in vis.items():
        if v:
            ret.append(k)
    return ret


def topsort(graph):
    if len(graph) == 0:
        return []
    inedge_count = {k: len(v.inputs) + len(v.control_inputs) for k, v in graph.items()}
    ret = []
    curboundary = [k for k, v in inedge_count.items() if v == 0]
    nextboundary = []
    if len(curboundary) == 0:
        raise ValueError("Graph is not a DAG!")

    while len(curboundary) > 0:
        ret.extend(curboundary)
        for b in curboundary:
            for o in graph[b].outputs + graph[b].control_outputs:
                inedge_count[o] -= 1
                if inedge_count[o] == 0:
                    nextboundary.append(o)
        curboundary = nextboundary
        nextboundary = []
    if len(ret) != len(graph):
        raise ValueError("Graph is not a DAG!")
    return ret


def simple_topsort(inputs):
    if len(inputs) == 0:
        return []
    outputs = {k: [] for k in inputs}
    for k in inputs:
        for o in inputs[k]:
            outputs[o].append(k)

    inedge_count = {k: len(v) for k, v in inputs.items()}
    ret = []
    curboundary = [k for k, v in inedge_count.items() if v == 0]
    nextboundary = []
    if len(curboundary) == 0:
        raise ValueError("Graph is not a DAG!")

    while len(curboundary) > 0:
        ret.extend(curboundary)
        for b in curboundary:
            for o in outputs[b]:
                inedge_count[o] -= 1
                if inedge_count[o] == 0:
                    nextboundary.append(o)
        curboundary = nextboundary
        nextboundary = []
    if len(ret) != len(inputs):
        raise ValueError("Graph is not a DAG!")
    return ret

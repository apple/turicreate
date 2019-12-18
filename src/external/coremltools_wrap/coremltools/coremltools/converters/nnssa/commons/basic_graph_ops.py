# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _


def connect_edge(g, source, dest):
    g[source].outputs.append(dest)
    g[dest].inputs.append(source)


def replace_source(g, source, dest, new_source):
    for idx, d in enumerate(g[dest].inputs):
        if d == source:
            g[dest].inputs[idx] = new_source
            g[new_source].outputs.append(dest)

    g[source].outputs = [i for i in g[source].outputs if i != dest]


def replace_control_source(g, source, dest, new_source):
    for idx, d in enumerate(g[dest].control_inputs):
        if d == source:
            g[dest].control_inputs[idx] = new_source
            g[new_source].control_outputs.append(dest)

    g[source].control_outputs = [i for i in g[source].control_outputs if i != dest]


def replace_dest(g, source, dest, new_dest):
    for idx, d in enumerate(g[source].outputs):
        if d == dest:
            g[source].outputs[idx] = new_dest
            g[new_dest].inputs.append(source)

    g[dest].inputs = [i for i in g[dest].inputs if i != source]


def replace_control_dest(g, source, dest, new_dest):
    for idx, d in enumerate(g[source].control_outputs):
        if d == dest:
            g[source].control_outputs[idx] = new_dest
            g[new_dest].control_inputs.append(source)

    g[dest].control_inputs = [i for i in g[dest].control_inputs if i != source]


def connect_dests(g, source, dests):
    for i in dests:
        connect_edge(g, source, i)


def connect_sources(g, sources, dest):
    for i in sources:
        connect_edge(g, i, dest)


def disconnect_edge(g, source, dest):
    g[source].outputs = [i for i in g[source].outputs if i != dest]
    g[dest].inputs = [i for i in g[dest].inputs if i != source]


def disconnect_control_edge(g, source, dest):
    g[source].control_outputs = [i for i in g[source].control_outputs if i != dest]
    g[dest].control_inputs = [i for i in g[dest].control_inputs if i != source]


def disconnect_vertex_outs(g, source):
    source_node = g[source]
    for out in source_node.outputs:
        g[out].inputs = [i for i in g[out].inputs if i != source_node.name]
    source_node.outputs = []


def disconnect_vertex_ins(g, dest):
    dest_node = g[dest]
    for innode in dest_node.inputs:
        g[innode].outputs = [i for i in g[innode].outputs if i != dest_node.name]
    dest_node.inputs = []


def disconnect_vertex_control_ins(g, dest):
    dest_node = g[dest]
    for innode in dest_node.control_inputs:
        g[innode].control_outputs = [i for i in g[innode].control_outputs if i != dest_node.name]
    dest_node.control_inputs = []


def disconnect_vertex_control_outs(g, source):
    source_node = g[source]
    for out in source_node.control_outputs:
        g[out].control_inputs = [i for i in g[out].control_inputs if i != source_node.name]
    source_node.control_outputs = []


def delete_node(g, name):
    disconnect_vertex_ins(g, name)
    disconnect_vertex_outs(g, name)
    disconnect_vertex_control_ins(g, name)
    disconnect_vertex_control_outs(g, name)
    del g[name]


def replace_node(g, original_node, new_node):
    for o in list(g[original_node].control_outputs):
        replace_control_source(g, original_node, o, new_node)
    for o in list(g[original_node].outputs):
        replace_source(g, original_node, o, new_node)


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
    get_tuple_ops = ['Split', 'SplitV', 'LSTMBlock']
    for k, v in gd.items():
        if v.op in get_tuple_ops:
            outputs = [[out, int(gd[out].attr['index'])] for out in v.outputs]
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
            assert (k in gd[i].outputs)
        for i in v.outputs:
            assert (k in gd[i].inputs)
        for i in v.control_inputs:
            assert (k in gd[i].control_outputs)
        for i in v.control_outputs:
            assert (k in gd[i].control_inputs)


def const_determined_nodes(gd, assume_variable_nodes=[]):
    """
    Given a graph, extract all nodes that only depends on const nodes.
    
    # TODO: extract nodes that depends on the "const part" of placeholders.
    """
    vis = {}

    def visit(node):
        # make sure node is a ParsedNode
        from ..nnssa import ParsedNode
        if not isinstance(node, ParsedNode):
            node = gd[node]
        if node.name in vis:
            return

        if 'Const' in node.op:
            vis[node.name] = True
        elif 'Variable' in node.op:
            vis[node.name] = False
        elif 'Placeholder' in node.op:
            vis[node.name] = False
        elif 'TensorArray' in node.op:
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
                if innode not in vis:
                    visit(innode)
                if not vis[innode]:
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

    while (len(curboundary) > 0):
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

    while (len(curboundary) > 0):
        ret.extend(curboundary)
        for b in curboundary:
            for o in outputs[b]:
                inedge_count[o] -= 1
                if inedge_count[o] == 0:
                    nextboundary.append(o)
        curboundary = nextboundary
        nextboundary = []
    if len(ret) != len(inputs):
        import pdb
        pdb.set_trace()
        raise ValueError("Graph is not a DAG!")
    return ret

# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ...commons.basic_graph_ops import replace_source, delete_node, disconnect_edge
from ...commons import builtins


def remove_unused_nodes(ssa):
    # Very inefficient. Could be improved, not necessary considering graph size now.
    removed = True
    while removed:
        removed = False
        removed = sub_removal(ssa)


def sub_removal(ssa):
    removed = False
    for f in ssa.functions.keys():
        if not f.startswith("body_function_"):
            continue
        body_name = f
        cond_name = f.replace('body', 'cond')
        body = ssa.functions[body_name]
        cond = ssa.functions[cond_name]

        for bk, bv in body.graph.items():
            if bv.op == 'function':
                body_start = bk
            if bv.op == 'return':
                body_end = bk
        for ck, cv in cond.graph.items():
            if cv.op == 'function':
                cond_start = ck
            if cv.op == 'return':
                cond_end = ck

        inputs = [False] * len(body.graph[body_name].datatype.T)
        for gt in body.graph[body_name].outputs:
            assert (body.graph[gt].op == 'get_tuple')
            inputs[body.graph[gt].attr['index']] = True
        for gt in cond.graph[cond_name].outputs:
            assert (cond.graph[gt].op == 'get_tuple')
            inputs[cond.graph[gt].attr['index']] = True

        remove = []
        for idx, used in enumerate(inputs):
            if not used:
                remove.append(idx)
        if len(remove) == 0:
            continue
        else:
            removed = True

        gd = body.graph
        mt = gd[body_end].inputs[0]
        for idx in remove:
            disconnect_edge(gd, gd[mt].inputs[idx], mt)

        for gt in gd[body_name].outputs:
            # Need to do it in the stupid way, get_tuple are not sorted in index.
            rm_idx = 0
            idx = gd[gt].attr['index']
            while rm_idx < len(remove) and remove[rm_idx] < idx:
                rm_idx += 1
                gd[gt].attr['index'] -= 1
        for gt in cond.graph[cond_name].outputs:
            # Need to do it in the stupid way, get_tuple are not sorted in index.
            rm_idx = 0
            idx = cond.graph[gt].attr['index']
            while rm_idx < len(remove) and remove[rm_idx] < idx:
                rm_idx += 1
                cond.graph[gt].attr['index'] -= 1

        rm_nodes = []
        _, rm_nodes = DFS_check(gd, body_name)
        for node in rm_nodes:
            delete_node(gd, node)


def DFS_check(gd, node):
    if gd[node].op == 'return':
        return True, []
    if len(gd[node].outputs) == 0:
        return False, [node]
    valid = False
    rm_nodes = []
    for out in gd[node].outputs:
        v, rm = DFS_check(gd, out)
        if v:
            valid = True
        rm_nodes += rm
    if not valid:
        return False, rm_nodes + [node]
    return True, rm_nodes

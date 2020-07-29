# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ..basic_graph_ops import delete_node, check_connections, disconnect_edge

import logging


def convert_constant_nodes_to_const_ops(tfssa):
    """
    Convert nodes with known constant value to Const nodes
    """
    for fn_key in list(tfssa.functions.keys()):
        f = tfssa.functions[fn_key]
        for k in list(f.graph.keys()):
            v = f.graph.get(k, None)
            if v is None:
                continue
            if v.value is not None:
                v.op = "Const"
                # delete all upstream edges now that this is constant
                inv = v.inputs[:]
                for i in inv:
                    curnode = i
                    nextnode = v.name
                    disconnect_edge(f.graph, curnode, nextnode)

                    # keep deleting upwards as long as it is a chain
                    while curnode is not None:
                        prevnode = None
                        if len(f.graph[curnode].outputs) == 0:
                            if len(f.graph[curnode].inputs) == 1:
                                prevnode = f.graph[curnode].inputs[0]
                            delete_node(f.graph, curnode)
                        curnode = prevnode


def delete_nodes_with_only_constant_descendents(tfssa):
    # look for nodes whose value is known AND downstream values are known
    # and delete them
    delete_count = 0
    for fn_key in list(tfssa.functions.keys()):
        f = tfssa.functions[fn_key]
        keys = list(f.graph.keys())
        for k in keys:
            if k not in f.graph:
                continue
            to_delete = (f.graph[k].value is not None) and (k not in f.outputs)
            if to_delete:
                # check the outputs
                for o in f.graph[k].outputs:
                    if f.graph[o].value is None:
                        to_delete = False
                    else:
                        disconnect_edge(f.graph, k, o)
            if to_delete:
                delete_count += 1
                delete_node(f.graph, k)
        # also delete all Const nodes with no descendants
        keys = list(f.graph.keys())
        for k in keys:
            if k not in f.graph:
                continue
            if (
                f.graph[k].op == "Const"
                and len(f.graph[k].outputs) == 0
                and (k not in f.outputs)
            ):
                delete_count += 1
                delete_node(f.graph, k)
    return delete_count


def delete_unnecessary_constant_nodes(tfssa):
    delete_count = delete_nodes_with_only_constant_descendents(tfssa)
    for f in list(tfssa.functions.values()):
        check_connections(f.graph)
    convert_constant_nodes_to_const_ops(tfssa)
    logging.debug("%s nodes deleted", delete_count)
    return delete_count

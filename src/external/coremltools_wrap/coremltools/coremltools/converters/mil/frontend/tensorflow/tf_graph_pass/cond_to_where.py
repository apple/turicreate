# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..basic_graph_ops import delete_node, disconnect_edge
from .visitors import FindAllUpstreamTerminals

import logging
from coremltools._deps import _HAS_TF_2


def compute_max_rank(graph):
    #  highly inefficient way to calculate the rank of every node
    ret = {}
    # begin at max rank
    for v in graph.keys():
        if len(graph[v].inputs) == 0:
            ret[v] = 0
        else:
            ret[v] = len(graph)

    changes = True
    while changes:
        changes = False
        for v in graph.keys():
            if len(graph[v].inputs) > 0:
                rank = max(ret[i] for i in graph[v].inputs) + 1
                if ret[v] != rank:
                    changes = True
                    ret[v] = rank
    return ret


class CondToWhere(object):
    @staticmethod
    def _search(g, node_name):
        """
        Find the nearest Switch nodes upstream of node_name.
        """
        node = g[node_name]

        switches = (
            FindAllUpstreamTerminals(lambda x: x.op == "Switch")
            .visit(g, node.name)
            .get_result()
        )
        if len(switches) == 0:
            switches = (
                FindAllUpstreamTerminals(
                    lambda x: x.op == "Switch" or x.attr.get("was_switch") is not None
                )
                .visit(g, node.name)
                .get_result()
            )
        return switches

    @staticmethod
    def _fix_found_cond(g, merge, switches):
        """
        Convert a Merge's Switch nodes to Identity ops and the Merge to iff.
        """
        if g[switches[0]].op == "Switch":
            condition_input = g[switches[0]].inputs[1]
        else:
            condition_input = g[switches[0]].attr["was_switch"]

        # convert the merge to a select
        # TensorFlow seems to ensure the condition that the first
        # merge input is the True branch and the second merge input
        # is the false branch.

        # we convert switches to identity, detaching to switch condition
        for s in switches:
            if g[s].op == "Switch":
                g[s].op = "Identity"
                g[s].attr["was_switch"] = g[s].inputs[1]
                # detach input 1: the switch condition
                if g[s].inputs[0] == g[s].inputs[1]:
                    g[s].inputs.pop()
                    g[g[s].inputs[0]].outputs.pop()
                else:
                    disconnect_edge(g, g[s].inputs[1], s)

        # build the final select
        g[merge].op = "iff"
        if not _HAS_TF_2:
            # swap true branch with false branch to get the right semantics for IFF
            g[merge].inputs[0], g[merge].inputs[1] = (
                g[merge].inputs[1],
                g[merge].inputs[0],
            )

        g[merge].inputs = [condition_input] + g[merge].inputs
        g[condition_input].outputs.append(merge)

    def cond_to_where(self, graph):
        stuff_done = False
        g = graph
        ranks = compute_max_rank(graph)
        merges = [a for a in g if g[a].op == "Merge"]
        merges = sorted(merges, key=lambda k: ranks[k])
        if len(merges) == 0:
            return False
        for m in merges:
            logging.debug("Fixing cond at merge location: %s", m)
            switches = self._search(g, m)
            self._fix_found_cond(g, m, switches)
            stuff_done = True

        # delete the extra switches that seem to just lead to identities
        # which then lead nowhere but into control dependencies
        extra_switches = [a for a in g if g[a].op == "Switch"]
        for s in extra_switches:
            if all(
                [g[o].op == "Identity" and len(g[o].outputs) == 0 for o in g[s].outputs]
            ):
                nodes_to_delete = g[s].outputs + [s]
                for d in nodes_to_delete:
                    delete_node(g, d)
                    stuff_done = True
        return stuff_done


def cond_to_where(tfssa):
    for k, v in tfssa.functions.items():
        while True:
            stuff_done = CondToWhere().cond_to_where(v.graph)
            if not stuff_done:
                break

# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..parsed_tf_node import ParsedTFNode
from ....commons.basic_graph_ops import delete_node, disconnect_edge
from .functionalize_loops import *


def compute_max_rank(graph):
    #  highly inefficient way to calculate the rank of every node
    ret = {}
    # begin at max rank
    for v in graph.keys():
        if graph[v].inputs == 0:
            ret[v] = 0
        else:
            ret[v] = len(graph)

    changes = True
    while changes == True:
        changes = False
        for v in graph.keys():
            if len(graph[v].inputs) > 0:
                rank = max(ret[i] for i in graph[v].inputs) + 1
                if ret[v] != rank:
                    changes = True
                    ret[v] = rank
    return ret


class CondToWhere(object):
    # this should run AFTER functionalize loops
    def __init__(self):
        self.switches = None
        self.merge = ''

    def _search(self, g, node):
        if not isinstance(node, ParsedTFNode):
            node = g[node]
        self.merge = node.name
        # we look for Merge nodes
        if node.op == "Merge":
            print("Fixing cond at merge location: %s" % (node.name))
            self.switches = FindAllUpstreamTerminals(lambda x: x.op == 'Switch').visit(
                g, node.name).get_result()
            if len(self.switches) == 0:
                self.switches = FindAllUpstreamTerminals(
                    lambda x: x.op == 'Switch' or x.attr.get('was_switch') is not None).visit(
                        g, node.name).get_result()

    def _fix_found_cond(self, g):
        if g[self.switches[0]].op == 'Switch':
            condition_input = g[self.switches[0]].inputs[1]
        else:
            condition_input = g[self.switches[0]].attr['was_switch']

        # convert the merge to a select
        # Tensorflow seems to ensure the condition that the first
        # merge input is the True branch and the second merge input
        # is the false branch.
        #

        # we convert switches to identity, detaching to switch condition
        for s in self.switches:
            if g[s].op == 'Switch':
                g[s].op = 'Identity'
                g[s].attr['was_switch'] = g[s].inputs[1]
                # detach input 1: the switch condition
                if g[s].inputs[0] == g[s].inputs[1]:
                    g[s].inputs.pop()
                    g[g[s].inputs[0]].outputs.pop()
                else:
                    disconnect_edge(g, g[s].inputs[1], s)

        # build the final select
        g[self.merge].op = 'iff'
        # swap true branch with false branch to get the right semantics for IFF
        g[self.merge].inputs[0], g[self.merge].inputs[1] = g[self.merge].inputs[1], g[
            self.merge].inputs[0]

        g[self.merge].inputs = [condition_input] + g[self.merge].inputs
        g[condition_input].outputs.append(self.merge)
        return True

    def cond_to_where(self, graph):
        stuff_done = False
        g = graph
        ranks = compute_max_rank(graph)
        merges = [a for a in g if g[a].op == 'Merge']
        merges = sorted(merges, key=lambda k: ranks[k])
        if len(merges) == 0:
            return False
        for m in merges:
            self._search(g, m)
            ret = self._fix_found_cond(g)
            if ret:
                stuff_done = True
        # delete the extra switches that seem to just lead to identities
        # which then lead nowhere but into control dependencies
        extra_switches = [a for a in g if g[a].op == 'Switch']
        for s in extra_switches:
            if all([g[o].op == 'Identity' and len(g[o].outputs) == 0 for o in g[s].outputs]):
                nodes_to_delete = g[s].outputs + [s]
                for d in nodes_to_delete:
                    delete_node(g, d)
                    stuff_done = True
        return stuff_done


def cond_to_where(ssa):
    for k, v in ssa.functions.items():
        while True:
            stuff_done = CondToWhere().cond_to_where(v.graph)
            if stuff_done == False:
                break

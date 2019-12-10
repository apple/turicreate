# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ...commons.basic_graph_ops import replace_source, delete_node
from ...commons import builtins


class RemoveIdentities(object):
    def __init__(self, whole_graph):
        self.whole_graph = whole_graph

    def trace(self):
        # check that every function is only called in one place
        for fname, fn in self.whole_graph.functions.items():
            self._trace_function(fname)

    def _trace_function(self, fname):
        fn = self.whole_graph.functions[fname]
        nodenames = list(fn.graph.keys())
        for nodename in nodenames:
            node = fn.graph[nodename]
            if node.op == 'Identity':
                # the main function is a little special.
                # An identity node might be an output
                if fname == 'main' and len(node.outputs) == 0:
                    continue
                value = node.inputs[0]
                for o in node.outputs:
                    replace_source(fn.graph, nodename, o, value)
                delete_node(fn.graph, nodename)


def remove_identities(ssa):
    RemoveIdentities(ssa).trace()

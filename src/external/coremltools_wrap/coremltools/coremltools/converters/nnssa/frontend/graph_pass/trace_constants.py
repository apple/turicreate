# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ...commons.basic_graph_ops import topsort, delete_node, disconnect_edge, connect_edge
from ...commons import builtins


class ConstantTracing(object):
    def __init__(self, whole_graph):
        self.whole_graph = whole_graph
        self.node_value_trace = {}

    def trace(self):
        # check that every function is only called in one place
        callcount = {}
        for fname, fn in self.whole_graph.functions.items():
            for nodename, node in fn.graph.items():
                if node.op == 'while':
                    fname = node.attr['cond_function']
                    callcount[fname] = callcount.get(fname, 0) + 1
                    fname = node.attr['body_function']
                    callcount[fname] = callcount.get(fname, 0) + 1
        for v in callcount.values():
            assert (v == 1)
        self._trace_function('main')
        # now trace all functions which use a constant C
        # function F uses C is if there exists some node in F which
        # takes C as an input, and the node is not one of get_tuple, make_tuple and while
        const_usages = {}
        for (fname, nodename), v in self.node_value_trace.items():
            if type(v) is tuple and self.whole_graph.functions[v[0]].graph[v[1]].value is not None:
                fn = self.whole_graph.functions[fname]
                node = fn.graph[nodename]
                for o in node.outputs:
                    if fn.graph[o].op not in ['make_tuple', 'while']:
                        if v not in const_usages:
                            const_usages[v] = set()
                        const_usages[v].add(fname)
        # while loops are special. Because the inputs and outputs are bound.
        # So if I have to line up the corresponding input/output terms for each
        # while loop. Both terms have to be in const_usages for it to be
        # actually removable.
        for fname, fn in self.whole_graph.functions.items():
            for nodename, node in fn.graph.items():
                if node.op == 'while':
                    for i, o in zip(self.node_value_trace[(fname, node.inputs[0])],
                                    self.node_value_trace[(fname, nodename)]):
                        if i in const_usages and o not in const_usages:
                            del const_usages[i]
                        if o in const_usages and i not in const_usages:
                            del const_usages[o]

        remap_candidates = {}
        for k, v in const_usages.items():
            if k[0] not in v and len(v) == 1:
                remap_candidates[k] = list(v)[0]
                print(k, list(v)[0])

        # remap_candidates[(fn,const_node,)] -> target_function
        # means that we can move this const_node into that particular function
        for (fn, const_node), target_function in remap_candidates.items():
            self._move_constant(fn, const_node, target_function)

    def _trace_function(self, fname):
        fn = self.whole_graph.functions[fname]

        nodenames = topsort(fn.graph)
        for nodename in nodenames:
            node = fn.graph[nodename]
            if (
                    fname,
                    nodename,
            ) in self.node_value_trace:
                continue
            if node.value is not None:
                self.node_value_trace[(
                    fname,
                    nodename,
                )] = (
                    fname,
                    nodename,
                )
            elif node.op == 'make_tuple':
                val = [self.node_value_trace.get((
                    fname,
                    i,
                ), None) for i in node.inputs]
                self.node_value_trace[(
                    fname,
                    nodename,
                )] = val
            elif node.op == 'get_tuple':
                inval = self.node_value_trace[(
                    fname,
                    node.inputs[0],
                )]
                if type(inval) is list:
                    # input is a tuple we traced
                    self.node_value_trace[(
                        fname,
                        nodename,
                    )] = inval[node.attr['index']]
                else:
                    # input is a tuple type we don't recognize. give it a unique key
                    self.node_value_trace[(
                        fname,
                        nodename,
                    )] = (
                        fname,
                        nodename,
                    )
            elif node.op == 'Identity' or node.op == 'return':
                self.node_value_trace[(
                    fname,
                    nodename,
                )] = self.node_value_trace[(
                    fname,
                    node.inputs[0],
                )][:]
            elif node.op == 'while':
                # check that the cond and body function have only 1 input and one output
                cond_function = node.attr['cond_function']
                body_function = node.attr['body_function']
                cond_entry = self.whole_graph.functions[cond_function].inputs
                body_entry = self.whole_graph.functions[body_function].inputs
                cond_exit = self.whole_graph.functions[cond_function].outputs
                body_exit = self.whole_graph.functions[body_function].outputs
                assert (len(cond_entry) == 1)
                assert (len(body_entry) == 1)
                assert (len(cond_exit) == 1)
                assert (len(body_exit) == 1)
                cond_entry = cond_entry[0]
                body_entry = body_entry[0]
                cond_exit = cond_exit[0]
                body_exit = body_exit[0]

                inval = self.node_value_trace[(
                    fname,
                    node.inputs[0],
                )]
                if (
                        cond_function,
                        cond_entry,
                ) not in self.node_value_trace:
                    self.node_value_trace[(
                        cond_function,
                        cond_entry,
                    )] = inval[:]
                    self._trace_function(cond_function)
                if (
                        body_function,
                        body_entry,
                ) not in self.node_value_trace:
                    self.node_value_trace[(
                        body_function,
                        body_entry,
                    )] = inval[:]
                    self._trace_function(body_function)
                # unify exits
                body_outval = self.node_value_trace[(
                    body_function,
                    body_exit,
                )]
                self.node_value_trace[(
                    fname,
                    nodename,
                )] = body_outval[:]
            else:
                self.node_value_trace[(
                    fname,
                    nodename,
                )] = (
                    fname,
                    nodename,
                )

    def _move_constant(self, source_fname, source_node_name, target_fname):
        # now. this is the painful part. quite a lot of rewriting has to happen here

        source_fn = self.whole_graph.functions[source_fname]
        target_fn = self.whole_graph.functions[target_fname]
        source_const_key = (
            source_fname,
            source_node_name,
        )
        source_const_node = source_fn.graph[source_node_name]

        # move the nodes.

        # make sure the new const node is in the trace
        self.node_value_trace[(
            target_fname,
            source_const_node.name,
        )] = (
            target_fname,
            source_const_node.name,
        )

        # We first remove references to the source node from all functions
        fnames = list(self.whole_graph.functions.keys())
        fnames.pop(fnames.index(source_fname))
        fnames.pop(fnames.index(target_fname))
        fnames = [source_fname, target_fname] + fnames
        for fname in fnames:

            fn = self.whole_graph.functions[fname]
            # remove the const node everywhere including from the target function
            # This might seem awkward. but not really.
            #
            # Essentially, we are entirely removing the source constant node
            # entirely and re-associating everything which uses it in the target
            # function with the moved constant. This means that even the
            # entry point and exit points of the target function have to be
            # rewritten. Thus it is just simpler to just think about it
            # as a complete deletion of the source constant node.
            #
            # The only care we have to take is the get_tuple in the target graph
            # has be modified. And not deleted.
            delete_nodes = []
            nodenames = topsort(fn.graph)
            for nodename in nodenames:
                node = fn.graph[nodename]
                cur_node_key = (
                    fname,
                    nodename,
                )
                if type(self.node_value_trace[cur_node_key]) is list and \
                        self.node_value_trace[cur_node_key].count(source_const_key) > 0:
                    # remove the input
                    idx = self.node_value_trace[cur_node_key].index(source_const_key)
                    if node.op == 'make_tuple':
                        disconnect_edge(fn.graph, node.inputs[idx], nodename)
                    # update type
                    if node.datatype is not None:
                        newtype = list(node.datatype.T)[:]
                        newtype.pop(idx)
                        node.datatype = builtins.tuple(newtype)
                    # update the trace. maintain invariants
                    self.node_value_trace[cur_node_key].pop(idx)
                elif type(self.node_value_trace[cur_node_key]) is tuple and \
                        self.node_value_trace[cur_node_key] == source_const_key:
                    delete_nodes.append(nodename)
                elif node.op == 'get_tuple':
                    my_trace = self.node_value_trace[(
                        fname,
                        nodename,
                    )]
                    parent_trace = self.node_value_trace[(
                        fname,
                        node.inputs[0],
                    )]
                    if type(parent_trace) is list:
                        node.attr['index'] = parent_trace.index(my_trace)

            if fname == source_fname:
                delete_node(source_fn.graph, source_node_name)
                target_fn.graph[source_node_name] = source_const_node
                for d in delete_nodes:
                    if d != source_node_name:
                        delete_node(fn.graph, d)
            elif fname == target_fname:
                # if this is the target function, we rewrite.
                for d in delete_nodes:
                    assert (len(fn.graph[d].inputs) == 1)
                    fn.graph[d].op = 'Identity'
                    disconnect_edge(fn.graph, fn.graph[d].inputs[0], d)
                    connect_edge(fn.graph, source_const_node.name, d)
                    self.node_value_trace[(
                        fname,
                        d,
                    )] = (
                        fname,
                        source_const_node.name,
                    )
            else:
                for d in delete_nodes:
                    delete_node(fn.graph, d)

        # cleanup. delete the old node from the trace
        del self.node_value_trace[(
            source_fname,
            source_const_node.name,
        )]


def trace_constants(ssa):
    ConstantTracing(ssa).trace()

# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ..parsed_tf_node import ParsedTFNode


class FindAllDownstreamTerminals(object):
    # Find all nodes matching a particular function
    # which is downstream reachable from a set of nodes.
    def __init__(self, fn):
        self.result = []
        self.fn = fn
        self.memo = {}

    def visit(self, g, node):
        if not isinstance(node, ParsedTFNode):
            node = g[node]

        if node.name in self.memo:
            return self
        self.memo[node.name] = 1

        if self.fn(node):
            self.result.append(node.name)
            return self

        for i in node.outputs:
            self.visit(g, g[i])

        return self

    def visit_many(self, g, nodes):
        for i in nodes:
            self.visit(g, i)
        return self

    def get_result(self):
        return self.result


class FindAllReachableNodes(object):
    # Find all nodes reachable from a set of nodes which satisfy a criteria
    def __init__(self, fn):
        self.result = []
        self.fn = fn
        self.memo = {}

    def visit(self, g, node):
        if not isinstance(node, ParsedTFNode):
            node = g[node]

        if node.name in self.memo:
            return self
        self.memo[node.name] = 1

        if self.fn(node):
            self.result.append(node.name)

        for i in node.outputs:
            self.visit(g, g[i])

        for i in node.inputs:
            self.visit(g, g[i])

        return self

    def visit_many(self, g, nodes):
        for i in nodes:
            self.visit(g, i)
        return self

    def get_result(self):
        return self.result


class FindImmediateUpstreamNodes(object):
    # Find all nodes matching a particular function which is immediately above a set of nodes
    def __init__(self, fn):
        self.result = []
        self.fn = fn

    def visit(self, g, node):
        if not isinstance(node, ParsedTFNode):
            node = g[node]

        for i in node.inputs:
            if self.fn(g[i]):
                self.result.append(i)

        return self

    def visit_many(self, g, nodes):
        for i in nodes:
            self.visit(g, i)
        return self

    def get_result(self):
        return self.result


class FindImmediateDownstreamNodes(object):
    # Find all nodes matching a particular function which is immediately above a set of nodes
    def __init__(self, fn):
        self.result = []
        self.fn = fn

    def visit(self, g, node):
        if not isinstance(node, ParsedTFNode):
            node = g[node]

        for i in node.outputs:
            if self.fn(g[i]):
                self.result.append(i)

        return self

    def visit_many(self, g, nodes):
        for i in nodes:
            self.visit(g, i)
        self.result = list(set(self.result))
        return self

    def get_result(self):
        return self.result


class FindAllUpstreamTerminals(object):
    # Find the "upstream frontier" of nodes passing some predicate.
    # In other words, perform a pre-order traversal of a node and its inputs, collecting all nodes
    # passing a given predicate as we go along. Terminate the search along a given branch as soon
    # as a node is collected.
    def __init__(self, fn, control_dependencies=False):
        self.result = []
        self.fn = fn
        self.control_dependencies = control_dependencies
        self.memo = {}

    def visit(self, g, node):
        if not isinstance(node, ParsedTFNode):
            node = g[node]

        if node.name in self.memo:
            return self
        self.memo[node.name] = 1

        if self.fn(node):
            self.result.append(node.name)
            return self

        for i in node.inputs:
            self.visit(g, g[i])
        if self.control_dependencies:
            for i in node.control_inputs:
                self.visit(g, g[i])
        return self

    def visit_many(self, g, nodes):
        for i in nodes:
            self.visit(g, i)
        self.result = list(set(self.result))
        return self

    def get_result(self):
        return self.result


class FindSubgraph(object):
    # Find all nodes between a set of sources and a set of terminals
    # Sources are not returned, but reached terminals are returned
    def __init__(self, terminal_nodes):
        self.memo = {}
        self.terminal = terminal_nodes

    def visit_impl(self, g, node):
        if not isinstance(node, ParsedTFNode):
            node = g[node]

        if node.name in self.terminal:
            self.memo[node.name] = True
            return True

        if node.name in self.memo:
            return self.memo[node.name]

        # add self to memo first otherwise cycles will not terminate
        self.memo[node.name] = None
        reachable = None
        all_unreachable = True
        for i in node.outputs + node.control_outputs:
            visit_result = self.visit_impl(g, g[i])
            if visit_result == True:  # pylint: disable=singleton-comparison
                reachable = True
            if visit_result != False:  # pylint: disable=singleton-comparison
                all_unreachable = False

        if reachable:
            self.memo[node.name] = reachable
        elif all_unreachable:
            self.memo[node.name] = False
        else:
            self.memo[node.name] = None

        return reachable

    def visit(self, g, node):
        self.visit_impl(g, node)
        while True:
            if None in iter(self.memo.values()):
                revisit = [k for k, v in self.memo.items() if v is None]
                self.memo = {k: v for k, v in self.memo.items() if v is not None}
                for n in revisit:
                    self.visit_impl(g, n)
            else:
                break
        return self

    def visit_many(self, g, nodes):
        for node in nodes:
            self.visit_impl(g, node)
        while True:
            if None in iter(self.memo.values()):
                revisit = [k for k, v in self.memo.items() if v is None]
                self.memo = {k: v for k, v in self.memo.items() if v is not None}
                for n in revisit:
                    self.visit_impl(g, n)
            else:
                break
        return self

    def get_result(self):
        return [k for k, v in self.memo.items() if v]

# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..parsed_tf_node import ParsedTFNode
from ..basic_graph_ops import *  # pylint: disable=unused-wildcard-import,wildcard-import
from ..tfssa import SSAFunction
from .visitors import (
    FindAllReachableNodes,
    FindImmediateDownstreamNodes,
    FindImmediateUpstreamNodes,
    FindSubgraph,
)
import logging


class FunctionalizeLoops(object):
    """
    Turns while loops in TensorFlow dataflow graph into the functional form:
    while(cond_function, body_function)

    Usage:
    Given a graph in tfssa (the NetworkEnsemble defined in network.py) form:

    This will functionalize *ONE* loop in the main function.

        f = FunctionalizeLoops()
        ret = f.functionalize_loops(self, tfssa, "main")

    if ret is True, one loop has been functionalized, and the new functions
    added to tfssa. If False, there is no loop to functionalize.

    Generally, repeated calls to this will be necessary to catch all loops.

    Instead, use functionalize_loops.
    """

    def __init__(self):
        self.exits = None
        self.merges = None
        self.enters = None
        self.constant_enters = None
        self.switches = None
        self.subgraph = None
        self.loopcond = None
        self.is_constant = None
        self.next_iterations = None
        self.cond = None
        self.body = None

    def _search(self, g, node):
        if not isinstance(node, ParsedTFNode):
            node = g[node]

        # we look for NextIteration nodes
        assert node.op == "Enter"

        frame_name = node.attr["frame_name"]
        logging.debug("Fixing frame name: %s", frame_name)
        # find all the enter args
        # this is basically the enter frame
        # functionalize_control_flow.cc:FunctionalizeControlFlow (1160-1196)
        self.enters = [
            k for k, v in g.items() if v.attr.get("frame_name", "") == frame_name
        ]
        self.is_constant = [
            bool(g[n].attr.get("is_constant", False)) for n in self.enters
        ]
        self.merges = (
            FindImmediateDownstreamNodes(lambda x: x.op == "Merge")
            .visit_many(g, self.enters)
            .get_result()
        )
        self.next_iterations = (
            FindImmediateUpstreamNodes(lambda x: x.op == "NextIteration")
            .visit_many(g, self.merges)
            .get_result()
        )
        self.switches = (
            FindImmediateDownstreamNodes(lambda x: x.op == "Switch")
            .visit_many(g, self.merges)
            .get_result()
        )
        self.exits = (
            FindImmediateDownstreamNodes(lambda x: x.op == "Exit")
            .visit_many(g, self.switches)
            .get_result()
        )
        self.loopcond = list(
            set(
                FindImmediateUpstreamNodes(lambda x: x.op == "LoopCond")
                .visit_many(g, self.switches)
                .get_result()
            )
        )

        self.subgraph = FindSubgraph(self.exits).visit_many(g, self.enters).get_result()
        self.cond = FindSubgraph(self.switches).visit_many(g, self.merges).get_result()
        self.body = (
            FindSubgraph([node.name] + self.exits)
            .visit_many(g, self.switches)
            .get_result()
        )
        # drop merges and switches from cond and body
        self.cond = [
            i for i in self.cond if i not in (self.merges + self.switches + self.enters)
        ]
        self.body = (
            [i for i in self.body if i not in ([node.name] + self.switches)]
            + [node.name]
            + self.switches
            + self.merges
            + self.enters
        )

        # ok. we can now rebuild.

    def _fix_graph_invariants(self, g):
        import copy

        check = lambda x: x is not None and len(x) > 0
        check(self.exits)
        check(self.merges)
        check(self.enters)
        check(self.switches)
        check(self.subgraph)
        check(self.cond)
        check(self.loopcond)
        assert len(self.loopcond) == 1
        # maintain the invariant of a unique Enter node per argument
        # functionalize_control_flow.cc:FunctionalizeLoop (295)
        for i in copy.copy(self.enters):
            node = g[i]
            assert len(node.outputs) > 0
            assert len(node.inputs) == 1
            assert len(node.control_inputs) == 0
            assert len(node.control_outputs) == 0
            if len(node.outputs) == 1:
                continue
            node_output_copy = copy.copy(node.outputs)
            for j in range(1, len(node_output_copy)):
                # make a new enter node for each
                new_enter_node = copy.deepcopy(node)
                new_enter_node.inputs = []
                new_enter_node.outputs = []
                new_enter_node.name = node.name + "/trsplit%d" % (j)
                g[new_enter_node.name] = new_enter_node
                logging.debug("splitting %s", node.name)
                # connect the new node
                enter_output = node_output_copy[j]
                disconnect_edge(g, node.name, enter_output)
                connect_edge(g, new_enter_node.name, enter_output)
                connect_sources(g, node.inputs, new_enter_node.name)
                # insert into graph
                self.enters.append(new_enter_node.name)

    def functionalize_loops(self, tfssa, function_to_functionalize):
        g = tfssa.functions[function_to_functionalize].graph
        loopni = [a for a in g if g[a].op == "Enter"]
        if len(loopni) == 0:
            return False
        self._search(g, loopni[0])

        self.constant_enters = [
            self.enters[i] for i in range(len(self.enters)) if self.is_constant[i]
        ]
        self.enters = [
            self.enters[i] for i in range(len(self.enters)) if not self.is_constant[i]
        ]
        self._fix_graph_invariants(g)
        # for each enter node, find the corresponding downstream merge node
        enter_corresponding_merge = [
            FindImmediateDownstreamNodes(lambda x: x.op == "Merge")
            .visit(g, enter)
            .get_result()[0]
            for enter in self.enters
        ]
        merge_corresponding_ni = [
            FindImmediateUpstreamNodes(lambda x: x.op == "NextIteration")
            .visit(g, merge)
            .get_result()[0]
            for merge in enter_corresponding_merge
        ]
        switch_corresponding_merge = []
        for merge in enter_corresponding_merge:
            switch_after_merge = (
                FindImmediateDownstreamNodes(lambda x: x.op == "Switch")
                .visit(g, merge)
                .get_result()
            )
            if len(switch_after_merge) > 0:
                switch_corresponding_merge.append(switch_after_merge[0])
            else:
                # There are some situations there is no switch not for a given
                # merge. While odd... its ok. we construct one
                # In this situation there is no Exit either, but it can be
                # constructed later on
                new_switch_node = ParsedTFNode()
                new_switch_node.op = "Switch"
                new_switch_node.name = tfssa._find_free_name("fake_switch_")
                g[new_switch_node.name] = new_switch_node
                connect_edge(g, merge, new_switch_node.name)
                connect_edge(g, self.loopcond[0], new_switch_node.name)
                switch_corresponding_merge.append(new_switch_node.name)

        exit_corresponding_switch = []
        for switch in switch_corresponding_merge:
            res = (
                FindImmediateDownstreamNodes(lambda x: x.op == "Exit")
                .visit(g, switch)
                .get_result()
            )
            if len(res) > 0:
                exit_corresponding_switch.append(res[0])
            else:
                new_exit_node = ParsedTFNode()
                new_exit_node.op = "Exit"
                new_exit_node.name = tfssa._find_free_name("fake_exit_")
                g[new_exit_node.name] = new_exit_node
                connect_edge(g, switch, new_exit_node.name)
                exit_corresponding_switch.append(new_exit_node.name)

        while_loop = ParsedTFNode()
        while_loop.op = "while"
        while_loop.name = tfssa._find_free_name("while_")
        g[while_loop.name] = while_loop

        # Build the Loop Condition

        # replace all enters with a single make_tuple
        # we replace merge with get_tuple and turn it into a function call
        # terminated with LoopCond
        make_inputs = ParsedTFNode()
        make_inputs.op = "make_tuple"
        make_inputs.name = tfssa._find_free_name("make_input_")
        g[make_inputs.name] = make_inputs
        for enter in self.enters:
            replace_dest(g, g[enter].inputs[0], enter, make_inputs.name)
        constant_base_index = len(make_inputs.inputs)
        for enter in self.constant_enters:
            replace_dest(g, g[enter].inputs[0], enter, make_inputs.name)

        connect_edge(g, make_inputs.name, while_loop.name)
        connect_dests(g, while_loop.name, exit_corresponding_switch)

        # build the cond function
        cond_body = ParsedTFNode()
        cond_body.op = "function_entry"
        cond_body.name = tfssa._find_free_name("cond_function_")
        cond_body.inputs = []
        g[cond_body.name] = cond_body
        for merge_idx in range(len(enter_corresponding_merge)):
            merge = enter_corresponding_merge[merge_idx]
            switch = switch_corresponding_merge[merge_idx]
            enter_node = g[self.enters[merge_idx]]
            merge_node = g[merge]
            if switch is not None:
                switch_node = g[switch]
            else:
                switch_node = None
            merge_node.op = "get_tuple"
            merge_node.attr = {"index": merge_idx}
            # disconnect merge from switch
            # disconnect loopcond from switch
            disconnect_edge(g, enter_node.name, merge_node.name)
            if switch_node is not None:
                disconnect_edge(g, merge_node.name, switch_node.name)
                disconnect_edge(g, self.loopcond[0], switch_node.name)
            for i in merge_node.inputs[:]:
                disconnect_edge(g, i, merge_node.name)
            connect_edge(g, cond_body.name, merge_node.name)
            # delete get_tuple if it does nothing
            if len(merge_node.outputs) == 0:
                delete_node(g, merge)

        g[self.loopcond[0]].op = "return"

        # build the body function
        body = ParsedTFNode()
        body.op = "function_entry"
        body.name = tfssa._find_free_name("body_function_")
        body.inputs = []
        g[body.name] = body
        for switch_idx in range(len(switch_corresponding_merge)):
            switch = switch_corresponding_merge[switch_idx]
            exit = exit_corresponding_switch[switch_idx]
            disconnect_edge(g, switch, exit)

            # replace switch with a get_tuple
            switch_node = g[switch]
            switch_node.op = "get_tuple"
            switch_node.attr = {"index": switch_idx}
            connect_edge(g, body.name, switch_node.name)
            # delete get_tuple if it does nothing
            if len(switch_node.outputs) == 0:
                delete_node(g, switch)

        # replace all next_iteration with a single make_tuple
        # we replace merge with get_tuple and turn it into a function call
        # terminated with LoopCond
        make_outputs = ParsedTFNode()
        make_outputs.op = "make_tuple"
        make_outputs.name = tfssa._find_free_name("make_output_")
        g[make_outputs.name] = make_outputs
        for ni in merge_corresponding_ni:
            connect_edge(g, g[ni].inputs[0], make_outputs.name)

        # connect constant enters to come from function
        # connect constant enters to exit
        for idx, enter in enumerate(self.constant_enters):
            body_connected = False
            for output in list(g[enter].outputs):
                if output not in self.cond and output not in self.body:
                    cond_intersection = (
                        FindSubgraph(self.cond).visit(g, output).get_result()
                    )
                    body_intersection = (
                        FindSubgraph(self.body).visit(g, output).get_result()
                    )
                    if len(cond_intersection) > 0:
                        cond_intersection.append(output)
                        self.cond += cond_intersection
                    if len(body_intersection) > 0:
                        body_intersection.append(output)
                        self.body += body_intersection
                get_tuple = ParsedTFNode()
                get_tuple.op = "get_tuple"
                get_tuple.name = tfssa._find_free_name("get_tuple_const_")
                get_tuple.attr = {"index": idx + constant_base_index}
                g[get_tuple.name] = get_tuple

                if output in self.cond:
                    connect_edge(g, cond_body.name, get_tuple.name)
                elif output in self.body:
                    connect_edge(g, body.name, get_tuple.name)
                replace_source(g, enter, output, get_tuple.name)

            # body must accept and return everything
            get_tuple = ParsedTFNode()
            get_tuple.op = "get_tuple"
            get_tuple.name = tfssa._find_free_name("get_tuple_const_")
            get_tuple.attr = {"index": idx + constant_base_index}
            g[get_tuple.name] = get_tuple
            connect_edge(g, body.name, get_tuple.name)
            connect_edge(g, get_tuple.name, make_outputs.name)

        assert len(g[make_outputs.name].inputs) == len(g[make_inputs.name].inputs)

        output_return = ParsedTFNode()
        output_return.op = "return"
        output_return.name = tfssa._find_free_name("body_return_")
        g[output_return.name] = output_return
        connect_edge(g, make_outputs.name, output_return.name)
        while_loop.attr["cond_function"] = cond_body.name
        while_loop.attr["body_function"] = body.name
        for i in self.enters:
            delete_node(g, i)
        for i in self.next_iterations:
            delete_node(g, i)
        for i in self.constant_enters:
            delete_node(g, i)

        for i in range(len(exit_corresponding_switch)):
            exit_node = exit_corresponding_switch[i]
            g[exit_node].op = "get_tuple"
            g[exit_node].attr = {"index": i}
        cond_function = (
            FindSubgraph(self.loopcond[0]).visit(g, cond_body.name).get_result()
        )
        cond_function = set(cond_function + [self.loopcond[0], cond_body.name])
        body_function = (
            FindSubgraph(output_return.name).visit(g, body.name).get_result()
        )
        body_function = set(body_function + [body.name, output_return.name])

        # trace input constants associated with the cond_graph
        # and the body_graph. These constants can only have one consumer
        # for now. Any more and we will either need to associate
        # it as an argument, or split the constant.
        cond_constants = (
            FindImmediateUpstreamNodes(lambda x: x.op == "Const")
            .visit_many(g, cond_function)
            .get_result()
        )
        body_constants = (
            FindImmediateUpstreamNodes(lambda x: x.op == "Const")
            .visit_many(g, body_function)
            .get_result()
        )
        # for const_node in cond_constants + body_constants:
        #    assert(len(g[const_node].outputs) == 1)

        cond_function = cond_function.union(set(cond_constants))
        body_function = body_function.union(set(body_constants))

        downstream_cond = (
            FindAllReachableNodes(lambda x: True)
            .visit_many(g, cond_function)
            .get_result()
        )
        downstream_cond = set(downstream_cond) - cond_function
        if len(downstream_cond) > 0:
            logging.debug(
                "Disconnecting unused variables in condition function %s",
                downstream_cond,
            )
            for i in downstream_cond:
                delete_node(g, i)

        downstream_body = (
            FindAllReachableNodes(lambda x: True)
            .visit_many(g, body_function)
            .get_result()
        )
        downstream_body = set(downstream_body) - body_function
        if len(downstream_body) > 0:
            logging.debug(
                "Disconnecting unused variables in body function %s", downstream_body
            )
            for i in downstream_body:
                delete_node(g, i)

        cond_graph = {k: v for k, v in g.items() if k in cond_function}
        body_graph = {k: v for k, v in g.items() if k in body_function}
        g = {
            k: v
            for k, v in g.items()
            if k not in cond_function and k not in body_function
        }
        # localize control dependencies
        # In the main graph, reattach the control dependency to the while op
        for k, v in g.items():
            for idx in range(len(v.control_inputs)):
                if v.control_inputs[idx] not in g:
                    v.control_inputs[idx] = while_loop.name
                    while_loop.control_outputs.append(k)
            for idx in range(len(v.control_outputs)):
                if v.control_outputs[idx] not in g:
                    v.control_outputs[idx] = while_loop.name
                    while_loop.control_inputs.append(k)

        # in the cond and body graphs, drop non-local control dependencies
        # entirely
        for graph in [cond_graph, body_graph]:
            for k, v in graph.items():
                for idx in range(len(v.control_inputs) - 1, -1, -1):
                    if v.control_inputs[idx] not in graph:
                        v.control_inputs.pop(idx)

                for idx in range(len(v.control_outputs) - 1, -1, -1):
                    if v.control_outputs[idx] not in graph:
                        v.control_outputs.pop(idx)
        tfssa.functions[function_to_functionalize] = SSAFunction(g)
        tfssa.add_function(cond_body.name, SSAFunction(cond_graph))
        tfssa.add_function(body.name, SSAFunction(body_graph))
        return True


def functionalize_loops(tfssa):
    """
    Functionalize all loops in an tfssa
    """
    done = False
    while not done:
        done = True
        for f in list(tfssa.functions.keys()):
            functionalize = FunctionalizeLoops()
            ret = functionalize.functionalize_loops(tfssa, f)
            if ret:
                done = False

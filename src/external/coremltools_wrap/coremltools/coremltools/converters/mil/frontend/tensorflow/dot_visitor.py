# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from coremltools.converters.mil.mil import types


class DotVisitor(object):
    """
    Generates a dot description of a graph in dictionary form.
    """

    def __init__(self, annotation=None):
        self.result = []
        self.visited_memo = {}
        self.highlights = {}
        self.alternate_labeller = None
        self.annotation = annotation

    def labeller(self, labeller):
        self.alternate_labeller = labeller
        return self

    def highlight_nodes(self, nodeset, color="yellow"):
        for i in nodeset:
            self.highlights[i] = color
        return self

    def visit(self, graph, node, nodename_prefix=""):
        if node.name in self.visited_memo:
            return self

        # For printing datatype, breaks type
        if node.attr.get("symbolic_datatype", None) is not None:
            dtype = str(types.get_type_info(node.attr["symbolic_datatype"]))
        elif node.datatype is not None:
            dtype = str(types.get_type_info(node.datatype))
        else:
            dtype = "Unknown"

        label = ""
        if self.alternate_labeller is not None:
            label = self.alternate_labeller(node)
        else:
            if len(node.outputs) == 0:
                label = "\\n{" + node.name + "}"
            if "Placeholder" in node.op:
                label = "\\n{" + node.name + "}"
            if node.op == "while":
                label = (
                    "\\n{body: "
                    + node.attr["body_function"]
                    + " cond:"
                    + node.attr["cond_function"]
                    + "}"
                )
            if node.op == "function":
                label = "\\n{body: " + node.attr["function_name"] + "}"
            if node.op == "function_entry":
                label = "\\n{" + node.name + "}"
            label = node.op + ":" + dtype + label

        if node.name in self.highlights:
            self.result.append(
                '"'
                + nodename_prefix
                + node.name
                + '"'
                + '[label="'
                + label
                + '",fillcolor=%s,style=filled,fontcolor=%s]'
                % (
                    self.highlights[node.name],
                    "violetred" if node.attr.get(self.annotation, False) else "black",
                )
            )
        else:
            self.result.append(
                '"'
                + nodename_prefix
                + node.name
                + '"'
                + '[label="'
                + label
                + '",fontcolor=%s]'
                % ("violetred" if node.attr.get(self.annotation, False) else "black")
            )

        for i in node.inputs:
            input_name = i
            edge = (
                '"'
                + nodename_prefix
                + input_name
                + '"'
                + " -> "
                + '"'
                + nodename_prefix
                + node.name
                + '"'
            )
            innode = graph[input_name]
            self.result.append(edge)

        for i in node.control_inputs:
            input_name = i
            edge = (
                '"'
                + nodename_prefix
                + input_name
                + '"'
                + " -> "
                + '"'
                + nodename_prefix
                + node.name
                + '"'
            )
            innode = graph[input_name]
            edge = edge + " [style=dotted]"
            self.result.append(edge)

        self.visited_memo[node.name] = 1

        for i in node.inputs:
            input_name = i
            if input_name[0] == "^":
                input_name = input_name[1:]
            assert input_name in graph
            self.visit(graph, graph[input_name], nodename_prefix)
        return self

    def visit_all(self, graph, nodename_prefix=""):
        for i in graph:
            self.visit(graph, graph[i], nodename_prefix)
        return self

    def get_result(self, graphtype="digraph", graph_name="g"):
        return (
            graphtype
            + " "
            + graph_name
            + " {\n\t"
            + "\n\t".join(str(i) for i in self.result)
            + ';\n\tlabel="'
            + graph_name[8:]
            + '";\n\tfontsize=96;\n}'
        )

    def __str__(self):
        return self.get_result()

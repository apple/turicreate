# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..var import Var


def _get_input_vars(op, only_nonconst_vars=False):
    """
    Return type : List[Var]
    """
    input_vars = []
    for name, val in op.inputs.items():
        if isinstance(val, Var):
            if only_nonconst_vars:
                if val.op and val.op.op_type == "const":
                    continue
            input_vars.append(val)
        elif isinstance(val, (list, tuple)):
            for var in val:
                if not isinstance(var, Var):
                    msg = "unrecognized input type of op='{}', input='{}'"
                    raise ValueError(msg.format(op.name, name))
                if only_nonconst_vars:
                    if var.op and var.op.op_type == "const":
                        continue
                input_vars.append(var)
        else:
            msg = "unrecognized input type of op='{}', input='{}'"
            raise ValueError(msg.format(op.name, name))
    return input_vars


class DotVisitor(object):
    """
    Generates a dot description of a ssa block
    """

    def __init__(self, annotation=True):
        self.result = []
        self.visited_memo = {}
        self.highlights = {}
        self.alternate_labeller = lambda o: o.op_type + ": " + o.name
        self.annotation = annotation

    def labeller(self, labeller):
        self.alternate_labeller = labeller
        return self

    def highlight_nodes(self, nodeset, color="yellow"):
        for i in nodeset:
            self.highlights[i] = color
        return self

    def visit(self, block, op, nodename_prefix=""):
        """
        Append edges connecting parents of op to the op
        """

        if op in self.visited_memo:
            return self

        label = self.alternate_labeller(op)
        self.visited_memo[op] = 1

        if op.name in self.highlights and op.name not in [
            o.name for o in block.outputs
        ]:
            self.result.append(
                '"'
                + nodename_prefix
                + "op: "
                + op.name
                + '"'
                + '[label="'
                + label
                + '",fillcolor=%s,style=filled,fontcolor=%s]'
                % (self.highlights[op.name], "violetred")
            )
        else:
            self.result.append(
                '"'
                + nodename_prefix
                + "op: "
                + op.name
                + '"'
                + '[label="'
                + label
                + '",fontcolor=%s]' % ("violetred")
            )

        for input_var in _get_input_vars(op, only_nonconst_vars=True):
            if input_var.op is not None:
                input_name = "op: " + input_var.op.name
            else:
                input_name = input_var.name

            edge = (
                '"'
                + nodename_prefix
                + input_name
                + '"'
                + " -> "
                + '"'
                + nodename_prefix
                + "op: "
                + op.name
                + '"'
            )
            self.result.append(edge)
            if input_var.op is not None:
                self.visit(block, input_var.op, nodename_prefix)
            else:
                self.visit_input_var(input_var, nodename_prefix)

        return self

    def visit_input_var(self, var, nodename_prefix=""):
        label = "input: " + var.name

        if var.name in self.highlights:
            self.result.append(
                '"'
                + nodename_prefix
                + var.name
                + '"'
                + '[label="'
                + label
                + '",fillcolor=%s,style=filled,fontcolor=%s]'
                % (self.highlights[var.name], "violetred")
            )
        else:
            self.result.append(
                '"'
                + nodename_prefix
                + var.name
                + '"'
                + '[label="'
                + label
                + '",fontcolor=%s]' % ("violetred")
            )

    def visit_output_vars(self, block, var, nodename_prefix=""):

        label = "output: " + var.name
        if var.name in self.highlights:
            self.result.append(
                '"'
                + nodename_prefix
                + var.name
                + '"'
                + '[label="'
                + label
                + '",fillcolor=%s,style=filled,fontcolor=%s]'
                % (self.highlights[var.name], "violetred")
            )
        else:
            self.result.append(
                '"'
                + nodename_prefix
                + var.name
                + '"'
                + '[label="'
                + label
                + '",fontcolor=%s]' % ("violetred")
            )

        parent_op = var.op
        edge = (
            '"'
            + nodename_prefix
            + "op: "
            + parent_op.name
            + '"'
            + " -> "
            + '"'
            + nodename_prefix
            + var.name
            + '"'
        )
        self.result.append(edge)
        self.visit(block, parent_op, nodename_prefix=nodename_prefix)

    def visit_all(self, block, nodename_prefix=""):
        for out_var in block.outputs:
            self.visit_output_vars(block, out_var, nodename_prefix=nodename_prefix)
        for op in block.operations:
            if op.op_type != "const":
                self.visit(block, op, nodename_prefix=nodename_prefix)
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

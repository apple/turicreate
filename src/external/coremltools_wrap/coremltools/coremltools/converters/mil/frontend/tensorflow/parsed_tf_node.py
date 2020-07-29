# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from coremltools.converters.mil.mil import types
from .tfssa import ParsedNode


class ParsedTFNode(ParsedNode):
    """
    A parsed TensorFlow Node.

    name: The name of the node (str)
    op: The operation represented by the node (str)
    datatype: The type of the node. (type)
    value: The value of the node if available
    inputs: The list of nodes which are inputs to this node (list[str])
    control_inputs: The list of nodes which have to be executed before this node (list[str])
    attr: The attributes of the node
    outputs: The list of nodes which consume the result of this node (list[str])
    control_outputs: The list of nodes which have to be executed after this node (list[str])
    """

    def __init__(self, tfnode=None):
        super(ParsedTFNode, self).__init__()
        self.original_node = tfnode

        if tfnode is not None:
            from .parse import parse_attr

            self.name = tfnode.name
            if tfnode.op == "PlaceholderWithDefault":
                self.op = "Placeholder"
            else:
                self.op = tfnode.op
            self.inputs = [x for x in tfnode.input if not x.startswith("^")]
            self.control_inputs = [x[1:] for x in tfnode.input if x.startswith("^")]
            self.attr = {k: parse_attr(v) for k, v in tfnode.attr.items()}

    def parse_from_attr(self):
        if "value" in self.attr:
            self.datatype = self.attr["value"].__class__
        elif "_output_shapes" in self.attr:
            output_shapes = self.attr["_output_shapes"]
            if output_shapes[0] is not None and len(output_shapes[0]) > 0:
                if "dtype" in self.attr:
                    rettype = types.tensor(self.attr["dtype"], tuple(output_shapes[0]))
                elif "T" in self.attr:
                    rettype = types.tensor(self.attr["T"], tuple(output_shapes[0]))
                elif "Tparams" in self.attr:
                    rettype = types.tensor(
                        self.attr["Tparams"], tuple(output_shapes[0])
                    )
                else:
                    raise NotImplementedError(
                        "Op-(%s) %s not implemented\nWith attribute:"
                        + str(self.attr) % (self.op, self.name)
                    )
                self.datatype = rettype
            elif "dtype" in self.attr:
                self.datatype = self.attr["dtype"]
        elif "shape" in self.attr:
            shape = self.attr["shape"]
            assert "dtype" in self.attr
            if len(shape) == 0:
                self.datatype = self.attr["dtype"]
            else:
                self.datatype = types.tensor(self.attr["dtype"], shape)
        elif "dtype" in self.attr:
            self.datatype = self.attr["dtype"]

    def _copy_impl(self, dest):
        dest = super(ParsedTFNode, self)._copy_impl(dest)
        dest.original_node = self.original_node
        return dest

    def __copy__(self):
        return self._copy_impl(ParsedTFNode())

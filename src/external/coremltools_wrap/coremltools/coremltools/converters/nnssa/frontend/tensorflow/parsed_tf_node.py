# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ...commons import builtins
from ...nnssa import ParsedNode


class ParsedTFNode(ParsedNode):
    """
    A parsed Tensorflow Node.

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
        ParsedNode.__init__(self)
        self.original_node = tfnode
        if tfnode is not None:
            from .parse import parse_attr
            self.name = tfnode.name
            if tfnode.op == 'PlaceholderWithDefault':
                self.op = 'Placeholder'
            else:
                self.op = tfnode.op
            self.inputs = [x for x in tfnode.input if not x.startswith('^')]
            self.control_inputs = [x[1:] for x in tfnode.input if x.startswith('^')]
            self.attr = {k: parse_attr(v) for k, v in tfnode.attr.items()}

    def parse_from_attr(self):
        if 'value' in self.attr:
            self.datatype = self.attr['value'].__class__
        elif '_output_shapes' in self.attr:
            output_shapes = self.attr['_output_shapes']
            if len(output_shapes[0]) > 0:
                if 'dtype' in self.attr:
                    rettype = builtins.tensor(self.attr['dtype'], tuple(output_shapes[0]))
                elif 'T' in self.attr:
                    rettype = builtins.tensor(self.attr['T'], tuple(output_shapes[0]))
                elif 'Tparams' in self.attr:
                    rettype = builtins.tensor(self.attr['Tparams'], tuple(output_shapes[0]))
                else:
                    raise NotImplementedError(
                        "Op-(%s) %s not implemented\nWith attribute:" + str(self.attr) %
                        (self.op, self.name))
                self.datatype = rettype
            elif 'dtype' in self.attr:
                self.datatype = self.attr['dtype']
        elif 'shape' in self.attr:
            shape = self.attr['shape']
            assert ('dtype' in self.attr)
            if len(shape) == 0:
                self.datatype = self.attr['dtype']
            else:
                self.datatype = builtins.tensor(self.attr['dtype'], shape)
        elif 'dtype' in self.attr:
            self.datatype = self.attr['dtype']

    def __copy__(self):
        import copy
        ret = ParsedTFNode()
        ret.name = self.name
        ret.op = self.op
        ret.datatype = self.datatype
        ret.value = copy.deepcopy(self.value)
        ret.inputs = self.inputs[:]
        ret.control_inputs = self.control_inputs[:]
        ret.attr = {k: copy.deepcopy(v) for k, v in self.attr.items()}
        ret.outputs = self.outputs[:]
        ret.control_outputs = self.control_outputs[:]
        return ret

    def copy(self):
        return self.__copy__()

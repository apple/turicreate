#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import six
import logging
from coremltools.converters.mil.input_types import (
    InputType,
    TensorType,
    ImageType,
    RangeDim,
    _get_shaping_class,
)
from coremltools.converters.mil.input_types import Shape as InputShape
from coremltools.converters.mil.mil.var import Var
from coremltools.converters.mil.mil import get_new_symbol
from coremltools.converters.mil.mil.types.symbolic import is_symbolic

from coremltools.converters.mil.mil import types
from .basic_graph_ops import topsort, simple_topsort

from .convert_utils import convert_graph

from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.mil import Program
from coremltools.converters.mil.mil import Function
from .ssa_passes.tf_passes import tensorflow_passes
from coremltools.converters._profile_utils import _profile


# TranscriptionContext maintains a map of tf_node.name --> ssa_var available
# to the current TF --> tfssa transcription.
class TranscriptionContext:
    def __init__(self, name=None):
        self.name = name if name is not None else ""
        self.context = {}
        self.graphs = {}

        # TF loops are represented as functions, so nested loops becomes
        # stacked functions. Stacked functions are translated to nested
        # blocks in Program, like
        #
        # while_loop(loop_vars=(%a, %b))
        #  cond_block1(%a.x, %b.x) {
        #    ...some ops
        #  } -> (%bool_var1)
        #  body_block1(%a.x, %b.x) {
        #    %ret_axx = while_loop(loop_vars=(%a.x,))
        #      cond_block2(%a.x.x) {
        #        ...some ops
        #      } -> (%bool_var2)
        #      body_block2(%a.x.x) {
        #       ...some ops
        #      } -> (%new_a.x.x)
        #    } -> (%ret_axx)
        #    ....some ops using %ret_a
        #  } -> (%ret_ax, %ret_bx)
        #
        # During the translation of cond_block2, we'd have func_input_stack
        #
        # (%a.x.x,)
        # (%a.x, %b.x)
        #
        # where [%a.x.x] would be unstacked once cond_block2 is done.
        self.func_input_stack = []  # list of tuple[Var]

    def add(self, tf_name, ssa_vars, is_new_var=True):
        """
        ssa_vars: list[Var] / tuple[Var] (multiple outputs) or
        Var (single_output)
        is_new_var: True if ssa_vars are newly created for tf_name.
        """
        if tf_name in self.context:
            # Overriding allow us to translate while_loop body twice (which is
            # needed to figure out shapes changes during iterates)
            msg = "TF var %s is added again. Overriding previous value"
            logging.info(msg % tf_name)
        if is_new_var and isinstance(ssa_vars, Var) and tf_name != ssa_vars.name:
            msg = (
                "MIL op's name ({}) does not match TensorFlow's node name ({})."
                " Warning: Node added to context must have the same name as the name passed to context."
            )
            raise ValueError(msg.format(tf_name, ssa_vars.name))
        self.context[tf_name] = ssa_vars

    def add_graph(self, graph_name, graph):
        self.graphs[graph_name] = graph

    def get_graph(self, graph_name):
        if graph_name not in self.graphs:
            msg = "Graph '{}' not found in: {}"
            raise KeyError(msg.format(graph_name, list(self.graphs.keys())))
        return self.graphs[graph_name]

    def stack_func_inputs(self, inputs):
        self.func_input_stack.append(inputs)

    def unstack_func_inputs(self):
        if len(self.func_input_stack) == 0:
            raise ValueError("No func input available")
        self.func_input_stack.pop()

    def get_func_inputs(self):
        if len(self.func_input_stack) == 0:
            raise ValueError("No func input available")
        return self.func_input_stack[-1]

    def __getitem__(self, tf_name):
        if tf_name not in self.context:
            msg = "TF var {} not found in context {}"
            raise KeyError(msg.format(tf_name, self.name))
        return self.context[tf_name]

    def __contains__(self, tf_name):
        return tf_name in self.context


class TFConverter:
    def __init__(self, tfssa, inputs=None, outputs=None, **kwargs):
        """
        tfssa: TensorFlow IR.
        inputs: list of TensorType or ImageType, optional, defaults to None.
        outputs: list of str or str, optional, defaults to None.
            A list of names of the output nodes or a str for single output name.
            If None, the converter will try to extract the output information from
            TensorFlow model.
        """
        self.tfssa = tfssa
        self.global_type = {}
        self.inputs = None

        main_func = tfssa.functions["main"]
        graph = main_func.graph

        # Filter the inputs to only Placeholder names
        tf_placeholder_names = [n for n in graph if graph[n].op == "Placeholder"]
        placeholder_names = []
        if inputs is not None:
            # Check inputs format
            if not isinstance(inputs, (list, tuple)):
                raise ValueError(
                    "Type of inputs should be list or tuple, got {} instead.".format(
                        type(inputs)
                    )
                )
            if not all([isinstance(i, InputType) for i in inputs]):
                raise ValueError(
                    "Type of inputs should be list or tuple of TensorType or ImageType, got {} instead.".format(
                        [type(i) for i in inputs]
                    )
                )

            # Special case: if there's only 1 input and 1 placeholder, we match them.
            if len(tf_placeholder_names) == 1 and len(inputs) == 1:
                if inputs[0].name is None:
                    inputs[0].name = tf_placeholder_names[0]

            # filter out those inputs which is not in tf_placeholder_names
            inputs = [x for x in inputs if x.name in tf_placeholder_names]

            # We fill in shapes for user-specified input that doesn't have shape
            for inp in inputs:
                # Check inputs existence
                if inp.name is None:
                    raise ValueError(
                        "Unable to infer input's name or input name was not provided"
                    )
                if inp.name not in tf_placeholder_names:
                    raise ValueError(
                        "Input ({}) provided is not found in given tensorflow graph. Placeholders in graph are: {}".format(
                            inp.name, tf_placeholder_names
                        )
                    )
                if inp.shape is None:
                    if graph[inp.name].attr.get("_output_shapes", None) is not None:
                        shape = graph[inp.name].attr["_output_shapes"][0]
                        if shape is None:
                            # Scalar is given as None
                            shape = []
                    elif graph[inp.name].attr.get("shape", None) is not None:
                        shape = graph[inp.name].attr["shape"]
                    else:
                        raise ValueError(
                            "Can't extract shape from attribute of ({})".format(
                                inp.name
                            )
                        )
                    inp.shape = _get_shaping_class(shape)

            # Extract placeholders that users didn't specify.
            user_input_names = [inp.name for inp in inputs]
            for name in tf_placeholder_names:
                if name not in user_input_names:
                    placeholder_names.append(name)
        else:
            inputs = []
            placeholder_names = tf_placeholder_names

        placeholder_inputs = {}
        for inp in main_func.inputs:
            if inp not in placeholder_names:
                continue
            if graph[inp].attr.get("_output_shapes", None) is not None:
                placeholder_inputs.update({inp: graph[inp].attr["_output_shapes"][0]})
            elif graph[inp].attr.get("shape", None) is not None:
                placeholder_inputs.update({inp: graph[inp].attr["shape"]})
            else:
                raise ValueError("Can't find input shape for ({})".format(inp))

        if len(placeholder_inputs) > 0:
            logging.info(
                "Adding Input not specified by users: '{}'".format(placeholder_inputs)
            )

        for k, v in placeholder_inputs.items():
            inputs.append(TensorType(name=k, shape=v))
        for idx, inp in enumerate(inputs):
            # We set the default image format in TF as NHWC, since NHWC is used
            # for TF unless GPU is specified as device.
            if isinstance(inp, ImageType) and inputs[idx].channel_first is None:
                inputs[idx].channel_first = False
        self.inputs = tuple(inputs)

        for inputtype in self.inputs:
            if not isinstance(inputtype.shape, InputShape):
                continue
            if any([isinstance(s, RangeDim) for s in inputtype.shape.shape]):
                continue
            node = graph[inputtype.name]
            shape = [-1 if is_symbolic(s) else s for s in inputtype.shape.shape]
            node.attr["_output_shapes"] = [shape]  # list of length 1

        # infer outputs if not provided
        self._validate_outputs(tfssa, outputs)
        outputs = main_func.outputs if outputs is None else outputs
        outputs = outputs if isinstance(outputs, (tuple, list)) else [outputs]
        outputs = [x if isinstance(x, six.string_types) else x.name for x in outputs]
        self.outputs = outputs

        # We would like a stack so that we run conversion sequentially.
        self.graph_stack = self._get_stack(tfssa, root="main")
        self.context = TranscriptionContext()
        self.tensorflow_passes = tensorflow_passes

    def _get_stack(self, tfssa, root="main"):
        # We're trying to get a order of how to loop through the graphs.
        # This is NOT necessarily a DAG.
        dep = {x: [] for x in tfssa.functions}
        for fname in tfssa.functions:
            for node in tfssa.functions[fname].graph.values():
                func_x, func_y = None, None

                if node.op == "while":
                    func_x = node.attr["body_function"]
                    func_y = node.attr["cond_function"]

                if func_x and fname not in dep[func_x]:
                    dep[func_x].append(fname)
                if func_y and fname not in dep[func_y]:
                    dep[func_y].append(fname)

        assert len(dep[root]) == 0
        graph_stack = simple_topsort(dep)

        return graph_stack

    @staticmethod
    def _get_tensor_name(tensor):
        ret = None
        if isinstance(tensor, six.string_types):
            ret = tensor
        else:
            ret = tensor.name
        return ret.split(":")[0]

    @staticmethod
    def _create_placeholder(node):
        node.parse_from_attr()
        shape = []
        dtype = node.attr["dtype"]
        if types.is_tensor(node.datatype):
            shape = node.datatype.get_shape()
            shape = tuple(get_new_symbol() if s is None or s < 0 else s for s in shape)
        return mb.placeholder(shape, dtype=dtype)

    def _validate_outputs(self, tfssa, outputs):
        if outputs is None:
            return
        outputs = outputs if isinstance(outputs, (tuple, list)) else [outputs]
        output_nodes = []
        for f in tfssa.functions.values():
            output_nodes += list(f.outputs)
        all_nodes = []
        for f in tfssa.functions.values():
            all_nodes += list(f.graph.keys())
        for n in outputs:
            if self._get_tensor_name(n) not in output_nodes + all_nodes:
                raise KeyError('Output node name "{}" does exist.'.format(n))

    def check_placeholder_output(self, prog):
        """
        Handle the cases where placeholder is output.
        There is a case where the program is like
            main(%Placeholder: (5,fp32)) {
                block3() {
                } -> (%Placeholder)
            }
        But self.outputs = ["Placeholder:0"]
        We need to change the block output to Placeholder:0 by inserting an identity
        """
        block = prog["main"]
        input_name = [x.name for x in list(block.inputs.values())]
        output_name = [x.name for x in block.outputs]
        placeholder_output_name = [
            x for x in output_name if x in input_name and x not in self.outputs
        ]
        with block:
            new_outputs = [
                x for x in block.outputs if x.name not in placeholder_output_name
            ]
            for name in placeholder_output_name:
                x = block.inputs[name]
                x = mb.identity(x=x, name=name + ":0")
                new_outputs.append(x)
            block.set_outputs(new_outputs)

    def convert_main_graph(self, prog, graph):
        func_inputs = {}
        for input_type in self.inputs:
            node = graph[input_type.name]
            func_inputs[input_type.name] = TFConverter._create_placeholder(node)
        prog.set_main_input_types(self.inputs)

        with Function(func_inputs) as ssa_func:
            # Get the input Var
            for name in func_inputs.keys():
                self.context.add(name, ssa_func.inputs[name])
            outputs = convert_graph(self.context, graph, self.outputs)
            ssa_func.set_outputs(outputs)
            prog.add_function("main", ssa_func)

        # Rename outputs to TF's name. This is needed when the last op doesn't
        # generate a new Var (e.g., get_tuple, Identity etc.), and thus the
        # last Var would have a different name than the last TF op's name.
        #
        # Example:
        #
        # TF code:
        #    x = tf.placeholder(tf.float32, shape=(1,))
        #    y = tf.placeholder(tf.float32, shape=(1,))
        #    c = lambda i, j: \
        #            tf.less(tf.math.reduce_mean(i), tf.math.reduce_mean(j))
        #    b = lambda i, j: (tf.add(i, 1), j)
        #    res = tf.while_loop(c, b, [x, y])
        #
        # Resulting nodes (excluding the nodes in while loop cond & body):
        #
        # node name: Placeholder op type: Placeholder inputs: []
        # node name: Placeholder_1 op type: Placeholder inputs: []
        # node name: make_input_0 op type: make_tuple inputs: ['Placeholder',
        #         'Placeholder_1']
        # node name: while_0 op type: while inputs: ['make_input_0']
        # node name: while/Exit op type: get_tuple inputs: ['while_0']
        # node name: while/Exit_1 op type: get_tuple inputs: ['while_0']
        #
        # Observe that return node `while/Exit` is an output from get_tuple,
        # which in our translation simply unpack a python tuple of Vars
        # ('while_0:0', 'while_0:1') returned from while_0 SSA op. We need to
        # rename `while_0:0` to `while/Exit` in order for users to find the
        # output.
        # Note: only rename the output if the output is not Placeholder.

        input_names = [x.name for x in self.inputs]
        for v_o, out_name in zip(prog["main"].outputs, self.outputs):
            if v_o.name != out_name and v_o.name not in input_names:
                logging.info(
                    "Renaming output var: '{}' -> '{}'".format(v_o.name, out_name)
                )
                v_o.name = out_name
        self.check_placeholder_output(prog)

    @_profile
    def convert(self):
        prog = Program()
        if len(self.graph_stack) == 0:
            raise ValueError("At least one TF function must be present")
        if self.graph_stack[0] != "main":
            msg = "TF root graph must be named 'main'. Got {}"
            raise ValueError(msg.format(self.graph_stack[0]))
        graph = self.tfssa.functions["main"].graph
        for g_name in self.graph_stack[1:]:
            self.context.add_graph(g_name, self.tfssa.functions[g_name].graph)
        self.convert_main_graph(prog, graph)

        # Apply TF frontend passes on Program. These passes are different
        # from passes applied to tfssa.
        self.tensorflow_passes(prog)

        return prog

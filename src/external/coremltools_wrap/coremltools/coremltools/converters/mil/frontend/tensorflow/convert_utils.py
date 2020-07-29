#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import logging
from .basic_graph_ops import topsort
from coremltools.converters.mil.mil.types.symbolic import is_symbolic, any_variadic
from coremltools.converters.mil.mil import types
from .tf_op_registry import _TF_OPS_REGISTRY
from coremltools.converters.mil.mil.var import ListVar
from collections import defaultdict
from tqdm import tqdm as _tqdm


def compatible_shapes(tf_shape, inf_shape):
    def compare_elem(dt, ds):
        if dt is None or dt < 0:
            return True
        elif dt == ds:
            return True
        else:
            return False

    if tf_shape is None or any_variadic(inf_shape):
        return True
    else:
        return all(compare_elem(dt, ds) for dt, ds in zip(tf_shape, inf_shape))


def check_output_shapes(x, node):
    """
    x: list[Var] or tuple[Var]
    node: ParsedTFNode
    """
    if isinstance(x, ListVar):
        # No check on list.
        return
    if not isinstance(x, (list, tuple)):
        x = [x]
    tf_shapes = node.attr.get("_output_shapes", None)
    if tf_shapes is None:
        return
    inf_shapes = []
    for y in x:
        if y is None:
            msg = "TF convert returns None type in TF node {}"
            raise TypeError(msg.format(node.name))
        if types.is_tensor(y.sym_type):
            inf_shapes.append(list(y.shape))
        elif types.is_scalar(y.sym_type):
            inf_shapes.append([])
        else:
            msg = "Output type {} not understood"
            raise ValueError(msg.format(y))

    for t, s in zip(tf_shapes, inf_shapes):
        if not compatible_shapes(t, s):
            msg = (
                "Op {} ({}) type inference ({}) and TF output shape " + "({}) mismatch"
            )
            raise ValueError(msg.format(node.name, node.op, s, t))


def connect_global_initializer(graph):
    # In TF1, variable initialization (from frozen graph) is done by a
    # DAG in main function that is disconnected from the rest of the main
    # function. For example:
    #
    # Initialization DAG (disconnected from Main DAG):
    #   Const -> set_global(variable='v1')
    #
    # Main DAG:
    #   Placeholder               ---
    #                               |
    #   get_global(variable='v1') ----> some_output
    #
    # (Note that in this example there's no loop or other function.)
    #
    # If the variable does not cross block boundary, we can always represent
    # `get_global` by the input to `set_global`, which may or may not be
    # Const, following the control dependency.
    #
    # Note that this is incorrect if global variable crosses, say,
    # while_loop block boundary, which needs a more complex resource inference
    # to support and is not supported in this function.
    #
    # Due to the lack of control depeendency between thhe two DAG, we could be
    # converting `set_global` after `get_global`, which makes it impossible to
    # perform eager type inference, as type information (e.g., tensor shape)
    # is only provided by `set_global` (whether setting it to a const or a
    # non-const).
    #
    # Here we remedy the simpler case: when `set_global` takes in a Const,
    # we assume it's initialization and thus must
    # run before get_global, i.e. all get_global(variable='v1') must be a
    # control_output of set_global(variable='v1') where set_global's input is
    # Const (with and control_inputs set symmetrically). Note that multiple
    # `get_global(variable='v1')` might have dependences among themselves, but
    # they should all take the constant `set_global(variable='v1')` as control
    # dependency.

    # Phase 1: Collect get_global nodes for each variable.
    # variable name to list[ParsedTFNode]
    var_to_get_global_nodes = defaultdict(list)
    for node in graph.values():
        if node.op == "get_global":
            variable_name = node.attr["variable"]
            var_to_get_global_nodes[variable_name].append(node)

    # Phase 2: Find set_global with compile time values
    for node_name, node in graph.items():
        if node.op != "set_global":
            continue
        input_name = node.inputs[0]
        input_node = graph[input_name]
        if input_node.op != "Const":
            continue
        variable_name = node.attr["variable"]
        for get_node in var_to_get_global_nodes[variable_name]:
            logging.info(
                "add {} as control inputs of {}".format(node_name, get_node.name)
            )
            get_node.control_inputs.append(node_name)
            node.control_outputs.append(get_node.name)


def convert_graph(context, graph, outputs=None):
    """
    Construct Core ML ops corresponding to `graph`.

    Inputs:

    - context (TranscriptContext)

    - graph (dict of str -> ParsedTFNode): op name --> ParsedTFNode

    - outputs (list[str]): List of output names. If outputs is None, the last
      node graph (after topsort) must have op type return.

    Returns:

    list[Var]: the output Vars of the constructed Block.
    """
    connect_global_initializer(graph)
    nodes = topsort(graph)

    if outputs is None:
        # infer outputs from return
        last_node = graph[nodes[-1]]
        if last_node.op != "return":
            msg = "Expect the last node in graph to be 'return'; Got {}"
            raise ValueError(msg.format(last_node.op))
        second_last_node = graph[last_node.inputs[0]]
        if second_last_node.op == "make_tuple":
            outputs = second_last_node.inputs
        else:
            # single output function
            outputs = second_last_node.name

    # Translate the non-placeholder ops.
    num_nodes = len(nodes)
    for i, node_name in enumerate(
        _tqdm(nodes, desc="Converting Frontend ==> MIL Ops", unit=" ops")
    ):
        node = graph[node_name]
        if node.op == "return":
            continue
        logging.info(
            "[{}/{}] Converting {} op '{}'".format(i + 1, num_nodes, node.op, node.name)
        )

        if node.op == "NoOp":
            continue
        _add_op = _TF_OPS_REGISTRY.get(node.op, None)
        if _add_op is None:
            msg = "Conversion for TF op '{0}' not implemented.\n \n{1}".format(
                node.op, node.original_node
            )
            raise NotImplementedError(msg)
        _add_op(context, node)

        if len(node.outputs) > 0:
            # set_global / get_global / NoOp has no direct consumer / outputs
            x = context[node.name]
            check_output_shapes(x, node)

    output_is_list = isinstance(outputs, (tuple, list))
    if not output_is_list:
        outputs = [outputs]

    output_vars = []
    for output in outputs:
        x = context[output.split(":")[0]]
        if isinstance(x, (tuple, list)):
            idx = int(output.split(":")[1])
            output_vars.append(x[idx])
        else:
            output_vars.append(x)

    return output_vars if output_is_list else output_vars[0]

from collections import OrderedDict
import logging as _logging

from .internal_graph import *


def transform_inplace_ops(graph, name_remap_dict=None):
    # TODO: one recent 1P model has included the op `copy_`. This is another
    # in-place op that should be fixed by this pass.
    # See rdar://64267506

    # As we modify ops, we'll need to remap symbols.
    if name_remap_dict is None:
        name_remap_dict = {}

    for node in graph.nodes:
        for k, v in name_remap_dict.items():
            node.replace_name(k, v)

        if node.kind == "append":
            if isinstance(node.parent, InternalTorchIRGraph):
                # If append appears in a graph (outer block), replace
                # subsequent uses of its input symbol with its output symbol.
                name_remap_dict[node.inputs[0]] = node.outputs[0]
            elif node.parent.parent.kind == "loop":
                # If append appears in a loop block, add its inputs to the block
                # inputs and loop inputs, and its outputs to the block outputs
                # and loop outputs.

                # This is the global input to append. We need to add it to the
                # loop's input list, and replace any uses after the node with
                # @global_output below.
                global_input = node.inputs[0]
                # This will be the name of the input to append within the
                # block. We need to add it to the block inputs.
                local_input = node.parent.parent.name + ".0"
                # This is the output of append. We need to add it to the list
                # of block outputs.
                local_output = node.outputs[0]
                # This is the name of the new output from the loop. It should
                # replace any uses of @global_input after the loop op.
                global_output = local_output + ".out"
                name_remap_dict[global_input] = global_output

                node.parent.parent.inputs.append(global_input)
                node.parent.inputs.append(local_input)
                node.replace_name(global_input, local_input)
                node.parent.outputs.append(local_output)
                node.parent.parent.outputs.append(global_output)
                node.parent.parent.name = node.parent.parent.outputs[0]
            elif node.parent.parent.kind == "if":
                # If append appears in an if/else block, add its outputs to the
                # block outputs and loop outputs.
                # Note that we can't assume the append appears in both blocks.
                raise NotImplementedError(
                    "inplace_ops pass doesn't yet support append op inside conditional"
                )

        for block in node.blocks:
            transform_inplace_ops(block, name_remap_dict)

    # Replace names in graph outputs
    for k, v in name_remap_dict.items():
        try:
            idx = graph.outputs.index(k)
        except ValueError:
            pass
        else:
            graph.outputs[idx] = v


def flatten_graph_input_values(graph):
    """ CoreML can't handle nested iterables of tensors, so we flatten the
        inputs of any graph that expects them.
    """
    new_graph_inputs = graph.inputs
    all_new_nodes = []
    changed = True
    notified = False

    while changed:
        old_graph_inputs = new_graph_inputs
        new_graph_inputs = OrderedDict()
        new_nodes = []
        changed = False
        for _input_name, _input_val in old_graph_inputs.items():
            if isinstance(_input_val, (tuple, list)):
                changed = True
                if not notified:
                    notified = True
                    _logging.warning(
                        "Tuple detected at graph input. This will be flattened in the converted model."
                    )
                # If this input to the graph is a tuple, we want to replace it
                # with a flattened version and add an op to construct the tuple.
                node_inputs = []
                for idx, item in enumerate(_input_val):
                    name = _input_name + "_{}".format(idx)
                    new_graph_inputs[name] = item
                    node_inputs.append(name)
                new_nodes.append(
                    InternalTorchIRNode(
                        inputs=node_inputs,
                        outputs=[_input_name],
                        kind="tupleconstruct",
                    )
                )
            else:
                # This input isn't a tuple, keep it as is.
                new_graph_inputs[_input_name] = _input_val
        all_new_nodes = new_nodes + all_new_nodes
    graph.inputs = new_graph_inputs
    graph.nodes = all_new_nodes + graph.nodes


def flatten_graph_output_values(graph):
    """ CoreML can't handle nested iterables of tensors, so we flatten the
        outputs of any graph that produces them.
    """
    node_names = [node.name for node in graph.nodes]
    new_graph_outputs = graph.outputs
    changed = True
    notified = False

    while changed:
        old_graph_outputs = new_graph_outputs
        new_graph_outputs = []
        changed = False
        for outp in old_graph_outputs:
            # Find the node that generates this output var.
            # It is possible to not find the output var in the list of node
            # names since nodes are named after their first output. In that
            # case, it means the output var comes from a node that returns
            # multiple outputs, which means that node cannot be a construct op.
            try:
                node_idx = node_names.index(outp)
            except:
                # @outp doesn't come from a construct op
                new_graph_outputs.append(outp)
                continue
            if graph.nodes[node_idx].kind in [
                "tupleconstruct",
                "listconstruct",
            ]:
                # Since this output came from a construct op, we can replace it
                # with the inputs to the op.
                new_graph_outputs.extend(graph.nodes[node_idx].inputs)
                changed = True
                if not notified:
                    notified = True
                    _logging.warning(
                        "Tuple detected at graph output. This will be flattened in the converted model."
                    )
            else:
                new_graph_outputs.append(outp)
    # Note: if we flattened outputs, there are likely to be construct ops
    # that are no longer needed. These will be removed in a later DCE pass.
    graph.outputs = new_graph_outputs

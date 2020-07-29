from __future__ import absolute_import as _
from __future__ import division as _
from __future__ import print_function as _
from __future__ import unicode_literals as _

from typing import Sequence, Text, Dict, List, Tuple
import numpy as np

from onnx import TensorProto

from ._graph import Graph, Node


def _get_fully_defined_shape(shape, blob_name, graph):
    if not np.any(shape == -1):
        return shape
    if blob_name not in graph.shape_dict:
        return shape
    else:
        return graph.shape_dict[blob_name]


def _remove_single_input_output_node(node):
    for child in node.children:
        for i, child_input in enumerate(child.inputs):
            if child_input == node.outputs[0]:
                # Pass input to child
                child.inputs[i] = node.inputs[0]
                # If input tensor is known, pass down the input tensor value
                if node.inputs[0] in node.input_tensors:
                    child.input_tensors[node.inputs[0]] = node.input_tensors[
                        node.inputs[0]
                    ]
                # Remove link as a parent from child node
                child.parents.remove(node)
                # Link current nodes parent and current child
                for parent in node.parents:
                    child.parents.append(parent)
                    parent.children.append(child)
                break

    for parent in node.parents:
        parent.children.remove(node)


class NodesFuser(object):
    """
    An abstract helper for merging nodes
    """

    def __init__(
        self, num_nodes,  # type: int
    ):
        # type: (...) -> None
        assert num_nodes >= 2, "Algorithm only works if fusing multiple nodes"
        self.num_nodes = num_nodes

    def __call__(self, graph):  # type: (Graph) -> Graph
        nodes = graph.nodes
        merged_nodes = {}
        for node in nodes:
            nodes_window = []  # type: List[Node]
            n = node
            for _ in range(self.num_nodes - 1):
                if len(n.parents) != 1:
                    # We're only fusing nodes with single parents
                    break
                p = n.get_only_parent()
                if len(p.children) != 1:
                    # We can only fuse a node if its parent's
                    # value isn't used by any other node.
                    break
                nodes_window.insert(0, n)
                n = p
            if len(nodes_window) > 0:
                # add parent of chained nodes
                first = nodes_window[0]
                p = first.get_only_parent()
                if len(p.children) == 1:
                    nodes_window.insert(0, p)
            if len(nodes_window) != self.num_nodes:
                continue
            if not self.is_eligible(graph, nodes_window):
                continue
            merged = self.merge(graph, nodes_window)
            first, last = nodes_window[0], nodes_window[-1]
            for parent in first.parents:
                parent.children.remove(first)
                if merged[0] not in parent.children:
                    parent.add_child(merged[0])
            for child in last.children:
                child.parents.remove(last)
                if merged[-1] not in child.parents:
                    child.add_parent(merged[-1])
            for n in nodes_window:
                merged_nodes[n.name] = merged

        transformed_nodes = []
        added_merged = []  # type: List[Node]
        for node in nodes:
            if node.name in merged_nodes:
                merged = merged_nodes[node.name]
                if merged[0] not in added_merged:
                    for n in merged:
                        transformed_nodes.append(n)
                    added_merged.append(merged[0])
            else:
                transformed_nodes.append(node)
        return graph.create_graph(nodes=transformed_nodes)

    def is_eligible(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> bool
        """Returns true if this subset of nodes is eligible for fusion."""
        raise NotImplementedError("Must be implemented by subclass.")

    def merge(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> Sequence[Node]
        """Merge nodes"""
        nodes[0].outputs = nodes[-1].outputs
        return [nodes[0]]


class ConvAddFuser(NodesFuser):
    """
    Fuses Add layer into parent convolution layer.
    """

    def __init__(self):  # type: () -> None
        super(ConvAddFuser, self).__init__(2)

    def is_eligible(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> bool
        parent, child = nodes[0], nodes[1]
        if parent.op_type != "Conv":
            return False
        if child.op_type != "Add":
            return False
        if "broadcast" not in child.attrs:
            return False
        if "axis" not in child.attrs:
            return False
        if parent.inputs[1] not in parent.input_tensors:
            return False
        if len(parent.inputs) > 2 and parent.inputs[2] not in parent.input_tensors:
            return False
        if child.inputs[1] not in child.input_tensors:
            return False

        broadcast = child.attrs["broadcast"]
        if broadcast != 1:
            return False

        axis = child.attrs["axis"]
        if axis != 1:
            return False

        return True

    def merge(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> Sequence[Node]
        parent, child = nodes[0], nodes[1]
        output_channels = parent.input_tensors[parent.inputs[1]].shape[0]
        if len(parent.inputs) > 2:
            bias_input_name = parent.inputs[2]
            bias = parent.input_tensors[bias_input_name]
        else:
            bias_input_name = "{}_bias".format(parent.name,)
            parent.inputs.append(bias_input_name)
            bias = np.zeros((output_channels,), dtype=np.float32)
            parent.input_tensors[bias_input_name] = bias
        bias = bias + child.input_tensors[child.inputs[1]]
        parent.input_tensors[bias_input_name] = bias
        parent.outputs = child.outputs
        parent.children.remove(child)
        child.parents.remove(parent)
        return [parent]


class BNBroadcastedMulFuser(NodesFuser):
    """
    Fuses Mul into BatchNorm
    """

    def __init__(self):  # type: () -> None
        super(BNBroadcastedMulFuser, self).__init__(2)

    def is_eligible(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> bool
        parent, child = nodes[0], nodes[1]
        if parent.op_type != "BatchNormalization":
            return False
        if child.op_type != "Mul":
            return False
        if len(child.inputs) != 2:
            return False
        if child.inputs[1] not in child.input_tensors:
            return False
        t = child.input_tensors[child.inputs[1]]
        if len(np.squeeze(t).shape) != 1:
            return False
        if parent.inputs[1] not in parent.input_tensors:
            return False
        if parent.inputs[2] not in parent.input_tensors:
            return False
        return True

    def merge(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> Sequence[Node]
        parent, child = nodes[0], nodes[1]
        weight = parent.input_tensors[parent.inputs[1]]
        bias = parent.input_tensors[parent.inputs[2]]
        W = np.squeeze(child.input_tensors[child.inputs[1]])
        parent.input_tensors[parent.inputs[1]] = np.multiply(weight, W)
        parent.input_tensors[parent.inputs[2]] = np.multiply(bias, W)
        parent.outputs = child.outputs
        parent.children.remove(child)
        child.parents.remove(parent)
        return [parent]


class BNBroadcastedAddFuser(NodesFuser):
    """
    Fuses Add into BatchNorm
    """

    def __init__(self):  # type: () -> None
        super(BNBroadcastedAddFuser, self).__init__(2)

    def is_eligible(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> bool
        parent, child = nodes[0], nodes[1]
        if parent.op_type != "BatchNormalization":
            return False
        if child.op_type != "Add":
            return False
        if len(child.inputs) != 2:
            return False
        if child.inputs[1] not in child.input_tensors:
            return False
        t = child.input_tensors[child.inputs[1]]
        if len(np.squeeze(t).shape) != 1:
            return False
        if parent.inputs[1] not in parent.input_tensors:
            return False
        if parent.inputs[2] not in parent.input_tensors:
            return False
        return True

    def merge(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> Sequence[Node]
        parent, child = nodes[0], nodes[1]
        bias = parent.input_tensors[parent.inputs[2]]
        b = np.squeeze(child.input_tensors[child.inputs[1]])
        parent.input_tensors[parent.inputs[2]] = bias + b
        parent.outputs = child.outputs
        parent.children.remove(child)
        child.parents.remove(parent)
        return [parent]


class DropoutRemover(NodesFuser):
    """
    Removes Dropout layer
    """

    def __init__(self):  # type: () -> None
        super(DropoutRemover, self).__init__(2)

    def is_eligible(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> bool
        child = nodes[1]
        return child.op_type == "Dropout"

    def merge(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> Sequence[Node]
        parent, child = nodes[0], nodes[1]
        parent.children.remove(child)
        child.parents.remove(parent)
        parent.outputs = [child.outputs[0]]
        return [parent]


class ReshapeInitTensorFuser(object):
    """
    Fuses Reshape operator if it is used only to reshape blob in
    graph initializer. We can reshape here instead of runtime.
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        nodes = graph.nodes
        removed = []
        for node in nodes:
            if node.op_type != "Reshape":
                continue
            if not (len(node.input_tensors) == 2 or len(node.input_tensors) == 1):
                continue
            tensor_name = node.inputs[0]
            if tensor_name not in node.input_tensors:
                continue
            if len(node.inputs) > 1:
                shape_name = node.inputs[1]
                if shape_name not in node.input_tensors:
                    continue
            is_non_constant_parent = False
            if len(node.parents) > 0:
                for parent in node.parents:
                    if parent.op_type != "Constant":
                        is_non_constant_parent = True
                        break
            if is_non_constant_parent:
                continue

            removed.append(node)
            output_name = node.outputs[0]

            tensor = node.input_tensors[tensor_name]
            if "shape" in node.attrs:
                shape = tuple(node.attrs["shape"])
            else:
                shape = node.input_tensors[shape_name]  # type: ignore

            # ONNX spec supports setting dimension to '0', in which case
            # it should be taken from old dimension.
            # This isn't supported in numpy, so don't transform.
            # TODO Should we support this case?
            if any([s == 0 for s in shape]):
                continue

            reshaped_tensor = tensor.reshape(shape.astype(int))

            for child in node.children:
                child.parents.remove(node)
                child.input_tensors[output_name] = reshaped_tensor

        transformed_nodes = [node for node in nodes if node not in removed]
        return graph.create_graph(nodes=transformed_nodes)


class OutputRenamer(object):
    """
    Rename outputs according to mapping
    """

    def __init__(
        self, mapping,  # type: Dict[Text, Text]
    ):
        # type: (...) -> None
        self.mapping = mapping

    def __call__(self, graph):  # type: (Graph) -> Graph
        mapping = self.mapping.copy()
        nodes = graph.nodes
        for node in nodes:
            for i in range(len(node.outputs)):
                output = node.outputs[i]
                if output not in mapping:
                    continue
                node.outputs[i] = mapping[output]
                for child in node.children:
                    for j in range(len(child.inputs)):
                        input_ = child.inputs[j]
                        if input_ != output:
                            continue
                        child.inputs[j] = mapping[output]
                del mapping[output]
                if len(mapping) == 0:
                    break
        return graph


class ReshapeTransposeReshape_pattern1(NodesFuser):
    """
    Detects certain types of patterns of "reshape-> (rank 6) -> transpose (rank 6) -> reshape (rank 4)" that can be converted
    """

    def __init__(self):  # type: () -> None
        super(ReshapeTransposeReshape_pattern1, self).__init__(3)
        self.num_added = 0

    def is_eligible(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> bool
        if not (
            nodes[0].op_type == "Reshape"
            and nodes[1].op_type == "Transpose"
            and nodes[2].op_type == "Reshape"
        ):
            return False
        if len(nodes[0].inputs) == 1 or len(nodes[2].inputs) == 1:
            return False  # it's an old version of onnx Reshape op that had shape as an attribute
        if nodes[0].inputs[1] not in nodes[0].input_tensors:
            return False
        if nodes[2].inputs[1] not in nodes[2].input_tensors:
            return False

        shape_1 = nodes[0].input_tensors[nodes[0].inputs[1]]
        shape_final = nodes[2].input_tensors[nodes[2].inputs[1]]

        shape_1 = _get_fully_defined_shape(shape_1, nodes[0].outputs[0], graph)
        shape_final = _get_fully_defined_shape(shape_final, nodes[2].outputs[0], graph)

        if len(shape_1) != 6 or shape_1[0] != 1 or len(shape_final) != 4:
            return False

        # check if coreml can convert this sequence using 1 transpose layer
        perm = nodes[1].attrs.get("perm", [])
        if len(perm) != 6:
            return False
        if perm[0] != 0:
            return False

        consecutive_indices = False
        perm = perm[1:]
        for i in range(1, 5):
            if perm[i] - perm[i - 1] == 1:
                consecutive_indices = True
                break

        if not consecutive_indices:
            return False

        return True

    def get_unique_edge_name(self, graph, name):  # type: (Graph, Text) -> Text
        self.num_added += 1
        return graph.get_unique_edge_name(name + "_" + str(self.num_added))

    def merge(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> Sequence[Node]
        """
        In general, CoreML Reshape and Transpose layers don't support tensors with more
        than 4 dimensions. However, certain patterns in onnx like
            "reshape-> (rank 6) -> transpose (rank 6) -> reshape (rank 4)"
        can be translated to CoreML as (i.e. without going to rank 6)
            "reshape-> (rank 4) -> transpose (rank 4) -> reshape (rank 4)"
        """
        reshape_1 = nodes[0]
        transpose_1 = nodes[1]
        final_reshape = nodes[2]

        shape_1 = reshape_1.input_tensors[reshape_1.inputs[1]]
        shape_1 = _get_fully_defined_shape(shape_1, nodes[0].outputs[0], graph)
        shape_1 = shape_1[1:]
        perm = nodes[1].attrs.get("perm", [])
        perm = perm[1:]
        perm = [x - 1 for x in perm]
        # now perm is length 5 list

        new_perm = []
        new_shape = [1, 1, 1, 1]
        i = 0
        found_consecutive_pair = False
        while i < 5:
            if not found_consecutive_pair and i < 4 and perm[i + 1] - perm[i] == 1:
                new_perm.append(perm[i])
                new_shape[perm[i]] = shape_1[perm[i]] * shape_1[perm[i + 1]]
                i = i + 2
                found_consecutive_pair = True
                continue
            else:
                new_perm.append(perm[i] - 1)
                new_shape[perm[i] - 1] = shape_1[perm[i]]
            i += 1

        reshape_1.input_tensors[reshape_1.inputs[1]] = np.asarray(new_shape)
        transpose_1.attrs["perm"] = new_perm

        return [reshape_1, transpose_1, final_reshape]


class PixelShuffleFuser(NodesFuser):
    def __init__(self):  # type: () -> None
        super(PixelShuffleFuser, self).__init__(3)
        self.num_added = 0

    def is_eligible(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> bool
        if not (
            nodes[0].op_type == "Reshape"
            and nodes[1].op_type == "Transpose"
            and nodes[2].op_type == "Reshape"
        ):
            return False
        if len(nodes[0].inputs) == 1 or len(nodes[2].inputs) == 1:
            return False  # it's an old version of onnx Reshape op that had shape as an attribute
        if nodes[0].inputs[1] not in nodes[0].input_tensors:
            return False
        if nodes[2].inputs[1] not in nodes[2].input_tensors:
            return False

        shape_1 = nodes[0].input_tensors[nodes[0].inputs[1]]
        shape_final = nodes[2].input_tensors[nodes[2].inputs[1]]

        shape_1 = _get_fully_defined_shape(shape_1, nodes[0].outputs[0], graph)
        shape_final = _get_fully_defined_shape(shape_final, nodes[2].outputs[0], graph)

        if len(shape_1) != 6 or shape_1[0] != 1 or len(shape_final) != 4:
            return False

        if nodes[1].attrs.get("perm", []) != [0, 1, 4, 2, 5, 3]:
            return False

        return True

    def get_unique_edge_name(self, graph, name):  # type: (Graph, Text) -> Text
        self.num_added += 1
        return graph.get_unique_edge_name(name + "_" + str(self.num_added))

    def merge(self, graph, nodes):  # type: (Graph, Sequence[Node]) -> Sequence[Node]
        """
        Pixel shuffle is implemented using 3 operators:
            - Reshape --> rank 6 (1, x1, x2, x3, x4, x5)
            - Transpose(0, 1, 4, 2, 5, 3) --> (1, x1, x4, x2, x5, x3)
            - Reshape ---> rank 4
        CoreML Reshape and Transpose layers don't support tensors with more
        than 4 dimensions. Thus we change above sequence of operators to the
        following equivalent sequence:
            - Reshape --> (x1, x2, x3, x4 * x5)
            - Transpose(0, 3, 1, 2) --> (x1, x4 * x5, x2, x3)
            - Reshape --> (x1 * x4, x5, x2, x3)
            - Transpose(0, 2, 1, 3) --> (x1 * x4, x2, x5, x3)
            - Reshape --> rank 4
        """
        reshape_1 = nodes[0]
        transpose_1 = nodes[1]
        final_reshape = nodes[2]

        # first reshape
        shape_1 = reshape_1.input_tensors[reshape_1.inputs[1]]
        shape_1 = _get_fully_defined_shape(shape_1, nodes[0].outputs[0], graph)
        x1 = shape_1[1]
        x2 = shape_1[2]
        x3 = shape_1[3]
        x4 = shape_1[4]
        x5 = shape_1[5]
        reshape_1.input_tensors[reshape_1.inputs[1]] = np.asarray([x1, x2, x3, x4 * x5])

        # first transpose
        transpose_1.children = []
        transpose_1.attrs["perm"] = [0, 3, 1, 2]

        reshape_output_name = final_reshape.name + "_pixel_shuffle_reshape"
        transpose_output_name = final_reshape.name + "_pixel_shuffle_transpose"

        transpose_1.outputs = [self.get_unique_edge_name(graph, transpose_output_name)]

        shape_name_second_reshape = self.get_unique_edge_name(
            graph, reshape_output_name
        )
        output_name_second_reshape = self.get_unique_edge_name(
            graph, reshape_output_name
        )

        # second reshape
        reshape_2 = Node(
            reshape_output_name,
            "Reshape",
            {},
            [transpose_1.outputs[0], shape_name_second_reshape],
            [output_name_second_reshape],
        )
        reshape_2.input_tensors[shape_name_second_reshape] = np.asarray(
            [x1 * x4, x5, x2, x3]
        )
        transpose_1.add_child(reshape_2)

        # second transpose
        transpose_2 = Node(
            transpose_output_name,
            "Transpose",
            {"perm": [0, 2, 1, 3]},
            reshape_2.outputs,
            [self.get_unique_edge_name(graph, transpose_output_name)],
        )
        reshape_2.add_child(transpose_2)

        # third reshape
        final_reshape.inputs = [transpose_2.outputs[0], nodes[2].inputs[1]]
        final_reshape.parents = []
        transpose_2.add_child(final_reshape)

        return [reshape_1, transpose_1, reshape_2, transpose_2, final_reshape]


class AddModelInputsOutputs(object):
    """
    Expose hidden states of recurrent layers as model inputs and outputs
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        input_names = [str(input_[0]) for input_ in graph.inputs]
        output_names = [str(output_[0]) for output_ in graph.outputs]
        for node in graph.nodes:
            if str(node.op_type) == "LSTM":
                input_h = (
                    node.inputs[5]
                    if len(node.inputs) > 5
                    else node.inputs[0] + "_h_input"
                )
                input_c = (
                    node.inputs[6]
                    if len(node.inputs) > 6
                    else node.inputs[0] + "_c_input"
                )
                output_h = (
                    node.outputs[1]
                    if len(node.outputs) > 1
                    else node.outputs[0] + "_h_output"
                )
                output_c = (
                    node.outputs[2]
                    if len(node.outputs) > 2
                    else node.outputs[0] + "_c_output"
                )
                h = node.attrs["hidden_size"]
                for input_ in [str(input_h), str(input_c)]:
                    if input_ not in input_names:
                        graph.inputs.append(tuple((input_, TensorProto.FLOAT, (h,))))  # type: ignore
                    if input_ not in graph.blob_to_op_type:
                        graph.blob_to_op_type[input_] = ["LSTM"]
                for output_ in [str(output_h), str(output_c)]:
                    if output_ not in output_names:
                        graph.outputs.append(tuple((output_, TensorProto.FLOAT, (h,))))  # type: ignore
                    graph.blob_from_op_type[output_] = "LSTM"
        return graph


class ConstantsToInitializers(object):
    """
    Takes onnx Constant nodes and puts the tensor into graph initializers instead.
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        output_names = [str(output_[0]) for output_ in graph.outputs]
        nodes_to_be_removed = []
        for node in graph.nodes:
            if node.op_type == "Constant" and (node.name not in output_names):
                nodes_to_be_removed.append(node)
                x = node.attrs["value"]
                for child in node.children:
                    child.input_tensors[node.outputs[0]] = x
                    child.parents.remove(node)
                graph.shape_dict[node.outputs[0]] = x.shape

        transformed_nodes = []
        for node in graph.nodes:
            if node not in nodes_to_be_removed:
                transformed_nodes.append(node)
        return graph.create_graph(nodes=transformed_nodes)


class ConstantFillToInitializers(object):
    """
    Takes onnx ConstantFill nodes and puts the tensor into graph initializers instead, for simple cases only.
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        output_names = [str(output_[0]) for output_ in graph.outputs]
        nodes_to_be_removed = []
        for node in graph.nodes:
            if (
                node.op_type == "ConstantFill"
                and (node.name not in output_names)
                and node.attrs.get("input_as_shape", 0)
                and node.inputs[0] in node.input_tensors
                and node.attrs.get("extra_shape", None) is None
            ):

                s = node.input_tensors[node.inputs[0]]
                x = np.ones(tuple(s.astype(int))) * node.attrs.get("value", 0.0)
                nodes_to_be_removed.append(node)
                for child in node.children:
                    child.input_tensors[node.outputs[0]] = x
                    child.parents.remove(node)
                graph.shape_dict[node.outputs[0]] = x.shape

        transformed_nodes = []
        for node in graph.nodes:
            if node not in nodes_to_be_removed:
                transformed_nodes.append(node)
        return graph.create_graph(nodes=transformed_nodes)


class ShapeOpRemover(object):
    """
    remove shape op, if the input shape is fully known
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        nodes_to_be_removed = []
        output_names = [str(output_[0]) for output_ in graph.outputs]
        for node in graph.nodes:
            if (
                node.op_type == "Shape"
                and (node.name not in output_names)
                and node.inputs[0] in graph.shape_dict
            ):
                x_tuple = graph.shape_dict[node.inputs[0]]  # type: Tuple[int, ...]
                is_well_defined = True
                for i in x_tuple:
                    if not (isinstance(i, int) and i > 0):
                        is_well_defined = False
                        break
                if is_well_defined:
                    x = np.asarray(x_tuple, dtype=np.float32)
                    nodes_to_be_removed.append(node)
                    for child in node.children:
                        child.input_tensors[node.outputs[0]] = x
                        child.parents.remove(node)
                    for parent in node.parents:
                        parent.children.remove(node)
                    graph.shape_dict[node.outputs[0]] = x.shape

        transformed_nodes = []
        for node in graph.nodes:
            if node not in nodes_to_be_removed:
                transformed_nodes.append(node)
        return graph.create_graph(nodes=transformed_nodes)


class CastOpRemover(object):
    """
    Remove Cast Op: onnx-coreml treats all tensor as Float and hence, Cast operator should be removed
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        global cast_i
        nodes_to_be_removed = []
        output_names = [str(output_[0]) for output_ in graph.outputs]
        for node in graph.nodes:
            if (
                node.op_type == "Cast"
                and (node.name not in output_names)
                and node.inputs[0] in graph.shape_dict
            ):
                nodes_to_be_removed.append(node)
                _remove_single_input_output_node(node)

        transformed_nodes = []
        for node in graph.nodes:
            if node not in nodes_to_be_removed:
                transformed_nodes.append(node)
        return graph.create_graph(nodes=transformed_nodes)


class PaddingOpRemover(object):
    """
    Remove Pad Op if all the pad values are 0
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        global cast_i
        nodes_to_be_removed = []
        output_names = [str(output_[0]) for output_ in graph.outputs]
        for node in graph.nodes:
            if (
                node.op_type == "Pad"
                and (node.name not in output_names)
                and node.inputs[0] in graph.shape_dict
            ):
                pads = node.attrs.get("pads", [])
                if len(pads) > 0 and sum(pads) == 0:
                    nodes_to_be_removed.append(node)
                    _remove_single_input_output_node(node)

        transformed_nodes = []
        for node in graph.nodes:
            if node not in nodes_to_be_removed:
                transformed_nodes.append(node)
        return graph.create_graph(nodes=transformed_nodes)


class ImageScalerRemover(object):
    """
    Removes ImageScaler layer if connected to a model input and single parent child nodes
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        input_names = [str(input_[0]) for input_ in graph.inputs]
        nodes_to_be_removed = []
        for node in graph.nodes:
            if (
                (node.op_type != "ImageScaler")
                or (len(node.parents) != 0)
                or (node.inputs[0] not in input_names)
            ):
                continue
            nodes_to_be_removed.append(node.name)
            for child in node.children:
                for i, child_input in enumerate(child.inputs):
                    if child_input == node.outputs[0]:
                        child.inputs[i] = node.inputs[0]
                        child.parents.remove(node)
                        break

        transformed_nodes = []
        for node in graph.nodes:
            if node.name not in nodes_to_be_removed:
                transformed_nodes.append(node)
        return graph.create_graph(nodes=transformed_nodes)


class ConstantRemover(object):
    """
    Removes Op if its input is constant
    Currently, Supports: Gather, Floor, Div, Mul, Slice, Transpose, Concat, Unsqueeze, Squeeze
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        nodes_to_be_removed = []
        for node in graph.nodes:
            are_all_inputs_constant = True
            for input_ in node.inputs:
                if input_ not in node.input_tensors:
                    are_all_inputs_constant = False
                    break

            transformation_performed = False
            if len(node.parents) != 0 or are_all_inputs_constant == False:
                continue
            # TODO: Replace If -> ElIf with more general transformation block
            if node.op_type == "Gather":
                data = node.input_tensors[node.inputs[0]]
                idx = node.input_tensors[node.inputs[1]]
                axis = node.attrs.get("axis", 0)
                output = np.take(data, idx, axis=axis)
                transformation_performed = True
            elif node.op_type == "Floor":
                input = node.input_tensors[node.inputs[0]]
                output = np.floor(input)
                transformation_performed = True
            elif node.op_type == "Div" or node.op_type == "Mul":
                x = node.input_tensors[node.inputs[0]]
                y = node.input_tensors[node.inputs[1]]
                for child_node in node.children:
                    # child_node.parents.remove(node)
                    if node.op_type == "Div":
                        output = x / y
                    else:
                        output = x * y
                transformation_performed = True
            elif node.op_type == "Slice":
                x = node.input_tensors[node.inputs[0]]
                ends = node.attrs["ends"]
                starts = node.attrs["starts"]
                axes = node.attrs.get("axes", range(len(starts)))
                output = x
                for i, a in enumerate(axes):
                    s = starts[i]
                    e = ends[i]
                    n = x.shape[a]
                    if s < 0:
                        s += n
                    if e < 0:
                        e += n
                    output = np.take(x, range(s, e), axis=a)  # type: ignore
                transformation_performed = True
            elif node.op_type == "Transpose":
                x = node.input_tensors[node.inputs[0]]
                perm = node.attrs.get("perm", None)
                output = np.transpose(x, axes=perm)  # type: ignore
                transformation_performed = True
            elif node.op_type == "Concat":
                x_arr = []
                for input_ in node.inputs:
                    x_arr.append(node.input_tensors[input_])
                axis = node.attrs.get("axis", 0)
                output = np.concatenate(x_arr, axis=axis)  # type: ignore
                transformation_performed = True
            elif node.op_type == "Unsqueeze" or node.op_type == "Squeeze":
                x = node.input_tensors[node.inputs[0]]
                if node.op_type == "Unsqueeze":
                    axes = node.attrs["axes"]
                    axes.sort()
                    for axis in axes:
                        output = np.expand_dims(x, axis=axis)  # type: ignore
                else:
                    axes = node.attrs.get("axes", None)
                    output = np.squeeze(x, axis=tuple(axes))
                transformation_performed = True
            elif node.op_type == "Gemm":
                alpha = node.attrs.get("alpha", 1.0)
                beta = node.attrs.get("beta", 1.0)
                transA = node.attrs.get("transA", False)
                transB = node.attrs.get("transB", False)

                A_tensor = node.input_tensors[node.inputs[0]]
                B_tensor = node.input_tensors[node.inputs[1]]
                C_tensor = node.input_tensors[node.inputs[2]]

                A_tensor = np.transpose(A_tensor) if transA else A_tensor
                B_tensor = np.transpose(B_tensor) if transB else B_tensor

                output = alpha * np.dot(A_tensor, B_tensor) + beta * C_tensor
                transformation_performed = True

            if transformation_performed:
                nodes_to_be_removed.append(node)
                graph.shape_dict[node.outputs[0]] = output.shape
                for child_node in node.children:
                    child_node.parents.remove(node)
                    child_node.input_tensors[node.outputs[0]] = output
        transformed_nodes = []
        for node in graph.nodes:
            if node not in nodes_to_be_removed:
                transformed_nodes.append(node)
        return graph.create_graph(nodes=transformed_nodes)


class DeadCodeElimination(object):
    """
    Removes nodes with unused outputs
    """

    def __call__(self, graph):  # type: (Graph) -> Graph
        input_names = [str(input_[0]) for input_ in graph.inputs]
        output_names = set([str(output_[0]) for output_ in graph.outputs])

        nodes_to_be_removed = []
        uses = {}

        for _output in output_names:
            uses[_output] = uses.get(_output, 0) + 1

        for node in graph.nodes:
            for _input in node.inputs:
                uses[_input] = uses.get(_input, 0) + 1

        for node in reversed(graph.nodes):
            output_used = False
            for _output in node.outputs:
                if _output in uses:
                    output_used = True
                    break

            if not output_used:
                # Remove current node
                for _input in node.inputs:
                    uses[_input] -= 1
                    if uses[_input] == 0:
                        del uses[_input]
                nodes_to_be_removed.append(node.name)
                for parent in node.parents:
                    parent.children.remove(node)

        transformed_nodes = []
        for node in graph.nodes:
            if node.name not in nodes_to_be_removed:
                transformed_nodes.append(node)

        for _input in input_names:
            if _input not in uses:
                for i in range(len(graph.inputs)):
                    if graph.inputs[i][0] is _input:
                        graph.inputs.remove(graph.inputs[i])
                        break

        return graph.create_graph(nodes=transformed_nodes)

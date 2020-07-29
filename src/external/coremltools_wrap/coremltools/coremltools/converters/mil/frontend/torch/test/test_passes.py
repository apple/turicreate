import itertools

import numpy as np
import pytest
import torch

from ..internal_graph import *
from ..torchir_passes import *


def _build_flattening_test_graph():
    # This test graph is:
    #    graph(
    #        %1 : (Tensor[1, 1], (Tensor[1, 2], Tensor[1, 3]))
    #    ):
    #    %2, %3 = tupleunpack[](%1)
    #    %4, %5 = tupleunpack[](%3)
    #    %6 = tupleconstruct[](%2, %4)
    #    %7 = tupleconstruct[](%6, %5)
    #    return (%7)
    #
    # And if you were to run the graph it would turn
    #    (a, (b, c))
    # into
    #    ((a, b), c)

    graph_params = {}
    graph_inputs = OrderedDict()
    graph_inputs["1"] = (
        torch.rand(1, 1),
        (
            torch.rand(1, 2),
            torch.rand(1, 3),
        ),
    )
    graph_nodes = [
        InternalTorchIRNode(
            inputs=["1"],
            outputs=["2", "3"],
            kind="tupleunpack",
        ),
        InternalTorchIRNode(
            inputs=["3"],
            outputs=["4", "5"],
            kind="tupleunpack",
        ),
        InternalTorchIRNode(
            inputs=["2", "4"],
            outputs=["6"],
            kind="tupleconstruct",
        ),
        InternalTorchIRNode(
            inputs=["6", "5"],
            outputs=["7"],
            kind="tupleconstruct",
        ),
    ]
    graph_outputs = ["7"]

    return InternalTorchIRGraph(
        nodes=graph_nodes,
        params=graph_params,
        inputs=graph_inputs,
        outputs=graph_outputs,
    )


class TestTorchPasses:
    """Class containing tests for InternalTorchIR optimization passes.
    """

    @pytest.fixture
    def set_random_seeds(self):
        torch.manual_seed(1)
        np.random.seed(1)

    def test_flatten_input_values(self):
        graph = _build_flattening_test_graph()

        flatten_graph_input_values(graph)

        # The graph input tuple should have been flattened.
        np.testing.assert_equal(len(graph.inputs.keys()), 3)
        # Tuple flattening should introduce two new ops.
        np.testing.assert_equal(len(graph.nodes), 6)
        # The new ops at the beginning of the graph should be a tupleconstruct.
        np.testing.assert_equal(graph.nodes[0].kind, "tupleconstruct")
        np.testing.assert_equal(graph.nodes[1].kind, "tupleconstruct")
        # The inputs to the tupleconstructs should be the new flattened inputs.
        input_names = [k for k in graph.inputs.keys()]
        np.testing.assert_equal(input_names[1:], graph.nodes[0].inputs)
        np.testing.assert_equal(input_names[0], graph.nodes[1].inputs[0])
        np.testing.assert_equal(graph.nodes[0].outputs[0], graph.nodes[1].inputs[1])
        # The last inserted tuple construct should produce the input for the
        # next op.
        np.testing.assert_equal(graph.nodes[1].outputs[0], graph.nodes[2].inputs[0])

    def test_flatten_output_values(self):
        graph = _build_flattening_test_graph()

        flatten_graph_output_values(graph)

        # The graph output tuple should have been flattened.
        np.testing.assert_equal(len(graph.outputs), 3)
        # The outputs of the graph should come from intermediate ops.
        np.testing.assert_equal(graph.outputs[0], graph.nodes[0].outputs[0])
        np.testing.assert_equal(graph.outputs[1], graph.nodes[1].outputs[0])
        np.testing.assert_equal(graph.outputs[2], graph.nodes[1].outputs[1])

    def test_transform_inplace_ops_graph(self):
        # The test graph is:
        #    graph(
        #        %x : Tensor[1],
        #    ):
        #      %1 = constant[value=0]()
        #      %2 = constant[value=10]()
        #      %3 = listconstruct[](%1)
        #      %4 = append[](%3, %2)
        #      return (%3)
        graph_params = {}
        graph_inputs = OrderedDict()
        graph_inputs["x"] = torch.rand(1)
        graph_nodes = [
            InternalTorchIRNode(
                inputs=[],
                attr={"value": 0},
                outputs=["1"],
                kind="constant",
            ),
            InternalTorchIRNode(
                inputs=[],
                attr={"value": 10},
                outputs=["2"],
                kind="constant",
            ),
            InternalTorchIRNode(
                inputs=["1"],
                outputs=["3"],
                kind="listconstruct",
            ),
            InternalTorchIRNode(
                inputs=["3", "2"],
                outputs=["4"],
                kind="append",
            ),
        ]
        graph_outputs = ["3"]
        graph = InternalTorchIRGraph(
            nodes=graph_nodes,
            params=graph_params,
            inputs=graph_inputs,
            outputs=graph_outputs,
        )
        for node in graph.nodes:
            node.parent = graph

        transform_inplace_ops(graph)

        np.testing.assert_equal(len(graph.outputs), 1)
        np.testing.assert_equal(graph.outputs[0], graph.nodes[-1].outputs[0])


    def test_transform_inplace_ops_loop(self):
        # The test graph is:
        #    graph(
        #        %x : Tensor[1],
        #    ):
        #      %1 = constant[value=True]()
        #      %2 = constant[value=-1]()
        #      %3 = constant[value=10]()
        #      %4 = listconstruct[](%2)
        #       = loop[](%3, %1)
        #        block(%i.1):
        #          %6 = append[](%4, %i.1)
        #        return (%1)
        #    return (%4)
        graph_params = {}
        graph_inputs = OrderedDict()
        graph_inputs["x"] = torch.rand(1)
        loop_block = InternalTorchIRBlock(
            inputs=["i.1"],
            outputs=["1"],
            nodes=[
                InternalTorchIRNode(
                    inputs=["4", "i.1"],
                    outputs=["6"],
                    kind="append",
                ),
            ],
        )
        loop_block.nodes[0].parent = loop_block
        loop_node = InternalTorchIRNode(
            inputs=["3", "1"],
            outputs=[],
            kind="loop",
            blocks=[loop_block],
        )
        loop_block.parent = loop_node
        graph_nodes = [
            InternalTorchIRNode(
                inputs=[],
                attr={"value": True},
                outputs=["1"],
                kind="constant",
            ),
            InternalTorchIRNode(
                inputs=[],
                attr={"value": -1},
                outputs=["2"],
                kind="constant",
            ),
            InternalTorchIRNode(
                inputs=[],
                attr={"value": 10},
                outputs=["3"],
                kind="constant",
            ),
            InternalTorchIRNode(
                inputs=["2"],
                outputs=["4"],
                kind="listconstruct",
            ),
            loop_node,
        ]
        graph_outputs = ["4"]
        graph = InternalTorchIRGraph(
            nodes=graph_nodes,
            params=graph_params,
            inputs=graph_inputs,
            outputs=graph_outputs,
        )
        for node in graph.nodes:
            node.parent = graph

        transform_inplace_ops(graph)

        # There should be an additional input to the loop.
        np.testing.assert_equal(len(loop_node.inputs), 3)
        # That input should be the output of the previous op.
        np.testing.assert_equal(loop_node.inputs[2], graph.nodes[3].outputs[0])
        # The loop block should have an additional input.
        np.testing.assert_equal(len(loop_block.inputs), 2)
        # The loop block's new input should be the input to append.
        np.testing.assert_equal(loop_block.inputs[1], loop_block.nodes[0].inputs[0])
        # The loop block should have an additional output.
        np.testing.assert_equal(len(loop_block.outputs), 2)
        # Append's output should be returned from the loop block.
        np.testing.assert_equal(loop_block.outputs[1], loop_block.nodes[0].outputs[0])
        # The loop should now have an output.
        np.testing.assert_equal(len(loop_node.outputs), 1)
        # The loop's name should now be the name of its output.
        np.testing.assert_equal(loop_node.name, loop_node.outputs[0])
        # That graph output should now be the output of the graph.
        np.testing.assert_equal(loop_node.outputs[0], graph.outputs[0])


    @pytest.mark.xfail(reason="rdar://64235006")
    def test_transform_inplace_ops_if(self):
        # The test graph is:
        #    graph(
        #        %x : Tensor[1],
        #    ):
        #      %1 = constant[value=True]()
        #      %2 = constant[value=0]()
        #      %3 = constant[value=1]()
        #      %4 = listconstruct[](%2)
        #       = if[](%1)
        #        block0():
        #          %5 = append[](%4, %3)
        #        return ()
        #        block1():
        #          %6 = append[](%4, %2)
        #        return ()
        #    return (%4)
        graph_params = {}
        graph_inputs = OrderedDict()
        graph_inputs["x"] = torch.rand(1)
        if_true_block = InternalTorchIRBlock(
            inputs=[],
            outputs=[],
            nodes=[
                InternalTorchIRNode(
                    inputs=["4", "3"],
                    outputs=["5"],
                    kind="append",
                ),
            ],
        )
        if_true_block.nodes[0].parent = if_true_block
        if_false_block = InternalTorchIRBlock(
            inputs=[],
            outputs=[],
            nodes=[
                InternalTorchIRNode(
                    inputs=["4", "2"],
                    outputs=["6"],
                    kind="append",
                ),
            ],
        )
        if_false_block.nodes[0].parent = if_false_block
        if_node = InternalTorchIRNode(
            inputs=["1"],
            outputs=[],
            kind="if",
            blocks=[if_true_block, if_false_block],
        )
        if_true_block.parent = if_node
        if_false_block.parent = if_node
        graph_nodes = [
            InternalTorchIRNode(
                inputs=[],
                attr={"value": True},
                outputs=["1"],
                kind="constant",
            ),
            InternalTorchIRNode(
                inputs=[],
                attr={"value": 0},
                outputs=["2"],
                kind="constant",
            ),
            InternalTorchIRNode(
                inputs=[],
                attr={"value": 1},
                outputs=["3"],
                kind="constant",
            ),
            InternalTorchIRNode(
                inputs=["2"],
                outputs=["4"],
                kind="listconstruct",
            ),
            if_node,
        ]
        graph_outputs = ["4"]
        graph = InternalTorchIRGraph(
            nodes=graph_nodes,
            params=graph_params,
            inputs=graph_inputs,
            outputs=graph_outputs,
        )
        for node in graph.nodes:
            node.parent = graph

        transform_inplace_ops(graph)

        # The true block should now have an output.
        np.testing.assert_equal(len(if_true_block.outputs), 1)
        # The true block should output the result of the append op.
        np.testing.assert_equal(if_true_block.outputs[0], if_true_block.nodes[0].outputs[0])
        # The false block should now have an output.
        np.testing.assert_equal(len(if_false_block.outputs), 1)
        # The false block should output the result of the append op.
        np.testing.assert_equal(if_false_block.outputs[0], if_false_block.nodes[0].outputs[0])
        # The if op should have an additional output.
        np.testing.assert_equal(len(if_node.outputs), 1)
        # The if's name should now be the name of its output.
        np.testing.assert_equal(if_node.name, if_node.outputs[0])
        # The graph output should be the if op output.
        np.testing.assert_equal(if_node.outputs[0], graph.outputs[0])
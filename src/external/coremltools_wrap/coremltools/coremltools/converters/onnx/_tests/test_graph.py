from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import unittest
from coremltools._deps import _HAS_ONNX, MSG_ONNX_NOT_FOUND

if _HAS_ONNX:
    import onnx
    from onnx import helper, numpy_helper, TensorProto
    from coremltools.converters.onnx._graph import Node, Graph
    from ._test_utils import (
        _onnx_create_single_node_model,
        _onnx_create_model,
        _conv_pool_output_size,
        _random_array,
    )


@unittest.skipUnless(_HAS_ONNX, MSG_ONNX_NOT_FOUND)
class NodeTest(unittest.TestCase):
    def test_create_node(self):  # type: () -> None
        model = _onnx_create_single_node_model(
            "Elu", [(1, 3, 224, 224)], [(1, 3, 224, 224)], alpha=0.5
        )
        graph = model.graph
        node = graph.node[0]
        node_ = Node.from_onnx(node)
        self.assertTrue(len(node_.inputs) == 1)
        self.assertTrue(len(node_.outputs) == 1)
        self.assertTrue(len(node_.attrs) == 1)
        self.assertTrue(node_.attrs["alpha"] == 0.5)


@unittest.skipUnless(_HAS_ONNX, MSG_ONNX_NOT_FOUND)
class GraphTest(unittest.TestCase):
    def test_create_graph(self):  # type: () -> None
        kernel_shape = (3, 2)
        strides = (2, 3)
        pads = (4, 2, 4, 2)
        dilations = (1, 2)
        group = 1
        weight = numpy_helper.from_array(_random_array((16, 3, 3, 2)), name="weight")

        input_shape = (1, 3, 224, 224)
        output_size = _conv_pool_output_size(
            input_shape, dilations, kernel_shape, pads, strides
        )

        output_shape = (1, int(weight.dims[0]), output_size[0], output_size[1])

        inputs = [("input0", input_shape)]
        outputs = [("output0", output_shape, TensorProto.FLOAT)]

        conv = helper.make_node(
            "Conv",
            inputs=[inputs[0][0], "weight"],
            outputs=["conv_output"],
            dilations=dilations,
            group=group,
            kernel_shape=kernel_shape,
            pads=pads,
            strides=strides,
        )

        relu = helper.make_node(
            "Relu", inputs=[conv.output[0]], outputs=[outputs[0][0]]
        )

        model = _onnx_create_model([conv, relu], inputs, outputs, [weight])
        graph_ = Graph.from_onnx(model.graph, onnx_ir_version=5)
        self.assertTrue(len(graph_.inputs) == 1)
        self.assertEqual(graph_.inputs[0][2], input_shape)
        self.assertTrue(len(graph_.outputs) == 1)
        self.assertEqual(graph_.outputs[0][2], output_shape)
        self.assertTrue(len(graph_.nodes) == 2)
        self.assertEqual(len(graph_.nodes[0].parents), 0)
        self.assertEqual(len(graph_.nodes[1].parents), 1)
        self.assertEqual(len(graph_.nodes[0].children), 1)
        self.assertEqual(len(graph_.nodes[1].children), 0)
